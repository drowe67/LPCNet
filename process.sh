#!/bin/bash -x
# process.sh
# David Rowe Jan 2019
#
# 1. Process an input set of wave files using LPCNet under a variety of conditions.
# 2. Name output files to make them convenient to listen to in a file manager.
# 3. Generate a HTML table of samples for convenient replay on the web.
# 4. Generate a HTML table of distortion metrics.

# set these paths to suit your system
CODEC2_PATH=$HOME/codec2-dev/build_linux/src
WAVIN_PATH=$HOME/Desktop/deep/quant
OUTPATH=$HOME/tmp/lpcnet_out

WAVOUT_PATH=$OUTPATH/wav
PATH=$PATH:$CODEC2_PATH
STATS=$OUTPATH/stats.txt
HTML=$OUTPATH/results.html
PNG_PATH=$OUTPATH/png
F32_PATH=$OUTPATH/f32
SV_PATH=$OUTPATH/sv
WAV_FILES="birch glue oak separately wanted wia"

# check we can find wave files
for f in $WAV_INFILES
do
    if [ ! -e $WAVIN_PATH/$f.wav ]; then
        echo "$WAVIN_PATH/$f.wav Not found"
    fi
done

# check we can find codec 2 tools
if [ ! -e $CODEC2_PATH/c2enc ]; then
    echo "$CODEC2_PATH/c2enc not found"
fi

#
# OK lets start processing ------------------------------------------------
#
mkdir -p $F32_PATH
mkdir -p $SV_PATH

mkdir -p $WAVOUT_PATH
mkdir -p $PNG_PATH
rm -f $STATS

# cp in originals
for f in $WAV_FILES
do
    cp $WAVIN_PATH/$f.wav $WAVOUT_PATH/$f'_0_orig.wav'
done

# Unquantised, baseline analysis-synthesis model, 10ms updates
for f in $WAV_FILES
do
    sox $WAVIN_PATH/$f.wav -t raw - | ./dump_data -test - - | \
    ./test_lpcnet - - | sox -r 16000 -t .s16 -c 1 - $WAVOUT_PATH/$f'_1_uq'.wav
done

# 3dB uniform quantiser, 10ms updates
for f in $WAV_FILES
do
    label=$(printf "3dB %-10s" "$f")
    sox $WAVIN_PATH/$f.wav -t raw - | ./dump_data -test - - | \
    ./quant_feat -l "$label" -d 1 --uniform 3 2>>$STATS | ./test_lpcnet - - | \
    sox -r 16000 -t .s16 -c 1 - $WAVOUT_PATH/$f'_2_3dB'.wav
                                                           
done

# decimate features to 20ms updates, then lineary interpolate back up to 10ms updates
for f in $WAV_FILES
do
    sox $WAVIN_PATH/$f.wav -t raw - | ./dump_data -test - - | \
        ./quant_feat -d 2 | ./test_lpcnet - - | sox -r 16000 -t .s16 -c 1 - $WAVOUT_PATH/$f'_3_20ms'.wav 
        
done

# 33 bit 3 stage VQ searched with mbest algorithm, 20ms updates
for f in $WAV_FILES
do
    label=$(printf "33bit_20ms %-10s" "$f")
    sox $WAVIN_PATH/$f.wav -t raw - | ./dump_data -test - - | \
    ./quant_feat -l "$label" -d 2 --mbest 5 -q pred2_stage1.f32,pred2_stage2.f32,pred2_stage3.f32 -s $SV_PATH/$f'_4_33bit_20ms'.txt  2>>$STATS | \
    ./test_lpcnet - - | sox -r 16000 -t .s16 -c 1 - $WAVOUT_PATH/$f'_4_33bit_20ms'.wav
done

# 33 bit 3 stage VQ searched with mbest algorithm, 30ms updates
for f in $WAV_FILES
do
    label=$(printf "33bit_30ms %-10s" "$f")
    sox $WAVIN_PATH/$f.wav -t raw - | ./dump_data -test - - | \
        ./quant_feat -l "$label" -d 3 --mbest 5 -q pred2_stage1.f32,pred2_stage2.f32,pred2_stage3.f32 -s $SV_PATH/$f'_5_33bit_30ms'.txt 2>>$STATS | \
        ./test_lpcnet - - | sox -r 16000 -t .s16 -c 1 - $WAVOUT_PATH/$f'_5_33bit_30ms'.wav
done

# 44 bit 4 stage VQ searched with mbest algorithm, 30ms updates
for f in $WAV_FILES
do
    label=$(printf "44bit_30ms %-10s" "$f")
    sox $WAVIN_PATH/$f.wav -t raw - | ./dump_data -test - - | \
    ./quant_feat -l "$label" -d 3 --mbest 5 -q pred2_stage1.f32,pred2_stage2.f32,pred2_stage3.f32,pred2_stage4.f32 -s $SV_PATH/$f'_6_44bit_30ms'.txt 2>>$STATS | \
    ./test_lpcnet - - | sox -r 16000 -t .s16 -c 1 - $WAVOUT_PATH/$f'_6_44bit_30ms'.wav
done

# Codec 2 and simulated SSB (10dB SNR, AWGN channel, 10Hz freq offset) reference samples
for f in $WAV_FILES
do
    sox $WAVIN_PATH/$f.wav -t raw -r 8000 - | c2enc 2400 - - | c2dec 2400 - - | \
    sox -r 8000 -t .s16 -c 1 - $WAVOUT_PATH/$f'_7_c2_2400'.wav
