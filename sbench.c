/*
 * Simple Benchmarks
 * 
 * * MEM: Shows the time it takes to allocate, commit and free memory.
 * * CPU: Shows the time it takes to perform some silly floating point calculus.
 *        It uses 100% of one CPU.
 * * DISK_W: Shows the time it takes to write chunks on a file
 * * DISK_R_SEQ: Shows the time it takes to read sequentially chunks from a file
 * * DISK_R_RAN: Shows the time it takes to random read chunks from a file
 * * HTTP_GET: Shows the time it takes to HTTP GET a file
 * * PING: Shows the round-trip time when pinging a host
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
#include <curl/curl.h>    // libcurl
#include <oping.h>        // octo's ping library
#include <pthread.h>      // pthread_create ...
#include <stdint.h>       // intmax_t

#define CURL_REFS_FOLDER "/var/lib/sbench/http_refs"
#define CURL_TIMEOUT_MS  30000 // 30s for HTTP is ~infinite

#define EXIT_CODE_OK       0
#define EXIT_CODE_WARNING  1
#define EXIT_CODE_CRITICAL 2
#define EXIT_CODE_UNKNOWN  3

enum type {CPU, MEM, DISK_W, DISK_R_SEQ, DISK_R_RAN, HTTP_GET, PING};

/* arguments for cpu tests */
typedef struct cpu_args {
  unsigned long  times;
  int            verbose;
  unsigned int   threadNumber;
  double         delta; // return value
} cpu_args_struct;


/* arguments for disk read */
typedef struct dw_args {
  unsigned long sizeInBytes;
  unsigned long times;
  char         *folderName;
  int           verbose;
  unsigned int  threadNumber;
  double        delta; // return value
} dw_args_struct;

/* arguments for disk write */
typedef struct dr_args {
  enum type      type;
  unsigned long  sizeInBytes;
  unsigned long  times;
  char          *targetFileName;
  int            verbose;
  unsigned int   threadNumber;
  unsigned long *blocks;
  double         delta; // return value
} dr_args_struct;


void usage() {
  printf("Simple benchmarks, a first approach to performance measuring\n");
  printf("Usage:\n");
  printf("sbench (-v) -t cpu        -p <times>\n");
  printf("sbench (-v) -t cpu        -p <times,numThreads>\n");
  printf("sbench (-v) -t mem        -p <times,sizeInBytes>\n");
  printf("sbench (-v) -t disk_w     -p <times,sizeInBytes,folderName>\n");
  printf("sbench (-v) -t disk_w     -p <times,sizeInBytes,numThreads,folderName>\n");
  printf("sbench (-v) -t disk_r_seq -p <times,sizeInBytes,fileName>\n");
  printf("sbench (-v) -t disk_r_ran -p <times,sizeInBytes,fileName>\n");
  printf("sbench (-v) -t disk_r_ran -p <times,sizeInBytes,numThreads,fileName>\n");
  printf("sbench (-v) -t ping       -p <times,sizeInBytes,dest>\n");
  printf("sbench (-v) -t http_get   -p <httpRef,url>\n");
  printf("\nExamples:\n");
  printf("* To allocate&commit 10 MiB of RAM and memset it 10 times:\n");
  printf("  sbench -t mem -p 10,104857600\n");
  printf("* To have 2 threads doing 100E6 silly calculus (2 pows):\n");
  printf("  sbench -t cpu -p 10000000,2\n");
  printf("* To create 4 threads each writing 10 MiB in a file in 4k blocks:\n");
  printf("  sbench -t disk_w -p 2560,4096,4,/tmp/_sbench.d\n");
  printf("* To read sequentially 100 MiB from a file in 4k blocks:\n");
  printf("  sbench -t disk_r_seq -p 25600,4096,/tmp/_sbench.testfile\n");
  printf("* To read by random access 100 MiB from a file\n");
  printf("      by 2 threads in 4k blocks:\n");
  printf("  sbench -t disk_r_ran -p 25600,4096,2,/tmp/_sbench.testfile\n");
  printf("* To get the mean round-trip time sending\n");
  printf("      4 ICMP echo request to ahost.adomain.net:\n");
  printf("  sbench -t ping -p 4,56,ahost.adomain.net\n");
  printf("* To download by HTTP GET http://www.test.com/file ,\n");
  printf("      and to compare it with the reference:\n");
  printf("      file 'my_ref_file' located at %s :\n", CURL_REFS_FOLDER);
  printf("  sbench -t http_get -p my_ref_file,http://www.test.com/file\n");
  printf("\nzoquero@gmail.com https://github.com/zoquero/simplebenchmark\n");
  exit(1);
}

