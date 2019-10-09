#!/bin/bash -x
# peter_david_train.sh
#
# LPCNet produced rough speech with these too speakers.  This script
# is an experiment to train and test a LPCNet model using _just_ these
# two speakers.
#
# usage:
#   $ cd LPCNet/build_linux
#   $ ../src/peter_david_train.sh

SRC1=david_16kHz.wav             # 122s
SRC2=vk5apr_recording_21_may.wav # 64s
SRC=train_src

DATE=190920b

synth() {
   ./src/dump_data --test --c2pitch --c2voicing ~/Downloads/$1.sw $1.f32 
   ./src/test_lpcnet $1.f32 "$2".raw
}

train() {
    sox ~/Downloads/$SRC1 ~/Downloads/$SRC2 -t sw $SRC.sw
    ./src/dump_data --train --c2pitch --c2voicing -z 1 -n 1E6 $SRC.sw $SRC.f32 $SRC.pcm
    ../src/train_lpcnet.py $SRC.f32 $SRC.pcm lpcnet_$DATE
    ../src/dump_lpcnet.py lpcnet_"$DATE"_10.h5
    cp nnet_data.c src
    make test_lpcnet
}

train
synth bob $DATE'_bob'
synth cq_16kHz $DATE'_cq_16kHz'
synth peter $DATE'_peter'
