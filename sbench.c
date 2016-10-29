/*
 * Simple Benchmarks
 * 
 * * Mem: Shows the time it takes to allocate, commit and free memory.
 * * CPU: Shows the time it takes to perform some silly floating point calculus.
 *        It uses 100% of one CPU.
 * 
 * @since 20161027
 * @author zoquero@gmail.com
 */

#include <stdio.h>     // sscanf, printf y fputs
#include <stdlib.h>    // exit
#include <string.h>    // memcpy, strlen
#include <sys/time.h>  // gettimeofday
#include <unistd.h>    // getopt
#include <ctype.h>     // isprint
#include <getopt.h>    // getopt
#include <features.h>  // errno
#include <limits.h>    // LONG_MAX LONG_MIN
#include <errno.h>     // errno
#include <math.h>      // pow

enum type {CPU, MEM};

void usage() {
  printf("Usage:\n");
  printf("sbench (-v) -t cpu -p <times>\n");
  printf("sbench (-v) -t mem -p <times,sizeInBytes>\n");
  exit(1);
}

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

unsigned long parseUL(char *str, char *valNameForErrors) {
  unsigned long r;
  char *endptr;
  r = strtol(str, &endptr, 10);
  if((errno == ERANGE && (r == LONG_MAX || r == LONG_MIN)) || (errno != 0 && r == 0)) {
    fprintf(stderr, "Error parsing \"%s\". Probably value too long.\n", valNameForErrors);
    usage();
  }
  if(endptr == str) {
    fprintf(stderr, "No digits found parsing \"%s\"\n", valNameForErrors);
    usage();
  }
  return r;
}

int parseParams(char *params, enum type thisType, int verbose, unsigned long *times, unsigned long *sizeInBytes) {

  char *strTmp1, *strTmp2;
  if(thisType == CPU) {
    if(strlen(params) > 19) {
      fprintf(stderr, "\"times\" must fit in a long integer\n");
      usage();
    }
    if(sscanf(params, "%lu", times) != 1) {
      fprintf(stderr, "\"times\" must be an integer\n");
      usage();
    }
    if(verbose)
      printf("type=cpu, times=%lu verbose=%d\n", *times, verbose);
  }
  else if(thisType == MEM) {
    strTmp1 = strtok(params, ",");
    if(strTmp1 == NULL) {
      fprintf(stderr, "Params must be in \"num,num\" format\n");
      usage();
    }
    if(strlen(params) < strlen(strTmp1)) {
      fprintf(stderr, "Params must be in \"num,num\" format\n");
      usage();
    }
    strTmp2 = strtok(NULL, ",");
    if(strTmp2 == NULL) {
      fprintf(stderr, "Params must be in \"num,num\" format\n");
      usage();
    }

    *times       = parseUL(strTmp1, "times");
    *sizeInBytes = parseUL(strTmp2, "sizeInBytes");

    if(verbose)
      printf("type=mem, times=%lu, sizeInBytes=%lu, verbose=%d\n", *times, *sizeInBytes, verbose);
  }
  else {
    fprintf(stderr, "Unknown o missing type\n");
    usage();
  }

}


int getOpts(int argc, char **argv, char **params, enum type *thisType, int *verbose) {
  int c;
  extern char *optarg;
  extern int optind, opterr, optopt;
  opterr = 0;

  if(argc < 5) {
    fprintf(stderr, "Missing parameters\n");
    usage();
  }

  while ((c = getopt (argc, argv, "t:p:v")) != -1) {
    switch (c) {
      case 't':
        if(optarg == NULL) {
          fprintf (stderr, "Option -%c requires an argument\n", c);
          usage();
        }
        if(strcmp(optarg, "cpu") == 0) {
          *thisType = CPU;
        }
        else if(strcmp(optarg, "mem") == 0) {
          *thisType = MEM;
        }
        else {
          fprintf(stderr, "Unknown type '%s'\n", optarg);
          usage();
        }
        break;
      case 'p':
        *params = optarg;
        if(*params == NULL) {
          fprintf (stderr, "Option -%c requires an argument\n", c);
          usage();
        }
        break;
      case 'v':
        *verbose = 1;
        break;
      case '?':
        if (optopt == 'p')
          fprintf (stderr, "Option -%c requires an argument.\n", optopt);
        else if (isprint (optopt))
          fprintf (stderr, "Unknown option `-%c'.\n", optopt);
        else
          fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
        usage();
      default:
        fprintf (stderr, "Error in command line parameters\n");
        usage();
    }
  }
}

int main (int argc, char *argv[]) {
  int  verbose = 0;
  int  type    = 0;
  char *params = 0;
  enum type thisType;
  unsigned long sizeInBytes, times;
  struct timeval beginning, end, before, after;
  double delta;
  char *cptr;
  char msg[100];
 
  getOpts(argc, argv, &params, &thisType, &verbose);
  parseParams(params, thisType, verbose, &times, &sizeInBytes);
  if(thisType == CPU) {
    if(verbose) printf("CPU test:\n");

    gettimeofday(&beginning, NULL);
    double x=2;
    for(long int i = 0; i < times; i++) {
      x=pow(x, x);
      x=pow(x, 1/(x-1));
    }
    gettimeofday(&end, NULL);
    delta=timeval_diff(&end, &beginning);
    printf("%f s\n", delta);
  
  }
  if(thisType == MEM) {

    gettimeofday(&beginning, NULL);
    for(int i = 0; i < times; i++) {
      /* Just VmSize, isn't VmRSS */
      /* It takes longer on first time. */
      gettimeofday(&before, NULL);
      cptr = (char *) malloc(sizeInBytes);
      if(cptr == NULL) {
        sprintf(msg, "Can't allocate %lu bytes on memory", sizeInBytes);
        myAbort(msg);
      }
      gettimeofday(&after, NULL);
      delta=timeval_diff(&after, &before);
      if(verbose) printf("malloc : %f\n", delta);
     
      /* VmRSS ! */
      gettimeofday(&before, NULL);
      if(memset(cptr, 0xA5, sizeInBytes) == NULL) {
        sprintf(msg, "Can't memset on those %lu bytes on memory", sizeInBytes);
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

  }
  return 0;
}


int mainMem (int argc, char *argv[]) {
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

/*
int mainCpu (int argc, char *argv[]) {
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
*/
