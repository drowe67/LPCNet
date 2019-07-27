/*
  lpcnet_quant.c
  David Rowe Feb 2019

  David's experimental quanisation functions for LPCNet
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "lpcnet_quant.h"
#include "mbest.h"
#include "freq.h"

FILE *lpcnet_fsv = NULL;
int lpcnet_verbose = 0;

#define PITCH_MIN_PERIOD 32
#define PITCH_MAX_PERIOD 256

// defaults
#define DEFAULT_WEIGHT     1.0/sqrt(NB_BANDS)
#define DEFAULT_PRED       0.9
#define DEFAULT_NUM_STAGES 4
#define DEFAULT_MBEST      5
#define DEFAULT_PITCH_BITS 6
#define DEFAULT_DEC        3

static int quantise(const float * cb, float vec[], float w[], int k, int m, float *se);

LPCNET_QUANT *lpcnet_quant_create(int direct_split) {
    LPCNET_QUANT *q = (LPCNET_QUANT*)malloc(sizeof(LPCNET_QUANT));
    if (q == NULL) return NULL;
    if (direct_split) {
        q->weight = 1.0; q->pred = 0.0; 
        q->mbest = DEFAULT_MBEST; q->pitch_bits = DEFAULT_PITCH_BITS; q->dec = DEFAULT_DEC;
        q->num_stages = direct_split_num_stages; q->vq = direct_split_vq; q->m = direct_split_m; q->logmag = 1;
    }
    else {
        q->weight = DEFAULT_WEIGHT; q->pred = DEFAULT_PRED; 
        q->mbest = DEFAULT_MBEST; q->pitch_bits = DEFAULT_PITCH_BITS; q->dec = DEFAULT_DEC;
        q->num_stages = pred_num_stages; q->vq = pred_vq; q->m = pred_m; q->logmag = 0;
    }
    lpcnet_quant_compute_bits_per_frame(q);

    int i,d;
    for(i=0; i<NB_FEATURES; i++) q->features_quant[i] = 0.0;
    for(d=0; d<2; d++)
        for(i=0; i<NB_FEATURES; i++)
            q->features_lin[d][i] = 0.0;
    q->f = 0;
    return q;
}

// call this if you change any parameters from default
void lpcnet_quant_compute_bits_per_frame(LPCNET_QUANT *q) {
    int i;
    q->bits_per_frame = q->pitch_bits + 2;
    for(i=0; i<q->num_stages; i++)
        q->bits_per_frame += log2(q->m[i]);
}

void lpcnet_quant_destroy(LPCNET_QUANT *q) { free(q); }

// print vector debug function

void pv(char s[], float v[]) {
    int i;
    if (lpcnet_verbose) {
        fprintf(stderr, "%s",s);
        for(i=0; i<NB_BANDS; i++)
            fprintf(stderr, "%4.2f ", v[i]);
        fprintf(stderr, "\n");
    }
}

void quant_pred(float vec_out[],  /* prev quant vector, and output */
                float vec_in[],
                float pred,
                int num_stages,
                float vq[],
                int m[], int k)
{
    float err[k], w[k], se, se1, se2;
    int i,s,ind;

    pv("\nvec_in: ", vec_in);
    pv("vec_out: ", vec_out);
    se1 = 0.0;
    for(i=0; i<k; i++) {
        err[i] = (vec_in[i] - pred*vec_out[i]);
        se1 += err[i]*err[i];
        vec_out[i] = pred*vec_out[i];
        w[i] = 1.0;
    }
    se1 /= k;
    pv("err: ", err);
    if (lpcnet_fsv != NULL) fprintf(lpcnet_fsv, "%f\t%f\t", vec_in[0],sqrt(se1));
    for(s=0; s<num_stages; s++) {
        ind = quantise(&vq[s*k*MAX_ENTRIES], err, w, k, m[s], &se);
        pv("entry: ", &vq[s+k*MAX_ENTRIES+ind*k]);
        se2 = 0.0;
        for(i=0; i<k; i++) {
            err[i] -= vq[s*k*MAX_ENTRIES+ind*k+i];
            se2 += err[i]*err[i];
            vec_out[i] += vq[s*k*MAX_ENTRIES+ind*k+i];
        }
        se2 /= k;
        if (lpcnet_fsv != NULL) fprintf(lpcnet_fsv, "%f\t", sqrt(se2));
        if (lpcnet_verbose) fprintf(stderr, "se1: %f se2: %f s: %d/%d m[s]: %d ind: %d\n", se1, se2, s, num_stages, m[s], ind);
        pv("err: ", err);
        pv("vec_out: ",vec_out);
    }
    if (lpcnet_fsv != NULL) fprintf(lpcnet_fsv, "\n");
}

