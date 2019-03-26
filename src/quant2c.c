/*
  quant2c.c
  David Rowe Feb 2019

  Build C float arrays from quantiser files. Derived from quant_feat.c
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STAGES 5

#include "lpcnet_quant.h"

int main(int argc, char *argv[]) {
    int k=NB_BANDS, i;
    int num_stages = 0;
    float features[NB_BANDS];
    float vq[MAX_STAGES*NB_BANDS*MAX_ENTRIES];
    int   m[MAX_STAGES];
    
    char fnames[256];
    char fn[256];
    char *comma, *p;
    FILE *fq;

    if (argc != 2) {
        fprintf(stderr, "usage: %s stage1vq.f32, stage2vq.f32,.....\n", argv[0]);
    }

    for(i=0; i<MAX_STAGES*NB_BANDS*MAX_ENTRIES; i++) vq[i] = 0.0;
    
    /* list of comma delimited file names */
    strcpy(fnames, argv[1]);
    p = fnames;
    num_stages = 0;
    do {
        assert(num_stages < MAX_STAGES);
        strcpy(fn, p);
        comma = strchr(fn, ',');
        if (comma) {
            *comma = 0;
            p = comma+1;
        }
        /* load quantiser files */
        fprintf(stderr, "stage: %d loading %s ...", num_stages, fn);
        fq=fopen(fn, "rb");
        if (fq == NULL) {
            fprintf(stderr, "Couldn't open: %s\n", fn);
            exit(1);
        }
        m[num_stages] = 0;
        while (fread(features, sizeof(float), k, fq) == (size_t)k) m[num_stages]++;
        assert(m[num_stages] <= MAX_ENTRIES);
        fprintf(stderr, "%d entries of vectors width %d\n", m[num_stages], k);
        rewind(fq);                       
        int rd = fread(&vq[num_stages*k*MAX_ENTRIES], sizeof(float), m[num_stages]*k, fq);
        assert(rd == m[num_stages]*k);
        num_stages++;
        fclose(fq);
    } while(comma);

    /* write m[] and vq[] tables to C source file on stdout */

    printf("#include \"lpcnet_quant.h\"\n");
    printf("int num_stages = %d;\n", num_stages);
    printf("int m[MAX_STAGES] = {");
    for(i=0; i<num_stages; i++) {
        printf("%d, ", m[i]);
    }
    printf("%d};\n ",m[i]);
    printf("float vq[MAX_STAGES*NB_BANDS*MAX_ENTRIES] = {\n");
    for(i=0; i<num_stages*NB_BANDS*MAX_ENTRIES-1; i++) {
        printf("%.9g, ", vq[i]);
    }
    printf("%.9g};\n ",vq[i]);
   
    return 0;
}
