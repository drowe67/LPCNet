#!/bin/bash -x
# 700c_train.sh
# David Rowe March 2020
# Experiments in LPCNet decoding of Codec 2 700C

PATH=$HOME/codec2/build_linux/src:$HOME/LPCNet/build_linux/src:$HOME/LPCNet/src:$PATH

if [ "$#" -ne 1 ]; then
    echo "usage: ./700c_train.sh datestamp"
    echo "       ./700c_train.sh 200404"
    exit 0
fi

train1=all_speechcat_8k
train2=train_8k
test1=all_8k
test2=all_speech_subset_8k
datestamp=$1
epochs=05
log=${1}.txt
train=${datestamp}_train

# synth "c2sim arg for experiment" "experiment label" "filename"
synth() {
    test=$3
    c2sim ~/Downloads/${test}.sw --rateKWov ${test}.f32 ${1}
    test_lpcnet --mag 2 --frame_size 80 --pre 0 ${test}.f32 ${datestamp}_${test}_${2}.sw
}

# experient "c2sim arg for experiment" "experiment label"
experiment() {
    echo "------------------------------------------------------------------------------"
    echo "train starting" ${2}
    echo "------------------------------------------------------------------------------"
    
    c2sim ${train}.sw --ten_ms_centre ${train}_10ms.sw --rateKWov ${train}.f32 ${1}
    sw2packedulaw --frame_size 80 ${train}_10ms.sw ${train}.f32 ${train}_10ms.pulaw
    
    train_lpcnet.py ${train}.f32 ${train}_10ms.pulaw ${datestamp}_${2} --epochs ${epochs} --frame_size 80
    dump_lpcnet.py ${datestamp}_${2}_${epochs}.h5
    cp nnet_data.c src
    make test_lpcnet

    synth "${1}" "${2}" "${test1}"
    synth "${1}" "${2}" "${test2}"
}

rm -f $log

# assemble some training speech
sox -r 8000 -c 1 ~/Downloads/${train1}.sw \
    -r 8000 -c 1 ~/Downloads/${train2}.sw \
    -t sw -r 8000 -c 1 ${train}.sw
    
(
    : '
    experiment "" "none"           # no prediction

    # no prediction, Codec 2 700C at 40ms frame rate (700 bits/s) from c2dec
    c2enc 700C ~/Downloads/${test2}.sw - --eq --var | c2dec 700C - /dev/null --mlfeat ${test2}_dec4.f32
    test_lpcnet --mag 2 --frame_size 80 --pre 0 ${test2}_dec4.f32 ${datestamp}_${test2}_40.sw
    
    experiment "--first"  "first" # first order predictor
    '
    experiment "--lpc 10" "lpc"   # standard LPC (albiet 10th order)

) |& tee $log


