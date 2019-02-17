/*
  tweak_pitch.c
  David Rowe Feb 2019

  Modify pitch of feature set, used to augment training vectors for
  low pitch speakers.
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define NB_FEATURES 55
#define NB_BANDS 18
#define PITCH_MIN_PERIOD 32
#define PITCH_MAX_PERIOD 256

int main(int argc, char *argv[]) {
    FILE *fin, *fout;
    float features[NB_FEATURES], delta_f0;
    fin = stdin; fout = stdout;
    int ret;

    if (argc != 2) {
        fprintf(stderr, "usage: %s delta_F0\n", argv[0]);
        exit(1);
    }
  
    delta_f0 = atof(argv[1]);
    
    while(fread(features, sizeof(float), NB_FEATURES, fin) == NB_FEATURES) {
        // Note we work in f0 domain, rather than pitch period
        float feat = features[2*NB_BANDS];
        float pitch_index = 100*feat + 200;
        float Fs=16000.0;
        float f0 = 2*Fs/pitch_index;
        float f0_ = f0+delta_f0;
	float pitch_index_ = 2*Fs/f0_;
	if (pitch_index_ > 2*PITCH_MAX_PERIOD) pitch_index_ = 2*PITCH_MAX_PERIOD;
	if (pitch_index_ < 2*PITCH_MIN_PERIOD) pitch_index_ = 2*PITCH_MIN_PERIOD;
	assert(pitch_index_ <= 2*PITCH_MAX_PERIOD);
	assert(pitch_index_ >= 2*PITCH_MIN_PERIOD);
	
        features[2*NB_BANDS] = 0.01*(pitch_index_-200);
        ret = fwrite(features, sizeof(float), NB_FEATURES, fout);
        assert(ret == NB_FEATURES);
        //fprintf(stderr, "pitch_index %f %f f0: %f %f feat: %f %f\n",
        //        pitch_index, pitch_index_, f0, f0_, feat, features[2*NB_BANDS]);
    }
    return 0;
}
