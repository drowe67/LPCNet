/*
  quant_feat.c
  David Rowe Jan 2019

  Tool for processing a .f32 file of LPCNet features to simulate quantisation:

  1/ Can decimate cepstrals to 20/30/40/... ms update rate and
     liniearly interpolate back up to 10ms
  2/ Quantise using multistage VQs
  3/ Replace the LPCNet pitch estimate with estimates from external files
  4/ Works from stdin -> stdout to facilitate streaming real time simulations.
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <getopt.h>

#include "common.h"
#include "freq.h"
#include "mbest.h"

#define NB_FEATURES    55
#define NB_BANDS       18
#define MAX_STAGES     5    /* max number of VQ stages                */
#define MAX_ENTRIES    4096 /* max number of vectors per stage        */
#define NOUTLIERS      5    /* range of outiles to track in 1dB steps */

int verbose = 0;
FILE *fsv = NULL;

int quantise(const float * cb, float vec[], float w[], int k, int m, float *se);

void quant_pred(float vec_out[],  /* prev quant vector, and output */
                float vec_in[],
                float pred,
                int num_stages,
                float vq[],
                int m[], int k);

void quant_pred_mbest(float vec_out[],  /* prev quant vector, and output */
                      float vec_in[],
                      float pred,
                      int num_stages,
                      float vq[],
                      int m[], int k,
                      int mbest_survivors);

