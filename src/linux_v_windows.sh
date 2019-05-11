#!/bin/bash
# linux_v_windows.sh
# David Rowe May 2019
#
# Part of system to generate and compare Linux and Windows test files
# that contain LPCNet states.  Use to track down issue with Windows version

export WINEPATH=$HOME/freedv-gui/codec2/build_win/src';'$HOME/freedv-gui/build_win/_CPack_Packages/win64/NSIS/FreeDV-1.4.0-devel-win64/bin/

# start in LPCNet dir
p=$PWD

# Windows
cd build_win/src && make test_lpcnet
wine test_lpcnet.exe wia.f32 wia_out.raw
cd $p

# Linux
cd build_linux/src && make test_lpcnet
./test_lpcnet ../../build_win/src/wia.f32 wia_out.raw
./diff32 test_lpcnet_states.f32 ../../build_win/src/test_lpcnet_states.f32 210
cd $p


