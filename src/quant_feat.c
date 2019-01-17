/*
 quant_feat.c

 Tool for processing a .f32 file of LPCNet features to simulate quantisation.

 Quantises and decimates/interpolate LPCNet features on stdin, and sends output to stdout.
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>

#include "common.h"
#include "freq.h"

#define NB_FEATURES    55
#define NB_BANDS       18
#define MAX_STAGES     5
#define MAX_ENTRIES    4096

int quantise(const float * cb, float vec[], float w[], int k, int m, float *se);
void quant_pred(float vec_out[],  /* prev quant vector, and output */
                float vec_in[],
                float pred,
                int num_stages,
                float vq[][NB_BANDS*MAX_ENTRIES],
                int m[]);

int main(int argc, char *argv[]) {
    FILE *fin, *fout;
    float features[NB_FEATURES], features_out[NB_FEATURES];
    int f = 0, dec = 2;
    /* delay line so we can pass some features (like pitch and voicing) through unmodified */
    /* adjacent vectors used for linear interpolation */
    float features_lin[2][NB_BANDS];
    float features_quant[NB_BANDS];
    float sum_sq_err = 0.0;
    int d,i,n = 0;
    float fract;

    int c;
    int num_stages = 0;
    float vq[MAX_STAGES][NB_BANDS*MAX_ENTRIES];
    int   m[MAX_STAGES];
    
    char fnames[256];
    char fn[256];
    char *comma, *p;
    FILE *fq;

    opterr = 0;

    while ((c = getopt (argc, argv, "d:q:")) != -1) {
        switch (c) {
        case 'd':
            dec = atoi(optarg);
            fprintf(stderr, "dec = %d\n", dec);
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
                while (fread(features, sizeof(float), NB_BANDS, fq) == NB_BANDS) m[num_stages]++;
                assert(m[num_stages] <= MAX_ENTRIES);
                fprintf(stderr, "%d entries of vectors width %d\n", m[num_stages], NB_BANDS);
                rewind(fq);                       
                int rd = fread(&vq[num_stages], sizeof(float), m[num_stages]*NB_BANDS, fq);
                assert(rd == m[num_stages]*NB_BANDS);
                num_stages++;
                fclose(fq);
            } while(comma);
            break;
        default:
            fprintf(stderr,"usage: %s [-d decimation] [-q quantfile1,quantfile2,....]\n", argv[0]);
            exit(1);
        }
    }
  
    float features_prev[dec+1][NB_FEATURES];
    
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
        In.:          f3        f4          f5       f6       f7
        Out: ....     f0  2f0/3 + f3/3  f0/3 + 2f2/3  f3  2f3/3 + f6/3

        features_prev
        3             f3        f4          f5       f6       f7
        2             f2        f3          f4       f5       f6     
        1             f1        f2          f3       f4       f5
        0             f0        f1          f2       f3       f4

        features_lin
        1             f3        f3          f3       f6       f6
        0             f0        f0          f0       f3       f3
    */

    /* If quantiser enabled, prediction is betwwen every dec vectors */

    while(fread(features, sizeof(float), NB_FEATURES, fin) == NB_FEATURES) {
        /* maintain delay line of unquantised features for partial quantisation and distortion measure */
        for(d=0; d<dec; d++)
            for(i=0; i<NB_FEATURES; i++)
                features_prev[d][i] = features_prev[d+1][i];
        for(i=0; i<NB_FEATURES; i++)
            features_prev[dec][i] = features[i];
        
        if ((f % dec) == 0) {

            /* optional quantisation */
            if (num_stages) {
                quant_pred(features_quant, features, 0.9, num_stages, vq, m);
            }
            else {
                for(i=0; i<NB_BANDS; i++)
                    features_quant[i] = features[i];
            }

            /* update linear interpolation arrays */
            for(i=0; i<NB_BANDS; i++) {
                features_lin[0][i] = features_lin[1][i];
                features_lin[1][i] = features_quant[i];                
            }

            /* pass (quantised) frame though */
            for(i=0; i<NB_BANDS; i++) {
                features_out[i] = features_lin[0][i];
                sum_sq_err += pow(10.0*(features_out[i]-features_prev[0][i]), 2.0); n++;
            }
        }
        else {
            for(i=0; i<NB_FEATURES; i++)
                features_out[i] = 0.0;
            /* interpolate frame */
            d = f%dec;
            for(i=0; i<NB_BANDS; i++) {
                fract = (float)d/(float)dec;
                features_out[i] = (1.0-fract)*features_lin[0][i] + fract*features_lin[1][i];
                //features_out[i] = features_prev[0][i];
                sum_sq_err += pow(10.0*(features_out[i]-features_prev[0][i]), 2.0);
                n++;
            }

            /* set up LPCs from interpolated cepstrals */
            float g = lpc_from_cepstrum(&features_out[2*NB_BANDS+3], features_out);

            /* pass some features through at the original (undecimated) sample rate for now */
            
            features_out[2*NB_BANDS]   = features_prev[0][2*NB_BANDS];   /* original undecimated pitch      */
            features_out[2*NB_BANDS+1] = features_prev[0][2*NB_BANDS+1]; /* original undecimated gain       */
            features_out[2*NB_BANDS+2] = log10(g);                       /* original undecimated LPC energy */
        }
        f++;
        
        fwrite(features_out, sizeof(float), NB_FEATURES, fout);
        fflush(stdin);
        fflush(stdout);
    }

    float var = sum_sq_err/(n*NB_BANDS);
    fprintf(stderr, "var: %f sd: %f n: %d\n", var, sqrt(var), n);
    fclose(fin); fclose(fout);
}

void pv(char s[], float v[]) {
    int i;
    fprintf(stderr, "%s",s);
    for(i=0; i<NB_BANDS; i++)
            fprintf(stderr, "%4.2f ", v[i]);
    fprintf(stderr, "\n");            
}

void quant_pred(float vec_out[],  /* prev quant vector, and output */
                float vec_in[],
                float pred,
                int num_stages,
                float vq[][NB_BANDS*MAX_ENTRIES],
                int m[])
{
    float err[NB_BANDS], w[NB_BANDS], se, se1, se2;
    int i,s,ind;

    pv("\nvec_in: ", vec_in);
    pv("vec_out: ", vec_out);
    se1 = 0.0;
    for(i=0; i<NB_BANDS; i++) {
        err[i] = 10.0*(vec_in[i] - pred*vec_out[i]);
        //err[i] = vq[0][NB_BANDS+i];
        se1 += err[i]*err[i];
        vec_out[i] = pred*vec_out[i];
        w[i] = 1.0;
    }
    se1 /= NB_BANDS;
    pv("err: ", err);
    for(s=0; s<num_stages; s++) {
        ind = quantise(&vq[s][0], err, w, NB_BANDS, m[s], &se);
        pv("entry: ", &vq[s][ind*NB_BANDS]);
        se2 = 0.0;
        for(i=0; i<NB_BANDS; i++) {
            err[i] -= vq[s][ind*NB_BANDS+i];
            se2 += err[i]*err[i];
            vec_out[i] += vq[s][ind*NB_BANDS+i]/10.0;
        }
        se2 /= NB_BANDS;
        fprintf(stderr, "se1: %f se2: %f s: %d/%d m[s]: %d ind: %d\n", se1, se2, s, num_stages, m[s], ind);
        pv("err: ", err);
        pv("vec_out: ",vec_out);
    }
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