int main(int argc, char *argv[]) {
    FILE *fin, *fout;
    float features[NB_FEATURES], features_out[NB_FEATURES];
    int f = 0, dec = 2;
    float features_quant[NB_BANDS];
    float sum_sq_err = 0.0;
    int d,i,n = 0;
    float fract;

    int c, first = 0, k=NB_BANDS;
    int num_stages = 0;
    float vq[MAX_STAGES*NB_BANDS*MAX_ENTRIES];
    int   m[MAX_STAGES];
    float pred = 0.9;
    
    char fnames[256];
    char fn[256];
    char *comma, *p;
    FILE *fq;

    FILE *fpitch = NULL;
    float Fs = 16000.0;
    float uniform_step = 0.0;
    int   mbest_survivors = 0;
    char label[80] = "";
    /* experimental limits for dctLy[0], first cepstral */
    float lower_limit = -200.0;
    float upper_limit =  200.00;
    /* weight applied to first cepstral */
    float weight = 1.0;    
    float pitch_gain = 0.0;
    
    static struct option long_options[] = {
        {"decimate", required_argument, 0, 'd'},
        {"extpitch", required_argument, 0, 'e'},
        {"first",    required_argument, 0, 'f'},
        {"gain",     required_argument, 0, 'g'},
        {"hard",     required_argument, 0, 'h'},
        {"label",    required_argument, 0, 'l'},
        {"mbest",    required_argument, 0, 'm'},
        {"pred",     required_argument, 0, 'p'},
        {"quant",    required_argument, 0, 'q'},
        {"stagevar", required_argument, 0, 's'},
        {"uniform",  required_argument, 0, 'u'},
        {"verbose",  no_argument,       0, 'v'},
        {"weight",   no_argument,       0, 'w'},
        {0, 0, 0, 0}
    };

    int opt_index = 0;

    while ((c = getopt_long (argc, argv, "d:q:vs:f:p:e:u:l:m:h:wg:", long_options, &opt_index)) != -1) {
        switch (c) {
        case 'f':
            /* start VQ at band first+1 */
            first = atoi(optarg);
            k = NB_BANDS-first;
            fprintf(stderr, "first = %d k = %d\n", first, k);
            break;
        case 's':
            /* text file to dump error (variance) per stage */
            fsv = fopen(optarg, "wt"); assert(fsv != NULL);            
            break;
        case 'd':
            dec = atoi(optarg);
            fprintf(stderr, "dec = %d\n", dec);
            break;
        case 'e':
            /* external pitch estimate, one F0 est (Hz) per line of text file */
            fpitch = fopen(optarg, "rt"); assert(fpitch != NULL);            
            fprintf(stderr, "ext pitch F0 file: %s\n", optarg);
            break;
        case 'g':
            pitch_gain = atof(optarg);
            fprintf(stderr, "pitch_gain: %f\n", pitch_gain);
            break;
        case 'h':
            /* hard limit (saturate) first feature (energy) */
            lower_limit = atof(optarg);            
            fprintf(stderr, "lower_limit: %f upper_limit: %f\n", lower_limit, upper_limit);
            break;
        case 'l':
            /* text label to pront with results */
            strcpy(label, optarg);
            break;
        case 'm':
            mbest_survivors = atoi(optarg);
            fprintf(stderr, "mbest_survivors = %d\n",  mbest_survivors);
            break;
        case 'p':
            pred = atof(optarg);
            fprintf(stderr, "pred = %f\n", pred);
            break;
        case 'q':
            /* list of comma delimited file names */
            strcpy(fnames, optarg);
            p = fnames;
            num_stages = 0;
            do {
                assert(num_stages < MAX_STAGES);
                strcpy(fn, p);
                comma = strchr(fn, ',');
                if (comma) {
                    *comma = 0;
                    p = comma+1;
                }
                /* load quantiser file */
                fprintf(stderr, "stage: %d loading %s ...", num_stages, fn);
                fq=fopen(fn, "rb");
                if (fq == NULL) {
                    fprintf(stderr, "Couldn't open: %s\n", fn);
                    exit(1);
                }
                m[num_stages] = 0;
                while (fread(features, sizeof(float), k, fq) == (size_t)k) m[num_stages]++;
                assert(m[num_stages] <= MAX_ENTRIES);
                fprintf(stderr, "%d entries of vectors width %d\n", m[num_stages], k);
                rewind(fq);                       
                int rd = fread(&vq[num_stages*k*MAX_ENTRIES], sizeof(float), m[num_stages]*k, fq);
                assert(rd == m[num_stages]*k);
                num_stages++;
                fclose(fq);
            } while(comma);
            break;
        case 'u':
            uniform_step = atof(optarg);
            fprintf(stderr, "uniform quant step size: %3.2f dB\n", uniform_step);
            break;
        case 'v':
            verbose = 1;
            break;
        case 'w':
            weight = 1.0/sqrt(NB_BANDS);
            break;
         default:
            fprintf(stderr,"usage: %s [Options]:\n  [-d --decimation 1/2/3...]\n  [-q --quant quantfile1,quantfile2,....]\n", argv[0]);
            fprintf(stderr,"  [-h --hard lowerLimitdB\n");
            fprintf(stderr,"  [-l --label txtLabel]\n");
            fprintf(stderr,"  [-m --mbest survivors]\n  [-p --pred predCoff]\n  [-f --first firstElement]\n  [-s --stagevar TxtFile]\n");
            fprintf(stderr,"  [-e --extpitch ExtPitchFile]\n  [-u --uniform stepSizedB]\n  [-v --verbose]\n");
            fprintf(stderr,"  [-w --weight first cepstral by 1/sqrt(NB_BANDS)\n");
            exit(1);
        }
    }

    fprintf(stderr, "dec: %d pred: %3.2f num_stages: %d mbest: %d", dec, pred, num_stages, mbest_survivors);
    fprintf(stderr, "\n");
    
    /* delay line so we can pass some features (like pitch and voicing) through unmodified */
    float features_prev[dec+1][NB_FEATURES];
     /* adjacent vectors used for linear interpolation */
    float features_lin[2][NB_BANDS];
    float features_mem[NB_BANDS];
    for(i=0; i<NB_BANDS; i++)
        features_mem[i] = 0.0;
    
    for(d=0; d<dec+1; d++)
        for(i=0; i<NB_FEATURES; i++)
            features_prev[d][i] = 0.0;
    for(d=0; d<2; d++)
        for(i=0; i<NB_BANDS; i++)
            features_lin[d][i] = 0.0;
    for(i=0; i<NB_BANDS; i++)
        features_quant[i] = 0.0;
    
    fin = stdin;
    fout = stdout;

    /* dec == 2:
       In.:          f2     f3     f4    f5     f6
       Out:          f0 (f0+f2)/2) f2 (f2+f4)/2 f4 ....

       features_prev
       2             f2     f3     f4    f5     f6     
       1             f1     f2     f3    f4     f5
       0             f0     f1     f2    f3     f4

       features_lin
       1             f2     f2     f4    f4     f6
       0             f0     f0     f2    f2     f4
    */
    
    /* dec == 3:
       In.:          f3        f4          f5        f6       f7
       Out: ....     f0  2f0/3 + f3/3  f0/3 + 2f2/3  f3  2f3/3 + f6/3

       features_prev
       3             f3        f4          f5        f6       f7
       2             f2        f3          f4        f5       f6     
       1             f1        f2          f3        f4       f5
       0             f0        f1          f2        f3       f4

       features_lin
       1             f3        f3          f3        f6       f6
       0             f0        f0          f0        f3       f3
    */

    long noutliers[NOUTLIERS];
    for(i=0; i<NOUTLIERS; i++)
        noutliers[i] = 0;
    int qv = 0;
    
    while(fread(features, sizeof(float), NB_FEATURES, fin) == NB_FEATURES) {
       
        /* convert cepstrals to dB */
        for(i=0; i<NB_BANDS; i++)
            features[i] *= 10.0;

        /* optional weight on first cepstral which increases at
           sqrt(NB_BANDS) for every dB of speech input power.  Note by
           doing it here, we won't be measuring SD of this step, SD
           results will be on weighted vector. */
        features[0] *= weight;
        
        /* apply lower limit to features[0] */

        if (features[0] < lower_limit) features[0] = lower_limit;
        if (features[0] > upper_limit) features[0] = upper_limit;
       
        /* optionally load external pitch est sample and replace pitch feature */
        if (fpitch != NULL) {
            float f0;
            if (fscanf(fpitch,"%f\n", &f0)) {
                float pitch_index = 2.0*Fs/f0;
                features[2*NB_BANDS] = 0.01*(pitch_index-200.0);
                //fprintf(stderr,"%d: %f %f %f\n", f, f0, pitch_index, features[2*NB_BANDS]);
            }
            else
                fprintf(stderr, "f0 not read\n");
        }

        /* optionally set pitch gain (voicing) to constant */
        if (pitch_gain != 0.0) {
            features[2*NB_BANDS+1] = pitch_gain;
        }
        
        /* maintain delay line of unquantised features for partial quantisation and distortion measure */
        for(d=0; d<dec; d++)
            for(i=0; i<NB_FEATURES; i++)
                features_prev[d][i] = features_prev[d+1][i];
        for(i=0; i<NB_FEATURES; i++)
            features_prev[dec][i] = features[i];

       /*
        for(i=0; i<NB_BANDS; i++)
             features_mem[i] = 0.5*features_mem[i] + 0.5*features[i];
        */
        
        if ((f % dec) == 0) {
            /* non-interpolated frame ----------------------------------------*/

            /* optional quantisation */
            if (num_stages || (uniform_step != 0.0)) {
                if (num_stages) {
                    if (mbest_survivors) {
                        /* mbest predictive VQ */
                        quant_pred_mbest(&features_quant[first], &features[first], pred, num_stages, vq, m, k, mbest_survivors);
                    }
                    else {
                        /* standard predictive VQ */
                        quant_pred(&features_quant[first], &features[first], pred, num_stages, vq, m, k);
                    }
                    for(i=0; i<first; i++)
                        features_quant[i] = features[i];
                }
                if (uniform_step != 0.0) {
                    for(i=0; i<NB_BANDS; i++) {
                        features_quant[i] = uniform_step*round(features[i]/uniform_step);
                        //fprintf(stderr, "%d %f %f\n", i, features[i], features_quant[i]);
                    }
                }
            }
            else {
                /* unquantsed */
                for(i=0; i<NB_BANDS; i++) {
                    features_quant[i] = features[i];
                }
            }

            /* update linear interpolation arrays */
            for(i=0; i<NB_BANDS; i++) {
                features_lin[0][i] = features_lin[1][i];
                features_lin[1][i] = features_quant[i];                
            }

            /* pass (quantised) frame though */
            for(i=0; i<NB_BANDS; i++) {
                features_out[i] = features_lin[0][i];
            }

            /* measure quantisation error power (variance).  The
               dec/interp also adds significant distortion however we
               are just counting quantiser distortion here. */

            float e = 0.0;
            for(i=0; i<NB_BANDS; i++) {
                e += pow(features_out[i]-features_prev[0][i], 2.0);
            }
            sum_sq_err += e; n+= NB_BANDS;
            for (i=NOUTLIERS; i>=0; i--)
                if (sqrt(e/NB_BANDS) > (float)(i+1.0)) {
                    noutliers[i]++;
                    break;
                }
            qv++;
            
            features_out[2*NB_BANDS+2] = features_prev[0][2*NB_BANDS+2];  /* pass through LPC energy */

        } else {
            /* interpolated frame ----------------------------------------*/
            
            for(i=0; i<NB_FEATURES; i++)
                features_out[i] = 0.0;
            /* interpolate frame */
            d = f%dec;
            for(i=0; i<NB_BANDS; i++) {
                fract = (float)d/(float)dec;
                features_out[i] = (1.0-fract)*features_lin[0][i] + fract*features_lin[1][i];
            }
            
            /* set up LPCs from interpolated cepstrals */
            float g = lpc_from_cepstrum(&features_out[2*NB_BANDS+3], features_out);
            features_out[2*NB_BANDS+2] = log10(g);  /* LPC energy comes from interpolated ceptrals */
        }
        
        /* pass some features through at the original (undecimated) sample rate for now */
            
        features_out[2*NB_BANDS]   = features_prev[0][2*NB_BANDS];   /* original undecimated pitch      */
        features_out[2*NB_BANDS+1] = features_prev[0][2*NB_BANDS+1]; /* original undecimated gain       */
        f++;
        
        features_out[0] /= weight;    

        /* convert cespstrals back from dB */
        for(i=0; i<NB_BANDS; i++)
            features_out[i] *= 1/10.0;

        fwrite(features_out, sizeof(float), NB_FEATURES, fout);
        fflush(stdin);
        fflush(stdout);
    }

    float var = sum_sq_err/n;
    fprintf(stderr, "RESULTS %s var: %4.3f sd: %4.3f n: %4d", label, var, sqrt(var), n);
    fprintf(stderr, " outliers > ");
    for (i=0; i<NOUTLIERS; i++)
        fprintf(stderr, "%d ", i+1);
    fprintf(stderr, " dB = ");
    for (i=0; i<NOUTLIERS; i++)
        fprintf(stderr, "%5.4f ", (float)noutliers[i]/qv);
    fprintf(stderr, "\n");
    fclose(fin); fclose(fout); if (fsv != NULL) fclose(fsv); if(fpitch != NULL) fclose(fpitch);
}


