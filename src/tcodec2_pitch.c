/*---------------------------------------------------------------------------*\

  FILE........: tcodec2_pitch.c
  AUTHOR......: David Rowe
  DATE CREATED: Feb 2019

  Test program for Codec 2 pitch module.

\*---------------------------------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>
#include "codec2_pitch.h"

int frames;

/*---------------------------------------------------------------------------*\

                                    MAIN

\*---------------------------------------------------------------------------*/

int main(int argc, char *argv[])
{
    if (argc < 3) {
	printf("\nusage: %s InputRawSpeechFile Outputf0PitchTextFile\n", argv[0]);
        exit(1);
    }

    int Sn_size, new_samples_each_call;
    CODEC2_PITCH *c2_pitch = codec2_pitch_create(&Sn_size, &new_samples_each_call);

    short buf[new_samples_each_call];
    float Sn[Sn_size];	               /* float buffer of input speech samples */
    FILE *fin,*fout;
    int   pitch_samples;
    float f0, voicing;    
    int   i;

    /* Input file */

    if ((fin = fopen(argv[1],"rb")) == NULL) {
      printf("Error opening input speech file: %s\n",argv[1]);
      exit(1);
    }

    /* Output file */

    if ((fout = fopen(argv[2],"wt")) == NULL) {
      printf("Error opening output text file: %s\n",argv[2]);
      exit(1);
    }

    for(i=0; i<Sn_size; i++) {
      Sn[i] = 0.0;
    }

    frames = 0;
    while(fread(buf, sizeof(short), new_samples_each_call, fin)) {
      /* Update input speech buffers */

      for(i=0; i<Sn_size-new_samples_each_call; i++)
        Sn[i] = Sn[i+new_samples_each_call];
      for(i=0; i<new_samples_each_call; i++)
        Sn[i+Sn_size-new_samples_each_call] = buf[i];
      pitch_samples = codec2_pitch_est(c2_pitch, Sn, &f0, &voicing);

      fprintf(fout,"%f %d\n", f0, pitch_samples);
    }

    fclose(fin);
    fclose(fout);
    
    codec2_pitch_destroy(c2_pitch);
    
    return 0;
}


