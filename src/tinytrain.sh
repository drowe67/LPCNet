#!/bin/bash -x
# tinytrain.sh
# train using a tiny database, synthesis a few samples from within
# training database.  Used to perform quick sanity checks with a few hrs training
#
# usage:
#   $ cd LPCNet/build_linux
#   $ ../src/tinytrain.sh

SRC=all_speech
DATE=190804b

synth() {
  ./src/dump_data --mag --test --c2pitch --c2voicing ~/Downloads/$1.sw $1.f32
  ./src/test_lpcnet --mag $1.f32 "$2".raw
}

train() {
  ./src/dump_data --mag --train --c2pitch --c2voicing -z 0 -n 1E6 ~/Downloads/$SRC.sw $SRC.f32 $SRC.pcm
  ../src/train_lpcnet.py $SRC.f32 $SRC.pcm lpcnet_$DATE
  ../src/dump_lpcnet.py lpcnet_"$DATE"_10.h5
  cp nnet_data.c src
  make test_lpcnet
}

train
synth c01_01 $DATE'_f'
synth mk61_01 $DATE'_m'