void myAbort(char* msg) {
  fprintf(stderr, "Error: %s\n", msg);
  exit(EXIT_CODE_CRITICAL);
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

void parseParams(char *params, enum type thisType, int verbose, unsigned long *times, unsigned long *sizeInBytes, unsigned int *nThreads, char *folderName, char *targetFileName, char *url, char *httpRefFileBasename, unsigned long *timeoutInMS, char *dest, double warn, double crit) {
  if(thisType == CPU) {
    if(strlen(params) > 19) {
      fprintf(stderr, "Params must be in \"num,num\" format\n");
      usage();
    }
    if(sscanf(params, "%lu,%u", times, nThreads) != 2) {
      *nThreads = 1;
      if(sscanf(params, "%lu", times) != 1) {
        fprintf(stderr, "Params must be in \"num,num\" format\n");
        usage();
      }
    }
    if(verbose)
      printf("type=cpu, times=%lu, nThreads=%u, warnLevel=%f, critLevel=%f, verbose=%d\n", *times, *nThreads, warn, crit, verbose);
  }
  else if(thisType == MEM) {
    if(sscanf(params, "%lu,%lu", times, sizeInBytes) != 2) {
      fprintf(stderr, "Params must be in \"num,num\" format\n");
      usage();
    }
    if(verbose)
      printf("type=mem, times=%lu, sizeInBytes=%lu, warnLevel=%f, critLevel=%f, verbose=%d\n", *times, *sizeInBytes, warn, crit, verbose);
  }
  else if(thisType == DISK_W) {
    if(sscanf(params, "%lu,%lu,%u,%s", times, sizeInBytes, nThreads, folderName) != 4) {
      *nThreads = 1;
      if(sscanf(params, "%lu,%lu,%s", times, sizeInBytes, folderName) != 3) {
        fprintf(stderr, "Params must be in \"num,num,(num,)path\" format\n");
        usage();
      }
    }
    if(verbose)
      printf("type=disk_w, times=%lu, sizeInBytes=%lu, nThreads=%d, folderName=%s, warnLevel=%f, critLevel=%f, verbose=%d\n", *times, *sizeInBytes, *nThreads, folderName, warn, crit, verbose);
  }
  else if(thisType == DISK_R_SEQ) {
    *nThreads = 1;
    if(sscanf(params, "%lu,%lu,%s", times, sizeInBytes, targetFileName) != 3) {
      fprintf(stderr, "Params must be in \"num,num,path\" format\n");
      usage();
    }
    if(verbose) {
      printf("type=disk_r_seq, times=%lu, sizeInBytes=%lu, targetFileName=%s verbose=%d\n", *times, *sizeInBytes, targetFileName, verbose);
    }
  }
  else if(thisType == DISK_R_RAN) {
    if(sscanf(params, "%lu,%lu,%u,%s", times, sizeInBytes, nThreads, targetFileName) != 4) {
      *nThreads = 1;
      if(sscanf(params, "%lu,%lu,%s", times, sizeInBytes, targetFileName) != 3) {
        fprintf(stderr, "Params must be in \"num,num,(num),path\" format\n");
        usage();
      }
    }
    if(verbose) {
      printf("type=disk_r_ran, times=%lu, sizeInBytes=%lu, nThreads=%u, targetFileName=%s verbose=%d\n", *times, *sizeInBytes, *nThreads, targetFileName, verbose);
    }
  }
  else if(thisType == HTTP_GET) {
    if(sscanf(params, "%[^,],%s", httpRefFileBasename, url) != 2) {
      fprintf(stderr, "Params must be in \"refName,url\" format\n");
      usage();
    }
    if(verbose)
      printf("type=http_get, httpRefFileBasename=%s, url=%s, verbose=%d\n", httpRefFileBasename, url, verbose);
  }
  else if(thisType == PING) {
    if(sscanf(params, "%lu,%lu,%s", times, sizeInBytes, dest) != 3) {
      fprintf(stderr, "Params must be in \"times,sizeInBytes,dest\" format\n");
      usage();
    }
    if(verbose)
      printf("type=ping, sizeInBytes=%lu, times=%lu, dest=%s, verbose=%d\n", *sizeInBytes, *times, dest, verbose);
  }
  else {
    fprintf(stderr, "Unknown o missing type\n");
    usage();
  }
}


void getOpts(int argc, char **argv, char **params, enum type *thisType, int *verbose, int *nagiosPluginOutput, double *warn, double  *crit) {
  int c;
  extern char *optarg;
  extern int optind, opterr, optopt;
  opterr = 0;

  if(argc < 5) {
    fprintf(stderr, "Missing parameters\n");
    usage();
  }

  while ((c = getopt (argc, argv, ":t:p:vw:c:")) != -1) {
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
        else if(strcmp(optarg, "ping") == 0) {
          *thisType = PING;
        }
        else if(strcmp(optarg, "http_get") == 0) {
          *thisType = HTTP_GET;
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
      case 'w':
        if(sscanf(optarg, "%lf", warn) != 1) {
          fprintf (stderr, "Option -%c requires an argument\n", c);
          usage();
        }
        break;
      case 'c':
        if(sscanf(optarg, "%lf", crit) != 1) {
          fprintf (stderr, "Option -%c requires an argument\n", c);
          usage();
        }
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
        fprintf (stderr,
          "Wrong command line arguments, probably a parameter without data\n");
        usage();
    }
  }
  // If you set warn or crit levels then you should set both
  if( (*warn == -1) != (*crit == -1) ) {
    fprintf (stderr, "If you set warn level then you must also set critical level and vice versa\n");
    usage();
  }
  // If you haven't set warn nor crit levels then you don't want a nagios plugin-like output
  if( (*warn == -1) || (*crit == -1) ) {
    if(*verbose)
      printf("You prefer simple output, not nagios-like\n");
    *nagiosPluginOutput=0;
  }
  else
    if(*verbose)
      printf("You prefer nagios-like output\n");
}

  /**
    * Simple way to have a CPU busy for a while.
    * It just does simple floating-point operations
    * (sums, substractions, powers and divisions).
    * 
    * If you are looking for a more complex CPU tests
    * then you may be looking for specs like these: https://www.spec.org/cpu/
    * 
    */
void *cpuTestStartupRoutine(void *arg) {
  struct timeval beginning, end;
  cpu_args_struct *args = (cpu_args_struct *) arg;

  // output is not serialized, so verbose mode will have an ugly look
  if(args->verbose)
    printf("thread #%d that will perform %lu calculus", 
      args->threadNumber,
      args->times);

  // Let's work:
  gettimeofday(&beginning, NULL);
  double x=2;
  for(long int i = 0; i < args->times; i++) {
    x=pow(x, x);
    x=pow(x, 1/(x-1));
  }
  gettimeofday(&end, NULL);
  args->delta=timeval_diff(&end, &beginning);
  return NULL;
}

/**
  * Waste some CPU cycles and return the number of econd sto do it
  * @param times Number of times that each thread has to calculate
  * @param nThreads Number of threads
  * @param verbose if verbose
  * @return double Average time that took each thread to do it
  */
double doCpuTest(unsigned long times, int nThreads, int verbose) {
  char msg[100];
  double delta = 0;

  // Thread creation
  pthread_t       *threads = (pthread_t *)       malloc(nThreads * sizeof(pthread_t));
  cpu_args_struct *args    = (cpu_args_struct *) malloc(nThreads * sizeof(cpu_args_struct));

  if(verbose) printf("Let's create %d threads:\n", nThreads);

  // let's fill the args for the n-th thread.
  for (int i = 0; i < nThreads; i++) {
    args[i].times        = times,
    args[i].verbose      = verbose,
    args[i].threadNumber = i;
    args[i].delta        = 0.;

    if(pthread_create(&(threads[i]), NULL, cpuTestStartupRoutine, (void *) &args[i]) ) {
      sprintf(msg, "Can't create the %d-th thread", i);
      myAbort(msg);
    }
  }

  if(verbose) printf("Threads created, waiting for completion...:\n");
  for (int i = 0; i < nThreads; i++) {
    if(pthread_join(threads[i], NULL)) {
      sprintf(msg, "Can't join to %d-th thread", i);
      myAbort(msg);
    }
    if(verbose) printf("The thread #%d has finished with delta = %f\n", i, args[i].delta);
    delta+=args[i].delta;
  }
  delta/=nThreads; // Average!!
  free(threads);
  free(args);

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
    if(verbose) printf("* malloc : %f\n", delta);
   
    /* VmRSS ! */
    gettimeofday(&before, NULL);
    if(memset(cptr, 0xA5, sizeInBytes) == NULL) {
      sprintf(msg, "Can't memset on those %lu bytes on memory", sizeInBytes);
      myAbort(msg);
    }
    gettimeofday(&after, NULL);
    delta=timeval_diff(&after, &before);
    if(verbose) printf("  memset : %f\n", delta);
  
    gettimeofday(&before, NULL);
    free(cptr);
    gettimeofday(&after, NULL);
    delta=timeval_diff(&after, &before);
    if(verbose) printf("  free   : %f\n", delta);
    //getchar();
  }
  gettimeofday(&end, NULL);
  delta=timeval_diff(&end, &beginning);

  return delta;
}


