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

void usage() {
  printf("PENDENT DE DEFINIR EL usage\n");
// (-v) -t cpu -p <times>
// (-v) -t mem -p <times,sizeInBytes>
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

int main (int argc, char *argv[]) {

  int  verbose = 0;
  int  type    = 0;
  char *params = 0;
//  char *cvalue = NULL;
  int index;
  int c;
  enum type {CPU, MEM};
  enum type thisType;
  extern char *optarg;
  extern int optind, opterr, optopt;
  unsigned long sizeInBytes, times;

  opterr = 0;

  while ((c = getopt (argc, argv, "t:p:v")) != -1)
    switch (c) {
      case 't':
        if(optarg == NULL) {
          fprintf (stderr, "Option -%c requires an argument\n", c);
          usage();
        }
        if(strcmp(optarg, "cpu") == 0) {
          thisType = CPU;
        }
        else if(strcmp(optarg, "mem") == 0) {
          thisType = MEM;
        }
        else {
          fprintf(stderr, "Unknown type\n");
          usage();
        }
        break;
      case 'p':
        params = optarg;
        if(params == NULL) {
          fprintf (stderr, "Option -%c requires an argument\n", c);
          usage();
        }
        break;
      case 'v':
        verbose = 1;
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

  if(thisType == CPU) {
    if(strlen(params) > 19) {
      fprintf(stderr, "\"times\" must fit in a long integer\n");
      usage();
    }
    if(sscanf(params, "%lu", &times) != 1) {
      fprintf(stderr, "\"times\" must be an integer\n");
      usage();
    }
  }
  else if(thisType == MEM) {
    char *strTmp1, *strTmp2;
/*
    if(sscanf(params, "%s,%s", strTmp1, strTmp2) != 1) {
      fprintf(stderr, "Param must be \"times,sizeInBytes\"\n");
      usage();
    }
printf("%s , %s\n", strTmp1, strTmp2);

    if(strlen(strTmp1) > 19) {
      fprintf(stderr, "\"times\" must fit in a long integer\n");
      usage();
    }
    if(sscanf(strTmp1, "%lu", &times) != 1) {
      fprintf(stderr, "\"times\" must be an integer\n");
      usage();
    }

    if(strlen(strTmp2) > 19) {
      fprintf(stderr, "\"sizeInBytes\" must fit in a long integer\n");
      usage();
    }
    if(sscanf(strTmp2, "%lu", &sizeInBytes) != 1) {
      fprintf(stderr, "\"sizeInBytes\" must be an integer\n");
      usage();
    }
*/

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

printf("strTmp1=%s strTmp2=%s\n", strTmp1, strTmp2);

    if(strlen(strTmp1) > 19) {
      fprintf(stderr, "\"times\" must fit in a long integer\n");
      usage();
    }
    if(sscanf(strTmp1, "%lu", &times) != 1) {
      fprintf(stderr, "\"times\" must be an integer\n");
      usage();
    }

    if(strlen(strTmp2) > 19) {
      fprintf(stderr, "\"sizeInBytes\" must fit in a long integer\n");
      usage();
    }
    if(sscanf(strTmp2, "%lu", &sizeInBytes) != 1) {
      fprintf(stderr, "\"sizeInBytes\" must be an integer\n");
      usage();
    }

printf("times=%lu sizeInBytes=%lu\n", times, sizeInBytes);
  }
  else {
  }

  if(verbose)
    printf("type=%d params=%s verbose=%d\n", thisType, params, verbose);

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
