/*
 * Simple Benchmarks
 * 
 * @since 20161027
 * @author zoquero@gmail.com
 */

#include <unistd.h>       // read, write, fsync, lseek, access
#include <stdlib.h>       // exit, malloc, free
#include <string.h>       // memcpy, strlen
#include <math.h>         // pow
#include <sys/stat.h>     // stat
#include <fcntl.h>        // open
#include <curl/curl.h>    // libcurl
#include <pthread.h>      // pthread_create ...
#include <stdint.h>       // intmax_t

#ifdef OPING_ENABLED
#include <oping.h>        // octo's ping library
#else // OPING_ENABLED
#define BUFSIZE 1024
#include <regex.h>
#endif // OPING_ENABLED

#include "sbenchfuncs.h"


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
double doDiskReadTest(enum btype thisType, unsigned long sizeInBytes, unsigned long times, int nThreads, char *targetFileName, int verbose) {
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

#ifdef OPING_ENABLED
/**
  *
  * Ping using Octo's ping library,
  * it returns an structure with average latency and packet loss.
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
pingResponse doPing(unsigned long sizeInBytes, unsigned long times, char *dest,
             int verbose) {
  pingobj_t *ping;
  pingobj_iter_t *iter;
  char msg[100];
  int i = 1;
  double accumulatedLatency = 0;
  size_t successfullResponses = 0;
  double averageLatency = 0;
  pingResponse pr = {1, -1};

  if(verbose) printf("Sending %lu ICMP echo request paquets "
                     "%lu bytes-long to %s\n",times, sizeInBytes, dest);
  
  if((ping = ping_construct()) == NULL) {
    sprintf(msg, "ping_construct: failed\n");
    myAbort(msg);
  }
  if(verbose) printf("ping_construct(): success\n");
  
  if(ping_host_add(ping, dest) < 0) {
    const char *errMsg = ping_get_error(ping);
    sprintf(msg, "ping_host_add(%s): failed: %s. "
                 "If 'the operation is not permitted' you could use "
                 "something like \"sudo setcap cap_net_raw=ep\" "
                 "on your executable\n", dest, errMsg);
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
      double latencyMs;
      size_t len; // format for size_t : %zu
      
      // if(verbose) printf("ping_iterator_get() #%d: success\n", i);
      len = 100;
      ping_iterator_get_info(iter, PING_INFO_HOSTNAME, hostname, &len);
      len = sizeof(double);
      ping_iterator_get_info(iter, PING_INFO_LATENCY, &latencyMs, &len);
      if(latencyMs != -1) {
        successfullResponses++;
        accumulatedLatency  += latencyMs;
      }
      
      if(verbose) printf("ping #%d: hostname = %s, latency = %f\n", i, hostname, latencyMs);
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

  pr.latencyMs     = averageLatency;
  pr.lossPerCent = 100.*(times - successfullResponses)/times;
  if(verbose) printf("Returning latency=%.1f, packet loss=%.1f\n", pr.latencyMs, pr.lossPerCent);
  return pr;
}

#else  // OPING_ENABLED

/**
  * Ping running external ping program,
  * it returns an structure with average latency and packet loss.
  * It just requires ping on PATH, parses it's output with regular expressions.
  *
  * Lacking of an API is terrible because you have to parse standard output,
  * and it can lead to problems with different versions of the command
  * and locale dependencies. Luckily the ping command looks like consistent
  * in different Linux distros with different locales:
  *
  * Ping output on SLES 11 SP4:
  * # ping -c 10 192.168.0.1
  * PING 192.168.0.1 (192.168.0.1) 56(84) bytes of data.
  * 64 bytes from 192.168.0.1: icmp_seq=1 ttl=63 time=7.23 ms
  * 64 bytes from 192.168.0.1: icmp_seq=2 ttl=63 time=5.41 ms
  * ...
  * --- 192.168.0.1 ping statistics ---
  * 10 packets transmitted, 10 received, 0% packet loss, time 9003ms
  * rtt min/avg/max/mdev = 4.124/5.438/7.666/1.584 ms
  *
  * Ping output on Ubuntu 16.04.1:
  * $ ping -c 10 192.168.0.1
  * PING 192.168.0.1 (192.168.0.1) 56(84) bytes of data.
  * 64 bytes from 192.168.0.1: icmp_seq=1 ttl=64 time=1.89 ms
  * 64 bytes from 192.168.0.1: icmp_seq=2 ttl=64 time=1.78 ms
  * ...
  * --- 192.168.0.1 ping statistics ---
  * 10 packets transmitted, 10 received, 0% packet loss, time 9005ms
  * rtt min/avg/max/mdev = 1.677/2.123/2.718/0.429 ms
  *
  */
pingResponse doPing(unsigned long sizeInBytes, unsigned long times, char *dest,
             int verbose) {
  char msg[100];
  int  i = 1;
  char command[BUFSIZ];
  char buf[BUFSIZE];
  FILE *fp;

  float q = 0;
  float f;
  char *end;
  char *regex1String = "(.+) packets transmitted, (.+) received, .+% packet loss";
  char *regex2String = ".* = [^/]+/([^/]+)/[^/]+/[^/]+ ms";
  regex_t regex1Compiled;
  regex_t regex2Compiled;
  pingResponse pr = {1, -1};

  if(verbose) printf("Sending %lu ICMP echo request paquets "
                     "%lu bytes-long to %s\n",times, sizeInBytes, dest);

//sprintf(command, "ping -s %lu -c %lu %s | cut -d \":\" -f 2 | cut -d \"=\" -f 4 | grep \" ms$\" | cut -d \" \" -f 1", sizeInBytes, times, dest);
  sprintf(command, "ping -s %lu -c %lu %s", sizeInBytes, times, dest);
  if(verbose) printf("ping command that will be used: [%s]\n", command);

  if ((fp = popen(command, "r")) == NULL) {
    sprintf(msg, "Can't open a pipe for running the command [%s]", command);
    myAbort(msg);
  }

  if(verbose) printf("regex1: %s\n", regex1String);
  if(verbose) printf("regex2: %s\n", regex2String);

  if (regcomp(&regex1Compiled, regex1String, REG_EXTENDED)) {
    printf("Could not compile regular expression [%s]\n", regex1String);
    exit(1);
  }
  if (regcomp(&regex2Compiled, regex2String, REG_EXTENDED)) {
    printf("Could not compile regular expression [%s]\n", regex2String);
    exit(1);
  }

  while (fgets(buf, BUFSIZE, fp) != NULL) {
    // f = atof(buf);
    // http://en.cppreference.com/w/c/string/byte/strtof
    // f = strtod(buf, &end);
    if(verbose) printf("* %s", buf);
    parsePingOutput(buf, &pr, &regex1Compiled, &regex2Compiled);
  }

  if(verbose) printf("Ping response: loss=%.1f, latency=%.1f\n", pr.lossPerCent, pr.latencyMs);
  regfree(&regex1Compiled);
  regfree(&regex2Compiled);
  return pr;
}

void parsePingOutput (char *source, pingResponse *pr, regex_t *regex1Compiled, regex_t *regex2Compiled) {
  size_t maxGroups = 3;
  char   numStr[strlen(source) + 1];
  char  *end;
  float  num;
  unsigned int g = 0;
  char sourceCopy[strlen(source) + 1];
  regmatch_t groupArray[maxGroups];
  int tx=-1, rx=-1;

  // example about C regexp: http://stackoverflow.com/a/11864144/939015
  if (regexec(regex1Compiled, source, maxGroups, groupArray, 0) == 0) {
    for (g = 0; g < maxGroups; g++) {
      if (groupArray[g].rm_so == (size_t)-1)
        break;  // No more groups

      strcpy(sourceCopy, source);
      sourceCopy[groupArray[g].rm_eo] = 0;

      if(g == 1 || g == 2) {
        strcpy(numStr, sourceCopy + groupArray[g].rm_so);
  
        num = strtod(numStr, &end);
        if (end == numStr) {
          // parse error
          continue;
        }
        if(g == 1) {
          tx = num;
        }
        else if(g == 2) {
          rx = num;
        }
      }

    }
    if(tx == 0)
      pr->lossPerCent=100;
    else {
      pr->lossPerCent=100.*(tx-rx)/tx;
    }
//  printf("packets transmitted=%d , received=%d, calculated loss = %f per cent\n", tx, rx, pr->lossPerCent);
  }

  maxGroups = 2;
  if (regexec(regex2Compiled, source, maxGroups, groupArray, 0) == 0) {
    for (g = 0; g < maxGroups; g++) {
      if (groupArray[g].rm_so == (size_t)-1)
        break;  // No more groups

      strcpy(sourceCopy, source);
      sourceCopy[groupArray[g].rm_eo] = 0;

      if(g == 1) {
//      printf("Group %u: [%2u-%2u]: %s\n",
//        g, groupArray[g].rm_so, groupArray[g].rm_eo,
//          sourceCopy + groupArray[g].rm_so);
        strcpy(numStr, sourceCopy + groupArray[g].rm_so);
  
        num = strtod(numStr, &end);
        if (end == numStr) {
          // parse error
          continue;
        }
        pr->latencyMs=num;
      }
    }
  }
}
#endif // else OPING_ENABLED

