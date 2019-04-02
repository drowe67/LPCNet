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

extern int pred_num_stages;
extern float pred_vq[MAX_STAGES*NB_BANDS*MAX_ENTRIES];
extern int   pred_m[MAX_STAGES];

int main(int argc, char *argv[]) {
    FILE *fin, *fout;
    float features_out[NB_FEATURES];
    int i, c, dec = 3;
    float pred = 0.9;
    
    /* weight applied to first cepstral */
    float weight = 1.0/sqrt(NB_BANDS);    
    int   pitch_bits = 6;

    int num_stages = pred_num_stages;
    
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

    LPCNET_QUANT *q = lpcnet_quant_create(0);
    q->weight = weight; q->pred = pred; 
    q->pitch_bits = pitch_bits; q->dec = dec;
    lpcnet_quant_compute_bits_per_frame(q);
    
    char frame[q->bits_per_frame];
    fprintf(stderr, "dec: %d pred: %3.2f num_stages: %d mbest: %d bits_per_frame: %d frame: %2d ms bit_rate: %5.2f bits/s",
            q->dec, q->pred, q->num_stages, q->mbest, q->bits_per_frame, dec*10, (float)q->bits_per_frame/(dec*0.01));
    fprintf(stderr, "\n");
       
    fin = stdin;
    fout = stdout;

    int bits_read = 0;
    
    do {
        
        if ((q->f % q->dec) == 0)
            bits_read = fread(frame, sizeof(char), q->bits_per_frame, fin);
        lpcnet_frame_to_features(q, frame, features_out);

        for(i=0; i<NB_FEATURES; i++) {
            if (isnan(features_out[i])) {
                fprintf(stderr, "f: %d i: %d\n", q->f, i);
                exit(0);
            }
        }
        
        fwrite(features_out, sizeof(float), NB_FEATURES, fout);
        fflush(stdin);
        fflush(stdout);
        
    } while(bits_read);

    fprintf(stderr,"f: %d\n",q->f);
    
    fclose(fin); fclose(fout);
}


