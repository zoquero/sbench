/*
 * Simple Benchmarks
 * 
 * * Mem: Shows the time it takes to allocate, commit and free memory.
 * * CPU: Shows the time it takes to perform some silly floating point calculus.
 *        It uses 100% of one CPU.
 * * DISK_W: Shows the time it takes to write chunks on a file
 * * DISK_R_SEQ: Shows the time it takes to read sequentially chunks from a file
 * * DISK_R_RAN: Shows the time it takes to random read chunks from a file
 * 
 * Sources: https://github.com/zoquero/simplebenchmark/
 * 
 * @since 20161027
 * @author zoquero@gmail.com
 */

#include <stdio.h>        // sscanf, printf y fputs
#include <stdlib.h>       // exit
#include <string.h>       // memcpy, strlen
#include <sys/time.h>     // gettimeofday
#include <unistd.h>       // getopt
#include <ctype.h>        // isprint
#include <getopt.h>       // getopt
#include <features.h>     // errno
#include <limits.h>       // LONG_MAX LONG_MIN
#include <linux/limits.h> // PATH_MAX
#include <errno.h>        // errno
#include <math.h>         // pow
#include <sys/types.h>    // stat
#include <sys/stat.h>     // stat
#include <fcntl.h>        // open

enum type {CPU, MEM, DISK_W, DISK_R_SEQ, DISK_R_RAN};

void usage() {
  printf("Simple benchmarks, a first approach to performance measuring\n");
  printf("Usage:\n");
  printf("sbench (-v) -t cpu -p <times>\n");
  printf("sbench (-v) -t mem -p <times,sizeInBytes>\n");
  printf("sbench (-v) -t disk_w -p <times,sizeInBytes,folderName>\n");
  printf("sbench (-v) -t disk_r_seq -p <times,sizeInBytes,fileName>\n");
  printf("sbench (-v) -t disk_r_ran -p <times,sizeInBytes,fileName>\n");
  printf("\nExamples:\n");
  printf("* To allocate&commit 10 MiB of RAM and memset it 10 times:\n");
  printf("  sbench -t mem -p 10,104857600\n");
  printf("* To do silly calculus (2 pows) 100E6 times (it takes ~6E6/s):\n");
  printf("  sbench -t cpu -p 100000000\n");
  printf("* To write 100 MiB in a file in 4k blocks:\n");
  printf("  sbench -t disk_w -p 25600,4096,/tmp/_sbench.d\n");
  printf("* To read sequentially 100 MiB from a file in 4k blocks:\n");
  printf("  sbench -t disk_r_seq -p 25600,4096,/tmp/_sbench.testfile\n");
  printf("* To read by random access 100 MiB from a file in 4k blocks:\n");
  printf("  sbench -t disk_r_ran -p 25600,4096,/tmp/_sbench.testfile\n");
  printf("\nzoquero@gmail.com https://github.com/zoquero/simplebenchmark\n");
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

void parseParams(char *params, enum type thisType, int verbose, unsigned long *times, unsigned long *sizeInBytes, char *folderName, char *targetFileName) {
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
      printf("type=disk_w, times=%lu, sizeInBytes=%lu, folderName=%s verbose=%d\n", *times, *sizeInBytes, folderName, verbose);
  }
  else if(thisType == DISK_R_SEQ || thisType == DISK_R_RAN) {
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
    strcpy(targetFileName, strTmp3);

    if(verbose) {
      if(thisType == DISK_R_SEQ) {
        printf("type=disk_r_seq, times=%lu, sizeInBytes=%lu, targetFileName=%s verbose=%d\n", *times, *sizeInBytes, targetFileName, verbose);
      }
      else if(thisType == DISK_R_RAN) {
        printf("type=disk_r_ran, times=%lu, sizeInBytes=%lu, targetFileName=%s verbose=%d\n", *times, *sizeInBytes, targetFileName, verbose);
      }
    }
  }
  else {
    fprintf(stderr, "Unknown o missing type\n");
    usage();
  }
}


