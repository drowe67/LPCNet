/*
  idct.c
  David Rowe Mar 2019

  inverse DCT so we can experiment with training in the Ly (log magnitude) domain.
*/

#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "freq.h"

#define NB_BANDS 18

int main(void) {
    FILE *fin, *fout;
    float dctLy[NB_BANDS], Ly[NB_BANDS];
    fin = stdin; fout = stdout;
    int ret;
    
    while(fread(dctLy, sizeof(float), NB_BANDS, fin) == NB_BANDS) {
        idct(Ly, dctLy);
        ret = fwrite(Ly, sizeof(float), NB_BANDS, fout);
        assert(ret == NB_BANDS);
    }

    return 0;
}
