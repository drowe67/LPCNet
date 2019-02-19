/*
  quant_test.c
  David Rowe Feb 2019

  Prototype encoder, derived from quant_test.c
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

extern int num_stages;
extern float vq[MAX_STAGES*NB_BANDS*MAX_ENTRIES];
extern int   m[MAX_STAGES];

int main(int argc, char *argv[]) {
    FILE *fin, *fout;
    float features[NB_FEATURES];
    int f = 0, dec = 3;
    float features_quant[NB_FEATURES];
    int   indexes[MAX_STAGES];
    int i;

    int c, k=NB_BANDS;
    float pred = 0.9;
    
    int   mbest_survivors = 5;
    /* weight applied to first cepstral */
    float weight = 1.0/sqrt(NB_BANDS);    
    int   pitch_bits = 6;
    
    static struct option long_options[] = {
        {"decimate",   required_argument, 0, 'd'},
        {"numstages",  required_argument, 0, 'n'},
        {"pitchquant", required_argument, 0, 'o'},
        {"pred",       required_argument, 0, 'p'},
        {"verbose",    no_argument,       0, 'v'},
        {0, 0, 0, 0}
    };

    int opt_index = 0;

    while ((c = getopt_long (argc, argv, "d:n:o:p:v", long_options, &opt_index)) != -1) {
        switch (c) {
        case 'd':
            dec = atoi(optarg);
            fprintf(stderr, "dec = %d\n", dec);
            break;
        case 'n':
            num_stages = atoi(optarg);
            fprintf(stderr, "%d VQ stages\n",  num_stages);
            break;
        case 'o':
            pitch_bits = atoi(optarg);
            fprintf(stderr, "pitch quantised to %d bits\n",  pitch_bits);
            break;
        case 'p':
            pred = atof(optarg);
            fprintf(stderr, "pred = %f\n", pred);
            break;
        case 'v':
            lpcnet_verbose = 1;
            break;
         default:
            fprintf(stderr,"usage: %s [Options]:\n  [-d --decimation 1/2/3...]\n", argv[0]);
            fprintf(stderr,"  [-n --numstages]\n  [-o --pitchbits nBits]\n");
            fprintf(stderr,"  [-p --pred predCoff]\n");
            fprintf(stderr,"  [-v --verbose]\n");
            exit(1);
        }
    }

    int bits_per_frame = pitch_bits + 2;
    for(i=0; i<num_stages; i++)
        bits_per_frame += log2(m[i]);
    char frame[bits_per_frame];
    fprintf(stderr, "dec: %d pred: %3.2f num_stages: %d mbest: %d bits_per_frame: %d frame: %2d ms bit_rate: %5.2f bits/s",
            dec, pred, num_stages, mbest_survivors, bits_per_frame, dec*10, (float)bits_per_frame/(dec*0.01));
    fprintf(stderr, "\n");

    /* memory of quant values from prev frame */
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

    int bits_written = 0;
    
    while(fread(features, sizeof(float), NB_FEATURES, fin) == NB_FEATURES) {
       
        for(i=0; i<NB_FEATURES; i++) {
            if (isnan(features[i])) {
                fprintf(stderr, "f: %d i: %d\n", f, i);
            }
        }
        
        /* convert cepstrals to dB */
        for(i=0; i<NB_BANDS; i++)
            features[i] *= 10.0;

        /* optional weight on first cepstral which increases at
           sqrt(NB_BANDS) for every dB of speech input power.  Note by
           doing it here, we won't be measuring SD of this step, SD
           results will be on weighted vector. */
        features[0] *= weight;
                   
        int pitch_ind, pitch_gain_ind;
        
        /* encoder */
        
        if ((f % dec) == 0) {
            /* non-interpolated frame ----------------------------------------*/

            quant_pred_mbest(features_quant, indexes, features, pred, num_stages, vq, m, k, mbest_survivors);
            pitch_ind = pitch_encode(features[2*NB_BANDS], pitch_bits);
            pitch_gain_ind =  pitch_gain_encode(features[2*NB_BANDS+1]);
            pack_frame(num_stages, m, indexes, pitch_bits, pitch_ind, pitch_gain_ind, frame);
            bits_written += fwrite(frame, sizeof(char), bits_per_frame, fout);
        }
        f++;
        
        fflush(stdin);
        fflush(stdout);
    }

    fprintf(stderr, "bits_written %d\n", bits_written);
    fclose(fin); fclose(fout); if (lpcnet_fsv != NULL) fclose(lpcnet_fsv);
}