void getOpts(int argc, char **argv, char **params, enum type *thisType, int *verbose) {
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
        else if(strcmp(optarg, "disk_r_seq") == 0) {
          *thisType = DISK_R_SEQ;
        }
        else if(strcmp(optarg, "disk_r_ran") == 0) {
          *thisType = DISK_R_RAN;
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
  struct timeval beginning, end;
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
  struct timeval beginning, end;
  double delta;
  int  fd;
  char fileName[PATH_MAX];
  char *buffer;

  // The folder must pre-exist
  struct stat s = {0};
  if(stat(folderName, &s) != 0 || ! S_ISDIR(s.st_mode))  {
    sprintf(msg, "%s must exist previously and must be a folder", folderName);
    myAbort(msg);
  }

  // The file cannot pre-exist (we won't overwrite!)
  sprintf((char *) fileName, "%s/disk_w.out", folderName);
  if(access(fileName, F_OK) != -1 ) {
    sprintf(msg, "Ensure that the target file %s doesn't exist previously",
      fileName);
    myAbort(msg);
  }

  // Allocate RAM for the block of sizeInBytes bytes
  buffer=malloc(sizeInBytes);
  if(buffer == NULL) {
    sprintf(msg, "Can't allocate %lu bytes for the buffer", sizeInBytes);
    myAbort(msg);
  }
  if(memset(buffer, 0xA5, sizeInBytes) == NULL) {
    sprintf(msg, "Can't memset on those %lu bytes on memory", sizeInBytes);
    myAbort(msg);
  }

  // open
  fd = open(fileName, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
  if(fd == -1) {
    sprintf(msg, "Can't open the target file %s for writing", fileName);
    myAbort(msg);
  }
  // loop for writing and storing (fflush+msync)
  if(verbose) printf("Let's write %lu bytes %lu types on %s\n",
                sizeInBytes, times, fileName);
  gettimeofday(&beginning, NULL);
  for(unsigned long i = 0; i < times; i++) {
    // write
    if(write(fd, buffer, sizeInBytes) != sizeInBytes) {
      sprintf(msg, "Couldn't write %lu bytes to %s", sizeInBytes, fileName);
      myAbort(msg);
    }
    /*
     * fsync for flushing all modified in-core data to the disk device.
     * This way we'll be able to send burst of BIOs if needed.
     */
    if(fsync(fd) != 0) {
      sprintf(msg, "Couldn't flush after writing %lu-th block on %s", i, fileName);
      myAbort(msg);
    }
  }
  gettimeofday(&end, NULL);
  delta=timeval_diff(&end, &beginning);
  // close
  if(close(fd) == -1) {
    sprintf(msg, "Can't close the target file %s", fileName);
    myAbort(msg);
  }

  // delete the file
  if(remove(fileName) != 0) {
    sprintf(msg, "Can't delete the target file %s after the test", fileName);
    myAbort(msg);
  }

  // let's free the buffer
  free(buffer);

  return delta;
}

void shuffle(unsigned long *array, size_t n) {
  if (n > 1) {
    size_t i;
    for (i = 0; i < n - 1; i++) {
      size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
      int t = array[j];
      array[j] = array[i];
      array[i] = t;
    }
  }
}

double doDiskReadTest(enum type thisType, unsigned long sizeInBytes, unsigned long times, char *targetFileName, int verbose) {
  char msg[100];
  struct timeval beginning, end;
  double delta;
  int  fd;
  char *buffer;
  unsigned long *blocks;

  blocks = (unsigned long *) malloc(sizeof(unsigned long)*times);
  for(unsigned long i = 0; i < times; i++) {
    blocks[i] = i;
  }
  shuffle(blocks, times);

  // The file must exist
  if(access(targetFileName, F_OK) == -1 ) {
    sprintf(msg, "Can't find the target file %s", targetFileName);
    myAbort(msg);
  }

  // Allocate RAM for the block of sizeInBytes bytes
  buffer=malloc(sizeInBytes);
  if(buffer == NULL) {
    sprintf(msg, "Can't allocate %lu bytes for the buffer", sizeInBytes);
    myAbort(msg);
  }

  // check size
  struct stat s;
  stat(targetFileName, &s);
  if(s.st_size < sizeInBytes*times) {
    sprintf(msg, "The size of the file %s is less than %lu*%lu bytes", targetFileName, sizeInBytes, times);
    myAbort(msg);
  }

  // open
  fd = open(targetFileName, O_RDONLY);
  if(fd == -1) {
    sprintf(msg, "Can't open the target file %s for reading", targetFileName);
    myAbort(msg);
  }
  // loop for reading
  if(verbose) printf("Let's read %lu bytes %lu types from %s\n",
                sizeInBytes, times, targetFileName);
  gettimeofday(&beginning, NULL);
  for(unsigned long i = 0; i < times; i++) {
    // lseek for random read if DISK_R_RAN is choosen
    if(thisType == DISK_R_RAN) {
      if(lseek(fd, sizeInBytes*i, SEEK_SET) == -1) {
        sprintf(msg, "Can't lseek to reposition before reading on random-access to %s on %lu-th iteration", targetFileName, i);
        myAbort(msg);
      }
    }
    // now can read
    ssize_t ret_in = read(fd, buffer, sizeInBytes);
    if(verbose) printf("Read %lu bytes on %lu-th iteration\n", ret_in, i);
    if(ret_in != sizeInBytes) {
      sprintf(msg, "Read just %lu bytes from %s on %lu-th iteration", ret_in, targetFileName, i);
      myAbort(msg);
    }
  }
  gettimeofday(&end, NULL);
  delta=timeval_diff(&end, &beginning);
  // close
  if(close(fd) == -1) {
    sprintf(msg, "Can't close the target file %s", targetFileName);
    myAbort(msg);
  }

  // let's free the buffer
  free(buffer);
  free(blocks);

  return delta;
}



int main (int argc, char *argv[]) {
  int  verbose = 0;
  char *params = 0;
  enum type thisType;
  unsigned long sizeInBytes, times;
  char folderName[PATH_MAX-12];
  char targetFileName[PATH_MAX];
 
  getOpts(argc, argv, &params, &thisType, &verbose);
  parseParams(params, thisType, verbose, &times, &sizeInBytes, folderName, targetFileName);
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
  else if(thisType == DISK_R_SEQ || thisType == DISK_R_RAN) {
    double r = doDiskReadTest(thisType, sizeInBytes, times, targetFileName, verbose);
    printf("%f s\n", r);
  }
  else {
    myAbort(/* bug */ "Unknown type");
  }
  return 0;
}

