/*
  weight0.c
  David Rowe Feb 2019

  Weight applied to first element of each vector, to test weighting of VQ.
*/

#include <assert.h>
#include <stdio.h>
#include <math.h>

#define NB_BANDS 18

int main(void) {
    FILE *fin, *fout;
    float v[NB_BANDS];
    fin = stdin; fout = stdout;
    int ret;
    
    while(fread(v, sizeof(float), NB_BANDS, fin) == NB_BANDS) {
        v[0] *= 1.0/sqrt(NB_BANDS);
        ret = fwrite(v, sizeof(float), NB_BANDS, fout);
        assert(ret == NB_BANDS);
    }

    return 0;
}
