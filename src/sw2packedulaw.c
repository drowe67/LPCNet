/*
  sw2packedulaw.c

  Convert signed word samples to packed ulaw samples to drive LPCNet
  training, this code is a cut/paste from dump_data.c witha few other
  options.

  By varying the LPC predictor coefficients we can try no predictor,
  first order, and regular LPC.

  1. No prediction (WaveRNN I guess):
    $ ~/codec2/build_linux/src/c2sim ~/Downloads/all_8k.sw --ten_ms_centre all_8k_10ms.sw --rateKWov all_8k.f32 
    $ ./src/sw2packedulaw --frame_size 80 all_8k_10ms.sw all_8k.f32 all_8k_none.pulaw
    $ ../src/plot_pulaw.py all_8k_none.pulaw

  2. First order predictor:
    $ ~/codec2/build_linux/src/c2sim ~/Downloads/all_8k.sw --ten_ms_centre all_8k_10ms.sw --rateKWov all_8k.f32 --first
    $ ./src/sw2packedulaw --frame_size 80 all_8k_10ms.sw all_8k.f32 all_8k_first.pulaw

  3. LPC with ulaw Q in the loop and noise injection (standard LPCNet design):
    $ ~/codec2/build_linux/src/c2sim ~/Downloads/all_8k.sw --ten_ms_centre all_8k_10ms.sw --rateKWov all_8k.f32 --lpc 10
    $ ./src/sw2packedulaw --frame_size 80all_8k_10ms.sw  all_8k.f32 all_8k.pulaw

  4. LPC with no Q in the loop or noise injection (linear):
    $ ./src/sw2packedulaw --frame_size 80 --linear all_8k_10ms.sw all_8k.f32 all_8k_linear.pulaw

  See plot_pulaw.py to inspect output .pulaw files
*/

#include <stdlib.h>
#include <stdio.h>
#include "common.h"
#include <math.h>
#include "freq.h"
#include "pitch.h"
#include "arch.h"
#include "celt_lpc.h"
#include <assert.h>
#include <getopt.h>

#define NB_FEATURES 55
#define CODEC2_LPC_ORDER 10

typedef struct {
  float lpc[LPC_ORDER];
  float sig_mem[LPC_ORDER];
  int exc_mem;
} DenoiseState;

void write_audio(DenoiseState *st, const short *pcm, float noise_std, FILE *file, int frame_size) {
  int i;
  unsigned char data[4*frame_size];
  for (i=0;i<frame_size;i++) {
    int noise;
    float p=0;
    float e;
    int j;
    for (j=0;j<LPC_ORDER;j++) p -= st->lpc[j]*st->sig_mem[j];
    e = lin2ulaw(pcm[i] - p);
    /* Signal. */
    data[4*i] = lin2ulaw(st->sig_mem[0]);
    /* Prediction. */
    data[4*i+1] = lin2ulaw(p);
    /* Excitation in. */
    data[4*i+2] = st->exc_mem;
    /* Excitation out. */
    data[4*i+3] = e;
    /* Simulate error on excitation. */
    noise = (int)floor(.5 + noise_std*.707*(log_approx((float)rand()/RAND_MAX)-log_approx((float)rand()/RAND_MAX)));
    e += noise;
    e = IMIN(255, IMAX(0, e));
    
    RNN_MOVE(&st->sig_mem[1], &st->sig_mem[0], LPC_ORDER-1);
    st->sig_mem[0] = p + ulaw2lin(e);
    st->exc_mem = e;
  }
  fwrite(data, 4*frame_size, 1, file);
}

/* takes ulaw out of predictor path, and no noise injection */
void write_audio_linear(DenoiseState *st, const short *pcm, FILE *file, int frame_size) {
  int i;
  unsigned char data[4*frame_size];
  for (i=0;i<frame_size;i++) {
    float p=0;
    float e;
    int j;
    for (j=0;j<LPC_ORDER;j++) p -= st->lpc[j]*st->sig_mem[j];
    e = pcm[i] - p;
    //fprintf(stderr,"pcm: %d p: %f e: %f\n", pcm[i], p, e);
    /* Signal. */
    data[4*i] = lin2ulaw(st->sig_mem[0]);
    /* Prediction. */
    data[4*i+1] = lin2ulaw(p);
    /* Excitation in. */
    data[4*i+2] = st->exc_mem;
    /* Excitation out. */
    data[4*i+3] = lin2ulaw(e);

    RNN_MOVE(&st->sig_mem[1], &st->sig_mem[0], LPC_ORDER-1);
    st->sig_mem[0] = pcm[i];
    st->exc_mem = lin2ulaw(e);
  }
  fwrite(data, 4*frame_size, 1, file);
}

int main(int argc, char *argv[]) {
    int linear = 0;
    int frame_size = FRAME_SIZE;
    
    DenoiseState st;
    memset(&st, 0, sizeof(DenoiseState));
    st.exc_mem = 128;
    
    int o = 0;
    int opt_idx = 0;
    while( o != -1 ) {
        static struct option long_opts[] = {
            {"linear", no_argument, 0, 'l'},
            {"frame_size", required_argument, 0, 'f'},
            {0, 0, 0, 0}
        };
        
	o = getopt_long(argc,argv,"l",long_opts,&opt_idx);
        
	switch(o){
	case 'f':
	    frame_size = atoi(optarg);
	    fprintf(stderr, "frame_size: %d\n", frame_size);
	    break;
	case 'l':
	    linear = 1;
	    break;
	case '?':
	    goto helpmsg;
	    break;
	}
    }
    int dx = optind;

    if ((argc - dx) < 3) {
    helpmsg:
        fprintf(stderr, "usage: s2packedulaw Input.s16 FeatureFile.f32 Output.pulaw\n");
        return 0;
    }

    FILE *fsw = fopen(argv[dx], "rb");
    if (fsw == NULL) {
	fprintf(stderr, "Can't open %s\n", argv[dx]);
	exit(1);
    }
    
    FILE *ffeature = fopen(argv[dx+1], "rb");
    if (ffeature == NULL) {
	fprintf(stderr, "Can't open %s\n", argv[dx+1]);
	exit(1);
    }
    
    FILE *fpackedpcm = fopen(argv[dx+2], "wb");
    if (fpackedpcm == NULL) {
	fprintf(stderr, "Can't open %s\n", argv[dx+2]);
	exit(1);
    }
    
    short frame[frame_size];
    while (fread(frame, sizeof(short), frame_size, fsw) == (unsigned)frame_size) {
	float features[NB_FEATURES];
	int ret = fread(features, sizeof(float), NB_FEATURES, ffeature);
	if (ret != NB_FEATURES) {
	    fprintf(stderr, "feature file ended early!\n");
	    exit(1);		
	}
	for(int i=0; i<CODEC2_LPC_ORDER; i++) {
	    st.lpc[i] = features[18+i];
	}
	if (linear)
	    write_audio_linear(&st, frame, fpackedpcm, frame_size);
	else {
	    write_audio(&st, frame, 0.5, fpackedpcm, frame_size);
	}
    }

    fclose(fsw);
    fclose(ffeature);
    fclose(fpackedpcm);
    return 0;
}

