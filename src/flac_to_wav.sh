#!/bin/bash
# Convert all .flac files under this folder to .wav files
# source: several GitHub repos

find . -iname "*.flac" | wc

for flacfile in `find . -iname "*.flac"`
do
    ffmpeg -y -f flac -i $flacfile -ab 64k -ac 1 -ar 16000 -f wav "${flacfile%.*}.wav"
done