done
for f in $WAV_FILES
do
    sox -r 16000 -c 1 $WAVIN_PATH/$f.wav -r 8000 -t raw - | \
        cohpsk_ch - - -35 --Fs 8000 -f 10 --ssbfilt 1 |  \
    sox -r 8000 -t .s16 -c 1 - $WAVOUT_PATH/$f'_8_ssb_10dB'.wav
done

#
# Create a HTML table of samples ----------------------------------------------------
#

cat << EOF > $HTML
<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01//EN">
<html>
<head>
  <title>LPCNet Samples</title>
  <style type="text/css">
  <body>table {
    table-layout: fixed ;
    width: 100% ; 
  }
  td {
    width: 10% ;
  } 
  </style>
</head>
<body>
EOF

function heading_row {
    w=$(echo $WAV_FILES | cut -d ' ' -f 1)
    headings=$(ls $WAVOUT_PATH/$w* | sed -r "s/.*$w.[[:digit:]]_//" | sed -r 's/.wav//')
    printf "<tr>\n  <th align="left">Sample</th>\n" >> $HTML
    for h in $headings
    do
        printf "  <th>%s</th>\n" $h >> $HTML
    done
    printf "</tr>\n" >> $HTML
}

heading_row

# for each wave file, create a row

printf "<table>\n" >> $HTML
printf "<caption>Samples</caption>\n" >> $HTML

for f in $WAV_FILES
do
    files=$(ls $WAVOUT_PATH/$f*);
    printf "<tr>\n  <td>%s</td>\n" $f >> $HTML
    for w in $files
    do
        b=$(basename $w)
        printf "  <td align="center"><a href=\"wav/%s\">play</a></td>\n" $b >> $HTML
    done
    printf "</tr>\n" >> $HTML
done
printf "</table><p>\n" >> $HTML

# HTML table of results ---------------------------------------------------------

function table_of_values {
    printf "<table>\n" >> $HTML
    printf "<caption>%s</caption>\n" "$2" >> $HTML

    heading_row

    # for each wave file, create a row

    for f in $WAV_FILES
    do
        files=$(ls $WAVOUT_PATH/$f*);
        printf "<tr>\n  <td>%s</td>\n" $f >> $HTML
        for h in $headings
        do
            # extract variance from stats file
            if [ "$1" = "var" ]; then
                s=$(cat $STATS | sed -n "s/RESULTS $h $f.*var: \(.*\) sd.*/\1/p")
            fi
            if [ "$1" = "sd" ]; then
                s=$(cat $STATS | sed -n "s/RESULTS $h $f.*sd: \(.*\) n.*/\1/p")
            fi
            if [ "$s" = "" ]; then
               s="-"
            fi
            if [ $1 = "outliers" ]; then
                outliers=$(cat $STATS | sed -n "s/RESULTS $h $f.*dB = \(.*\)/\1/p")
                if [ ! "$outliers" = "" ]; then
                    png=$PNG_PATH/$f'_'$h'.png'
                    cmd="graphics_toolkit ('gnuplot'); o=[$outliers]; bar([1-sum(o) o],'hist'); axis([0 4 0 1]); print(\"$png\",'-dpng','-S120,120')"
                    octave --no-gui -qf --eval "$cmd"
                    b=$(basename $png)
                    printf "  <td align=center><img src=\"png/%s\" ></img></td>\n" $b >> $HTML
                else
                    printf "  <td></td>\n" >> $HTML                    
                fi
            elif [ $1 = "quant" ]; then
                sf=$SV_PATH/$f'_?_'$h.txt
                if [ -e $sf ]; then
                    png=$PNG_PATH/$f'_'$h'_quant.png'
                    t=$(echo $h | sed -n "s/.*_\(.*\)ms/\1/p")
                    octave --no-gui -p src -qf src/plot_speech_quant.m $F32_PATH/$f.raw $sf $png $t
                    b=$(basename $png)
                    printf "  <td align=center><a href="png/%s"><img width=100 height=100 src=\"png/%s\" ></img></a></td>\n" $b $b >> $HTML
                else
                    printf "  <td></td>\n" >> $HTML                                        
                fi
            else
                printf "  <td align="center">%s</td>\n" $s >> $HTML
            fi
        done
        printf "</tr>\n" >> $HTML
    done

    printf "</table><p>\n" >> $HTML
}

table_of_values "var" "Variance"
table_of_values "sd"  "Standard Deviation"
table_of_values "outliers" "Outliers"

#
# Table of Speech/Pitch countours ----------------------------------------------
#

printf "<table>\n" >> $HTML
printf "<caption>Pitch Countours</caption>\n" >> $HTML

# heading row
printf "<tr>\n" >> $HTML
for f in $WAV_FILES
do
    printf "  <th>%s</th>\n" $f >> $HTML
done
printf "</tr>\n" >> $HTML

# manually set pitch axis to make plots easier to read
mx=(200 400 400 200 400 200)
count=0

# row of links to PNGs
printf "<tr>\n" >> $HTML
for f in $WAV_FILES
do
    sox $WAVIN_PATH/$f.wav -t raw $F32_PATH/$f.raw
    ./dump_data -test $F32_PATH/$f.raw $F32_PATH/$f.f32
    octave --no-gui -p src -qf src/plot_speech_pitch.m $F32_PATH/$f.raw  $F32_PATH/$f.f32 $PNG_PATH/$f'_pitch.png' ${mx[count]}
    count=$(( $count + 1 ))
    b=$f'_pitch.png'
    printf "  <td align="center"><a href=\"png/%s\"><img width=100 height=100 src=\"png/%s\" /></a></td>\n" $b $b >> $HTML
done
printf "</tr>\n" >> $HTML
printf "</table><p>\n" >> $HTML


table_of_values "quant" "Quantiser Error Countours"

printf "</body>\n</html>\n" >> $HTML

