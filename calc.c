/*
 * Shows the time it takes to do some silly calculus.
 * It uses 100% of one CPU.
 * 
 * @since 20161028
 * @author zoquero@gmail.com
 */
#include <stdio.h>     // sscanf, printf and fputs
#include <stdlib.h>    // exit
#include <string.h>    // strlen
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
  unsigned long times;
  struct timeval beginning, end;
  char msg[100];
  double delta;
 
  if(argc != 2) {
    sprintf(msg, "Usage: cpubound <times>");
    myAbort(msg);
  }
  if(argv[1] == NULL || strlen(argv[1]) > 19) {
    sprintf(msg, "Usage: cpubound <times>  # (times must fit in a long integer)");
    myAbort(msg);
  }
  if(sscanf(argv[1], "%lu", &times) != 1) {
    sprintf(msg, "Usage: cpubound <times>  # (times must be an integer)");
    myAbort(msg);
  }

  gettimeofday(&beginning, NULL);
  double x=2;
  for(long int i = 0; i < times; i++) {
    x*=x;
    x/=(x-1);
  }
  gettimeofday(&end, NULL);
  delta=timeval_diff(&end, &beginning);
  printf("%f s\n", delta);

  return 0;
}
