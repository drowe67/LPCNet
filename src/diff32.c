/* test tool to diffs two .f32 files */

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define NB_FEATURES 55

int main(int argc, char *argv[]) {
    float fdiff, fdiff_tot=0.0;
    int f=0;
    unsigned int ret, i, stride = NB_FEATURES;
    FILE *file1 = fopen(argv[1],"rb");
    assert(file1 != NULL);
    FILE *file2 = fopen(argv[2],"rb");
     assert(file2 != NULL);
   if (argc == 4) {
        stride = atoi(argv[3]);
    }
   float f1[stride],f2[stride];
    while(fread(&f1,sizeof(float),stride,file1) == stride) {
        ret = fread(&f2,sizeof(float),stride,file2);
        if (ret != stride) break;
        for(i=0; i<stride; i++) {
            fdiff = fabs(f1[i]-f2[i]);
            fdiff_tot += fdiff;
            
            if (isnan(fdiff) || (fdiff > 1E-3)) {
                printf("f: %d i: %d %f %f %f\n", f, i, f1[i], f2[i], fdiff);
                exit(0);
            }
        }
        f++;
    }
    fprintf(stderr,"stride: %d f: %d fdiff_tot: %f\n", stride, f, fdiff_tot);
    fclose(file1); fclose(file2);
}
