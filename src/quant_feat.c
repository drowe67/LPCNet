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

/*
   how to load in quantisers
   could be multiple stages, comma delimited list?
 */

int main(int argc, char *argv[]) {
    FILE *fin, *fout;
    float features[NB_FEATURES], features_out[NB_FEATURES];
    int f = 0, dec = 2;
    float features_prev[dec+1][NB_FEATURES];
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

    while ((c = getopt (argc, argv, "dq:")) != -1) {
        switch (c) {
        case 'd':
            dec = atoi(optarg);
            break;
        case 'q':
            /* list of comma delimited file names */
            strcpy(fnames, optarg);
            p = fnames;
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
                num_stages = 0;
                m[num_stages] = 0;
                while (fread(features, sizeof(float), NB_BANDS, fq) == NB_BANDS) m[num_stages]++;
                fprintf(stderr, "%d entries of vectors width %d\n", m[num_stages], NB_BANDS);
                rewind(fq);                       
                int rd = fread(&vq[num_stages], sizeof(float), m[num_stages]*NB_BANDS, fq);
                assert(rd == m[num_stages]*NB_BANDS);
                num_stages++;
                fclose(fq);
            } while(comma);
            break;
        default:
            fprintf(stderr,"usage: %s [-d decimation] [-q quantfile1,quantfile2,....]", argv[0]);
            exit(1);
        }
    }
  
    for(d=0; d<dec+1; d++)
        for(i=0; i<NB_FEATURES; i++)
            features_prev[d][i] = 0.0;
    
    fin = stdin;
    fout = stdout;

    /* dec == 2:
        In.:      f2     f3     f4    f5     f6
        Out: .... f0 (f0+f2)/2) f2 (f2+f4)/2 f4 ....
    */
    
    /* dec == 3:
        In.:      f3        f4           f5       f6       f7
        Out: .... f0  2f0/3 + f3/3  f0/3 + 2f2/3  f3  2f6/3 + f9/3
    */

    /* If quantiser enabled, prediction is betwwen every dec vectors */

    while(fread(features, sizeof(float), NB_FEATURES, fin) == NB_FEATURES) {
        for(d=0; d<dec; d++)
            for(i=0; i<NB_FEATURES; i++)
                features_prev[d][i] = features_prev[d+1][i];
        for(i=0; i<NB_FEATURES; i++)
            features_prev[dec][i] = features[i];
        
        if ((f % dec) == 0) {
            /* pass frame though */
            for(i=0; i<NB_FEATURES; i++)
                features_out[i] = features_prev[dec-1][i];
        }
        else {
            for(i=0; i<NB_FEATURES; i++)
                features_out[i] = 0.0;
            /* interpolate frame */
            for(i=0; i<NB_BANDS; i++) {
                fract = (float)(f%dec)/(float)dec;
                features_out[i] = (1.0-fract)*features_prev[0][i] + fract*features_prev[dec][i];
                sum_sq_err += pow(10.0*(features_out[i]-features_prev[dec-1][i]), 2.0);
                n++;
            }
            float g = lpc_from_cepstrum(&features_out[2*NB_BANDS+3], features_out);

            /* pass some features through at the original (undecimated) sample rate for now */
            
            features_out[2*NB_BANDS] = features_prev[dec-1][2*NB_BANDS];   /* original undecimated pitch      */
            features_out[2*NB_BANDS+1] = features_prev[dec-1][2*NB_BANDS]; /* original undecimated gain       */
            features_out[2*NB_BANDS+2] = log10(g);                         /* original undecimated LPC energy */
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
