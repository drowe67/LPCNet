#!/bin/sh -x
# ext_pitch.sh
# David Rowe Jan 2019
# Compare internal and external pitch est

PATH=$PATH:$HOME/codec2-dev/build_linux/misc

if [ $# -ne 1 ]; then
    echo "usage: ./ext_pitch WaveFile"
    exit 1
fi

wav=$1
bname=$(basename "$wav" .wav)
tnlp_out=$bname'_f0_pp.txt'
tnlp_f0=$bname'_f0.txt'
raw=$(mktemp)
feat=$bname.f32
feat_ext=$bname'_ext'.f32

echo $bname
sox $1 -t raw $raw
tnlp $raw $tnlp_out --Fs 16000 > /dev/null
cat $tnlp_out | cut -f 1 -d ' ' > $tnlp_f0
./dump_data -test $raw $feat && cat $feat | ./quant_feat -d 1 -e $tnlp_f0 > $feat_ext
octave --no-gui -p src -qf src/plot_wo_test_ext.m $raw $feat $feat_ext $bname'_pitch.png'