void *diskWriteStartupRoutine(void *arg) {
  char msg[100];
  struct timeval beginning, end;
  int  fd;
  char fileName[PATH_MAX];
  char *buffer;
  dw_args_struct *args = (dw_args_struct *) arg;

  // output is not serialized, so verbose mode will have an ugly look

  if(args->verbose)
    printf("thread #%d that will write %lu bytes %lu times on a file on %s", 
      args->threadNumber,
      args->sizeInBytes ,
      args->times       ,
      args->folderName);

  // Let's work:

  sprintf((char *) fileName, "%s/disk_w.out.%d", args->folderName, args->threadNumber);

/*
 * Ok, we'll overwrite
 *
  if(access(fileName, F_OK) != -1 ) {
    sprintf(msg, "Ensure that the target file %s doesn't exist previously",
      fileName);
    myAbort(msg);
  }
*/

  // Allocate RAM for the block of sizeInBytes bytes
  buffer=malloc(args->sizeInBytes);
  if(buffer == NULL) {
    sprintf(msg, "Can't allocate %lu bytes for the buffer", args->sizeInBytes);
    myAbort(msg);
  }
  if(memset(buffer, 0xA5, args->sizeInBytes) == NULL) {
    sprintf(msg, "Can't memset on the %lu bytes of the buffer", args->sizeInBytes);
    myAbort(msg);
  }

  // open creating or truncating
  fd = open(fileName, O_CREAT | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR);
  if(fd == -1) {
    sprintf(msg, "Can't open the target file %s for writing", fileName);
    myAbort(msg);
  }
  // loop for writing and storing (fflush+msync)
  if(args->verbose) printf("Let's write %lu bytes %lu types on %s\n",
                args->sizeInBytes, args->times, fileName);
  gettimeofday(&beginning, NULL);
  for(unsigned long i = 0; i < args->times; i++) {
    // write
    if(write(fd, buffer, args->sizeInBytes) != args->sizeInBytes) {
      sprintf(msg, "Can't write %lu bytes to %s", args->sizeInBytes, fileName);
      myAbort(msg);
    }
    /*
     * fsync for flushing all modified in-core data to the disk device.
     * This way we'll be able to send burst of BIOs if needed.
     */
    if(fsync(fd) != 0) {
      sprintf(msg, "Can't flush after writing %lu-th block on %s", i, fileName);
      myAbort(msg);
    }
  }
  gettimeofday(&end, NULL);
  args->delta=timeval_diff(&end, &beginning);

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

  // free the buffer
  free(buffer);

  return NULL;
}


