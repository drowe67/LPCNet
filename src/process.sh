#!/bin/bash -x
# process.sh
# David Rowe Jan 2019
#
: '
  1. Process an input set of wave files using LPCNet under a variety of conditions.
  2. Name output files to make them convenient to listen to in a file manager.
  3. Generate a HTML table of samples for convenient replay on the web.
  4. Generate a bunch of other HTML files and PNGs.

  usage: ./process.sh [--lite] OutPath
         ./process.sh ~/tmp/lpcnet_out
  
  To compare generate new samples OutPathA, and compare with those from a previous 
  run of this script in OutPathB:

        ./process.sh OutPathA OutPathB

  --lite generates a much smaller page with just the basic LPCNet model case
'

# command line arguments

if [ $# -lt 1 ]; then
    echo "usage: ./process2.sh [--lite] /output/path/1 [/output/path/2]"
    echo "       $ ./process.sh ~/tmp/lpcnet_outA"
    exit 1
fi

lite=0
for i in "$@"
do
case $i in
    --lite)
        lite=1
        shift
    ;;
esac
done

OUTPATH=$1
if [ $# -eq 2 ]; then
    OUTPATHB=$2
fi

# set these paths to suit your system
CODEC2_PATH=$HOME/codec2-dev/build_linux/src
WAVIN_PATH=$HOME/Desktop/deep/quant

WAVOUT_PATH=$OUTPATH/wav
PATH=$PATH:$CODEC2_PATH
STATS=$OUTPATH/stats.txt
HTML=$OUTPATH/index.html
PNG_PATH=$OUTPATH/png
F32_PATH=$OUTPATH/f32
SV_PATH=$OUTPATH/sv
WAV_FILES="all birch canadian glue oak separately wanted wia"

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

vq=pred_v2_stage
vq2=split_stage

# cp in originals
for f in $WAV_FILES
do
    cp $WAVIN_PATH/$f.wav $WAVOUT_PATH/$f'_0_orig.wav'
done

# Unquantised, baseline analysis-synthesis model, 10ms updates
for f in $WAV_FILES
do
    sox $WAVIN_PATH/$f.wav -t raw - | ./dump_data --test --c2pitch - - | \
    ./test_lpcnet - - | sox -r 16000 -t .s16 -c 1 - $WAVOUT_PATH/$f'_1_uq'.wav
done

if [ $lite -eq 0 ]; then 

     # 3dB uniform quantiser, 10ms updates
    for f in $WAV_FILES
    do
        label=$(printf "3dB %-10s" "$f")
        sox $WAVIN_PATH/$f.wav -t raw - | ./dump_data --test --c2pitch - - | \
        ./quant_feat -l "$label" -d 1 --uniform 3 2>>$STATS | ./test_lpcnet - - | \
        sox -r 16000 -t .s16 -c 1 - $WAVOUT_PATH/$f'_2_3dB'.wav

    done

    # decimate features to 20ms updates, then linearly interpolate back up to 10ms updates
    for f in $WAV_FILES
    do
        sox $WAVIN_PATH/$f.wav -t raw - | ./dump_data --test --c2pitch - - | \
            ./quant_feat -d 2 | ./test_lpcnet - - | sox -r 16000 -t .s16 -c 1 - $WAVOUT_PATH/$f'_3_20ms'.wav 

    done

    # decimate features to 20ms updates, then linearly interpolate back up to 10ms updates, incl pitch + voicing quant
    for f in $WAV_FILES
    do
        sox $WAVIN_PATH/$f.wav -t raw - | ./dump_data --test --c2pitch - - | \
            ./quant_feat -d 2 -o 6 | ./test_lpcnet - - | sox -r 16000 -t .s16 -c 1 - $WAVOUT_PATH/$f'_4_20ms_pq'.wav 

    done

    # 33 bit 3 stage VQ searched with mbest algorithm, 20ms updates
    for f in $WAV_FILES
    do
        label=$(printf "33bit_20ms %-10s" "$f")
        sox $WAVIN_PATH/$f.wav -t raw - | ./dump_data --test --c2pitch - - | \
        ./quant_feat -l "$label" -d 2 -o 6 -w --mbest 5 -q $vq'1.f32',$vq'2.f32',$vq'3.f32' -s $SV_PATH/$f'_5_33bit_20ms'.txt  2>>$STATS | \
        ./test_lpcnet - - | sox -r 16000 -t .s16 -c 1 - $WAVOUT_PATH/$f'_5_33bit_20ms'.wav
    done

    # 33 bit 3 stage VQ searched with mbest algorithm, 30ms updates
    for f in $WAV_FILES
    do
        label=$(printf "33bit_30ms %-10s" "$f")
        sox $WAVIN_PATH/$f.wav -t raw - | ./dump_data --test --c2pitch - - | \
            ./quant_feat -l "$label" -d 3 -o 6 -w --mbest 5 -q $vq'1.f32',$vq'2.f32',$vq'3.f32' -s $SV_PATH/$f'_6_33bit_30ms'.txt 2>>$STATS | \
            ./test_lpcnet - - | sox -r 16000 -t .s16 -c 1 - $WAVOUT_PATH/$f'_6_33bit_30ms'.wav
    done

    # 44 bit 4 stage VQ searched with mbest algorithm, 30ms updates
    for f in $WAV_FILES
    do
        label=$(printf "44bit_30ms %-10s" "$f")
        sox $WAVIN_PATH/$f.wav -t raw - | ./dump_data --test --c2pitch - - | \
        ./quant_feat -l "$label" -d 3 -o 6 -w --mbest 5 -q $vq'1.f32',$vq'2.f32',$vq'3.f32',$vq'4.f32' -s $SV_PATH/$f'_7_44bit_30ms'.txt 2>>$STATS | \
        ./test_lpcnet - - | sox -r 16000 -t .s16 -c 1 - $WAVOUT_PATH/$f'_7_44bit_30ms'.wav
    done

    # non-predictive (direct) 44 bit 4 stage split VQ searched with mbest algorithm, 30ms updates
    for f in $WAV_FILES
    do
        label=$(printf "44bit_sp_30ms %-10s" "$f")
        sox $WAVIN_PATH/$f.wav -t raw - | ./dump_data --test --c2pitch - - | \
        ./quant_feat -l "$label" -d 3 -o 6 -i -p 0 --mbest 5 -q $vq2'1.f32',$vq2'2.f32',$vq2'3.f32',$vq2'4.f32' -s $SV_PATH/$f'_8_44bit_sp_30ms'.txt 2>>$STATS | \
        ./test_lpcnet - - | sox -r 16000 -t .s16 -c 1 - $WAVOUT_PATH/$f'_8_44bit_sp_30ms'.wav
    done

fi # ... if [ $lite -eq 0 ] ...

#
# Create a HTML table of samples ----------------------------------------------------
#

cat << EOF > $HTML
<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01//EN">
<html>
<head>
  <title>LPCNet Samples</title>

</head>
<body>
EOF

if [ $lite -eq 0 ]; then
    cat << EOF > $HTML
    <table>
    <col width="10%">
    <col width="70%">
    <caption>Glossary</caption>
    <tr><th align="left">Term</th><th align="left">Description</th></tr>
    <tr><td>Orig</td><td>Original source input speech</td></tr>
    <tr><td>UQ</td><td>Baseline LPCNet synthesis using unquantised features</td></tr>
    <tr><td>3dB</td><td>Cesptral features uniform quantiser with 3dB steps</td></tr>
    <tr>
      <td>20ms</td>
      <td>Cesptral features decimated to 20ms frame rate, linear interpolation back to 10ms</td>
    </tr>
    <tr>
      <td>20ms_pq</td>
      <td>As above but pitch quantised to 6 bits, pitch gain to 2 bits</td>
    </tr>
    <tr>
      <td>33bit_20ms</td>
      <td>3 stage VQ of prediction error, 11 bits/stage, at 20ms frame rate, (33+8)/0.02 = 2050 bits/s</td>
    </tr>
    <tr>
      <td>33bit_30ms</td>
      <td>Same 33 bit VQ, but decimated down to 30ms rate, (33+8)/0.03 = 1367 bits/s</td>
    </tr>
    <tr>
      <td>44bit_30ms</td>
      <td>4 stage VQ, at 30ms update rate, (44+8)/0.03 = 1733 bits/s</td>
    </tr>
    <tr>
      <td>44bit_sp_30ms</td>
      <td>Direct (non predictive) 4 stage split VQ, at 30ms update rate, (44+8)/0.03 = 1733 bits/s.  First 3 stages are 18 elements wide, last stage is just 12.  We quantise log magnitudes (Ly) rather than Ceptrals (dct(Ly)). Targeted at HF radio channel where predictive schemes perform poorly due to high bit error/packet error rate</td>
    </tr>
    </table>
    <p>
EOF
fi

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

# for each wave file, create a row

printf "<table>\n" >> $HTML
printf "<caption>Samples</caption>\n" >> $HTML

heading_row

for f in $WAV_FILES
do
    files=$(ls $WAVOUT_PATH/$f*);
    printf "<tr>\n  <td>%s</td>\n" $f >> $HTML
    for w in $files
    do
        b=$(basename $w)
        if [ -z "${OUTPATHB}" ]; then
            # no comparison
            printf "  <td align="center"><a href=\"wav/%s\">play</a></td>\n" $b >> $HTML
        else
            # compare with another process.sh run
            printf "  <td align="center"><a href=\"wav/%s\">play</a> (<a href=\"%s\">playB</a>) </td>\n" $b $OUTPATHB/wav/$b >> $HTML
        fi
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

if [ $lite -eq 0 ]; then
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

    # manually set pitch axis to make plots easier to read.  TODO this breaks when we add new samples, need an associative array
    mx=(400 200 200 400 400 200 400 200)
    count=0

    # row of links to PNGs
    printf "<tr>\n" >> $HTML
    for f in $WAV_FILES
    do
        sox $WAVIN_PATH/$f.wav -t raw $F32_PATH/$f.raw
        ./dump_data --test --c2pitch $F32_PATH/$f.raw $F32_PATH/$f'_c2'.f32
        octave --no-gui -p src -qf src/plot_speech_pitch.m $F32_PATH/$f.raw  $F32_PATH/$f'_c2'.f32 - $PNG_PATH/$f'_pitch.png' ${mx[count]}
        count=$(( $count + 1 ))
        b=$f'_pitch.png'
        printf "  <td align="center"><a href=\"png/%s\"><img width=100 height=100 src=\"png/%s\" /></a></td>\n" $b $b >> $HTML
    done
    printf "</tr>\n" >> $HTML
    printf "</table><p>\n" >> $HTML

    table_of_values "quant" "Quantiser Error Countours"
fi

printf "</body>\n</html>\n" >> $HTML
