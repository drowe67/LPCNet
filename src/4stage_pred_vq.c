#include "lpcnet_quant.h"
int pred_num_stages = 4;
int pred_m[MAX_STAGES] = {2048, 2048, 2048, 2048, 0};
 float pred_vq[MAX_STAGES*NB_BANDS*MAX_ENTRIES] = {
 