double doDiskWriteTest(unsigned long sizeInBytes, unsigned long times, unsigned int nThreads, char *folderName, int verbose) {
  char msg[100];
  double delta = 0;

  struct stat s = {0};
  if(stat(folderName, &s) == 0)  {
    if(! S_ISDIR(s.st_mode))  {
      sprintf(msg, "%s must be a folder", folderName);
      myAbort(msg);
    }
  }
  else {
    if(mkdir(folderName, 0700) != 0) {
      sprintf(msg, "Can't create the folder %s", folderName);
      myAbort(msg);
    }
  }

  // Thread creation
  pthread_t      *threads = (pthread_t *)      malloc(nThreads * sizeof(pthread_t));
  dw_args_struct *args    = (dw_args_struct *) malloc(nThreads * sizeof(dw_args_struct));

  if(verbose) printf("Let's create %d threads:\n", nThreads);

  // let's fill the args for the n-th thread.
  for (int i = 0; i < nThreads; i++) {
    args[i].sizeInBytes  = sizeInBytes,
    args[i].times        = times,
    args[i].folderName   = folderName,
    args[i].verbose      = verbose,
    args[i].threadNumber = i,
    args[i].delta        = 0.;

    if(pthread_create(&(threads[i]), NULL, diskWriteStartupRoutine, (void *) &args[i]) ) {
      sprintf(msg, "Can't create the %d-th thread", i);
      myAbort(msg);
    }
  }

  if(verbose) printf("Threads created, waiting for completion...:\n");
  for (int i = 0; i < nThreads; i++) {
    if(pthread_join(threads[i], NULL)) {
      sprintf(msg, "Can't join to %d-th thread", i);
      myAbort(msg);
    }
    if(verbose) printf("The thread #%d has finished with delta = %f\n", i, args[i].delta);
    delta+=args[i].delta;
  }
  delta/=nThreads; // Average!!
  free(threads);
  free(args);

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

void *diskReadStartupRoutine(void *arg) {
  char msg[100];
  struct timeval beginning, end;
  double delta;
  int  fd;      // Each thread must have its own file descriptor for the file
  char *buffer;
  dr_args_struct *args = (dr_args_struct *) arg;
  unsigned long position;
  unsigned long position2;
  unsigned long *positions;

  if(args->verbose) printf("Thread #%d started:\n", args->threadNumber);
  if(args->verbose) printf("Thread #%d started, with first byte of first bloc: %lu\n", args->threadNumber, args->blocks[args->threadNumber * args->times] * args->sizeInBytes );

  // create the array with positions to read, case DISK_R_RAN
  if(args->type == DISK_R_RAN) {
    positions = (unsigned long *) malloc(sizeof(unsigned long) * args->times);
    for(unsigned long i = 0; i < args->times; i++) {
      position  = args->threadNumber * args->times + i;
      position2 = args->blocks[position] * args->sizeInBytes;
      // printf("Thread %d, iteration %lu: position # %lu, position2 # %lu\n", args->threadNumber, i, position, position2);
      positions[i] = position2;
    }
  }

  // The file must exist previously
  if(access(args->targetFileName, F_OK) == -1 ) {
    sprintf(msg, "Can't find the target file %s", args->targetFileName);
    myAbort(msg);
  }

  // Allocate RAM for the block of sizeInBytes bytes
  buffer=malloc(args->sizeInBytes);
  if(buffer == NULL) {
    sprintf(msg, "Can't allocate %lu bytes for the buffer", args->sizeInBytes);
    myAbort(msg);
  }

  // open file
  fd = open(args->targetFileName, O_RDONLY);
  if(fd == -1) {
    sprintf(msg, "Can't open the target file %s for reading", args->targetFileName);
    myAbort(msg);
  }

  if(args->verbose)
    printf("Thread #%d will read %lu bytes %lu times from %s\n",
      args->threadNumber, args->sizeInBytes, args->times, args->targetFileName);

  // loop for reading
  gettimeofday(&beginning, NULL);
  for(unsigned long i = 0; i < args->times; i++) {
    // lseek for random read if DISK_R_RAN is choosen
    if(args->type == DISK_R_RAN) {
      // printf("Thread %d, iteration %lu: lseek to byte #%lu\n", args->threadNumber, i, positions[i]);
      if(lseek(fd, positions[i], SEEK_SET) == -1) {
        sprintf(msg, "Can't lseek to reposition to %lu byte before reading on random-access to %s on %lu-th iteration", position2, args->targetFileName, i);
        myAbort(msg);
      }
    }

    // now can read, current file offset is right, be DISK_R_RAN or DISK_R_SEQ
    ssize_t ret_in = read(fd, buffer, args->sizeInBytes);
    // format: %zd for ssize_t
    if(args->verbose) printf("Thread #%d read %zd bytes on %lu-th iteration\n", args->threadNumber, ret_in, i);
    if(ret_in != args->sizeInBytes) {
      sprintf(msg, "Read just %zd bytes from %s on %lu-th iteration", ret_in, args->targetFileName, i);
      myAbort(msg);
    }
  }
  gettimeofday(&end, NULL);
  delta=timeval_diff(&end, &beginning);

  // close file
  if(close(fd) == -1) {
    sprintf(msg, "Can't close the target file %s", args->targetFileName);
    myAbort(msg);
  }

  // let's free the buffer and the array with positions
  free(buffer);
  if(args->type == DISK_R_RAN) {
    free(positions);
  }

  // return value
  args->delta=delta;
  return NULL;
}



/**
  * This function allows to access concurrently to a file.
  * It does it this way:
  * * Creates an array of "times * nthreads" positions,1
  * * shuffles it,
  * * creates "nThreads" threads
  * * asks each thread to read "times" of those positions
  * The result is a random concurrent access to that single file.
  */
double doDiskReadTest(enum type thisType, unsigned long sizeInBytes, unsigned long times, int nThreads, char *targetFileName, int verbose) {
  char msg[100];
  double delta;
  unsigned long *blocks;

  // allocate the array that will contain the block positions of the file
  blocks = (unsigned long *) malloc(sizeof(unsigned long) * times * nThreads);
  for(unsigned long i = 0; i < times * nThreads; i++) {
    blocks[i] = i;
  }
  shuffle(blocks, times * nThreads);

  // check file size
  struct stat s;
  stat(targetFileName, &s);
  if(s.st_size < times*nThreads*sizeInBytes) {
    // format: %jd (intmax_t) for off_t
    sprintf(msg, "The size of the file %s is %jd bytes" \
                 " and must be greater or equal to %lu*%d*%lu bytes",
                 targetFileName, (intmax_t) s.st_size, times,
                 nThreads, sizeInBytes);
    myAbort(msg);
  }

  // Thread creation
  pthread_t      *threads = (pthread_t *)      malloc(nThreads * sizeof(pthread_t));
  dr_args_struct *args    = (dr_args_struct *) malloc(nThreads * sizeof(dr_args_struct));

  if(verbose) printf("Let's create %d threads:\n", nThreads);

  // let's fill the args for the n-th thread.
  for (int i = 0; i < nThreads; i++) {
    args[i].type           = thisType,
    args[i].sizeInBytes    = sizeInBytes,
    args[i].times          = times,
    args[i].targetFileName = targetFileName,
    args[i].verbose        = verbose,
    args[i].threadNumber   = i,
    args[i].blocks         = blocks,
    args[i].delta          = 0.;

    if(pthread_create(&(threads[i]), NULL, diskReadStartupRoutine, (void *) &args[i]) ) {
      sprintf(msg, "Can't create the %d-th thread", i);
      myAbort(msg);
    }
  }

  // sit back and enjoy
  if(verbose) printf("All threads created, waiting for its completion...:\n");
  for (int i = 0; i < nThreads; i++) {
    if(pthread_join(threads[i], NULL)) {
      sprintf(msg, "Can't join to %d-th thread", i);
      myAbort(msg);
    }
    if(verbose) printf("Thread #%d finished with delta = %f\n", i, args[i].delta);
    delta+=args[i].delta;
  }
  delta/=nThreads; // Average!!
  free(threads);
  free(args);
  return delta;
}


size_t writeToFile(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}


/**
  * Compare two files
  * 
  * @arg FILE* stream open to first file
  * @arg FILE* stream open to second file
  * @return int 0 if are equal, 1 if are different
  */
int compare(FILE *fp1, FILE *fp2) {
  char c1, c2;
  int areDifferent = 0; // 0 ok, 1 differences found

  while (((c1 = fgetc(fp1)) != EOF) &&((c2 = fgetc(fp2)) != EOF)) {
    /* character by character comparision until end of file */
    // printf("Comparing %c == %c\n", c1, c2);
    if (c1 == c2)
      continue;
    /*
      * If not equal then returns the byte position
      */
    else {
      areDifferent = 1;
      break;
    }
  }
  return areDifferent;
}


/**
  * Get a file by HTTP GET, but will not follow redirections,
  * because it would add latency unnecessarily and the results would get biased
  * 
  * Done using libcurl: https://curl.haxx.se/libcurl/c/
  */
double httpGet(char *url, char *httpRefFileBasename, int *different, int verbose) {
  CURL *curl;
  CURLcode res;
  char msg[100];
  int  fd;
  FILE *fds;
  FILE *refFileStream;
  char fileNameTemplate[PATH_MAX];
  char refFilePath[PATH_MAX];
  struct timeval beginning, end;
  double delta;

  // get TMPDIR , TMP , TEMP , TEMPDIR ... /tmp/
  char const *tmpfolder = getenv("TMPDIR");
  if (tmpfolder == 0) {
    tmpfolder = getenv("TMP");
    if (tmpfolder == 0) {
      tmpfolder = getenv("TEMP");
      if (tmpfolder == 0) {
        tmpfolder = getenv("TEMPDIR");
        if (tmpfolder == 0) {
          // last chance, /tmp hardcoded
          tmpfolder = "/tmp";
        }
      }
    }
  }

  // get temporary file
  sprintf(fileNameTemplate, "%s/_sbench.libcurl.XXXXXX", tmpfolder);
  fd = mkstemp(fileNameTemplate);
  if(verbose) printf("Using temporary file %s\n", fileNameTemplate);

  // get a stream from the file descriptor
  fds = fdopen(fd, "w");
  if(fds == NULL) {
    sprintf(msg, "Can't re-open (r) the stream of the output of libcurl %s", fileNameTemplate);
    myAbort(msg);
  }

  // get the reference file
  sprintf(refFilePath, "%s/%s", CURL_REFS_FOLDER, httpRefFileBasename);
  if(verbose) printf("Using refFilePath = %s\n", refFilePath);

  // http://stackoverflow.com/questions/1636333/download-file-using-libcurl-in-c-c
 
  curl = curl_easy_init();

  // We'll set a max timeout (CURL_TIMEOUT_MS), just for sane connection
  // http://stackoverflow.com/questions/10486119/libcurl-c-timeout-and-success-transfer
  curl_easy_setopt(curl, CURLOPT_TIMEOUT,    CURL_TIMEOUT_MS);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, CURL_TIMEOUT_MS);

  /*
   * PENDING: proxy settings
   * anyway libcurl uses the environment variables like http_proxy
  curl_easy_setopt(curl, CURLOPT_PROXY, "proxy-host.com:8080"); 
  curl_easy_setopt(curl, CURLOPT_PROXYUSERPWD, "username:password"); 
   *
  */

  if(curl) {
    // Set the url
    curl_easy_setopt(curl, CURLOPT_URL, url);

    /*
     * You should use just final URLs and avoid HTTP redirections,
     * you'll surely agree. If you prefer to hardcode
     * that this software doesn't allow redirections,
     * then remove or comment the next line so that
     * CURLOPT_FOLLOWLOCATION will be disabled (by default):
     */
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToFile);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fds);
 
    // Perform the request, res will get the return code
    gettimeofday(&beginning, NULL);
    res = curl_easy_perform(curl);
    gettimeofday(&end, NULL);
    delta=timeval_diff(&end, &beginning);

    // Check for errors
    if(res != CURLE_OK) {
      sprintf(msg, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
      myAbort(msg);
    }

    /* cleanup libcurl stuff */ 
    curl_easy_cleanup(curl);

    // Compare with the reference
    refFileStream = fopen(refFilePath, "r");
    if(refFileStream == NULL) {
      fclose(fds); // let's try to close blindly
      sprintf(msg, "Can't open the reference file %s", refFilePath);
      myAbort(msg);
    }

    /*
     * Let's reopen libcurl's output just to compare with the reference:
     * Don't know why rewinding the stream doesn't work.
     */

    // close the file stream fed by libcurl
    if(fclose(fds) != 0) {
      sprintf(msg, "Can't close (w) the stream of the output of libcurl %s", fileNameTemplate);
      myAbort(msg);
    }

    // reopen the file stream previously fed by libcurl
    fds = fopen(fileNameTemplate, "r");
    if(fds == NULL) {
      sprintf(msg, "Can't re-open (r) the stream of the output of libcurl %s", fileNameTemplate);
      myAbort(msg);
    }

    // compare both files
    *different = compare(refFileStream, fds);

    // Close the file streams
    if(fclose(fds) != 0) {
      sprintf(msg, "Can't close (r) the stream of the output of libcurl %s", fileNameTemplate);
      myAbort(msg);
    }
    if(fclose(refFileStream) != 0) {
      sprintf(msg, "Can't close the stream of the reference file %s", refFilePath);
      myAbort(msg);
    }

    // remove the temporary file
    if(remove(fileNameTemplate) != 0) {
      sprintf(msg, "Can't delete the target file %s after the test", fileNameTemplate);
      myAbort(msg);
    }
  }
  else {
    sprintf(msg, "Can't get a libcurl handler for %s", url);
    myAbort(msg);
  }
  return delta;
}

