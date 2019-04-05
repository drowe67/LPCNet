/*
  lpcnet_freedv.h
  David Rowe April 2019

  LPCNet API functions for FreeDV.
*/

#ifndef __LPCNET_FREEDV__
#define __LPCNET_FREEDV__

typedef struct LPCNetFreeDV LPCNetFreeDV;

LPCNetFreeDV* lpcnet_freedv_create(int direct_split);
void lpcnet_freedv_destroy(LPCNetFreeDV *lf);
void lpcnet_enc(LPCNetFreeDV *lf, short *pcm, char *frame);
void lpcnet_dec(LPCNetFreeDV *lf, char *frame, short* pcm);
int lpcnet_bits_per_frame(LPCNetFreeDV *lf);
int lpcnet_samples_per_frame(LPCNetFreeDV *lf);

#endif
