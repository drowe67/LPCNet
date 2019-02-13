/*
   lpcnet_dump.h
   Feb 2019

   LPCnet "dump" functions is API form.  These functions take input
   speech frames and extract features.
*/

/* Copyright (c) 2017-2018 Mozilla */
/*
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef __LPCNET_DUMP__
#define __LPCNET_DUMP__

#include "freq.h"
#include "codec2_pitch.h"

#define LPCNET_NB_FEATURES 55

typedef struct DenoiseState_s DenoiseState;

typedef struct {
    DenoiseState *st;
    float mem_hp_x[2];
    float mem_preemph;
    short tmp[FRAME_SIZE];
    CODEC2_PITCH *c2pitch;
    int c2_Sn_size, c2_frame_size;
    float *c2_Sn;
} LPCNET_DUMP;

LPCNET_DUMP *lpcnet_dump_create(void);
void lpcnet_dump_destroy(LPCNET_DUMP *d);
void lpcnet_dump(LPCNET_DUMP *d, float x[], float features[]);

#endif
