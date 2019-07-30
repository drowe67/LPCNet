/*
  nnet2f32.c

  Writes current compiled-in model to a binary file of floats.
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nnet_data.h"

void write_embedding_weights(char *name, const EmbeddingLayer *l, FILE *f32) {
    int n = l->nb_inputs*l->dim;
    printf("%s: %d\n", name, n);
    fwrite(l->embedding_weights, sizeof(float), n, f32);
}

void check_embedding_weights(char *name, const EmbeddingLayer *l, FILE *f32) {
    int n = l->nb_inputs*l->dim;
    printf("%s: %d", name, n);
    float *buf = malloc(sizeof(float)*n);
    assert(buf != NULL);
    int ret = fread(buf, sizeof(float), n, f32);    
    assert(ret == n);
    if (memcmp(l->embedding_weights, buf, n*sizeof(float)) == 0)
	printf(" OK\n");
    else {
	printf(" FAIL\n"); exit(1);
    }
    free(buf);
}

int main(int argc, char **argv) {

    if (argc != 2) {
	fprintf(stderr, "usage: %s model_file.f32\n", argv[0]);
	exit(0);
    }
    
    FILE *f32 = fopen(argv[1], "wb");
    assert(f32 != NULL);

    write_embedding_weights("gru_a_embed_sig_weights", &gru_a_embed_sig, f32);
    fclose(f32);
    
    f32 = fopen(argv[1], "rb");
    check_embedding_weights("gru_a_embed_sig_weights", &gru_a_embed_sig, f32);
    
    return 0;
}
