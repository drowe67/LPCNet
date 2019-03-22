/*
  quant_test.c
  David Rowe Jan 2019

  Test program for porting from quant_feat to stand alone encoder/decoder.
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

// use predictive quantiser for this test
#define num_stages pred_num_stages 
#define vq pred_vq 
#define m pred_m 

extern int num_stages;
extern float vq[MAX_STAGES*NB_BANDS*MAX_ENTRIES];
extern int   m[MAX_STAGES];

int main(int argc, char *argv[]) {
    FILE *fin, *fout;
    float features[NB_FEATURES], features_out[NB_FEATURES];
    int f = 0, dec = 3;
    float features_quant[NB_FEATURES], features_quant_[NB_FEATURES], err[NB_BANDS];
    int   indexes[MAX_STAGES];
    float sum_sq_err = 0.0;
    int d,i,n = 0;
    float fract;

    int c, k=NB_BANDS;
    float pred = 0.9;
    
    int   mbest_survivors = 5;
    char label[80] = "";
    /* weight applied to first cepstral */
    float weight = 1.0/sqrt(NB_BANDS);    
    int   pitch_bits = 6;

    //for(i=0; i<MAX_STAGES*NB_BANDS*MAX_ENTRIES; i++) vq[i] = 0.0;
    
    static struct option long_options[] = {
        {"decimate",   required_argument, 0, 'd'},
        {"label",      required_argument, 0, 'l'},
        {"pitchquant", required_argument, 0, 'o'},
        {"pred",       required_argument, 0, 'p'},
        {"stagevar",   required_argument, 0, 's'},
        {"verbose",    no_argument,       0, 'v'},
        {0, 0, 0, 0}
    };

    int opt_index = 0;

    while ((c = getopt_long (argc, argv, "d:l:o:p:s:v", long_options, &opt_index)) != -1) {
        switch (c) {
        case 's':
            /* text file to dump error (variance) per stage */
            lpcnet_fsv = fopen(optarg, "wt"); assert(lpcnet_fsv != NULL);            
            break;
        case 'd':
            dec = atoi(optarg);
            fprintf(stderr, "dec = %d\n", dec);
            break;
        case 'l':
            /* text label to pront with results */
            strcpy(label, optarg);
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
            fprintf(stderr,"  [-l --label txtLabel]\n");
            fprintf(stderr,"  [-m --mbest survivors]\n  [-o --pitchbits nBits]\n");
            fprintf(stderr,"  [-p --pred predCoff]\n [-s --stagevar TxtFile]\n");
            fprintf(stderr,"  [-e --extpitch ExtPitchFile]\n [-v --verbose]\n");
            exit(1);
        }
    }

    int bits_per_frame = pitch_bits + 2;
    for(i=0; i<num_stages; i++)
        bits_per_frame += log2(m[i]);
    char frame[bits_per_frame];
    fprintf(stderr, "dec: %d pred: %3.2f num_stages: %d mbest: %d bits_per_frame: %d bit_rate: %5.2f",
            dec, pred, num_stages, mbest_survivors, bits_per_frame, (float)bits_per_frame/(dec*0.01));
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

    /* decoder */
    for(i=0; i<NB_FEATURES; i++)
        features_quant_[i] = 0.0;
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
        
        /* convert cepstrals to dB */
        for(i=0; i<NB_BANDS; i++)
            features[i] *= 10.0;

        /* optional weight on first cepstral which increases at
           sqrt(NB_BANDS) for every dB of speech input power.  Note by
           doing it here, we won't be measuring SD of this step, SD
           results will be on weighted vector. */
        features[0] *= weight;
                   
        /* maintain delay line of unquantised features for partial quantisation and distortion measure */
        for(d=0; d<dec; d++)
            for(i=0; i<NB_FEATURES; i++)
                features_prev[d][i] = features_prev[d+1][i];
        for(i=0; i<NB_FEATURES; i++)
            features_prev[dec][i] = features[i];

        // clear output features to make sure we are not cheating.
        // Note we cant clear quant_out as we need memory of last
        // frames output for pred quant
        
        for(i=0; i<NB_FEATURES; i++)
            features_out[i] = 0.0;

        int pitch_ind, pitch_gain_ind;
        
        /* encoder */
        
        if ((f % dec) == 0) {
            /* non-interpolated frame ----------------------------------------*/

            quant_pred_mbest(features_quant_, indexes, features, pred, num_stages, vq, m, k, mbest_survivors);
            pitch_ind = pitch_encode(features[2*NB_BANDS], pitch_bits);
            pitch_gain_ind =  pitch_gain_encode(features[2*NB_BANDS+1]);
            pack_frame(num_stages, m, indexes, pitch_bits, pitch_ind, pitch_gain_ind, frame);

        }

        /* decoder */
        
        if ((f % dec) == 0) {
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
    fclose(fin); fclose(fout); if (lpcnet_fsv != NULL) fclose(lpcnet_fsv);
}


