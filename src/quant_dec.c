/*
  quant_dec.c
  David Rowe Feb 2019

  Prototype decoder, derived from quant_test.c
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

extern int num_stages;
extern float vq[MAX_STAGES*NB_BANDS*MAX_ENTRIES];
extern int   m[MAX_STAGES];

int main(int argc, char *argv[]) {
    FILE *fin, *fout;
    float features_out[NB_FEATURES];
    int f = 0, dec = 3;
    float features_quant[NB_FEATURES], err[NB_BANDS];
    int   indexes[MAX_STAGES];
    int d,i;
    float fract;

    int c, k=NB_BANDS;
    float pred = 0.9;
    
    int   mbest_survivors = 5;
    /* weight applied to first cepstral */
    float weight = 1.0/sqrt(NB_BANDS);    
    int   pitch_bits = 6;

    //for(i=0; i<MAX_STAGES*NB_BANDS*MAX_ENTRIES; i++) vq[i] = 0.0;
    
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
    
    /* adjacent vectors used for linear interpolation, note only 0..17 and 38,39 used */
    float features_lin[2][NB_FEATURES];
    
    for(d=0; d<2; d++)
        for(i=0; i<NB_FEATURES; i++)
            features_lin[d][i] = 0.0;
    for(i=0; i<NB_FEATURES; i++)
        features_quant[i] = 0.0;

    for(i=0; i<NB_BANDS; i++) err[i] = 0.0;
    
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

    int bits_read = 0;
    
    do {
        
        // clear output features to make sure we are not cheating.
        // Note we cant clear quant_out as we need memory of last
        // frames output for pred quant
        
        for(i=0; i<NB_FEATURES; i++)
            features_out[i] = 0.0;

        int pitch_ind, pitch_gain_ind;
        
        /* decoder */
        
        if ((f % dec) == 0) {

            bits_read = fread(frame, sizeof(char), bits_per_frame, fin);

            /* non-interpolated frame ----------------------------------------*/

            unpack_frame(num_stages, m, indexes, pitch_bits, &pitch_ind, &pitch_gain_ind, frame);
            quant_pred_output(features_quant, indexes, err, pred, num_stages, vq, k);

            features_quant[2*NB_BANDS] = pitch_decode(pitch_bits, pitch_ind);
            features_quant[2*NB_BANDS+1] = pitch_gain_decode(pitch_gain_ind);
            
            /* update linear interpolation arrays */
            for(i=0; i<NB_FEATURES; i++) {
                features_lin[0][i] = features_lin[1][i];
                features_lin[1][i] = features_quant[i];                
            }

            /* pass  frame through */
            for(i=0; i<NB_BANDS; i++) {
                features_out[i] = features_lin[0][i];
            }
            features_out[2*NB_BANDS]   = features_lin[0][2*NB_BANDS];
            features_out[2*NB_BANDS+1] = features_lin[0][2*NB_BANDS+1];            

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

        /* convert cepstrals back from dB */
        for(i=0; i<NB_BANDS; i++)
            features_out[i] *= 1/10.0;

        /* need to recompute LPCs after every frame, as we have quantised, or interpolated */
        lpc_from_cepstrum(&features_out[2*NB_BANDS+3], features_out);

        for(i=0; i<NB_FEATURES; i++) {
            if (isnan(features_out[i])) {
                fprintf(stderr, "f: %d i: %d\n", f, i);
                exit(0);
            }
        }
        
        fwrite(features_out, sizeof(float), NB_FEATURES, fout);
        fflush(stdin);
        fflush(stdout);
        
    } while(bits_read);

    fprintf(stderr,"f: %d\n",f);
    
    fclose(fin); fclose(fout);
}


