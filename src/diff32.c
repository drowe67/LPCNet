/* test tool to diffs two .f32 files */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define NB_FEATURES 55

int main(int argc, char *argv[]) {
    float f1[NB_FEATURES],f2[NB_FEATURES],fdiff=0.0;
    int f=0;
    unsigned int ret, i, stride = NB_FEATURES;
    FILE *file1 = fopen(argv[1],"rb");
    FILE *file2 = fopen(argv[2],"rb");
    if (argc == 4) {
        stride = atoi(argv[3]);
    }
    while(fread(&f1,sizeof(float),stride,file1) == stride) {
        ret = fread(&f2,sizeof(float),stride,file2);
        if (ret != stride) break;
        for(i=0; i<stride; i++) {
            fdiff += fabs(f1[i]-f2[i]);
        
            if (isnan(fdiff) || (fdiff > 1E-6)) {
                printf("f: %d i: %d %f %f %f\n", f, i, f1[i], f2[i], fdiff);
                exit(0);
            }
        }
        f++;
    }
    fprintf(stderr,"stride: %d f: %d fdiff: %f\n", stride, f, fdiff);
    fclose(file1); fclose(file2);
}
