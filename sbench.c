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
#include <sys/types.h> // stat
#include <sys/stat.h>  // stat
#include <fcntl.h>     // open


enum type {CPU, MEM, DISK_W};

void usage() {
  printf("Usage:\n");
  printf("sbench (-v) -t cpu -p <times>\n");
  printf("sbench (-v) -t mem -p <times,sizeInBytes>\n");
  printf("sbench (-v) -t disk_w -p <times,sizeInBytes,folderName>\n");
  printf("\nExamples:\n");
  printf("* To allocate&commit 10 MiB of RAM and memset it 10 times:\n");
  printf("  sbench -t mem -p 10,104857600\n");
  printf("* To do silly calculus (2 pows) 100E6 times (it takes ~6E6/s):\n");
  printf("  sbench -t cpu -p 100000000\n");
  printf("* To write 100 MiB in 4k blocks:\n");
  printf("  sbench -t disk_w -p 25600,4096,/tmp/_sbench.d\n");
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

int parseParams(char *params, enum type thisType, int verbose, unsigned long *times, unsigned long *sizeInBytes, char *folderName) {
  char *strTmp1, *strTmp2, *strTmp3;
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
  else if(thisType == DISK_W) {
    strTmp1 = strtok(params, ",");
    if(strTmp1 == NULL) {
      fprintf(stderr, "Params must be in \"num,num,path\" format\n");
      usage();
    }
    if(strlen(params) < strlen(strTmp1)) {
      fprintf(stderr, "Params must be in \"num,num,path\" format\n");
      usage();
    }
    strTmp2 = strtok(NULL, ",");
    if(strTmp2 == NULL) {
      fprintf(stderr, "Params must be in \"num,num,path\" format\n");
      usage();
    }
    strTmp3 = strtok(NULL, ",");
    if(strTmp3 == NULL) {
      fprintf(stderr, "Params must be in \"num,num,path\" format\n");
      usage();
    }

    *times       = parseUL(strTmp1, "times");
    *sizeInBytes = parseUL(strTmp2, "sizeInBytes");
    strcpy(folderName, strTmp3);

    if(verbose)
      printf("type=mem, times=%lu, sizeInBytes=%lu, folderName=%s verbose=%d\n", *times, *sizeInBytes, folderName, verbose);
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
        else if(strcmp(optarg, "disk_w") == 0) {
          *thisType = DISK_W;
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

double doCpuTest(unsigned long times, int verbose) {
  struct timeval beginning, end, before, after;
  double delta;

  if(verbose) printf("CPU test:\n");

  gettimeofday(&beginning, NULL);
  double x=2;
  for(long int i = 0; i < times; i++) {
    x=pow(x, x);
    x=pow(x, 1/(x-1));
  }
  gettimeofday(&end, NULL);
  delta=timeval_diff(&end, &beginning);
  return delta;
}

double doMemTest(unsigned long sizeInBytes, unsigned long times, int verbose) {
  char msg[100];
  char *cptr;
  struct timeval beginning, end, before, after;
  double delta;

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

  return delta;
}


double doDiskWriteTest(unsigned long sizeInBytes, unsigned long times, char *folderName, int verbose) {
  char msg[100];
  char *cptr;
  struct timeval beginning, end, before, after;
  double delta;
  int  fd;
  char fileName[PATH_MAX];
  char *buffer;

  // The folder must pre-exist
  struct stat s = {0};
  if(stat(folderName, &s) != 0 || ! (s.st_mode & S_IFDIR))  {
    sprintf(msg, "%s must exist previously and must be a folder\n", folderName);
    myAbort(msg);
  }

  // The file cannot pre-exist (we won't overwrite!)
  sprintf((char *) fileName, "%s/disk_w.out", folderName);
  if(access(fileName, F_OK) != -1 ) {
    sprintf(msg, "Target file %s cannot exist previously\n", fileName);
    myAbort(msg);
  }

  // Allocate RAM for the bloc of sizeInBytes bytes
  buffer=malloc(sizeInBytes);
  if(buffer == NULL) {
    sprintf(msg, "Can't allocate a buffer for storing %lu bytes\n", sizeInBytes);
    myAbort(msg);
  }
  if(memset(buffer, 0xA5, sizeInBytes) == NULL) {
    sprintf(msg, "Can't memset on those %lu bytes on memory", sizeInBytes);
    myAbort(msg);
  }

  printf("Testing on %s\n", fileName);
  // open
  fd = open("file", O_CREAT | O_WRONLY);
  if(fd == -1) {
    sprintf(msg, "Can't open the target file %s for writing\n", fileName);
    myAbort(msg);
  }
  // loop for writing and storing (fflush+msync)
  gettimeofday(&beginning, NULL);
printf("iterem\n");
  for(unsigned long i = 0; i < times; i++) {
printf("i=%lu\n", i);
    write(fd, buffer, sizeInBytes);
  }
  gettimeofday(&end, NULL);
  delta=timeval_diff(&end, &beginning);
  // close
  if(close(fd) == -1) {
    sprintf(msg, "Can't close the target file %s\n", fileName);
    myAbort(msg);
  }

  // delete the file
  if(remove(fileName) != 0) {
    sprintf(msg, "Can't delete the target file %s after the test\n", fileName);
    myAbort(msg);
  }

  // let's free the buffer
  free(buffer);

  return delta;
}

int main (int argc, char *argv[]) {
  int  verbose = 0;
  int  type    = 0;
  char *params = 0;
  enum type thisType;
  unsigned long sizeInBytes, times;
  char folderName[PATH_MAX-12];
 
  getOpts(argc, argv, &params, &thisType, &verbose);
  parseParams(params, thisType, verbose, &times, &sizeInBytes,  folderName);
  if(thisType == CPU) {
    double r = doCpuTest(times, verbose);
    printf("%f s\n", r);
  }
  else if(thisType == MEM) {
    double r = doMemTest(sizeInBytes, times, verbose);
    printf("%f s\n", r);
  }
  else if(thisType == DISK_W) {
    double r = doDiskWriteTest(sizeInBytes, times, folderName, verbose);
    printf("%f s\n", r);
  }
  else {
    myAbort(/* bug */ "Unknown type");
  }
  return 0;
}

