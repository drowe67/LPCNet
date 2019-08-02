/*

 nnet_rw.c

 Support for reading and writing NNs from disk at run time.
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "nnet_data.h"
#include "nnet_rw.h"

void write_embedding_weights(char *name, const EmbeddingLayer *l, FILE *f32) {
    int n = l->nb_inputs*l->dim;
    printf("%s: %d\n", name, n);
    fwrite(l->embedding_weights, sizeof(float), n, f32);
}

void read_embedding_weights(char *name, const EmbeddingLayer *l, FILE *f32) {
    int n = l->nb_inputs*l->dim;
    printf("%s: %d\n", name, n);
    int ret;
    ret = fread(l->embedding_weights, sizeof(float), n, f32); assert(ret == n);
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

void check_int(const int *target, int n, FILE *f32) {
    int *buf = malloc(sizeof(int)*n); assert(buf != NULL);
    int ret = fread(buf, sizeof(int), n, f32);    
    assert(ret == n);
    if (memcmp(target, buf, n*sizeof(int)) == 0)
	printf(" OK");
    else {
	printf(" FAIL"); exit(1);
    }
    free(buf);
}

void write_dense_weights(char *name, const DenseLayer *l, FILE *f32) {
    int nbias = l->nb_neurons;
    int nweights = l->nb_inputs*l->nb_neurons;
    printf("%s: %d %d\n", name, nweights, nbias);
    fwrite(l->bias, sizeof(float), nbias, f32);
    fwrite(l->input_weights, sizeof(float), nweights, f32);
}

void check_dense_weights(char *name, const DenseLayer *l, FILE *f32) {
    int nbias = l->nb_neurons;
    int nweights = l->nb_inputs*l->nb_neurons;
    printf("%s: %d %d", name, nweights, nbias);
    check(l->bias, nbias, f32);
    check(l->input_weights, nweights, f32);
    printf("\n");
}

void read_dense_weights(char *name, const DenseLayer *l, FILE *f32) {
    int nbias = l->nb_neurons;
    int nweights = l->nb_inputs*l->nb_neurons;
    printf("%s: %d %d\n", name, nweights, nbias);
    int ret;
    ret = fread(l->bias, sizeof(float), nbias, f32); assert(ret == nbias);
    ret = fread(l->input_weights, sizeof(float), nweights, f32); assert(ret == nweights);
}

void write_mdense_weights(char *name, const MDenseLayer *l, FILE *f32) {
    int ninput = l->nb_inputs*l->nb_neurons*l->nb_channels;
    int nbias = l->nb_neurons*l->nb_channels;
    int nfactor = l->nb_neurons*l->nb_channels;
    printf("%s: %d %d %d\n", name, ninput, nbias, nfactor);
    fwrite(l->bias, sizeof(float), nbias, f32);
    fwrite(l->input_weights, sizeof(float), ninput, f32);
    fwrite(l->factor, sizeof(float), nfactor, f32);
}

void check_mdense_weights(char *name, const MDenseLayer *l, FILE *f32) {
    int ninput = l->nb_inputs*l->nb_neurons*l->nb_channels;
    int nbias = l->nb_neurons*l->nb_channels;
    int nfactor = l->nb_neurons*l->nb_channels;
    printf("%s: %d %d %d", name, ninput, nbias, nfactor);
    check(l->bias, nbias, f32);
    check(l->input_weights, ninput, f32);
    check(l->factor, nfactor, f32);
    printf("\n");
}

void read_mdense_weights(char *name, const MDenseLayer *l, FILE *f32) {
    int ninput = l->nb_inputs*l->nb_neurons*l->nb_channels;
    int nbias = l->nb_neurons*l->nb_channels;
    int nfactor = l->nb_neurons*l->nb_channels;
    printf("%s: %d %d %d\n", name, ninput, nbias, nfactor);
    int ret;
    ret = fread(l->bias, sizeof(float), nbias, f32); assert(ret == nbias);
    ret = fread(l->input_weights, sizeof(float), ninput, f32); assert(ret == ninput);
    ret = fread(l->factor, sizeof(float), nfactor, f32); assert(ret == nfactor);
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

void read_conv1d_weights(char *name, const Conv1DLayer *l, FILE *f32) {
    int n = l->nb_inputs*l->kernel_size*l->nb_neurons;
    printf("%s: %d %d\n", name, n, l->nb_neurons);
    int ret;
    ret = fread(l->input_weights, sizeof(float), n, f32); assert(ret == n);
    ret = fread(l->bias, sizeof(float), l->nb_neurons, f32); assert(ret == l->nb_neurons);
}

void write_gru_weights(char *name, const GRULayer *l, FILE *f32) {
    int nbias = l->nb_neurons*6;
    int ninput = l->nb_inputs*l->nb_neurons*3;
    int nrecurrent = l->nb_neurons*l->nb_neurons*3;
    printf("%s: %d %d %d\n", name, nbias, ninput, nrecurrent);
    fwrite(l->bias, sizeof(float), nbias, f32);
    fwrite(l->input_weights, sizeof(float), ninput, f32);
    fwrite(l->recurrent_weights, sizeof(float), nrecurrent, f32);
}

void check_gru_weights(char *name, const GRULayer *l, FILE *f32) {
    int nbias = l->nb_neurons*6;
    int ninput = l->nb_inputs*l->nb_neurons*3;
    int nrecurrent = l->nb_neurons*l->nb_neurons*3;
    printf("%s: %d %d %d", name, nbias, ninput, nrecurrent);
    check(l->bias, nbias, f32);
    check(l->input_weights, ninput, f32);
    check(l->recurrent_weights, nrecurrent, f32);
    printf("\n");
}

void read_gru_weights(char *name, const GRULayer *l, FILE *f32) {
    int nbias = l->nb_neurons*6;
    int ninput = l->nb_inputs*l->nb_neurons*3;
    int nrecurrent = l->nb_neurons*l->nb_neurons*3;
    printf("%s: %d %d %d\n", name, nbias, ninput, nrecurrent);
    int ret;
    ret = fread(l->bias, sizeof(float), nbias, f32); assert(ret == nbias);
    ret = fread(l->input_weights, sizeof(float), ninput, f32); assert(ret == ninput);
    ret = fread(l->recurrent_weights, sizeof(float), nrecurrent, f32); assert(ret == nrecurrent);
}

void write_sparse_gru_weights(char *name, const SparseGRULayer *l, FILE *f32) {
    int nbias = l->nb_neurons*6;
    int ndiag = l->nb_neurons*3;
    int nrecurrent = l->nb_neurons*l->nb_neurons*3;
    int nidx = 32767;
    printf("%s: %d %d %d %d\n", name, nbias, ndiag, nrecurrent, nidx);
    fwrite(l->bias, sizeof(float), nbias, f32);
    fwrite(l->diag_weights, sizeof(float), ndiag, f32);
    fwrite(l->recurrent_weights, sizeof(float), nrecurrent, f32);
    fwrite(l->idx, sizeof(int), nidx, f32);
}

void check_sparse_gru_weights(char *name, const SparseGRULayer *l, FILE *f32) {
    int nbias = l->nb_neurons*6;
    int ndiag = l->nb_neurons*3;
    int nrecurrent = l->nb_neurons*l->nb_neurons*3;
    int nidx = 32767;
    printf("%s: %d %d %d %d", name, nbias, ndiag, nrecurrent, nidx);
    check(l->bias, nbias, f32);
    check(l->diag_weights, ndiag, f32);
    check(l->recurrent_weights, nrecurrent, f32);
    check_int(l->idx, nidx, f32);
    printf("\n");
}

void read_sparse_gru_weights(char *name, const SparseGRULayer *l, FILE *f32) {
    int nbias = l->nb_neurons*6;
    int ndiag = l->nb_neurons*3;
    int nrecurrent = l->nb_neurons*l->nb_neurons*3;
    int ret;
    int nidx = 32767;
    printf("%s: %d %d %d %d\n", name, nbias, ndiag, nrecurrent, nidx);
    ret = fread(l->bias, sizeof(float), nbias, f32); assert(ret == nbias);
    ret = fread(l->diag_weights, sizeof(float), ndiag, f32); assert(ret == ndiag);
    ret = fread(l->recurrent_weights, sizeof(float), nrecurrent, f32); assert(ret == nrecurrent);
    ret = fread(l->idx, sizeof(int), nidx, f32);
}

void nnet_write(char *fn) {
    FILE *f32 = fopen(fn, "wb");
    assert(f32 != NULL);

    printf("writing ....\n");
    write_embedding_weights("gru_a_embed_sig.....", &gru_a_embed_sig, f32);
    write_embedding_weights("gru_a_embed_pred....", &gru_a_embed_pred, f32);
    write_embedding_weights("gru_a_embed_exc.....", &gru_a_embed_exc, f32);
    write_dense_weights    ("gru_a_dense_feature.", &gru_a_dense_feature, f32);
    write_embedding_weights("embed_pitch.........", &embed_pitch, f32);
    write_conv1d_weights   ("feature_conv1.......", &feature_conv1, f32);
    write_conv1d_weights   ("feature_conv2.......", &feature_conv2, f32);
    write_dense_weights    ("feature_dense1......", &feature_dense1, f32);
    write_embedding_weights("embed_sig...........", &embed_sig, f32);
    write_dense_weights    ("feature_dense2......", &feature_dense2, f32);
    write_gru_weights      ("gru_a...............", &gru_a, f32);
    write_gru_weights      ("gru_b...............", &gru_b, f32);
    write_mdense_weights   ("dual_fc.............", &dual_fc, f32);
    write_sparse_gru_weights("sparse_gru_a........", &sparse_gru_a, f32);
    fclose(f32);
    printf("\n");
}    

void nnet_read_and_check(char *fn) {
    printf("reading back and check ....\n");
    FILE *f32 = fopen(fn, "rb"); assert(f32 != NULL);
    check_embedding_weights("gru_a_embed_sig.....", &gru_a_embed_sig, f32);
    check_embedding_weights("gru_a_embed_pred....", &gru_a_embed_pred, f32);
    check_embedding_weights("gru_a_embed_exc.....", &gru_a_embed_exc, f32);
    check_dense_weights    ("gru_a_dense_feature.", &gru_a_dense_feature, f32);
    check_embedding_weights("embed_pitch.........", &embed_pitch, f32);
    check_conv1d_weights   ("feature_conv1.......", &feature_conv1, f32);
    check_conv1d_weights   ("feature_conv2.......", &feature_conv2, f32);
    check_dense_weights    ("feature_dense1......", &feature_dense1, f32);
    check_embedding_weights("embed_sig...........", &embed_sig, f32);
    check_dense_weights    ("feature_dense2......", &feature_dense2, f32);
    check_gru_weights      ("gru_a...............", &gru_a, f32);
    check_gru_weights      ("gru_b...............", &gru_b, f32);
    check_mdense_weights   ("dual_fc.............", &dual_fc, f32);
    check_sparse_gru_weights("sparse_gru_a........", &sparse_gru_a, f32);
    fclose(f32);
    printf("\n");
}

void nnet_read(char *fn) {
    printf("read ....\n");
    FILE *f32 = fopen(fn, "rb"); assert(f32 != NULL);
    read_embedding_weights("gru_a_embed_sig.....", &gru_a_embed_sig, f32);
    read_embedding_weights("gru_a_embed_pred....", &gru_a_embed_pred, f32);
    read_embedding_weights("gru_a_embed_exc.....", &gru_a_embed_exc, f32);
    read_dense_weights    ("gru_a_dense_feature.", &gru_a_dense_feature, f32);
    read_embedding_weights("embed_pitch.........", &embed_pitch, f32);
    read_conv1d_weights   ("feature_conv1.......", &feature_conv1, f32);
    read_conv1d_weights   ("feature_conv2.......", &feature_conv2, f32);
    read_dense_weights    ("feature_dense1......", &feature_dense1, f32);
    read_embedding_weights("embed_sig...........", &embed_sig, f32);
    read_dense_weights    ("feature_dense2......", &feature_dense2, f32);
    read_gru_weights      ("gru_a...............", &gru_a, f32);
    read_gru_weights      ("gru_b...............", &gru_b, f32);
    read_mdense_weights   ("dual_fc.............", &dual_fc, f32);
    read_sparse_gru_weights("sparse_gru_a........", &sparse_gru_a, f32);
    fclose(f32);
    printf("\n");
}
