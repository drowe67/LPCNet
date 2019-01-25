#!/bin/sh

PATH=$PATH:/home/david/codec2-dev/build_linux/misc/
K=18
STOP=1E-3

echo "*********"
echo "Pred 2"
echo "*********"
extract all_speech_features_5e6.f32 all_speech_pred2.f32 0 17 10 0.9 2
vqtrain all_speech_pred2.f32 $K 2048 pred2_stage1.f32 s21.f32 $STOP | tee train_pred2.log
vqtrain s21.f32 $K 2048 pred2_stage2.f32 s22.f32 $STOP | tee -a train_pred2.log
vqtrain s22.f32 $K 2048 pred2_stage3.f32 s23.f32 $STOP | tee -a train_pred2.log
vqtrain s23.f32 $K 2048 pred2_stage4.f32 s24.f32 $STOP | tee -a train_pred2.log

