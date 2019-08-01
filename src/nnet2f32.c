/*
  nnet2f32.c

  Writes current compiled-in model to a binary file of floats, and runs a few tests.
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nnet_data.h"
#include "nnet_rw.h"

int main(int argc, char **argv) {

    if (argc != 2) {
	fprintf(stderr, "usage: %s model_file.f32\n", argv[0]);
	exit(0);
    }

    nnet_write(argv[1]);
    nnet_read_and_check(argv[1]);
    nnet_read(argv[1]);
    nnet_write("copy.f32");

    char cmd[256];
    int ret = sprintf(cmd, "set -x; diff %s copy.f32; if [ $? -eq 0 ]; then { echo PASS; exit 0; } else { echo FAIL; exit 1; } fi", argv[1]);
    ret = system(cmd);
    return ret;
}
