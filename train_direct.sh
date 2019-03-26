#!/bin/sh
# train_direct.sh
# David Rowe March 2019
# Train multi-stage VQ direct (non predictive) for LPCNet

PATH=$PATH:/home/david/codec2-dev/build_linux/misc/

if [ $# -lt 1 ]; then
    echo "usage: ./train_direct.sh [-i] VQprefix"
    echo "       $ ./train_direct.sh direct_v1"
    echo "  -i   work in Ly (log magnitude) domain"
    exit 1
fi

for i in "$@"
do
case $i in
    -i)
        LOGMAG=1
        shift # past argument=value
    ;;
esac
done

VQ_NAME=$1
echo $VQ_NAME

K=18
FINAL_K=12
STOP=1E-1

echo "*********"
echo "Direct"
echo "*********"
t=$(mktemp)
extract -e `expr $K - 1` -g 10 all_speech_features_5e6.f32 $t 
if [ -z "$LOGMAG" ]; then
  echo "weighting dctLy[0] ...."
  cat $t | ./weight > $VQ_NAME'_s0.f32'
else
  echo "working in Ly (log magnitude) domain"
  cat $t | ./idct > $VQ_NAME'_s0.f32'
fi
  
vqtrain $VQ_NAME'_s0.f32' $K 2048 $VQ_NAME'_stage1.f32' -r $VQ_NAME'_s1.f32' -s $STOP 
vqtrain $VQ_NAME'_s1.f32' $K 2048 $VQ_NAME'_stage2.f32' -r $VQ_NAME'_s2.f32' -s $STOP
vqtrain $VQ_NAME'_s2.f32' $K 2048 $VQ_NAME'_stage3.f32' -r $VQ_NAME'_s3.f32' -s $STOP 
if [ -z "$LOGMAG" ]; then
  echo "final two stages $K elements"
  vqtrain $VQ_NAME'_s3.f32' $K 2048 $VQ_NAME'_stage4.f32' -r $VQ_NAME'_s5.f32' -s $STOP 
  vqtrain $VQ_NAME'_s4.f32' $K 2048 $VQ_NAME'_stage5.f32' -r $VQ_NAME'_s6.f32' -s $STOP 
else
  echo "final stage $FINAL_K elements"
  t=$(mktemp)
  extract -e `expr $FINAL_K - 1` -t $K $VQ_NAME'_s3.f32' $t 
  vqtrain $t $FINAL_K 2048 $VQ_NAME'_stage4.f32' -r $VQ_NAME'_s5.f32' -s $STOP -t $K
fi
