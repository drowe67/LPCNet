#!/bin/bash -x
# run_regen.sh
# David Rowe April 2019

# original 
../build_linux/src/test_lpcnet --mag ../build_linux/c01_01.f32 ../build_linux/f.s16'
../build_linux/src/test_lpcnet --mag ../build_linux/mk61_01.f32 ../build_linux/m.s16'

# regenreate HF magnitudes, don't subtract mean of each vector
#OUT=0
#./train_regen.py ../build_linux/all_speech.f32
#./test_regen.py ~/LPCNet/build_linux/c01_01.f32 ~/LPCNet/build_linux/$OUT'_f.f32'
#./test_regen.py ~/LPCNet/build_linux/mk61_01.f32 ~/LPCNet/build_linux/$OUT'_m.f32'
#../build_linux/src/test_lpcnet --mag ../build_linux/$OUT'_f.f32' ../build_linux/$OUT'_f.s16'
#../build_linux/src/test_lpcnet --mag ../build_linux/$OUT'_m.f32' ../build_linux/$OUT'_m.s16'

# regenerate HF magnitudes, subtract mean of each vector
#OUT=1
#./train_regen.py ../build_linux/all_speech.f32 1
#./test_regen.py ~/LPCNet/build_linux/c01_01.f32 ~/LPCNet/build_linux/$OUT'_f.f32'
#./test_regen.py ~/LPCNet/build_linux/mk61_01.f32 ~/LPCNet/build_linux/$OUT'_m.f32'
#../build_linux/src/test_lpcnet --mag ../build_linux/$OUT'_f.f32' ../build_linux/$OUT'_f.s16'
#../build_linux/src/test_lpcnet --mag ../build_linux/$OUT'_m.f32' ../build_linux/$OUT'_m.s16'

# regenerate HF magnitudes, subtract mean of low bands from each vector
OUT=2
./train_regen.py ../build_linux/all_speech.f32 2
./test_regen.py ~/LPCNet/build_linux/c01_01.f32 ~/LPCNet/build_linux/$OUT'_f.f32'
./test_regen.py ~/LPCNet/build_linux/mk61_01.f32 ~/LPCNet/build_linux/$OUT'_m.f32'
../build_linux/src/test_lpcnet --mag ../build_linux/$OUT'_f.f32' ../build_linux/$OUT'_f.s16'
../build_linux/src/test_lpcnet --mag ../build_linux/$OUT'_m.f32' ../build_linux/$OUT'_m.s16'
