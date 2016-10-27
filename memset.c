/*
 * Shows the time it takes to allocate, commit and free memory.
 * 
 * @since 20161027
 * @author zoquero@gmail.com
 */
#include <stdio.h>     // sscanf, printf y fputs
#include <stdlib.h>    // exit
#include <string.h>    // memcpy
#include <sys/time.h>  // gettimeofday

void myAbort(char* msg) {
  fprintf(stderr, "Error: %s\n", msg);
  exit(1);
}

/**
  * Difference between two instants.
  * @arg start
  * @arg end
  * @return difference in seconds
  */
double timeval_diff(struct timeval *a, struct timeval *b) {
  return (double)(a->tv_sec + (double)a->tv_usec/1000000) - (double)(b->tv_sec + (double)b->tv_usec/1000000);
}


int main (int argc, char *argv[]) {
  char *cptr;
  char msg[100];
  short verbose = 0;
  unsigned int memSizeInBytes;
  unsigned int times;
  struct timeval before, after;
  struct timeval beginning, end;
  double delta;
 
  if(argc != 3 && argc != 4) {
    sprintf(msg, "Usage: memset <memSizeInBytes> <times> (-v)");
    myAbort(msg);
  }

  if(sscanf(argv[1], "%du", &memSizeInBytes) != 1) {
    sprintf(msg, "Usage: memset <memSizeInBytes> <times> (-v)  # (memSizeInBytes must be an integer)");
    myAbort(msg);
  }
  if(memSizeInBytes >= 2147483648) {
    sprintf(msg, "Usage: memset <memSizeInBytes> <times> (-v)  # (memSizeInBytes must be less than 2^31)");
    myAbort(msg);
  }

  if(sscanf(argv[2], "%du", &times) != 1) {
    sprintf(msg, "Usage: memset <memSizeInBytes> <times> (-v)  # (times must be an integer)");
    myAbort(msg);
  }
  if(memSizeInBytes >= 2147483648) {
    sprintf(msg, "Usage: memset <memSizeInBytes> <times> (-v)  # (times must be less than 2^31)");
    myAbort(msg);
  }
  if(argc == 4 && argv[3] != NULL && strlen(argv[3]) == 2 && strncmp(argv[3], "-v", 2) == 0) {
    verbose = 1;
  }

  gettimeofday(&beginning, NULL);
  for(int i = 0; i < times; i++) {
    /* Just VmSize, isn't VmRSS */
    /* It takes longer on first time. */
    gettimeofday(&before, NULL);
    cptr = (char *) malloc(memSizeInBytes);
    if(cptr == NULL) {
      sprintf(msg, "Can't allocate %d bytes on memory", memSizeInBytes);
      myAbort(msg);
    }
    gettimeofday(&after, NULL);
    delta=timeval_diff(&after, &before);
    if(verbose) printf("malloc : %f\n", delta);
   
    /* VmRSS ! */
    gettimeofday(&before, NULL);
    if(memset(cptr, 0xA5, memSizeInBytes) == NULL) {
      sprintf(msg, "Can't memset on those %d bytes on memory", memSizeInBytes);
      myAbort(msg);
    }
    gettimeofday(&after, NULL);
    delta=timeval_diff(&after, &before);
    if(verbose) printf("memset : %f\n", delta);
  
    gettimeofday(&before, NULL);
    free(cptr);
    gettimeofday(&after, NULL);
    delta=timeval_diff(&after, &before);
    if(verbose) printf("free   : %f\n", delta);
    //getchar();
  }
  gettimeofday(&end, NULL);
  delta=timeval_diff(&end, &beginning);
  printf("%f s\n", delta);

  return 0;
}
