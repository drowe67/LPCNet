/*
  lpcnet_quant.c
  David Rowe Feb 2019

  David's experimental quanisation functions for LPCNet
*/

#include <stdio.h>
#include <math.h>

#include "lpcnet_quant.h"
#include "mbest.h"

FILE *lpcnet_fsv = NULL;
int lpcnet_verbose = 0;

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
        mbest_stage[i] = mbest_create(mbest_survivors, num_stages);
        index[i] = 0;
    }

    /* predict based on last frame */
    
    se1 = 0.0;
    for(i=0; i<k; i++) {
        err[i] = (vec_in[i] - pred*vec_out[i]);
        se1 += err[i]*err[i];
        vec_out[i] = pred*vec_out[i];
        w[i] = 1.0;
    }
    se1 /= k;
    
    /* now quantise err[] using multi-stage mbest search, preserving
       mbest_survivors at each stage */
    
    mbest_search(vq, err, w, k, m[0], mbest_stage[0], index);
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
            mbest_search(&vq[s*k*MAX_ENTRIES], target, w, k, m[s], mbest_stage[s], index);
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
        mbest_destroy(mbest_stage[i]);
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

int quantise(const float * cb, float vec[], float w[], int k, int m, float *se)
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


