# Makefile for LPCNet

CC=gcc
CFLAGS+=-Wall -W -Wextra -Wno-unused-function -O3 -g -I../include -MD

AVX2:=$(shell cat /proc/cpuinfo | grep -c avx2)
AVX:=$(shell cat /proc/cpuinfo | grep -c avx)
NEON:=$(shell cat /proc/cpuinfo | grep -c neon)

ifneq ($(AVX2),0)
CFLAGS+=-mavx2 -mfma 
else
# AVX2 machines will also match on AVX
ifneq ($(AVX),0)
CFLAGS+=-mavx
endif
endif

ifneq ($(NEON),0)
CFLAGS+=-mfpu=neon -march=armv8-a -mtune=cortex-a53
endif

PROG=dump_data test_lpcnet test_vec quant_feat tcodec2_pitch weight tdump tweak_pitch quant_test \
     quant2c diff32 quant_enc quant_dec lpcnet_enc lpcnet_dec idct
all: $(PROG)

dump_data_objs := src/dump_data.o src/freq.o src/kiss_fft.o src/pitch.o src/celt_lpc.o src/codec2_pitch.o
dump_data_deps := $(dump_data_objs:.o=.d)
dump_data: $(dump_data_objs)
	gcc -o $@ $(CFLAGS) $(dump_data_objs) -lm -lcodec2

-include $dump_data_deps(_deps)

tdump_objs := src/tdump.o src/freq.o src/kiss_fft.o src/pitch.o src/celt_lpc.o src/codec2_pitch.o src/lpcnet_dump.o
 tdump_deps := $(tdump_objs:.o=.d)
tdump: $(tdump_objs)
	gcc -o $@ $(CFLAGS) $(tdump_objs) -lm -lcodec2

-include $tdump_deps(_deps)

test_lpcnet_objs := src/test_lpcnet.o src/lpcnet.o src/nnet.o src/nnet_data.o src/freq.o src/kiss_fft.o src/pitch.o src/celt_lpc.o
test_lpcnet_deps := $(test_lpcnet_objs:.o=.d)
test_lpcnet: $(test_lpcnet_objs)
	gcc -o $@ $(CFLAGS) $(test_lpcnet_objs) -lm

-include $(test_lpcnet_deps)

test_vec_objs := src/test_vec.o
test_vec_deps := $(test_vec_objs:.o=.d)
test_vec: $(test_vec_objs)
	gcc -o $@ $(CFLAGS) $(test_vec_objs) -lm

-include $(test_vec_deps)

quant_feat_objs := src/quant_feat.o src/freq.o src/kiss_fft.o src/celt_lpc.o src/pitch.o src/mbest.o src/lpcnet_quant.o
quant_feat_deps := $(quant_feat_objs:.o=.d)
quant_feat: $(quant_feat_objs)
	gcc -o $@ $(CFLAGS) $(quant_feat_objs) -lm
-include $(quant_feat_deps)

quant_test_objs := src/quant_test.o src/freq.o src/kiss_fft.o src/celt_lpc.o src/pitch.o src/mbest.o src/lpcnet_quant.o src/4stage_pred_vq.o
quant_test_deps := $(quant_test_objs:.o=.d)
quant_test: $(quant_test_objs)
	gcc -o $@ $(CFLAGS) $(quant_test_objs) -lm
-include $(quant_test_deps)

quant_enc_objs := src/quant_enc.o src/freq.o src/kiss_fft.o src/celt_lpc.o src/pitch.o src/mbest.o src/lpcnet_quant.o src/4stage_pred_vq.o
quant_enc_deps := $(quant_enc_objs:.o=.d)
quant_enc: $(quant_enc_objs)
	gcc -o $@ $(CFLAGS) $(quant_enc_objs) -lm
-include $(quant_enc_deps)

quant_dec_objs := src/quant_dec.o src/freq.o src/kiss_fft.o src/celt_lpc.o src/pitch.o src/mbest.o src/lpcnet_quant.o src/4stage_pred_vq.o
quant_dec_deps := $(quant_dec_objs:.o=.d)
quant_dec: $(quant_dec_objs)
	gcc -o $@ $(CFLAGS) $(quant_dec_objs) -lm
-include $(quant_dec_deps)

lpcnet_enc_objs := src/lpcnet_enc.o src/freq.o src/kiss_fft.o src/celt_lpc.o src/pitch.o src/codec2_pitch.o src/mbest.o src/lpcnet_quant.o \
                   src/4stage_pred_vq.o src/lpcnet_dump.o
lpcnet_enc_deps := $(lpcnet_enc_objs:.o=.d)
lpcnet_enc: $(lpcnet_enc_objs)
	gcc -o $@ $(CFLAGS) $(lpcnet_enc_objs) -lm -lcodec2
-include $(lpcnet_enc_deps)

lpcnet_dec_objs := src/lpcnet_dec.o src/freq.o src/kiss_fft.o src/celt_lpc.o src/pitch.o src/codec2_pitch.o src/mbest.o src/lpcnet_quant.o \
                   src/4stage_pred_vq.o src/lpcnet_dump.o src/lpcnet.o src/nnet.o src/nnet_data.o
lpcnet_dec_deps := $(lpcnet_dec_objs:.o=.d)
lpcnet_dec: $(lpcnet_dec_objs)
	gcc -o $@ $(CFLAGS) $(lpcnet_dec_objs) -lm -lcodec2
-include $(lpcnet_dec_deps)

quant2c_objs:= src/quant2c.o
quan2c_deps := $(quant2c_objs:.o=.d)
quant2c: $(quant2c_objs)
	gcc -o $@ $(CFLAGS) $(quant2c_objs) -lm
-include $(quant2c_deps)

tcodec2_pitch_objs := src/tcodec2_pitch.o src/codec2_pitch.o
tcodec2_pitch_deps := $(tcodec2_pitch_objs:.o=.d)
tcodec2_pitch: $(tcodec2_pitch_objs)
	gcc -o $@ $(CFLAGS) $(tcodec2_pitch_objs) -lm -lcodec2
-include $(tcodec2_pitch_deps)

weight_objs := src/weight.o
weight_deps := $(weight_objs:.o=.d)
weight: $(weight_objs)
	gcc -o $@ $(CFLAGS) $(weight_objs) -lm
-include $(weight_deps)

idct_objs := src/idct.o src/freq.o src/kiss_fft.o src/celt_lpc.o src/pitch.o
idct_deps := $(idct_objs:.o=.d)
idct: $(idct_objs)
	gcc -o $@ $(CFLAGS) $(idct_objs) -lm
-include $(idct_deps)

tweak_pitch_objs := src/tweak_pitch.o
tweak_pitch_deps := $(tweak_pitch_objs:.o=.d)
tweak_pitch: $(tweak_pitch_objs)
	gcc -o $@ $(CFLAGS) $(tweak_pitch_objs) -lm

diff32: src/diff32.c
	gcc -o $@ $(CFLAGS) src/diff32.c -lm

-include $(tweak_pitch_deps)

test: test_vec
	./test_vec

clean:
	rm -f $(PROG)
	rm -f $(dump_data_objs) $(dump_data_deps) 
	rm -f $(test_lpcnet_objs) $(test_lpcnet_deps) 
	rm -f $(test_vec_objs) $(test_vec_deps) 
	rm -f $(quant_feat_objs) $(quant_feat_deps) 
	rm -f $(tdump_objs) $(tdump_deps) 
