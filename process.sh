#!/bin/bash -x
# process.sh
# David Rowe Jan 2019
#
# 1. Process an input set of wave files using LPCNet under a variety of conditions.
# 2. Name output files to make them convenient to listen to in a file manager.
# 3. Generate a HTML table of samples for convenient replay on the web.
# 4. Generate a HTML table of distortion metrics.

# set these two paths to suit your system
CODEC2_PATH=$HOME/codec2-dev/build_linux/src
WAV_INPATH=$HOME/Desktop/deep/quant
WAV_OUTPATH=$HOME/tmp/lpcnet_out

PATH=$PATH:$CODEC2_PATH
WAV_FILES="birch glue oak separately wanted wia"
STATS=$WAV_OUTPATH/stats.txt

# check we can find wave files
for f in $WAV_INFILES
do
    if [ ! -e $WAV_INPATH/$f.wav ]; then
        echo "$WAV_INPATH/$f.wav Not found"
    fi
done

# check we can find codec 2 tools
if [ ! -e $CODEC2_PATH/c2enc ]; then
    echo "$CODEC2_PATH/c2enc not found"
fi

#
# OK lets start processing ------------------------------------------------
#

mkdir -p $WAV_OUTPATH
rm -f $STATS

# cp in originals
for f in $WAV_FILES
do
    cp $WAV_INPATH/$f.wav $WAV_OUTPATH/$f.wav
done

# Unquantised, baseline analysis-synthesis model, 10ms updates
for f in $WAV_FILES
do
    sox $WAV_INPATH/$f.wav -t raw - | ./dump_data -test - - | \
    ./test_lpcnet - - | sox -r 16000 -t .s16 -c 1 - $WAV_OUTPATH/$f'_1uq'.wav
done

# 3dB uniform quantiser, 10ms updates
for f in $WAV_FILES
do
    label=$(printf "3dB uniform %-10s" "$f")
    sox $WAV_INPATH/$f.wav -t raw - | ./dump_data -test - - | \
    ./quant_feat -l "$label" -d 1 --uniform 3 2>>$STATS | ./test_lpcnet - - | \
    sox -r 16000 -t .s16 -c 1 - $WAV_OUTPATH/$f'_2_3dB'.wav
                                                           
done

# decimate features to 20ms updates, then lineary interpolate back up to 10ms updates
for f in $WAV_FILES
do
    sox $WAV_INPATH/$f.wav -t raw - | ./dump_data -test - - | \
        ./quant_feat -d 2 | ./test_lpcnet - - | sox -r 16000 -t .s16 -c 1 - $WAV_OUTPATH/$f'_3_20ms'.wav 
        
done

# 33 bit 3 stage VQ searched with mbest algorithm, 20ms updates
for f in $WAV_FILES
do
    label=$(printf "33 bit 20ms %-10s" "$f")
    sox $WAV_INPATH/$f.wav -t raw - | ./dump_data -test - - | \
    ./quant_feat -l "$label" -d 2 --mbest 5 -q pred2_stage1.f32,pred2_stage2.f32,pred2_stage3.f32 2>>$STATS | \
    ./test_lpcnet - - | sox -r 16000 -t .s16 -c 1 - $WAV_OUTPATH/$f'_4_33bit_20ms'.wav
done

# 33 bit 3 stage VQ searched with mbest algorithm, 30ms updates
for f in $WAV_FILES
do
    label=$(printf "33 bit 30ms %-10s" "$f")
    sox $WAV_INPATH/$f.wav -t raw - | ./dump_data -test - - | \
        ./quant_feat -l "$label" -d 3 --mbest 5 -q pred2_stage1.f32,pred2_stage2.f32,pred2_stage3.f32 2>>$STATS | \
        ./test_lpcnet - - | sox -r 16000 -t .s16 -c 1 - $WAV_OUTPATH/$f'_5_33bit_30ms'.wav
done

# 44 bit 4 stage VQ searched with mbest algorithm, 30ms updates
for f in $WAV_FILES
do
    label=$(printf "44 bit 30ms %-10s" "$f")
    sox $WAV_INPATH/$f.wav -t raw - | ./dump_data -test - - | \
    ./quant_feat -l "$label" -d 3 --mbest 5 -q pred2_stage1.f32,pred2_stage2.f32,pred2_stage3.f32,pred2_stage4.f32 2>>$STATS | \
    ./test_lpcnet - - | sox -r 16000 -t .s16 -c 1 - $WAV_OUTPATH/$f'_6_44bit_30ms'.wav
done

# Codec 2 and simulated SSB (10dB SNR, AWGN channel, 10Hz freq offset) reference samples
for f in $WAV_FILES
do
    sox $WAV_INPATH/$f.wav -t raw -r 8000 - | c2enc 2400 - - | c2dec 2400 - - | \
    sox -r 8000 -t .s16 -c 1 - $WAV_OUTPATH/$f'_7_c2_2400'.wav
done
for f in $WAV_FILES
do
    sox -r 16000 -c 1 $WAV_INPATH/$f.wav -r 8000 -t raw - | \
        cohpsk_ch - - -35 --Fs 8000 -f 10 --ssbfilt 1 |  \
    sox -r 8000 -t .s16 -c 1 - $WAV_OUTPATH/$f'_8_ssb_10dB'.wav
done

# HTML table of samples
# HTML table of results