// mbest algorithm version

void quant_pred_mbest(float vec_out[],
                      int   indexes[],
                      float vec_in[],
                      float pred,
                      int num_stages,
                      float vq[],
                      int m[], int k,
                      int mbest_survivors)
{
    float err[k], w[k], se1;
    int i,j,s,s1,ind;
    
    struct MBEST *mbest_stage[num_stages];
    int index[num_stages];
    float target[k];
    
    for(i=0; i<num_stages; i++) {
        mbest_stage[i] = lpcnet_mbest_create(mbest_survivors, num_stages);
        index[i] = 0;
    }

    /* predict based on last frame */
    
    se1 = 0.0;
    for(i=0; i<k; i++) {
        err[i] = (vec_in[i] - pred*vec_out[i]);
        se1 += err[i]*err[i];
        w[i] = 1.0;
    }
    se1 /= k;
    
    /* now quantise err[] using multi-stage mbest search, preserving
       mbest_survivors at each stage */
    
    lpcnet_mbest_search(vq, err, w, k, m[0], mbest_stage[0], index);
    if (lpcnet_verbose) MBEST_PRINT("Stage 1:", mbest_stage[0]);
    
    for(s=1; s<num_stages; s++) {

        /* for each candidate in previous stage, try to find best vector in next stage */
        for (j=0; j<mbest_survivors; j++) {
            /* indexes that lead us this far */
            for(s1=0; s1<s; s1++) {
                index[s1+1] = mbest_stage[s-1]->list[j].index[s1];
            }
            /* target is residual err[] vector given path to this candidate */
            for(i=0; i<k; i++)
                target[i] = err[i];
            for(s1=0; s1<s; s1++) {
                ind = index[s-s1];
                if (lpcnet_verbose) fprintf(stderr, "   s: %d s1: %d s-s1: %d ind: %d\n", s,s1,s-s1,ind);
                for(i=0; i<k; i++) {
                    target[i] -= vq[s1*k*MAX_ENTRIES+ind*k+i];
                }
            }
            pv("   target: ", target);
            lpcnet_mbest_search(&vq[s*k*MAX_ENTRIES], target, w, k, m[s], mbest_stage[s], index);
        }
        char str[80]; sprintf(str,"Stage %d:", s+1);
        if (lpcnet_verbose) MBEST_PRINT(str, mbest_stage[s]);
    }

    for(s=0; s<num_stages; s++) {
        indexes[s] = mbest_stage[num_stages-1]->list[0].index[num_stages-1-s];
    }

    /* OK put it all back together using best survivor. Note we need
       to decode at encoder to keep record of last output for next
       frame's predictor */
    
    pv("\n  vec_in: ", vec_in);
    pv("  vec_out: ", vec_out);
    pv("    err: ", err);
    if (lpcnet_fsv != NULL) fprintf(lpcnet_fsv, "%f\t%f\t", vec_in[0],sqrt(se1));
    if (lpcnet_verbose) fprintf(stderr, "    se1: %f\n", se1);

    quant_pred_output(vec_out, indexes, err, pred, num_stages, vq, k);
    
    for(i=0; i<num_stages; i++)
        lpcnet_mbest_destroy(mbest_stage[i]);
}


void quant_pred_output(float vec_out[],
                       int   indexes[],
                       float err[],      /* used for development, set to zeros in real world decode side */
                       float pred,
                       int num_stages,
                       float vq[], int k)
{
    int s,i,ind;
    float se2;
       
    for(i=0; i<k; i++)
        vec_out[i] = pred*vec_out[i];
    
    for(s=0; s<num_stages; s++) {
        ind = indexes[s];
        se2 = 0.0;
        for(i=0; i<k; i++) {
            err[i] -= vq[s*k*MAX_ENTRIES+ind*k+i];
            vec_out[i] += vq[s*k*MAX_ENTRIES+ind*k+i];
            se2 += err[i]*err[i];
        }
        se2 /= k;
        if (lpcnet_fsv != NULL) fprintf(lpcnet_fsv, "%f\t", sqrt(se2));
        pv("    err: ", err);
        if (lpcnet_verbose) fprintf(stderr, "    se2: %f\n", se2);
    }
    pv("  vec_out: ",vec_out);

    if (lpcnet_fsv != NULL) fprintf(lpcnet_fsv, "\n");
}


