#!/bin/bash
# linux_v_windows.sh
# David Rowe May 2019
#
# Part of system to generate and compare Linux and Windows test files
# that contain LPCNet states.  Use to track down issue with Windows version

export WINEPATH=$HOME/freedv-gui/codec2/build_win/src';'$HOME/freedv-gui/build_win/_CPack_Packages/win64/NSIS/FreeDV-1.4.0-devel-win64/bin/

w=all

# start in LPCNet dir
p=$PWD

# Windows
cd build_win/src && make tdump test_lpcnet lpcnet_enc lpcnet_dec
wine lpcnet_enc.exe -s --infile ../../wav/$w.wav --outfile $w.bin
wine lpcnet_dec.exe -s --infile $w.bin --outfile $w'q_out.raw'
#wine tdump.exe ../../wav/$w.wav $w.f32
#wine test_lpcnet.exe $w.f32 $w'_out.raw'
cd $p

# Linux
cd build_linux/src && make test_lpcnet lpcnet_enc lpcnet_dec diff32
./lpcnet_dec -s --infile ../../build_win/src/$w.bin --outfile $w'q_out'.raw
#./test_lpcnet ../../build_win/src/$w.f32 $w'_out.raw'
./diff32 test_lpcnet_statesq.f32 ../../build_win/src/test_lpcnet_statesq.f32 1842
cd $p


