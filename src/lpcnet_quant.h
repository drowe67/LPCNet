/*
  lpcnet_quant.h
  David Rowe Feb 2019

  David's experimental quanisation functions for LPCNet.
*/

#ifndef __LPCNET_QUANT__
#define __LPCNET_QUANT__

#define NB_FEATURES    55   /* length of feature vector (only a subset used) */
#define NB_BANDS       18   /* number of bands quantised                     */
#define MAX_ENTRIES    4096 /* max number of vectors per stage               */
#define MAX_STAGES     5    /* max number of VQ stages                       */

#include <stdio.h>

typedef struct {
    float weight;         /* weight applied to first cepstral              */
    float pred;           /* prediction coefficient                        */
    int   num_stages;     /* number of VQ stages                           */
    int   *m;             /* VQ entries per stage                          */
    float *vq;            /* vector quantiser                              */
    int   mbest;          /* number of survivors for multi-stage VQ search */
    int   pitch_bits;     /* number of bits used for quantising pitch      */
    int   dec;            /* decimation rate 1,2,3...                      */
    int   f;              /* frame counter                                 */
    int   logmag;
    
    int   bits_per_frame;
    float features_quant[NB_FEATURES];    /* memory of quantised features from previous frame   */
    float features_lin[2][NB_FEATURES];   /* adjacent frames features for linear interpolation  */
} LPCNET_QUANT;

// Two sorts of VQs available
extern int   pred_num_stages;
extern float pred_vq[MAX_STAGES*NB_BANDS*MAX_ENTRIES];
extern int   pred_m[MAX_STAGES];
extern int   direct_split_num_stages;
extern float direct_split_vq[MAX_STAGES*NB_BANDS*MAX_ENTRIES];
extern int   direct_split_m[MAX_STAGES];

LPCNET_QUANT *lpcnet_quant_create(int direct_split);
void lpcnet_quant_destroy(LPCNET_QUANT *q);
void lpcnet_quant_compute_bits_per_frame(LPCNET_QUANT *q);

// debug/instrumentation globals
extern FILE *lpcnet_fsv;
extern int lpcnet_verbose;

void quant_pred(float vec_out[],  /* prev quant vector, and output */
                float vec_in[],
                float pred,
                int num_stages,
                float vq[],
                int m[], int k);

void quant_pred_mbest(float vec_out[],  /* prev quant vector, and output, need to keep this between calls */
                      int   indexes[],  /* indexes to transmit */
                      float vec_in[],
                      float pred,
                      int num_stages,
                      float vq[],
                      int m[], int k,
                      int mbest_survivors);

void quant_pred_output(float vec_out[],
                       int   indexes[],
                       float err[],      /* used for development, set to zeros in real world decode side */
                       float pred,
                       int num_stages,
                       float vq[],
                       int k);

int pitch_encode(float pitch_feature, int pitch_bits);
float pitch_decode(int pitch_bits, int q);
int pitch_gain_encode(float pitch_gain_feature);
float pitch_gain_decode(int pitch_bits);
void pack_frame(int num_stages, int m[], int indexes[], int pitch_bits, int pitch_ind, int pitch_gain_ind, char frame[]);
void unpack_frame(int num_stages, int m[], int indexes[], int pitch_bits, int *pitch_ind, int *pitch_gain_ind, char frame[]);

int lpcnet_features_to_frame(LPCNET_QUANT *q, float features[], char frame[]);
void lpcnet_frame_to_features(LPCNET_QUANT *q, char frame[],float features[]);

#endif
