/*
   lpcnet_enc.c
   Feb 2019

   LPCNet to bit stream encoder, takes 16 kHz signed 16 bit speech
   samples on stdin, outputs fully quantised bit stream on stdout (in
   1 bit per char format).
*/

/* Copyright (c) 2017-2018 Mozilla */
/*
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <getopt.h>
#include "lpcnet_freedv.h"
#include "lpcnet_dump.h"
#include "lpcnet_quant.h"
#include "lpcnet_freedv_internal.h"

int main(int argc, char **argv) {
    FILE *fin, *fout;

    /* quantiser defaults */

    int   dec = 3;
    float pred = 0.9;    
    int   mbest_survivors = 5;
    float weight = 1.0/sqrt(NB_BANDS);    
    int   pitch_bits = 6;
    int   num_stages = pred_num_stages;
    int   *m = pred_m;
    float *vq = pred_vq;
    int   logmag = 0;
    int   direct_split = 0;
    
    fin = stdin;
    fout = stdout;

    /* quantiser options */
    
    static struct option long_options[] = {
        {"infile",       required_argument, 0, 'i'},
        {"outfile",      required_argument, 0, 'u'},
        {"decimate",     required_argument, 0, 'd'},
        {"numstages",    required_argument, 0, 'n'},
        {"pitchquant",   required_argument, 0, 'o'},
        {"pred",         required_argument, 0, 'p'},
        {"directsplit",  no_argument,       0, 's'},
        {"verbose",      no_argument,       0, 'v'},
        {0, 0, 0, 0}
    };

    int   c;
    int opt_index = 0;

    while ((c = getopt_long (argc, argv, "d:n:o:p:svi:u:", long_options, &opt_index)) != -1) {
        switch (c) {
	case 'i':
            if ((fin = fopen(optarg, "rb")) == NULL) {
                fprintf(stderr, "Couldn't open input file: %s\n", optarg);
                exit(1);
            }
            break;
	case 'u':
            if ((fout = fopen(optarg, "wb")) == NULL) {
                fprintf(stderr, "Couldn't open output file: %s\n", optarg);
                exit(1);
            }
            break;
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
        case 's':
            direct_split = 1;
            m = direct_split_m; vq = direct_split_vq; pred = 0.0; logmag = 1; weight = 1.0;
            fprintf(stderr, "split VQ\n");
            break;
        case 'v':
            lpcnet_verbose = 1;
            break;
         default:
            fprintf(stderr,"usage: %s [Options]:\n  [-d --decimation 1/2/3...]\n", argv[0]);
            fprintf(stderr,"  [-i --infile]\n  [-u --outfile]\n");
            fprintf(stderr,"  [-n --numstages]\n  [-o --pitchbits nBits]\n");
            fprintf(stderr,"  [-p --pred predCoff] [-s --split]\n");
            fprintf(stderr,"  [-v --verbose]\n");
            exit(1);
        }
    }

    LPCNetFreeDV *lf = lpcnet_freedv_create(direct_split);
    LPCNET_QUANT *q = lf->q;

    q->weight = weight; q->pred = pred; q->mbest = mbest_survivors;
    q->pitch_bits = pitch_bits; q->dec = dec; q->m = m; q->vq = vq; q->num_stages = num_stages;
    q->logmag = logmag;
    lpcnet_quant_compute_bits_per_frame(q);
    
    fprintf(stderr, "dec: %d pred: %3.2f num_stages: %d mbest: %d bits_per_frame: %d frame: %2d ms bit_rate: %5.2f bits/s",
            q->dec, q->pred, q->num_stages, q->mbest, q->bits_per_frame, dec*10, (float)q->bits_per_frame/(dec*0.01));
    fprintf(stderr, "\n");

    char frame[lpcnet_bits_per_frame(lf)];
    int f=0;
    int bits_written=0;
    short pcm[lpcnet_samples_per_frame(lf)];

    while (1) {      
        int nread = fread(pcm, sizeof(short), lpcnet_samples_per_frame(lf), fin);
        if (nread != lpcnet_samples_per_frame(lf)) break;

        lpcnet_enc(lf, pcm, frame);
        bits_written += fwrite(frame, sizeof(char), lpcnet_bits_per_frame(lf), fout);

        fflush(stdin);
        fflush(stdout);
        f++;
    }

    lpcnet_freedv_destroy(lf);
    fprintf(stderr, "bits_written %d\n", bits_written);
    fclose(fin); fclose(fout);
    return 0;
}