/*---------------------------------------------------------------------------* \

  quantise

  Quantises vec by choosing the nearest vector in codebook cb, and
  returns the vector index.  The squared error of the quantised vector
  is added to se.

\*---------------------------------------------------------------------------*/

static int quantise(const float * cb, float vec[], float w[], int k, int m, float *se)
/* float   cb[][K];	current VQ codebook		*/
/* float   vec[];	vector to quantise		*/
/* float   w[];         weighting vector                */
/* int	   k;		dimension of vectors		*/
/* int     m;		size of codebook		*/
/* float   *se;		accumulated squared error 	*/
{
    float   e;		/* current error		*/
    long	   besti;	/* best index so far		*/
    float   beste;	/* best error so far		*/
    long	   j;
    int     i;
    float   diff;

    besti = 0;
    beste = 1E32;
    for(j=0; j<m; j++) {
	e = 0.0;
	for(i=0; i<k; i++) {
	    diff = cb[j*k+i]-vec[i];
	    e += powf(diff*w[i],2.0);
	}
	if (e < beste) {
	    beste = e;
	    besti = j;
	}
    }

    *se += beste;

    return(besti);
}


int pitch_encode(float pitch_feature, int pitch_bits) {
    
    assert(pitch_bits <= 8);
    // mapping we use as input to pembed layer, pemebed will only be trained
    // for these discrete values.  However I think all integers will be covered, so
    // we may not need any special precautions here.
    int periods = 0.1 + 50*pitch_feature + 100;
    if (periods < PITCH_MIN_PERIOD) periods = PITCH_MIN_PERIOD;
    if (periods >= PITCH_MAX_PERIOD) periods = PITCH_MAX_PERIOD-1;

    // should probably add rounding here
    int q = (periods - PITCH_MIN_PERIOD) >> (8 - pitch_bits);
    return q;
}

float pitch_decode(int pitch_bits, int q) {
    int periods_ = (q << (8 - pitch_bits)) + PITCH_MIN_PERIOD;
    /* bit errors can push periods_ to 63*(8-6)+20 = 272 which breaks embedd layer */
    if (periods_ < PITCH_MIN_PERIOD) periods_ = PITCH_MIN_PERIOD;
    if (periods_ >= PITCH_MAX_PERIOD) periods_ = PITCH_MAX_PERIOD-1;
    return ((float)periods_ - 100.0 - 0.1)/50.0;
}
                
static float pitch_gain_cb[] = {0.25, 0.25, 0.65, 0.80};

int pitch_gain_encode(float pitch_gain_feature) {
    // 2 bit pitch gain quantiser
    float w[1] = {1.0};
    float se;
    int ind = quantise(pitch_gain_cb, &pitch_gain_feature, w, 1, 4, &se);
    return ind;
}

float pitch_gain_decode(int ind) {
    return pitch_gain_cb[ind];
}

void pack_frame(int num_stages, int m[], int indexes[], int pitch_bits, int pitch_ind, int pitch_gain_ind, char frame[]) {
    int s,b,k=0,nbits;
    
    for(s=0; s<num_stages; s++) {
        nbits = log2(m[s]);
        for (b=0; b<nbits; b++)
            frame[k++] = (indexes[s] >> (nbits-1-b)) & 0x1;
    }
    for (b=0; b<pitch_bits; b++)
        frame[k++] = (pitch_ind >> (pitch_bits-1-b)) & 0x1;
    frame[k++] = (pitch_gain_ind >> 1) & 0x1;
    frame[k++] = pitch_gain_ind & 0x1;   
}

