#!/bin/sh

PATH=/home/david/codec2-dev/build_linux/misc/
VQTRAIN=$PATH/vqtrain
EXTRACT=$PATH/extract
VQTRAIN=/home/david/codec2-dev/build_linux/misc/vqtrain
K=8

echo "*********"
echo "Direct"
echo "*********"
$EXTRACT all_speech_features.f32 all_speech_direct.f32 0 7 10 0
$VQTRAIN all_speech_direct.f32 $K 2048 direct_stage1.f32 sd1.f32
$VQTRAIN sd1.f32 $K 2048 direct_stage2.f32 sd2.f32
$VQTRAIN sd2.f32 $K 2048 direct_stage3.f32 sd3.f32
$VQTRAIN sd3.f32 $K 2048 direct_stage4.f32 sd4.f32
$VQTRAIN sd4.f32 $K 2048 direct_stage5.f32 sd5.f32
