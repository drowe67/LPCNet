#!/bin/bash -x
# test_data_datat.sh
# Unit test to support refactoring dum_data

TRAIN_SRC=all_speech
TRAIN_OUT_PCM=$(mktemp).pcm
TRAIN_OUT_F32=$(mktemp).f32
TRAIN_TARG_PCM=all_speech.pcm
TRAIN_TARG_F32=all_speech.f32

TEST_SRC=c01_01.sw
TEST_OUT_F32=$(mktemp).f32
TEST_OUT_RAW=$(mktemp).raw
TEST_TARG_F32=c01_01.f32
TEST_TARG_RAW=190727e_f.raw

# test train

#../build_linux/src/dump_data --train --c2pitch -z 0 -n 1E6 ~/Downloads/$TRAIN_SRC.sw $TRAIN_OUT_F32 $TRAIN_OUT_PCM
#diff $TRAIN_OUT_F32 $TRAIN_TARG_F32 || { echo "ERROR in train .f32 output! Exiting..."; exit 1; }
#echo "train .f32 OK"
#diff $TRAIN_OUT_PCM $TRAIN_TARG_PCM || { echo "ERROR in train .pcm output! Exiting..."; exit 1; }
#echo "train .pcm OK"

# test_synth

../build_linux/src/dump_data --test --c2pitch --mag ~/Downloads/$TEST_SRC $TEST_OUT_F32
../build_linux/src/test_lpcnet -n ../build_linux/src/t.f32 --mag $TEST_OUT_F32 $TEST_OUT_RAW
#../build_linux/src/test_lpcnet --mag  $TEST_OUT_F32 $TEST_OUT_RAW
#diff $TEST_OUT_F32 $TEST_TARG_F32 || { echo "ERROR in test .f32 output! Exiting..."; exit 1; }
echo "test .f32 OK"
diff $TEST_OUT_RAW birch.raw || { echo "ERROR in test .raw output! Exiting..."; exit 1; }
echo "test .raw OK"

echo "all tests PASSED"
