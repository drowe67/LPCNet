#include <stdio.h>
#include <stdlib.h>
#define N 160
int main(void) {
    FILE *f;
    short buf[N];
    int nread;
    
    printf("Hello, World\n");
    f = fopen("all.wav","rb");
    if (f==NULL) {
        fprintf(stderr, "can't open all.wab!\n");
        exit(1);
    }
    while(1) {
        nread = fread(buf, sizeof(short), N, f);
        printf("nread: %d\n", nread);
        if (nread != N) break;
    }
    fclose(f);
    return 0;
}
