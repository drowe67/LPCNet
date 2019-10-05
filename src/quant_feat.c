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
#include "lpcnet_quant.h"

#define NB_FEATURES    55
#define MAX_STAGES     5    /* max number of VQ stages                */
#define NOUTLIERS      5    /* range of outilers to track in 1dB steps */

#define PITCH_MIN_PERIOD 32
#define PITCH_MAX_PERIOD 256

int main(int argc, char *argv[]) {
    FILE *fin, *fout;
    float features[NB_FEATURES], features_out[NB_FEATURES];
    int f = 0, dec = 2;
    float features_quant[NB_FEATURES];
    int   indexes[MAX_STAGES];
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
    float uniform_step2 = 0.0;
    int   mbest_survivors = 0;
    char label[80] = "";
    /* experimental limits for dctLy[0], first cepstral */
    float lower_limit = -200.0;
    float upper_limit =  200.00;
    /* weight applied to first cepstral */
    float weight = 1.0;    
    float pitch_gain_bias = 0.0;
    int   pitch_bits = 0;
    int   small_vec = 0;
    int   logmag = 0;
    
    for(i=0; i<MAX_STAGES*NB_BANDS*MAX_ENTRIES; i++) vq[i] = 0.0;
    
    static struct option long_options[] = {
        {"small",      required_argument, 0, 'a'},
        {"decimate",   required_argument, 0, 'd'},
        {"extpitch",   required_argument, 0, 'e'},
        {"first",      required_argument, 0, 'f'},
        {"gain",       required_argument, 0, 'g'},
        {"hard",       required_argument, 0, 'h'},
        {"label",      required_argument, 0, 'l'},
        {"mbest",      required_argument, 0, 'm'},
        {"mag",        required_argument, 0, 'i'},
        {"pitchquant", required_argument, 0, 'o'},
        {"pred",       required_argument, 0, 'p'},
        {"quant",      required_argument, 0, 'q'},
        {"stagevar",   required_argument, 0, 's'},
        {"uniform",    required_argument, 0, 'u'},
        {"verbose",    no_argument,       0, 'v'},
        {"uniform2",   required_argument, 0, 'x'},
        {"weight",     no_argument,       0, 'w'},
        {0, 0, 0, 0}
    };

    int opt_index = 0;
    
    while ((c = getopt_long (argc, argv, "ad:q:vs:f:p:e:u:l:m:h:wg:o:ix:", long_options, &opt_index)) != -1) {
        switch (c) {
        case 'a':
            /* small cpectral vectors - zero out several bands */
            small_vec = 1;
            break;
       case 'f':
            /* start VQ at band first+1 */
            first = atoi(optarg);
            k = NB_BANDS-first;
            fprintf(stderr, "first = %d k = %d\n", first, k);
            break;
        case 's':
            /* text file to dump error (variance) per stage */
            lpcnet_fsv = fopen(optarg, "wt"); assert(lpcnet_fsv != NULL);            
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
            pitch_gain_bias = atof(optarg);
            fprintf(stderr, "pitch_gain bias: %f\n", pitch_gain_bias);
            break;
        case 'h':
            /* hard limit (saturate) first feature (energy) */
            lower_limit = atof(optarg);            
            fprintf(stderr, "lower_limit: %f upper_limit: %f\n", lower_limit, upper_limit);
            break;
        case 'i':
            /* work in log mag rather than cepstral domain */
            logmag = 1;
            break;
        case 'l':
            /* text label to pront with results */
            strcpy(label, optarg);
            break;
        case 'm':
            mbest_survivors = atoi(optarg);
            fprintf(stderr, "mbest_survivors = %d\n",  mbest_survivors);
            break;
        case 'o':
            pitch_bits = atoi(optarg);
            fprintf(stderr, "pitch quantised to %d bits\n",  pitch_bits);
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
                /* count how many entries m of dimension k are in this VQ file */
                m[num_stages] = 0;
                while (fread(features, sizeof(float), k, fq) == (size_t)k)
                    m[num_stages]++;
                assert(m[num_stages] <= MAX_ENTRIES);
                fprintf(stderr, "%d entries of vectors width %d\n", m[num_stages], k);
                /* now load VQ into memory */
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
            uniform_step2 = uniform_step;
            break;
        case 'x':
            uniform_step2 = atof(optarg);
            fprintf(stderr, "uniform quant step size 12..17: %3.2f dB\n", uniform_step2);
            break;
        case 'v':
            lpcnet_verbose = 1;
            break;
        case 'w':
            weight = 1.0/sqrt(NB_BANDS);
            break;
         default:
            fprintf(stderr,"usage: %s [Options]:\n  [-d --decimation 1/2/3...]\n  [-q --quant quantfile1,quantfile2,....]\n", argv[0]);
            fprintf(stderr,"  [-g --gain pitch gain bias]\n");
            fprintf(stderr,"  [-h --hard lowerLimitdB\n");
            fprintf(stderr,"  [-i --mag\n");
            fprintf(stderr,"  [-l --label txtLabel]\n");
            fprintf(stderr,"  [-m --mbest survivors]\n  [-o --pitchbits nBits]\n");
            fprintf(stderr,"  [-p --pred predCoff]\n  [-f --first firstElement]\n  [-s --stagevar TxtFile]\n");
            fprintf(stderr,"  [-e --extpitch ExtPitchFile]\n  [-u --uniform stepSizedB]\n  [-v --verbose]\n");
            fprintf(stderr,"  [-w --weight first cepstral by 1/sqrt(NB_BANDS)\n");
            exit(1);
        }
    }

    fprintf(stderr, "dec: %d pred: %3.2f num_stages: %d mbest: %d small: %d logmag: %d",
            dec, pred, num_stages, mbest_survivors, small_vec, logmag);
    fprintf(stderr, "\n");
    
    /* delay line so we can pass some features (like pitch and voicing) through unmodified */
    float features_prev[dec+1][NB_FEATURES];
    /* adjacent vectors used for linear interpolation, note only 0..17 and 38,39 used */
    float features_lin[2][NB_FEATURES];
    
    for(d=0; d<dec+1; d++)
        for(i=0; i<NB_FEATURES; i++)
            features_prev[d][i] = 0.0;
    for(d=0; d<2; d++)
        for(i=0; i<NB_FEATURES; i++)
            features_lin[d][i] = 0.0;
    for(i=0; i<NB_FEATURES; i++)
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
       
        for(i=0; i<NB_FEATURES; i++) {
            if (isnan(features[i])) {
                fprintf(stderr, "f: %d i: %d\n", f, i);
            }
        }

        /* optionally convert cepstrals to log magnitudes */
        if (logmag) {
            float tmp[NB_BANDS];
            idct(tmp, features);
            for(i=0; i<NB_BANDS; i++) features[i] = tmp[i];
        }
        
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

        /* optionally pitch gain bias - but I would prefer a non-magic numbers approach */
        features[2*NB_BANDS+1] += pitch_gain_bias;
           
        /* maintain delay line of unquantised features for partial quantisation and distortion measure */
        for(d=0; d<dec; d++)
            for(i=0; i<NB_FEATURES; i++)
                features_prev[d][i] = features_prev[d+1][i];
        for(i=0; i<NB_FEATURES; i++)
            features_prev[dec][i] = features[i];

        // clear outpout features to make sure we are not cheating.
        // Note we cant clear quant_out as we need memory of last
        // frames output for pred quant
        
        for(i=0; i<NB_FEATURES; i++)
            features_out[i] = 0.0;

        if ((f % dec) == 0) {
            /* non-interpolated frame ----------------------------------------*/

            /* optional quantisation */
            if (num_stages || (uniform_step != 0.0)) {
                if (num_stages) {
                    if (mbest_survivors) {
                        /* mbest predictive VQ */
                        quant_pred_mbest(&features_quant[first], indexes, &features[first], pred, num_stages, vq, m, k, mbest_survivors);
                    }
                    else {
                        /* standard predictive VQ */
                        quant_pred(&features_quant[first], &features[first], pred, num_stages, vq, m, k);
                    }
                    for(i=0; i<first; i++)
                        features_quant[i] = features[i];
                }
                if (uniform_step != 0.0) {
                    for(i=0; i<12; i++) {
                        features_quant[i] = uniform_step*round(features[i]/uniform_step);
                    }
                    for(; i<NB_BANDS; i++) {
                        features_quant[i] = uniform_step2*round(features[i]/uniform_step2);
                    }
                }
            }
            else {
                /* unquantised */
                for(i=0; i<NB_BANDS; i++) {
                    features_quant[i] = features[i];
                }
            }

            if (pitch_bits) {
                int ind =  pitch_encode(features[2*NB_BANDS], pitch_bits);
                features_quant[2*NB_BANDS] = pitch_decode(pitch_bits, ind);
                ind =  pitch_gain_encode(features[2*NB_BANDS+1]);
                features_quant[2*NB_BANDS+1] = pitch_gain_decode(ind);
            }
            else {
                features_quant[2*NB_BANDS] = features[2*NB_BANDS]; 
                features_quant[2*NB_BANDS+1] = features[2*NB_BANDS+1];   /* pitch gain */
            }
            
            /* update linear interpolation arrays */
            for(i=0; i<NB_FEATURES; i++) {
                features_lin[0][i] = features_lin[1][i];
                features_lin[1][i] = features_quant[i];                
            }

            /* pass (quantised) frame through */
            for(i=0; i<NB_BANDS; i++) {
                features_out[i] = features_lin[0][i];
            }
            features_out[2*NB_BANDS]   = features_lin[0][2*NB_BANDS];
            features_out[2*NB_BANDS+1] = features_lin[0][2*NB_BANDS+1];

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
            

        } else {
            /* interpolated frame ----------------------------------------*/
            
            for(i=0; i<NB_FEATURES; i++)
                features_out[i] = 0.0;
            /* interpolate frame */
            d = f%dec;
            for(i=0; i<NB_FEATURES; i++) {
                fract = (float)d/(float)dec;
                features_out[i] = (1.0-fract)*features_lin[0][i] + fract*features_lin[1][i];
            }

        }
        
        f++;
        
        features_out[0] /= weight;    

        /* convert cespstrals back from dB */
        for(i=0; i<NB_BANDS; i++)
            features_out[i] *= 1/10.0;

        /* optionally log magnitudes convert back to cepstrals */
        if (logmag) {
            float tmp[NB_BANDS];
            dct(tmp, features_out);
            for(i=0; i<NB_BANDS; i++) features_out[i] = tmp[i];
        }

        for(i=0; i<NB_FEATURES; i++) {
            if (isnan(features_out[i])) {
                fprintf(stderr, "f: %d i: %d\n", f, i);
                exit(0);
            }
        }
        
        if (small_vec) {
            /* zero out unused cepstrals in small vec mode */
            for(i=12; i<NB_BANDS; i++)
                features_out[i] = 0.0;
        }
        
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
    fclose(fin); fclose(fout); if (lpcnet_fsv != NULL) fclose(lpcnet_fsv); if(fpitch != NULL) fclose(fpitch);
}


