#!/bin/bash
# test_core_nn.sh
#
# Some tests for core NN, e.g. generation of test data using
# dump_data, and unquantised synthesis using and test_lpcnet

TRAIN_SRC=all_speech
TRAIN_OUT_PCM=$(mktemp).pcm
TRAIN_OUT_F32=$(mktemp).f32
TRAIN_TARG_PCM=all_speech.pcm
TRAIN_TARG_F32=all_speech.f32

SYNTH_SRC=birch.wav
SYNTH_OUT_F32=birch.f32
SYNTH_OUT_RAW=birch_out.raw
SYNTH_STATES_OUT=birch_states_out.f32
SYNTH_TARG_F32=birch_targ.f32
SYNTH_TARG_RAW=birch_out_targ.raw
SYNTH_STATES_TARG=birch_states_targ.f32

SYNTH_TEST=1

# test generation of training data (doesn't really test training as that takes hours)

if [ ! -z $TRAIN_TEST ]; then
    ../build_linux/src/dump_data --train --c2pitch -z 0 -n 1E6 ~/Downloads/$TRAIN_SRC.sw $TRAIN_OUT_F32 $TRAIN_OUT_PCM
    diff $TRAIN_OUT_F32 $TRAIN_TARG_F32 || { echo "ERROR in train .f32 output! Exiting..."; exit 1; }
    echo "train .f32 OK"
    diff $TRAIN_OUT_PCM $TRAIN_TARG_PCM || { echo "ERROR in train .pcm output! Exiting..."; exit 1; }
    echo "train .pcm OK"
fi

# test basic synthesis with build in network

if [ ! -z $SYNTH_TEST ]; then
    ../build_linux/src/dump_data --test --c2pitch ../wav/birch.wav birch.f32
    diff birch_targ.f32 birch.f32 || { echo "ERROR in synth .f32 output! Exiting..."; exit 1; }
    echo "synth .f32 OK"
    ../build_linux/src/test_lpcnet -n lpcnet_190215.f32 -l birch_states.f32 birch.f32 birch_out.raw
    octave -p ../src --no-gui <<< "ret=compare_states('birch_states_targ.f32', 'birch_states.f32'); quit(ret)"
    if [ ! $? -eq 0 ]; then { echo "ERROR in synth states Octave output! Exiting..."; exit 1; } fi
    echo "synth states Octave OK"			    
    diff birch_states_targ.f32 birch_states.f32 || { echo "ERROR in synth states output! Exiting ..."; exit 1; }
    echo "synth states OK"
    diff birch_out_targ.raw birch_out.raw || { echo "ERROR in synth .raw output! Exiting..."; exit 1; }
    echo "synth .raw OK"
fi

echo "all tests PASSED"
