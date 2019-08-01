/*
  nnet_rw.h

  Support for reading and writing NNs from disk at run time.
*/

#ifndef __NNET_RW__
#define __NNET_RW__

void nnet_write(char *fn);
void nnet_read_and_check(char *fn);
void nnet_read(char *fn);

#endif
