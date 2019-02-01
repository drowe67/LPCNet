/*---------------------------------------------------------------------------*\

  FILE........: codec2_pitch.c
  AUTHOR......: David Rowe
  DATE CREATED: 23/3/93 (!) Modified for LPCNet 2019

  Codec 2 'NLP" Pitch estimator module so we can try it out with LPCNet.

\*---------------------------------------------------------------------------*/

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "codec2_pitch.h"
#include "codec2_kiss_fft.h"

#define FFT_ENC    512		/* size of FFT used for encoder         */
#define P_MAX_S    0.0200	/* maximum pitch period in s            */
#define N_S        0.01         /* internal proc frame length in secs   */

typedef struct {
  float real;
  float imag;
} COMP;

typedef struct {
    int   Fs;            /* sample rate of this instance             */
    int   n_samp;        /* number of samples per 10ms frame at Fs   */
    int   max_amp;       /* maximum number of harmonics              */
    int   m_pitch;       /* pitch estimation window size in samples  */
    int   p_min;         /* minimum pitch period in samples          */
    int   p_max;         /* maximum pitch period in samples          */
    float Wo_min;        
    float Wo_max;  
    int   nw;            /* analysis window size in samples          */      
    int   tw;            /* trapezoidal synthesis window overlap     */
} C2CONST;

struct CODEC2_PITCH_S {
    C2CONST       c2const;
    kiss_fft_cfg  fft_fwd_cfg;
    float         prev_f0;
    void         *nlp_states;
    float        *w;	                /* time domain hamming window */
    COMP          W[FFT_ENC];	        /* DFT of w[] */
};

C2CONST c2const_create(int Fs, float framelength_ms);
void make_analysis_window(C2CONST *c2const, kiss_fft_cfg fft_fwd_cfg, float w[], COMP W[]);
void dft_speech(C2CONST *c2const, kiss_fft_cfg fft_fwd_cfg, COMP Sw[], float Sn[], float w[]);
void *nlp_create(C2CONST *c2const);
void nlp_destroy(void *nlp_state);
float nlp(void *nlp_state, float Sn[], int n, 
	  float *pitch_samples, COMP Sw[], COMP W[], float *prev_f0);

CODEC2_PITCH *codec2_pitch_create(int *Sn_size, int *new_samples_each_call)
{
    CODEC2_PITCH *pitch = (CODEC2_PITCH*)malloc(sizeof(CODEC2_PITCH));
    assert(pitch != NULL);
    int Fs = 16000;
    pitch->c2const = c2const_create(Fs, N_S);    
    pitch->w = (float*)malloc(sizeof(float)*pitch->c2const.m_pitch);
    pitch->nlp_states = nlp_create(&pitch->c2const);
    pitch->fft_fwd_cfg = kiss_fft_alloc(FFT_ENC, 0, NULL, NULL);
    make_analysis_window(&pitch->c2const, pitch->fft_fwd_cfg, pitch->w, pitch->W);
    pitch->prev_f0 = 1/P_MAX_S;

    *Sn_size = pitch->c2const.m_pitch;
    *new_samples_each_call = pitch->c2const.n_samp;
        
    return pitch;
}

/* returns an estimate of the pitch period, input is a buffer of samples on length pitch->m */

int codec2_pitch_est(CODEC2_PITCH *pitch, float Sn[], float *f0)
{
    COMP  Sw[FFT_ENC];	        /* DFT of Sn[] */
    float pitch_samples;

    dft_speech(&pitch->c2const, pitch->fft_fwd_cfg, Sw, Sn, pitch->w);
    *f0 = nlp(pitch->nlp_states, Sn, pitch->c2const.n_samp, &pitch_samples, Sw, pitch->W, &pitch->prev_f0);

    return (int)2*pitch_samples;
}

void codec2_pitch_destroy(CODEC2_PITCH *pitch)
{
    nlp_destroy(pitch->nlp_states);
    free(pitch->w);
    free(pitch);
}

