/*
   tdump.c
   Feb 2019

   Simplified version of dump_data.c, stepping stone to lpcnet API.
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

#include <stdlib.h>
#include <stdio.h>
#include "lpcnet_dump.h"

int main(int argc, char **argv) {
  FILE *f1;
  FILE *ffeat;
  LPCNET_DUMP *d;
  
  d = lpcnet_dump_create();  

  if (argc != 3) {
      fprintf(stderr, "Too few arguments\n");
      fprintf(stderr, "usage: %s <speech> <features out>\n", argv[0]);
      exit(1);
  }
    
  if (strcmp(argv[1], "-") == 0)
      f1 = stdin;
  else {
      f1 = fopen(argv[1], "rb");
      if (f1 == NULL) {
          fprintf(stderr,"Error opening input .s16 16kHz speech input file: %s\n", argv[1]);
          exit(1);
      }
  }
  if (strcmp(argv[2], "-") == 0)
      ffeat = stdout;
  else {
      ffeat = fopen(argv[2], "wb");
      if (ffeat == NULL) {
          fprintf(stderr,"Error opening output feature file: %s\n", argv[2]);
          exit(1);
      }
  }
  
  float x[FRAME_SIZE];
  float features[LPCNET_NB_FEATURES];
  int i;
  int f=0;
  int nread;
  
  while (1) {      
      /* note one frame delay */
      for (i=0;i<FRAME_SIZE;i++) x[i] = d->tmp[i];
      nread = fread(&d->tmp, sizeof(short), FRAME_SIZE, f1);
      if (nread != FRAME_SIZE) break;
      lpcnet_dump(d,x,features);
      fwrite(features, sizeof(float), LPCNET_NB_FEATURES, ffeat);
      f++;
  }
  fprintf(stderr, "%d %d %d\n", f, FRAME_SIZE, LPCNET_NB_FEATURES);
  fclose(f1);
  fclose(ffeat);
  lpcnet_dump_destroy(d);
  return 0;
}

