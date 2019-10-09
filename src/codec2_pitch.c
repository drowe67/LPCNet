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
#define MAX_AMP    160		/* maximum number of harmonics          */

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

/* Structure to hold Codec 2 model parameters for one frame */

typedef struct {
    float Wo;		  /* fundamental frequency estimate in radians  */
    int   L;		  /* number of harmonics                        */
    float A[MAX_AMP+1];	  /* amplitiude of each harmonic                */
    float phi[MAX_AMP+1]; /* phase of each harmonic                     */
    int   voiced;	  /* non-zero if this frame is voiced           */
} MODEL;

struct CODEC2_PITCH_S {
    C2CONST       c2const;
    kiss_fft_cfg  fft_fwd_cfg;
    float         prev_f0;
    void         *nlp_states;
    float        *w;	                /* time domain hamming window */
    COMP          W[FFT_ENC];	        /* DFT of w[] */
};

/* prototypes for internal functions in libcodec2 */

C2CONST c2const_create(int Fs, float framelength_ms);
void make_analysis_window(C2CONST *c2const, kiss_fft_cfg fft_fwd_cfg, float w[], COMP W[]);
void dft_speech(C2CONST *c2const, kiss_fft_cfg fft_fwd_cfg, COMP Sw[], float Sn[], float w[]);
void *nlp_create(C2CONST *c2const);
void nlp_destroy(void *nlp_state);
float nlp(void *nlp_state, float Sn[], int n, 
	  float *pitch_samples, COMP Sw[], COMP W[], float *prev_f0);
void two_stage_pitch_refinement(C2CONST *c2const, MODEL *model, COMP Sw[]);
void estimate_amplitudes(MODEL *model, COMP Sw[], COMP W[], int est_phase);
float est_voicing_mbe(C2CONST *c2const, MODEL *model, COMP Sw[], COMP W[]);

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

int codec2_pitch_est(CODEC2_PITCH *pitch, float Sn[], float *f0, float *voicing)
{
    COMP  Sw[FFT_ENC];	        /* DFT of Sn[] */
    float pitch_samples, snr;
    MODEL model;
    
    *f0 = nlp(pitch->nlp_states, Sn, pitch->c2const.n_samp, &pitch_samples, Sw, pitch->W, &pitch->prev_f0);
    model.Wo = 2.0*M_PI/pitch_samples;
    dft_speech(&pitch->c2const, pitch->fft_fwd_cfg, Sw, Sn, pitch->w);
    two_stage_pitch_refinement(&pitch->c2const, &model, Sw);
    pitch_samples = 2.0*M_PI/model.Wo;
    estimate_amplitudes(&model, Sw, pitch->W, 1);
    snr = est_voicing_mbe(&pitch->c2const, &model, Sw, pitch->W);

    *voicing = 1.0 - 2.0/pow(10.0, snr/10.0);
    if (*voicing < 0.0) *voicing = 0.0;
    return round(2.0*pitch_samples);
}

void codec2_pitch_destroy(CODEC2_PITCH *pitch)
{
    nlp_destroy(pitch->nlp_states);
    free(pitch->w);
    free(pitch);
}

