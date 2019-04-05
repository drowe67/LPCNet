#ifndef __LPCNET_FREEDV_INTERNAL__
#define __LPCNET_FREEDV_INTERNAL__
typedef struct LPCNetState LPCNetState;
struct LPCNetFreeDV {
    LPCNET_DUMP  *d;
    LPCNET_QUANT *q;
    LPCNetState *net;
};
#endif