// print vector debug function

void pv(char s[], float v[]) {
    int i;
    if (verbose) {
        fprintf(stderr, "%s",s);
        for(i=0; i<NB_BANDS; i++)
            fprintf(stderr, "%4.2f ", v[i]);
        fprintf(stderr, "\n");
    }
}

void quant_pred(float vec_out[],  /* prev quant vector, and output */
                float vec_in[],
                float pred,
                int num_stages,
                float vq[],
                int m[], int k)
{
    float err[k], w[k], se, se1, se2;
    int i,s,ind;

    pv("\nvec_in: ", vec_in);
    pv("vec_out: ", vec_out);
    se1 = 0.0;
    for(i=0; i<k; i++) {
        err[i] = (vec_in[i] - pred*vec_out[i]);
        se1 += err[i]*err[i];
        vec_out[i] = pred*vec_out[i];
        w[i] = 1.0;
    }
    se1 /= k;
    pv("err: ", err);
    if (fsv != NULL) fprintf(fsv, "%f\t%f\t", vec_in[0],sqrt(se1));
    for(s=0; s<num_stages; s++) {
        ind = quantise(&vq[s*k*MAX_ENTRIES], err, w, k, m[s], &se);
        pv("entry: ", &vq[s+k*MAX_ENTRIES+ind*k]);
        se2 = 0.0;
        for(i=0; i<k; i++) {
            err[i] -= vq[s*k*MAX_ENTRIES+ind*k+i];
            se2 += err[i]*err[i];
            vec_out[i] += vq[s*k*MAX_ENTRIES+ind*k+i];
        }
        se2 /= k;
        if (fsv != NULL) fprintf(fsv, "%f\t", sqrt(se2));
        if (verbose) fprintf(stderr, "se1: %f se2: %f s: %d/%d m[s]: %d ind: %d\n", se1, se2, s, num_stages, m[s], ind);
        pv("err: ", err);
        pv("vec_out: ",vec_out);
    }
    if (fsv != NULL) fprintf(fsv, "\n");
}