/**
  *
  * Ping using Octo's ping library
  *
  * How to install this library on Ubuntu:
  *            * to run:     $ sudo apt-get install liboping0
  *            * to develop: $ sudo apt-get install liboping-dev libcurl4-openssl-dev
  *
  * To avoid having to run it as root (sudo or setuid)
  *   you can simply "setcap cap_net_raw=ep /opt/sbench/sbench"
  *
  * @seeAlso https://github.com/octo/liboping/
  */
float doPing(unsigned long sizeInBytes, unsigned long times, char *dest, int verbose) {
  pingobj_t *ping;
  pingobj_iter_t *iter;
  char msg[100];
  int i = 1;
  double accumulatedLatency = 0;
  size_t successfullResponses = 0;
  double averageLatency = 0;

  if(verbose) printf("Sending %lu ICMP echo request paquets %lu bytes-long to %s\n",times, sizeInBytes, dest);
  
  if((ping = ping_construct()) == NULL) {
    sprintf(msg, "ping_construct: failed\n");
    myAbort(msg);
  }
  if(verbose) printf("ping_construct(): success\n");
  
  if(ping_host_add(ping, dest) < 0) {
    const char *errMsg = ping_get_error(ping);
    sprintf(msg, "ping_host_add(%s): failed: %s. If the opperation is not permitted you could use something like \"sudo setcap cap_net_raw=ep\" on your executable\n", dest, errMsg);
    myAbort(msg);
  }
  if(verbose) printf("ping_host_add(): success\n");
  
  while(1) {
    if(ping_send(ping) < 0) {
      sprintf(msg, "ping_send #%d: failed\n", i);
      myAbort(msg);
    }
    // if(verbose) printf("ping_send() #%d: success\n", i);
    
    for (iter = ping_iterator_get(ping); iter != NULL; iter =
            ping_iterator_next(iter)) {
      char hostname[100];
      double latency;
      size_t len; // format for size_t : %zu
      
      // if(verbose) printf("ping_iterator_get() #%d: success\n", i);
      len = 100;
      ping_iterator_get_info(iter, PING_INFO_HOSTNAME, hostname, &len);
      len = sizeof(double);
      ping_iterator_get_info(iter, PING_INFO_LATENCY, &latency, &len);
      if(latency != -1) {
        successfullResponses++;
        accumulatedLatency  += latency;
      }
      
      if(verbose) printf("ping #%d: hostname = %s, latency = %f\n", i, hostname, latency);
    }
    // if(verbose) printf("ping iteration # %d\n", i);
    if(i++ == times)
      break;
    sleep(1);
  }
  if(successfullResponses == 0) {
    sprintf(msg, "Zero responses received when sending %lu echo requests to %s", times, dest);
    myAbort(msg);
  }
  averageLatency = accumulatedLatency/successfullResponses;
  return averageLatency;
}

