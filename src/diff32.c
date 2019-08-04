/* test tool to diffs two .f32 files */

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#define NB_FEATURES 55

int main(int argc, char *argv[]) {
    float fdiff, fdiff_tot=0.0;
    int f=0;
    unsigned int ret, i, stride = NB_FEATURES, cont = 0;

    int o = 0;
    int opt_idx = 0;
    while( o != -1 ) {
        static struct option long_opts[] = {
            {"stride", no_argument, 0, 's'},
            {"cont",   no_argument, 0, 'c'},
            {0, 0, 0, 0}
        };
        
	o = getopt_long(argc,argv,"sc",long_opts,&opt_idx);
        
	switch(o){
	case 's':
	    stride = atoi(optarg);
	    break;
	case 'c':
	    cont = 1;
	    break;
	case '?':
	    goto helpmsg;
	    break;
	}
    }
    int dx = optind;

    if ((argc - dx) < 2) {
    helpmsg:
        fprintf(stderr, "usage: diff32 [--stride] [--cont] file1.f32 file2.f32\n");
        return 0;
    }

    FILE *file1 = fopen(argv[dx], "rb");
    if (file1 == NULL) {
	fprintf(stderr, "Can't open %s\n", argv[dx]);
	exit(1);
    }
    
    FILE *file2 = fopen(argv[dx+1], "rb");
    if (file2 == NULL) {
	fprintf(stderr, "Can't open %s\n", argv[dx+1]);
	exit(1);
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
                if (cont == 0) exit(0);
            }
        }
        f++;
    }
    fprintf(stderr,"stride: %d f: %d fdiff_tot: %f\n", stride, f, fdiff_tot);
    fclose(file1); fclose(file2);
}
