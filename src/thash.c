/*---------------------------------------------------------------------------*\

  FILE........: thash.c
  AUTHOR......: David Rowe
  DATE CREATED: July 2020

  Simple test program for LPCNet API get hash function

\*---------------------------------------------------------------------------*/

#include <stdio.h>
#include "lpcnet_freedv.h"

int main(void) { 
    printf("%s\n", lpcnet_get_hash());
    return 0;
}


