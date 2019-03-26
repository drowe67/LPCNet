#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define NB_FEATURES 55

int main(int argc, char *argv[]) {
    float f1[NB_FEATURES],f2[NB_FEATURES],fdiff=0.0;
    int ret,f=0,i;
    FILE *file1 = fopen(argv[1],"rb");
    FILE *file2 = fopen(argv[2],"rb");
    while(fread(&f1,sizeof(float),NB_FEATURES,file1) == NB_FEATURES) {
        ret = fread(&f2,sizeof(float),NB_FEATURES,file2);
        if (ret != NB_FEATURES) break;
        for(i=0; i<NB_FEATURES; i++) {
            fdiff += fabs(f1[i]-f2[i]);
        
            if (isnan(fdiff) || (fdiff > 1E-6)) {
                printf("f: %d i: %d %f %f %f\n", f, i, f1[i], f2[i], fdiff);
                exit(0);
            }
        }
        f++;
    }
    fprintf(stderr,"f: %d fdiff: %f\n", f, fdiff);
    fclose(file1); fclose(file2);
}
