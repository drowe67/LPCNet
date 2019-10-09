/* generates a linear ramp to test quant_feat decimation/interpolation */

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#define NB_FEATURES 55
#define FRAMES      18

int main(void) {
    FILE *fout = fopen("ramp.f32", "wb"); assert(fout != NULL);
    float features[NB_FEATURES];
    int i,j;
    
    for(i=0; i<FRAMES; i++) {
        for(j=0; j<NB_FEATURES; j++)
            features[j] = i+j;
        fwrite(features, sizeof(float), NB_FEATURES, fout);
    }
    
    fclose(fout);

    return 0;
}