// mbest algorithm version

void quant_pred_mbest(float vec_out[],  /* prev quant vector, and output */
                      float vec_in[],
                      float pred,
                      int num_stages,
                      float vq[],
                      int m[], int k,
                      int mbest_survivors)
{
    float err[k], w[k], se1, se2;
    int i,j,s,s1,ind;
    
    struct MBEST *mbest_stage[num_stages];
    int index[num_stages];
    float target[k];
    
    for(i=0; i<num_stages; i++) {
        mbest_stage[i] = mbest_create(mbest_survivors, num_stages);
        index[i] = 0;
    }

    /* predict based on last frame */
    
    se1 = 0.0;
    for(i=0; i<k; i++) {
        err[i] = (vec_in[i] - pred*vec_out[i]);
        se1 += err[i]*err[i];
        vec_out[i] = pred*vec_out[i];
        w[i] = 1.0;
    }
    se1 /= k;
    
    /* now quantise err[] using multi-stage mbest search, preserving
       mbest_survivors at each stage */
    
    mbest_search(vq, err, w, k, m[0], mbest_stage[0], index);
    if (verbose) MBEST_PRINT("Stage 1:", mbest_stage[0]);
    
    for(s=1; s<num_stages; s++) {

        /* for each candidate in previous stage, try to find best vector in next stage */
        for (j=0; j<mbest_survivors; j++) {
            /* indexes that lead us this far */
            for(s1=0; s1<s; s1++) {
                index[s1+1] = mbest_stage[s-1]->list[j].index[s1];
            }
            /* target is residual err[] vector given path to this candidate */
            for(i=0; i<k; i++)
                target[i] = err[i];
            for(s1=0; s1<s; s1++) {
                ind = index[s-s1];
                if (verbose) fprintf(stderr, "   s: %d s1: %d s-s1: %d ind: %d\n", s,s1,s-s1,ind);
                for(i=0; i<k; i++) {
                    target[i] -= vq[s1*k*MAX_ENTRIES+ind*k+i];
                }
            }
            pv("   target: ", target);
            mbest_search(&vq[s*k*MAX_ENTRIES], target, w, k, m[s], mbest_stage[s], index);
        }
        char str[80]; sprintf(str,"Stage %d:", s+1);
        if (verbose) MBEST_PRINT(str, mbest_stage[s]);
    }

    /* OK put it all back together using best survivor */

    pv("\n  vec_in: ", vec_in);
    pv("  vec_out: ", vec_out);
    pv("    err: ", err);
    if (fsv != NULL) fprintf(fsv, "%f\t%f\t", vec_in[0],sqrt(se1));
    if (verbose) fprintf(stderr, "    se1: %f\n", se1);
   
    for(s=0; s<num_stages; s++) {
        ind = mbest_stage[num_stages-1]->list[0].index[num_stages-1-s];
        se2 = 0.0;
        for(i=0; i<k; i++) {
            err[i] -= vq[s*k*MAX_ENTRIES+ind*k+i];
            vec_out[i] += vq[s*k*MAX_ENTRIES+ind*k+i];
            se2 += err[i]*err[i];
        }
        se2 /= k;
        if (fsv != NULL) fprintf(fsv, "%f\t", sqrt(se2));
        pv("    err: ", err);
        if (verbose) fprintf(stderr, "    se2: %f\n", se2);
    }
    pv("  vec_out: ",vec_out);

    if (fsv != NULL) fprintf(fsv, "\n");
    
    for(i=0; i<num_stages; i++)
        mbest_destroy(mbest_stage[i]);
}

/*---------------------------------------------------------------------------*\

  quantise

  Quantises vec by choosing the nearest vector in codebook cb, and
  returns the vector index.  The squared error of the quantised vector
  is added to se.

  \*---------------------------------------------------------------------------*/

int quantise(const float * cb, float vec[], float w[], int k, int m, float *se)
/* float   cb[][K];	current VQ codebook		*/
/* float   vec[];	vector to quantise		*/
/* float   w[];         weighting vector                */
/* int	   k;		dimension of vectors		*/
/* int     m;		size of codebook		*/
/* float   *se;		accumulated squared error 	*/
{
    float   e;		/* current error		*/
    long	   besti;	/* best index so far		*/
    float   beste;	/* best error so far		*/
    long	   j;
    int     i;
    float   diff;

    besti = 0;
    beste = 1E32;
    for(j=0; j<m; j++) {
	e = 0.0;
	for(i=0; i<k; i++) {
	    diff = cb[j*k+i]-vec[i];
	    e += powf(diff*w[i],2.0);
	}
	if (e < beste) {
	    beste = e;
	    besti = j;
	}
    }

    *se += beste;

    return(besti);
}

