/*
  sw2packedulaw.c

  Convert signed word samples to packed ulaw samples to drive LPCNet
  training, cut/paste from dump_data.c
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

typedef struct {
  float lpc[LPC_ORDER];
  float sig_mem[LPC_ORDER];
  int exc_mem;
} DenoiseState;

void write_audio(DenoiseState *st, const short *pcm, float noise_std, FILE *file) {
  int i;
  unsigned char data[4*FRAME_SIZE];
  for (i=0;i<FRAME_SIZE;i++) {
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
  fwrite(data, 4*FRAME_SIZE, 1, file);
}

int main(int argc, char *argv[]) {
    FILE *fsw = fopen(argv[1],"rb");
    assert(fsw != NULL);
    FILE *fpackedpcm = fopen(argv[2],"wb");
    assert(fpackedpcm != NULL);

    /* running with LPCs, so WaveRNN rather than LPCNet */
    DenoiseState st;
    memset(&st, 0, sizeof(DenoiseState));
    
    short frame[FRAME_SIZE];
    while (fread(frame, sizeof(short), FRAME_SIZE, fsw) == FRAME_SIZE) {
      float tmp = (float)rand()/RAND_MAX;
      float noise_std = 4*tmp*tmp;
      write_audio(&st, frame, noise_std, fpackedpcm);
    }

    fclose(fsw);
    fclose(fpackedpcm);
    return 0;
}

