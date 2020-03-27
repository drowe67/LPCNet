#!/bin/bash
# 700c_train.sh
# David Rowe March 2020
# LPCnet for Codec 2 700C decoding

PATH=$PATH:$HOME/codec2/build_linux/src:$HOME/LPCNet/build_linux/src:$HOME/LPCNet/src

train=$1
model=$2
epochs=05

c2sim ~/Downloads/${train}.sw --ten_ms_centre ${train}_10ms.sw --rateKWov ${train}.f32
sw2packedulaw ${train}_10ms.sw ${train}_10ms.pulaw
train_lpcnet.py ${train}.f32 ${train}_10ms.pulaw ${model} --epochs ${epochs} --frame_size 80
dump_lpcnet.py ${model}_${epochs}.h5

cp nnet_data.c src
make test_lpcnet

test=all_8k
c2sim ~/Downloads/${test}.sw --rateKWov ${test}.f32
test_lpcnet --mag 2 --frame_size 80 ${test}.f32 - > test_700c.sw
