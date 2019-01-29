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
WAV_INPATH=$HOME/Desktop/deep/quant
OUTPATH=$HOME/tmp/lpcnet_out

WAV_OUTPATH=$OUTPATH/wav
PATH=$PATH:$CODEC2_PATH
STATS=$OUTPATH/stats.txt
HTML=$OUTPATH/lpcnet_results.html
PNG_OUTPATH=$OUTPATH/png
WAV_FILES="birch glue oak separately wanted wia"

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
: '
mkdir -p $WAV_OUTPATH
mkdir -p $PNG_OUTPATH
rm -f $STATS

# cp in originals
for f in $WAV_FILES
do
    cp $WAV_INPATH/$f.wav $WAV_OUTPATH/$f'_0_orig.wav'
done

# Unquantised, baseline analysis-synthesis model, 10ms updates
for f in $WAV_FILES
do
    sox $WAV_INPATH/$f.wav -t raw - | ./dump_data -test - - | \
    ./test_lpcnet - - | sox -r 16000 -t .s16 -c 1 - $WAV_OUTPATH/$f'_1_uq'.wav
done

# 3dB uniform quantiser, 10ms updates
for f in $WAV_FILES
do
    label=$(printf "3dB %-10s" "$f")
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
    label=$(printf "33bit_20ms %-10s" "$f")
    sox $WAV_INPATH/$f.wav -t raw - | ./dump_data -test - - | \
    ./quant_feat -l "$label" -d 2 --mbest 5 -q pred2_stage1.f32,pred2_stage2.f32,pred2_stage3.f32 2>>$STATS | \
    ./test_lpcnet - - | sox -r 16000 -t .s16 -c 1 - $WAV_OUTPATH/$f'_4_33bit_20ms'.wav
done

# 33 bit 3 stage VQ searched with mbest algorithm, 30ms updates
for f in $WAV_FILES
do
    label=$(printf "33bit_30ms %-10s" "$f")
    sox $WAV_INPATH/$f.wav -t raw - | ./dump_data -test - - | \
        ./quant_feat -l "$label" -d 3 --mbest 5 -q pred2_stage1.f32,pred2_stage2.f32,pred2_stage3.f32 2>>$STATS | \
        ./test_lpcnet - - | sox -r 16000 -t .s16 -c 1 - $WAV_OUTPATH/$f'_5_33bit_30ms'.wav
done

# 44 bit 4 stage VQ searched with mbest algorithm, 30ms updates
for f in $WAV_FILES
do
    label=$(printf "44bit_30ms %-10s" "$f")
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

#
# Create a HTML table of samples ----------------------------------------------------
#

'
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

printf "<table>\n" >> $HTML
printf "<caption>Samples</caption>\n" >> $HTML

function heading_row {
    w=$(echo $WAV_FILES | cut -d ' ' -f 1)
    headings=$(ls $WAV_OUTPATH/$w* | sed -r "s/.*$w.[[:digit:]]_//" | sed -r 's/.wav//')
    printf "<tr>\n  <th align="left">Sample</th>\n" >> $HTML
    for h in $headings
    do
        printf "  <th>%s</th>\n" $h >> $HTML
    done
    printf "</tr>\n" >> $HTML
}

heading_row

# for each wave file, create a row

for f in $WAV_FILES
do
    files=$(ls $WAV_OUTPATH/$f*);
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
    #headings1="3dB 33bit_20ms 33bit_30ms"
    # for each wave file, create a row

    for f in $WAV_FILES
    do
        files=$(ls $WAV_OUTPATH/$f*);
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
                    png=$PNG_OUTPATH/$f'_'$h'.png'
                    cmd="graphics_toolkit ('gnuplot'); barh([$outliers],'hist'); axis([0 1 0 5]); print(\"$png\",'-dpng','-S100,100')"
                    #cmd="barh([$outliers],'hist');print(\"$png\",'-dpng','-S100,100')"
                    octave --no-gui -qf --eval "$cmd"
                    b=$(basename $png)
                    printf "  <td align=center><img src=\"png/%s\" ></img></td>\n" $b >> $HTML
                    #printf "  <td align=center>%s</td>\n" $b >> $HTML
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

printf "</body>\n</html>\n" >> $HTML



# grep results line of stats file, extracting 
# sd=$(cat $STATS | sed -n "s/RESULTS $h $f.*sd: \(.*\) n.*/\1/p")
