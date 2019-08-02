/*
  lpcnet_freedv.c
  David Rowe April 2019

  LPCNet API functions for FreeDV.
*/

#include "arch.h"
#include "lpcnet_dump.h"
#include "lpcnet_quant.h"
#include "freq.h"
// NB_FEATURES has a different value in lpcnet.h, need to reconcile some time
#undef NB_FEATURES 
#include "lpcnet.h"
#include "lpcnet_freedv.h"
#include "lpcnet_freedv_internal.h"

LPCNetFreeDV* lpcnet_freedv_create(int direct_split) {
    LPCNetFreeDV *lf = (LPCNetFreeDV*)malloc(sizeof(LPCNetFreeDV));
    if (lf == NULL) return NULL;
    lf->d = lpcnet_dump_create();
    lf->q = lpcnet_quant_create(direct_split);
    lf->net = lpcnet_create();
    return lf;
}

void lpcnet_freedv_destroy(LPCNetFreeDV *lf) {
    lpcnet_dump_destroy(lf->d); lpcnet_destroy(lf->net); lpcnet_quant_destroy(lf->q);
    free(lf);
}

void lpcnet_enc(LPCNetFreeDV *lf, short *pcm, char *frame) {
    LPCNET_DUMP  *d = lf->d;
    LPCNET_QUANT *q = lf->q;
    float x[FRAME_SIZE];
    float features[LPCNET_NB_FEATURES];
   
    for (int j=0; j<q->dec; j++) {
        for (int i=0;i<FRAME_SIZE;i++) x[i] = pcm[i];
        pcm += FRAME_SIZE;
        
        lpcnet_dump(d,x,features);

        /* optionally convert cepstrals to log magnitudes */
        if (q->logmag) {
            float tmp[NB_BANDS];
            idct(tmp, features);
            for(int i=0; i<NB_BANDS; i++) features[i] = tmp[i];
        }
        
        if ((q->f % q->dec) == 0) {
            lpcnet_features_to_frame(q, features, frame);
        }
        q->f++;
    }
}

void lpcnet_dec(LPCNetFreeDV *lf, char *frame, short* pcm)
{
    LPCNET_QUANT *q = lf->q;
    LPCNetState *net = lf->net;
    float in_features[NB_TOTAL_FEATURES];
    float features[NB_TOTAL_FEATURES];

    for(int d=0; d<q->dec; d++) {
        lpcnet_frame_to_features(q, frame, in_features);
        /* optionally log magnitudes convert back to cepstrals */
        if (q->logmag) {
            float tmp[NB_BANDS];
            dct(tmp, in_features);
            for(int i=0; i<NB_BANDS; i++) in_features[i] = tmp[i];
        }
       
        RNN_COPY(features, in_features, NB_TOTAL_FEATURES);
        RNN_CLEAR(&features[18], 18);
        lpcnet_synthesize(net, pcm, features, FRAME_SIZE, 0);
        pcm += FRAME_SIZE;
    }
}

int lpcnet_samples_per_frame(LPCNetFreeDV *lf) { return FRAME_SIZE*lf->q->dec; } 
int lpcnet_bits_per_frame(LPCNetFreeDV *lf) { return lf->q->bits_per_frame; } 
