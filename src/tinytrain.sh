#!/bin/bash -x
# tinytrain.sh
#
# train using a tiny database, synthesise a few samples from within
# training database.  Used to perform quick sanity checks with a few hrs training

SRC=all_speech_david
DATE=190518d
EPOCHS=10

synth() {
  ./src/dump_data --test --c2pitch --mag ~/Downloads/$1.sw $1.f32
  ./src/test_lpcnet --mag $1.f32 "$2".s16
}

train() {
  ./src/dump_data --train --c2pitch -z 0 --mag -n 1E6 ~/Downloads/$SRC.sw $SRC.f32 $SRC.pcm
  ../src/train_lpcnet.py $SRC.f32 $SRC.pcm lpcnet_$DATE
}

dump() {
  ../src/dump_lpcnet.py lpcnet_"$DATE"_"$EPOCHS".h5
  cp nnet_data.c src
  make test_lpcnet
}

train
dump
synth c01_01  "$DATE"_c01_01
synth mk61_01 "$DATE"_mk61_01
synth cq_16kHz "$DATE"_cq_16kHz
synth wia "$DATE"_wia
