/* Test tool to diff two float (.f32) files that hold LPCNet
   "features".  Each file can be seen as a matrix, where each row has
   "stride" columns.  We calculate the "SNR" of each col measured
   between the two files, as each col represents a specific feature
   that will have it's own scaling. */

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#define NB_FEATURES   55
#define FDIFF_THRESH  0.001
#define SNR_THRESH    1000.0

int main(int argc, char *argv[]) {
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

    float fdiff;
    float f1[stride], f2[stride], s[stride], n[stride];
    for(i=0; i<stride; i++) {s[i] = 0.0; n[i] = 0.0; }
    
    while(fread(&f1,sizeof(float),stride,file1) == stride) {
        ret = fread(&f2,sizeof(float),stride,file2);
        if (ret != stride) break;
        for(i=0; i<stride; i++) {
            s[i] += f1[i]*f1[i];
            fdiff = fabs(f1[i]-f2[i]);
            n[i] += fdiff*fdiff;

            /* flag any gross errors straight away */
            if (isnan(fdiff) || (fdiff > FDIFF_THRESH)) {
                fprintf(stderr, "f: %d i: %d %f %f %f\n", f, i, f1[i], f2[i], fdiff);
                if (cont == 0) exit(1);
            }
        }
        f++;
    }

    fclose(file1); fclose(file2);

    /* calculate per col SNRs, as each feature might have a different scaling */
    float snr_min = 1E32;
    for(i=0; i<stride; i++) {
        float snr = s[i]/(n[i]+1E-12);
        if ((s[i] != 0) && (snr < snr_min)) snr_min = snr;
        fprintf(stderr, "i: %d s: %e n: %e SNR: %e %e\n",  i, s[i], n[i], snr, snr_min);
    }   
    
    if (snr_min > SNR_THRESH)
        exit(0);
    else
        exit(1);
}
