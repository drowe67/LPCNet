#!/bin/sh -x
# train_pred2.sh
# David Rowe Jan 2019
# Train multi-stage VQ for LPCNet

PATH=$PATH:/home/david/codec2-dev/build_linux/misc/

if [ $# -lt 1 ]; then
    echo "usage: ./train_pred2.sh [-w] VQprefix"
    echo "       $ ./train_pred2.sh pred2_v1"
    exit 1
fi

VQ_NAME=$1
echo $VQ_NAME

K=18
STOP=1E-2

echo "*********"
echo "Pred 2"
echo "*********"
echo "weighting dctLy[0] ...."
t=$(mktemp)
extract all_speech_features_5e6.f32 $t 0 17 10 1 2
cat $t | ./weight > $VQ_NAME'_s0.f32'
vqtrain $VQ_NAME'_s0.f32' $K 2048 $VQ_NAME'_stage1.f32' -r $VQ_NAME'_s1.f32' -s $STOP 
vqtrain $VQ_NAME'_s1.f32' $K 2048 $VQ_NAME'_stage2.f32' -r $VQ_NAME'_s2.f32' -s $STOP
vqtrain $VQ_NAME'_s2.f32' $K 2048 $VQ_NAME'_stage3.f32' -r $VQ_NAME'_s3.f32' -s $STOP 
vqtrain $VQ_NAME'_s3.f32' $K 2048 $VQ_NAME'_stage4.f32' -r $VQ_NAME'_s4.f32' -s $STOP 

