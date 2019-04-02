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

#define MAX_STAGES     5    /* max number of VQ stages                */
#define NOUTLIERS      5    /* range of outilers to track in 1dB steps */

extern int   pred_num_stages;
extern float pred_vq[MAX_STAGES*NB_BANDS*MAX_ENTRIES];
extern int   pred_m[MAX_STAGES];

int main(int argc, char *argv[]) {
    FILE *fin, *fout;
    float features[NB_FEATURES];
    int   dec = 3;
    int   c;
    float pred = 0.9;
    
    int   mbest_survivors = 5;
    /* weight applied to first cepstral */
    float weight = 1.0/sqrt(NB_BANDS);    
    int   pitch_bits = 6;
    int num_stages = pred_num_stages;
    
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
    q->weight = weight; q->pred = pred; q->mbest = mbest_survivors;
    q->pitch_bits = pitch_bits; q->dec = dec;
    lpcnet_quant_compute_bits_per_frame(q);
    
    char frame[q->bits_per_frame];
    fprintf(stderr, "dec: %d pred: %3.2f num_stages: %d mbest: %d bits_per_frame: %d frame: %2d ms bit_rate: %5.2f bits/s",
            q->dec, q->pred, q->num_stages, q->mbest, q->bits_per_frame, dec*10, (float)q->bits_per_frame/(dec*0.01));
    fprintf(stderr, "\n");

    fin = stdin;
    fout = stdout;

    int bits_written = 0;
    
    while(fread(features, sizeof(float), NB_FEATURES, fin) == NB_FEATURES) {
        if ((q->f % q->dec) == 0) {
            lpcnet_features_to_frame(q, features, frame);
            bits_written += fwrite(frame, sizeof(char), q->bits_per_frame, fout);
        }
        q->f++;
        
        fflush(stdin);
        fflush(stdout);
    }

    fprintf(stderr, "bits_written %d\n", bits_written);
    fclose(fin); fclose(fout); lpcnet_quant_destroy(q);
}


