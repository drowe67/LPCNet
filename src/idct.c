/*
  idct.c
  David Rowe Mar 2019

  Inverse DCT so we can experiment with training in the Ly (log
  magnitude) domain.  Optionally measures var and mean of each Ly
  feature, and can normalise.

*/

#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <getopt.h>
#include "freq.h"
#include "lpcnet_quant.h"

#define NB_BANDS    18

/* meaured using -m option, then pasted in here */
float mean[]={
    0.083715,0.805284,1.062910,0.915489,0.516443,0.423818,0.409049,0.432484,0.687720,
    0.812425,0.794717,0.776628,0.944007,1.002912,0.697656,0.547764,0.500786,-0.132882};
float std[] = {
    1.862319,2.160068,2.226298,2.215460,2.099963,2.032326,2.001524,1.976323,1.977272,
    1.949574,1.917224,1.920438,1.936331,1.944219,1.886086,1.877000,1.876090,1.805038};

int main(int argc, char *argv[]) {
    FILE *fin, *fout;
    fin = stdin; fout = stdout;
    unsigned int ret;
    unsigned int stride = NB_BANDS;
    int measure = 0;
    int scaling = 0;
    
    static struct option long_options[] = {
        {"stride",   required_argument, 0, 't'},
        {"scale",    required_argument, 0, 's'},
        {"meas",     required_argument, 0, 'm'},
        {0, 0, 0, 0}
    };

    int opt_index = 0;
    int c;
    
    while ((c = getopt_long (argc, argv, "t:sm", long_options, &opt_index)) != -1) {
        switch (c) {
        case 'm':
            measure = 1;
            break;
        case 's':
            scaling = 1;
            break;
        case 't':
            stride = atoi(optarg);
            break;
        default:
            fprintf(stderr, "%s [-t stride] [-s scale]\n", argv[0]);
            exit(1);
        }
    }

    float sum[NB_BANDS] = {0.0};
    float sumsq[NB_BANDS] = {0.0};
    float dctLy[stride], Ly[stride];
    unsigned int i; for(i=0; i<stride; i++) Ly[stride] = 0.0;
    long n = 0;
    while(fread(dctLy, sizeof(float), stride, fin) == stride) {
        idct(Ly, dctLy);
        if (scaling) {
            for (i=0; i<NB_BANDS; i++) {
                Ly[i] = (Ly[i] - mean[i])/std[i];
            }
        }
        if (measure) {
            for (i=0; i<NB_BANDS; i++) {
                sum[i] += Ly[i];
                sumsq[i] += Ly[i]*Ly[i];
            }
        } else {
            ret = fwrite(Ly, sizeof(float), stride, fout);
            assert(ret == stride);
        }
        
        n++;
    }
    
    if (measure) {
        fprintf(stderr, "n = %ld\n", n);
        
        for(i=0; i<NB_BANDS; i++) 
            printf("%f,", sum[i]/n);
        printf("\n");
        for(i=0; i<NB_BANDS; i++) 
            printf("%f,", sqrt((sumsq[i]-sum[i]*sum[i]/n)/n));
        printf("\n");
    }
    
    return 0;
}
