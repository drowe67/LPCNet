/*
   lpcnet_dump.c
   Feb 2019

   LPCnet "dump" functions is API form.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "kiss_fft.h"
#include "common.h"
#include <math.h>
#include "freq.h"
#include "pitch.h"
#include "arch.h"
#include "celt_lpc.h"
#include <assert.h>

#include "lpcnet_dump.h"

#define PITCH_MIN_PERIOD 32
#define PITCH_MAX_PERIOD 256
#define PITCH_FRAME_SIZE 320
#define PITCH_BUF_SIZE (PITCH_MAX_PERIOD+PITCH_FRAME_SIZE)

#define CEPS_MEM 8
#define NB_DELTA_CEPS 6

#define NB_FEATURES (2*NB_BANDS+3+LPC_ORDER)

struct DenoiseState_s {
  float analysis_mem[OVERLAP_SIZE];
  float cepstral_mem[CEPS_MEM][NB_BANDS];
  float pitch_buf[PITCH_BUF_SIZE];
  float last_gain;
  int last_period;
  float lpc[LPC_ORDER];
  float sig_mem[LPC_ORDER];
  int exc_mem;
};

static int rnnoise_get_size() {
  return sizeof(DenoiseState);
}

static int rnnoise_init(DenoiseState *st) {
  memset(st, 0, sizeof(*st));
  return 0;
}

static DenoiseState *rnnoise_create() {
  DenoiseState *st;
  st = malloc(rnnoise_get_size());
  rnnoise_init(st);
  return st;
}

static void rnnoise_destroy(DenoiseState *st) {
  free(st);
}

static short float2short(float x)
{
  int i;
  i = (int)floor(.5+x);
  return IMAX(-32767, IMIN(32767, i));
}

int lowpass = FREQ_SIZE;
int band_lp = NB_BANDS;

static void frame_analysis(DenoiseState *st, kiss_fft_cpx *X, float *Ex, const float *in) {
  int i;
  float x[WINDOW_SIZE];
  RNN_COPY(x, st->analysis_mem, OVERLAP_SIZE);
  RNN_COPY(&x[OVERLAP_SIZE], in, FRAME_SIZE);
  RNN_COPY(st->analysis_mem, &in[FRAME_SIZE-OVERLAP_SIZE], OVERLAP_SIZE);
  apply_window(x);
  forward_transform(X, x);
  for (i=lowpass;i<FREQ_SIZE;i++)
    X[i].r = X[i].i = 0;
  compute_band_energy(Ex, X);
}

static void compute_frame_features(DenoiseState *st, kiss_fft_cpx *X, 
                                  float *Ex, float *features, const float *in) {
  int i;
  float E = 0;
  float Ly[NB_BANDS];
  float pitch_buf[PITCH_BUF_SIZE];
  int pitch_index;
  float gain;
  float follow, logMax;
  float g;

  for(i=0; i<NB_FEATURES; i++) features[i] = 0.0;

  frame_analysis(st, X, Ex, in);

  /* keep LPCnet pitch est for now as we are using pitch gain */
  
  RNN_MOVE(st->pitch_buf, &st->pitch_buf[FRAME_SIZE], PITCH_BUF_SIZE-FRAME_SIZE);
  RNN_COPY(&st->pitch_buf[PITCH_BUF_SIZE-FRAME_SIZE], in, FRAME_SIZE);
  RNN_COPY(pitch_buf, &st->pitch_buf[0], PITCH_BUF_SIZE);
  pitch_downsample(pitch_buf, PITCH_BUF_SIZE);
  pitch_search(pitch_buf+PITCH_MAX_PERIOD, pitch_buf, PITCH_FRAME_SIZE<<1,
               (PITCH_MAX_PERIOD-3*PITCH_MIN_PERIOD)<<1, &pitch_index);
  pitch_index = 2*PITCH_MAX_PERIOD-pitch_index;
  gain = remove_doubling(pitch_buf, 2*PITCH_MAX_PERIOD, 2*PITCH_MIN_PERIOD,
          2*PITCH_FRAME_SIZE, &pitch_index, st->last_period, st->last_gain);
  st->last_period = pitch_index;
  st->last_gain = gain;

  /* smoothing of band energies Ly */
  
  logMax = -2;
  follow = -2;
  for (i=0;i<NB_BANDS;i++) {
    Ly[i] = log10(1e-2+Ex[i]);
    Ly[i] = MAX16(logMax-8, MAX16(follow-2.5, Ly[i]));
    logMax = MAX16(logMax, Ly[i]);
    follow = MAX16(follow-2.5, Ly[i]);
    E += Ex[i];
  }

  dct(features, Ly);
  features[0] -= 4;
  g = lpc_from_cepstrum(st->lpc, features);

  /* load up used features */
  
  features[2*NB_BANDS] = .01*(pitch_index-200);
  features[2*NB_BANDS+1] = gain;
  features[2*NB_BANDS+2] = log10(g);
  for (i=0;i<LPC_ORDER;i++) features[2*NB_BANDS+3+i] = st->lpc[i];
}

