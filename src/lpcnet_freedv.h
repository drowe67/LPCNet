/*
  lpcnet_freedv.h
  David Rowe April 2019

  LPCNet API functions for FreeDV.
*/

#ifndef __LPCNET_FREEDV__
#define __LPCNET_FREEDV__

#ifdef __cplusplus
  extern "C" {
#endif

// possible vq_type values in lpcnet_freedv_create()
#define LPCNET_PRED                   0
#define LPCNET_DIRECT_SPLIT           1
#define LPCNET_DIRECT_SPLIT_INDEX_OPT 2
      
typedef struct LPCNetFreeDV LPCNetFreeDV;

LPCNetFreeDV* lpcnet_freedv_create(int vq_type);
void lpcnet_freedv_destroy(LPCNetFreeDV *lf);
void lpcnet_enc(LPCNetFreeDV *lf, short *pcm, char *frame);
void lpcnet_dec(LPCNetFreeDV *lf, char *frame, short* pcm);
int lpcnet_bits_per_frame(LPCNetFreeDV *lf);
int lpcnet_samples_per_frame(LPCNetFreeDV *lf);
char *lpcnet_get_hash(void);

#ifdef __cplusplus
}
#endif

#endif
