#!/bin/bash -x
# test_core_nn.sh
#

# Some tests for core NN, e.g. generation of test data using
# dump_data, and unquantised synthesis using and test_lpcnet.  Used to
# ensure no existing features are broken during experimentation and
# development.

# Basic synthesis with compiled-in in network

if [ ! -z $SYNTH ]; then
    ../build_linux/src/dump_data --test --c2pitch --c2voicing ../wav/birch.wav birch.f32
    ../build_linux/src/diff32 birch_targ.f32 birch.f32 || { echo "ERROR in synth .f32 output! Exiting..."; exit 1; }
    echo "synth .f32 OK"
    ../build_linux/src/test_lpcnet -l birch_states.f32 birch.f32 birch_out.raw
    octave -p ../src --no-gui <<< "ret=compare_states('birch_states_targ.f32', 'birch_states.f32'); quit(ret)"
    if [ ! $? -eq 0 ]; then { echo "ERROR in synth states Octave output! Exiting..."; exit 1; } fi
    echo "synth states Octave OK"			    
    diff birch_states_targ.f32 birch_states.f32 || { echo "ERROR in synth states output! Exiting ..."; exit 1; }
    echo "synth states OK"
    diff birch_out_targ.raw birch_out.raw || { echo "ERROR in synth .raw output! Exiting..."; exit 1; }
    echo "synth .raw OK"
fi

# Synthesis with the 20h network, loaded up at run time
    
if [ ! -z $SYNTH_20h ]; then
    ../build_linux/src/dump_data --test --c2pitch --c2voicing ../wav/birch.wav birch.f32
    ../build_linux/src/diff32 birch_targ.f32 birch.f32 || { echo "ERROR in synth .f32 output! Exiting..."; exit 1; }
    echo "synth .f32 OK"
    ../build_linux/src/test_lpcnet -n lpcnet_20h.f32 -l birch_states.f32 birch.f32 birch_out.raw
    octave -p ../src --no-gui <<< "ret=compare_states('birch_20h_states_targ.f32', 'birch_states.f32'); quit(ret)"
    if [ ! $? -eq 0 ]; then { echo "ERROR in synth states Octave output! Exiting..."; exit 1; } fi
    echo "synth states Octave OK"			    
    diff birch_20h_states_targ.f32 birch_states.f32 || { echo "ERROR in synth states output! Exiting ..."; exit 1; }
    echo "synth states OK"
    diff birch_20h_targ.raw birch_out.raw || { echo "ERROR in synth .raw output! Exiting..."; exit 1; }
    echo "synth .raw OK"
fi

# Testing log mag operation, using 190804a network.  Not checking states in this test
    
if [ ! -z $SYNTH_mag ]; then
    ../build_linux/src/dump_data --mag --test --c2pitch --c2voicing ../wav/c01_01.wav c01_01.f32
    ../build_linux/src/diff32 c01_01_mag.f32 c01_01.f32 || { echo "ERROR in synth .f32 output! Exiting..."; exit 1; }
    echo "mag .f32 OK"
    ../build_linux/src/test_lpcnet --mag 1 -n lpcnet_190804a.f32 c01_01.f32 c01_01_out.raw
    diff c01_01_190804a_targ.raw c01_01_out.raw || { echo "ERROR in synth .raw output! Exiting..."; exit 1; }
    echo "mag .raw OK"
fi

echo "all tests PASSED"
