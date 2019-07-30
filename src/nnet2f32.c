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

void write_dense_weights(char *name, const DenseLayer *l, FILE *f32) {
    int n = l->nb_inputs*l->nb_neurons;
    printf("%s: %d %d\n", name, n, n);
    fwrite(l->bias, sizeof(float), n, f32);
    fwrite(l->input_weights, sizeof(float), n, f32);
}

void check(const float *target, int n, FILE *f32) {
    float *buf = malloc(sizeof(float)*n); assert(buf != NULL);
    int ret = fread(buf, sizeof(float), n, f32);    
    assert(ret == n);
    if (memcmp(target, buf, n*sizeof(float)) == 0)
	printf(" OK");
    else {
	printf(" FAIL"); exit(1);
    }
    free(buf);
}

void check_dense_weights(char *name, const DenseLayer *l, FILE *f32) {
    int n = l->nb_inputs*l->nb_neurons;
    printf("%s: %d %d", name, n, n);
    check(l->bias, n, f32);
    check(l->input_weights, n, f32);
    printf("\n");
}

void write_conv1d_weights(char *name, const Conv1DLayer *l, FILE *f32) {
    int n = l->nb_inputs*l->kernel_size*l->nb_neurons;
    printf("%s: %d %d\n", name, n, l->nb_neurons);
    fwrite(l->input_weights, sizeof(float), n, f32);
    fwrite(l->bias, sizeof(float), l->nb_neurons, f32);
}

void check_conv1d_weights(char *name, const Conv1DLayer *l, FILE *f32) {
    int n = l->nb_inputs*l->kernel_size*l->nb_neurons;
    printf("%s: %d %d", name, n, l->nb_neurons);
    check(l->input_weights, n, f32);
    check(l->bias, l->nb_neurons, f32);
    printf("\n");
}

int main(int argc, char **argv) {

    if (argc != 2) {
	fprintf(stderr, "usage: %s model_file.f32\n", argv[0]);
	exit(0);
    }
    
    FILE *f32 = fopen(argv[1], "wb");
    assert(f32 != NULL);

    write_embedding_weights("gru_a_embed_sig.....", &gru_a_embed_sig, f32);
    write_embedding_weights("gru_a_embed_pred....", &gru_a_embed_pred, f32);
    write_embedding_weights("gru_a_embed_exc.....", &gru_a_embed_pred, f32);
    write_dense_weights    ("gru_a_dense_feature.", &gru_a_dense_feature, f32);
    write_embedding_weights("embed_pitch.........", &embed_pitch, f32);
    write_conv1d_weights   ("feature_conv1.......", &feature_conv1, f32);
    fclose(f32);
    
    f32 = fopen(argv[1], "rb");
    check_embedding_weights("gru_a_embed_sig.....", &gru_a_embed_sig, f32);
    check_embedding_weights("gru_a_embed_pred....", &gru_a_embed_pred, f32);
    check_embedding_weights("gru_a_embed_exc.....", &gru_a_embed_pred, f32);
    check_dense_weights    ("gru_a_dense_feature.", &gru_a_dense_feature, f32);
    check_embedding_weights("embed_pitch.........", &embed_pitch, f32);
    check_conv1d_weights   ("feature_conv1.......", &feature_conv1, f32);
   
    return 0;
}
