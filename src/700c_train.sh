#!/bin/bash -x
# 700c_train.sh
# David Rowe March 2020
# Experiments in LPCNet decoding of Codec 2 700C

PATH=$HOME/codec2/build_linux/src:$HOME/LPCNet/build_linux/src:$HOME/LPCNet/src:$PATH

if [ "$#" -ne 2 ]; then
    echo "usage: ./700c_train.sh train_sw_file datestamp"
    echo "       ./700c_train.sh all_speech_8k 200404"
    exit 0
fi

train=$1
test=all_8k
datestamp=$2
epochs=05
log=${2}.txt

experiment() {
    echo "------------------------------------------------------------------------------"
    echo "train starting" ${2}
    echo "------------------------------------------------------------------------------"
    
    c2sim ~/Downloads/${train}.sw --ten_ms_centre ${train}_10ms.sw --rateKWov ${train}.f32 ${1}
    sw2packedulaw --frame_size 80 ${train}_10ms.sw ${train}.f32 ${train}_10ms.pulaw
    
    train_lpcnet.py ${train}.f32 ${train}_10ms.pulaw ${datestamp} --epochs ${epochs} --frame_size 80
    dump_lpcnet.py ${datestamp}_${epochs}.h5
    cp nnet_data.c src
    make test_lpcnet

    c2sim ~/Downloads/${test}.sw --rateKWov ${test}.f32 ${1}
    sw2packedulaw --frame_size 80 ${test}_10ms.sw ${test}.f32 ${test}_10ms.pulaw
    test_lpcnet --mag 2 --frame_size 80 --pre 0 ${test}.f32 ${datestamp}_${test}_${2}.sw
}

rm -f $log

# Quantised 700C vectors at 10ms frame rate (note LPCs unquantised)

(
    experiment "" "none"          # no prediction
    #experiment "--first"  "first" # first order predictor
    #experiment "--lpc 10" "lpc"   # standard LPC (albiet 10th order)

    # no prediction, Codec 2 700C at 40ms frame rate (700 bits/s) from c2dec
    echo $PATH
    c2enc 700C ~/Downloads/${test}.sw - --eq --var | c2dec 700C - /dev/null --mlfeat ${test}_dec4.f32
    test_lpcnet --mag 2 --frame_size 80 --pre 0 ${test}_dec4.f32 ${datestamp}_${test}_40.sw
) |& tee $log