int main (int argc, char *argv[]) {
  int  verbose = 0;
  char *params = 0;
  enum type thisType;
  unsigned long sizeInBytes, times;
  unsigned int  nThreads;
  char folderName[PATH_MAX-12];
  char targetFileName[PATH_MAX];
  char url[CURLINFO_EFFECTIVE_URL];
  char httpRefFileBasename[PATH_MAX];
  unsigned long timeoutInMS;
  int different = 1;
  char dest[HOST_NAME_MAX];
  double r;
  int nagiosPluginOutput = 1;
  double warn = -1., crit = -1.;

  getOpts(argc, argv, &params, &thisType, &verbose, &nagiosPluginOutput, &warn, &crit);
  parseParams(params, thisType, verbose, &times, &sizeInBytes, &nThreads, folderName, targetFileName, url, httpRefFileBasename, &timeoutInMS, dest, warn, crit);
  if(thisType == CPU) {
    r = doCpuTest(times, nThreads, verbose);
    double avgCalcsPerSecondPerCpu = times/r;
    if(nagiosPluginOutput) {
      if(avgCalcsPerSecondPerCpu >= crit) {
        printf("CPU Critical = %.2f avg calcs / s per CPU| avg_calcs_per_sec=%.2f\n", avgCalcsPerSecondPerCpu, avgCalcsPerSecondPerCpu);
        exit(EXIT_CODE_CRITICAL);
      }
      else if(avgCalcsPerSecondPerCpu >= warn) {
        printf("CPU Warning = %.2f avg calcs / s per CPU| avg_calcs_per_sec=%.2f\n", avgCalcsPerSecondPerCpu, avgCalcsPerSecondPerCpu);
        exit(EXIT_CODE_WARNING);
      }
      else {
        printf("CPU OK = %.2f avg calcs / s per CPU| avg_calcs_per_sec=%.2f\n", avgCalcsPerSecondPerCpu, avgCalcsPerSecondPerCpu);
        exit(EXIT_CODE_OK);
      }
    }
    else {
      printf("%.2f avg calcs / s per CPU\n", avgCalcsPerSecondPerCpu);
      exit(EXIT_CODE_OK);
    }
  }
  else if(thisType == MEM) {
    r = doMemTest(sizeInBytes, times, verbose);
    if(nagiosPluginOutput) {
      if(r >= crit) {
        printf("Mem Critical = %.2f s| time=%.2f\n", r, r);
        exit(EXIT_CODE_CRITICAL);
      }
      else if(r >= warn) {
        printf("Mem Warning = %.2f s| time=%.2f\n", r, r);
        exit(EXIT_CODE_WARNING);
      }
      else {
        printf("Mem OK = %.2f s| time=%.2f\n", r, r);
        exit(EXIT_CODE_OK);
      }
    }
    else {
      printf("%.2f s\n", r);
      exit(EXIT_CODE_OK);
    }
  }
  else if(thisType == DISK_W) {
    r = doDiskWriteTest(sizeInBytes, times, nThreads, folderName, verbose);
    if(nagiosPluginOutput) {
      if(r >= crit) {
        printf("DiskWrite Critical = %.2f s| time=%.2f\n", r, r);
        exit(EXIT_CODE_CRITICAL);
      }
      else if(r >= warn) {
        printf("DiskWrite Warning = %.2f s| time=%.2f\n", r, r);
        exit(EXIT_CODE_WARNING);
      }
      else {
        printf("DiskWrite OK = %.2f s| time=%.2f\n", r, r);
        exit(EXIT_CODE_OK);
      }
    }
    else {
      printf("%.2f s\n", r);
      exit(EXIT_CODE_OK);
    }
  }
  else if(thisType == DISK_R_SEQ || thisType == DISK_R_RAN) {
    r = doDiskReadTest(thisType, sizeInBytes, times, nThreads, targetFileName, verbose);
    if(nagiosPluginOutput) {
      if(r >= crit) {
        printf("%sDiskRead Critical = %.6f s| time=%.6f\n", thisType == DISK_R_SEQ ? "Seq" : "Ran", r, r);
        exit(EXIT_CODE_CRITICAL);
      }
      else if(r >= warn) {
        printf("%sDiskRead Warning = %.6f s| time=%.6f\n", thisType == DISK_R_SEQ ? "Seq" : "Ran", r, r);
        exit(EXIT_CODE_WARNING);
      }
      else {
        printf("%sDiskRead OK = %.6f s| time=%.6f\n", thisType == DISK_R_SEQ ? "Seq" : "Ran", r, r);
        exit(EXIT_CODE_OK);
      }
    }
    else {
      printf("%.6f s\n", r);
      exit(EXIT_CODE_OK);
    }
  }
  else if(thisType == HTTP_GET) {
    if(verbose) printf("getting %s by HTTP GET\n", url);
    r = httpGet(url, httpRefFileBasename, &different, verbose);

    if(nagiosPluginOutput) {
      if(different || r >= crit) {
        printf("%sHttpGet Critical = %.3f s| time=%.3f\n", thisType == DISK_R_SEQ ? "Seq" : "Ran", r, r);
        exit(EXIT_CODE_CRITICAL);
      }
      else if(r >= warn) {
        printf("%sHttpGet Warning = %.3f s| time=%.3f\n", thisType == DISK_R_SEQ ? "Seq" : "Ran", r, r);
        exit(EXIT_CODE_WARNING);
      }
      else {
        printf("%sHttpGet OK = %.3f s| time=%.3f\n", thisType == DISK_R_SEQ ? "Seq" : "Ran", r, r);
        exit(EXIT_CODE_OK);
      }
    }
    else {
      if(different) {
        printf("KO %.3f s\n", r);
        exit(EXIT_CODE_CRITICAL);
      }
      else {
        printf("OK %.3f s\n", r);
        exit(EXIT_CODE_OK);
      }
    }
  }
  else if(thisType == PING) {
    r = doPing(sizeInBytes, times, dest, verbose);
    if(nagiosPluginOutput) {
      if(r >= crit) {
        printf("PingRTT Critical = %.1f ms| time_ms=%.1f\n", r, r);
        exit(EXIT_CODE_CRITICAL);
      }
      else if(r >= warn) {
        printf("PingRTT Warning = %.1f ms| time_ms=%.1f\n", r, r);
        exit(EXIT_CODE_WARNING);
      }
      else {
        printf("PingRTT OK = %.1f ms| time_ms=%.1f\n", r, r);
        exit(EXIT_CODE_OK);
      }
    }
    else {
      printf("%.1f ms\n", r);
      exit(EXIT_CODE_OK);
    }
  }
  else {
    myAbort(/* bug */ "Unknown type");
    exit(2);
  }
}
