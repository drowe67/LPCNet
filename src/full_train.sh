#!/bin/bash -x
# full_train.sh
# Script to train using full database, material developed to give reasonable quality
# to test sample database.
#
# usage:
#   $ cd LPCNet/build_linux
#   $ ../src/full_train.sh

SRC1=david_16kHz.wav             # 122s
SRC2=vk5apr_recording_21_may.wav # 64s
SRC3=all_speechcat.sw            # 185 minutes, wide range of speakers
SRC=train_src

DATE=191005

synth() {
  ./src/dump_data --test --c2pitch ~/Downloads/$1.sw $1.f32
  ./src/test_lpcnet $1.f32 "$2".raw
}

train() {
    # repeat David and Peter to get 142 minutes worth, so it weights training
    # for this type of speaker that we are struggling with
    x=$(mktemp)
    sox ~/Downloads/$SRC1 ~/Downloads/$SRC2 $x'.wav' repeat 60
    ls -l $x.wav
    # combine all samples, evaluation data at end of larger database of mixed speakers
    sox $x.wav \
	-t sw -r 16000 -c 1 ~/Downloads/$SRC3 \
	-t sw $SRC.sw
    ls -l $SRC.sw
    ./src/dump_data --train --c2pitch -z 1 $SRC.sw $SRC.f32 $SRC.pcm
    ../src/train_lpcnet.py $SRC.f32 $SRC.pcm lpcnet_$DATE
    ../src/dump_lpcnet.py lpcnet_"$DATE"_10.h5
    cp nnet_data.c src
    make test_lpcnet
}

train
synth c01_01 $DATE'_f'
synth mk61_01 $DATE'_m'
synth cq_16kHz $DATE'_cq_16kHz'
synth peter $DATE'_peter'
# source in a different dir
./src/dump_data --test --c2pitch ~/LPCNet/wav/all.wav all.f32
./src/test_lpcnet all.f32 $DATE'_all'.raw