static void biquad(float *y, float mem[2], const float *x, const float *b, const float *a, int N) {
  int i;
  for (i=0;i<N;i++) {
    float xi, yi;
    xi = x[i];
    yi = x[i] + mem[0];
    mem[0] = mem[1] + (b[0]*(double)xi - a[0]*(double)yi);
    mem[1] = (b[1]*(double)xi - a[1]*(double)yi);
    y[i] = yi;
  }
}

static void preemphasis(float *y, float *mem, const float *x, float coef, int N) {
  int i;
  for (i=0;i<N;i++) {
    float yi;
    yi = x[i] + *mem;
    *mem = -coef*x[i];
    y[i] = yi;
  }
}

LPCNET_DUMP *lpcnet_dump_create(void) {
    LPCNET_DUMP *d = malloc(sizeof(LPCNET_DUMP));
    if (d == NULL) return NULL;

    d->st = rnnoise_create();

    d->mem_hp_x[0] = d->mem_hp_x[1] = 0.0;
    d->mem_preemph = 0.0;
    int i;
    for(i=0; i<FRAME_SIZE; i++) d->tmp[i] = 0;

    /* fire up Codec 2 pitch estimator */

    d->c2pitch = codec2_pitch_create(&d->c2_Sn_size, &d->c2_frame_size);
    assert(FRAME_SIZE == d->c2_frame_size);
    d->c2_Sn = (float*)malloc(sizeof(float)*d->c2_Sn_size);
    assert(d->c2_Sn != NULL);
    for(i=0; i<d->c2_Sn_size; i++) d->c2_Sn[i] = 0.0;

    assert(LPCNET_NB_FEATURES == NB_FEATURES);
    return d;
}
    
void lpcnet_dump_destroy(LPCNET_DUMP *d) {
    codec2_pitch_destroy(d->c2pitch);    
    free(d->c2_Sn);
    rnnoise_destroy(d->st);
    free(d);
}

void lpcnet_dump(LPCNET_DUMP *d, float x[], float features[])
{    
    int i;
    static const float a_hp[2] = {-1.99599, 0.99600};
    static const float b_hp[2] = {-2, 1};
    kiss_fft_cpx X[FREQ_SIZE];
    float Ex[NB_BANDS];

    biquad(x, d->mem_hp_x, x, b_hp, a_hp, FRAME_SIZE);
    preemphasis(x, &d->mem_preemph, x, PREEMPHASIS, FRAME_SIZE);
    for (i=0;i<FRAME_SIZE;i++) x[i] += rand()/(float)RAND_MAX - .5;
    compute_frame_features(d->st, X, Ex, features, x);

    /* inject pitch from Codec 2 pitch estimator */
    
    int c2_Sn_size = d->c2_Sn_size;
    int c2_frame_size = d->c2_frame_size;
    float *c2_Sn = d->c2_Sn;
    for(i=0; i<c2_Sn_size-c2_frame_size; i++)
        c2_Sn[i] = c2_Sn[i+c2_frame_size];
    for(i=0; i<c2_frame_size; i++)
        c2_Sn[i+c2_Sn_size-c2_frame_size] = x[i];
    float f0, voicing; int pitch_index;
    pitch_index = codec2_pitch_est(d->c2pitch, c2_Sn, &f0, &voicing);
    if (pitch_index >= 2*PITCH_MAX_PERIOD) pitch_index = 2*PITCH_MAX_PERIOD-1;
    if (pitch_index < 2*PITCH_MIN_PERIOD) pitch_index = 2*PITCH_MIN_PERIOD;
    features[2*NB_BANDS] = 0.01*(pitch_index-200);
}
    
