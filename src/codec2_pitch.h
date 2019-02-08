/*---------------------------------------------------------------------------*\

  FILE........: codec2_pitch.h
  AUTHOR......: David Rowe
  DATE CREATED: 23/3/93 (!) Modified for LPCNet 2019

  Codec 2 'NLP" Pitch estimator module for LPCNet.

\*---------------------------------------------------------------------------*/

#ifndef __CODEC2_PITCH__
#define __CODEC2_PITCH__

typedef struct CODEC2_PITCH_S CODEC2_PITCH;
CODEC2_PITCH *codec2_pitch_create(int *Sn_size, int *new_samples_each_call);
int codec2_pitch_est(CODEC2_PITCH *pitch, float Sn[], float *f0, float *voicing);
void codec2_pitch_destroy(CODEC2_PITCH *pitch);

#endif