void unpack_frame(int num_stages, int m[], int indexes[], int pitch_bits, int *pitch_ind, int *pitch_gain_ind, char frame[]) {
    int s,b,k=0,nbits;
    
    for(s=0; s<num_stages; s++) {
        nbits = log2(m[s]);
        indexes[s] = 0;
        for (b=0; b<nbits; b++)
            indexes[s] |= (int)frame[k++] << (nbits-1-b);
    }
    *pitch_ind = 0;
    for (b=0; b<pitch_bits; b++)
        *pitch_ind |= (int)frame[k++] << (pitch_bits-1-b);
    *pitch_gain_ind = ((int)frame[k]<<1) + frame[k+1];   
}

// Call every q->dec LPCNet frames

int lpcnet_features_to_frame(LPCNET_QUANT *q, float features[], char frame[]) {
    int i, k = NB_BANDS;
    int frame_valid = 0;
    int indexes[MAX_STAGES];
    
    /* convert cepstrals to dB */
    for(i=0; i<NB_BANDS; i++)
        features[i] *= 10.0;

    /* optional weight on first cepstral which increases at
       sqrt(NB_BANDS) for every dB of speech input power.  Note by
       doing it here, we won't be measuring SD of this step, SD
       results will be on weighted vector. */
    features[0] *= q->weight;
                   
    int pitch_ind, pitch_gain_ind;
                
    /* non-interpolated frame ----------------------------------------*/

    quant_pred_mbest(q->features_quant, indexes, features, q->pred, q->num_stages, q->vq, q->m, k, q->mbest);
    pitch_ind = pitch_encode(features[2*NB_BANDS], q->pitch_bits);
    pitch_gain_ind =  pitch_gain_encode(features[2*NB_BANDS+1]);
    pack_frame(q->num_stages, q->m, indexes, q->pitch_bits, pitch_ind, pitch_gain_ind, frame);
    frame_valid = 1;
    
    return frame_valid;
}

// Call every 10ms, supply a new frame of bits when (q->f % q->dec) == 0)
void lpcnet_frame_to_features(LPCNET_QUANT *q, char frame[], float features_out[]) {

    int i,d;
    int pitch_ind, pitch_gain_ind;
    int indexes[MAX_STAGES];
    float fract, err[NB_BANDS];
    
    for(i=0; i<NB_FEATURES; i++)
        features_out[i] = 0.0;
    // note this is an unused ouput, just clear it to satisfy warnings
    for(i=0; i<NB_BANDS; i++)
        err[i] = 0.0;
        
    /* decoder */
        
    if ((q->f % q->dec) == 0) {

        /* non-interpolated frame ----------------------------------------*/

        unpack_frame(q->num_stages, q->m, indexes, q->pitch_bits, &pitch_ind, &pitch_gain_ind, frame);
        quant_pred_output(q->features_quant, indexes, err, q->pred, q->num_stages, q->vq, NB_BANDS);

        q->features_quant[2*NB_BANDS] = pitch_decode(q->pitch_bits, pitch_ind);
        q->features_quant[2*NB_BANDS+1] = pitch_gain_decode(pitch_gain_ind);
            
        /* update linear interpolation arrays */
        for(i=0; i<NB_FEATURES; i++) {
            q->features_lin[0][i] = q->features_lin[1][i];
            q->features_lin[1][i] = q->features_quant[i];                
        }

        /* pass  frame through */
        for(i=0; i<NB_BANDS; i++) {
            features_out[i] = q->features_lin[0][i];
        }
        features_out[2*NB_BANDS]   = q->features_lin[0][2*NB_BANDS];
        features_out[2*NB_BANDS+1] = q->features_lin[0][2*NB_BANDS+1];            

    } else {
        /* interpolated frame ----------------------------------------*/
            
        d = q->f % q-> dec;
        for(i=0; i<NB_FEATURES; i++) {
            fract = (float)d/(float)q->dec;
            features_out[i] = (1.0-fract)*q->features_lin[0][i] + fract*q->features_lin[1][i];
        }

    }
        
    q->f++;
        
    features_out[0] /= q->weight;    

    /* convert cepstrals back from dB */
    for(i=0; i<NB_BANDS; i++)
        features_out[i] *= 1/10.0;

    /* need to recompute LPCs after every frame, as we have quantised, or interpolated */
    lpc_from_cepstrum(&features_out[2*NB_BANDS+3], features_out);
}
