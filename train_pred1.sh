#!/bin/sh

PATH=/home/david/codec2-dev/build_linux/misc/
VQTRAIN=$PATH/vqtrain
EXTRACT=$PATH/extract

echo "*********"
echo "Pred 1"
echo "*********"
$EXTRACT all_speech_features.f32 all_speech_pred1.f32 0 17 10 0.9
$VQTRAIN all_speech_pred1.f32 18 2048 /dev/null s11.f32
$VQTRAIN s11.f32 18 2048 /dev/null s12.f32
$VQTRAIN s12.f32 18 2048 /dev/null s13.f32
#$VQTRAIN s13.f32 18 512 /dev/null s14.f32
