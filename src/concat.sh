#!/bin/bash
# Concatenate .wav files into one headerless .sw training file
# usage: ./concat.sh concatfile.sw

for i in `find . -name '*.wav'`
do
sox $i -r 16000 -c 1 -t sw -
done > $1
