/*
  lpcnet_quant.h
  David Rowe Feb 2019

  David's experimental quanisation functions for LPCNet
*/

#ifndef __LPCNET_QUANT__
#define __LPCNET_QUANT__

#define NB_BANDS       18
#define MAX_ENTRIES    4096 /* max number of vectors per stage */

// debug/instrumentation globals
extern FILE *lpcnet_fsv;
extern int lpcnet_verbose;

int quantise(const float * cb, float vec[], float w[], int k, int m, float *se);

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

#endif
