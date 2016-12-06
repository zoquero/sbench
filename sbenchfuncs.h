/*
 * Simple Benchmarks
 * 
 * @since 20161027
 * @author zoquero@gmail.com
 */

#ifndef SBENCHFUNCS_H
#define SBENCHFUNCS_H

#define CURL_REFS_FOLDER "/var/lib/sbench/http_refs"
#define CURL_TIMEOUT_MS  30000 // 30s for HTTP is ~infinite

#define EXIT_CODE_OK       0
#define EXIT_CODE_WARNING  1
#define EXIT_CODE_CRITICAL 2
#define EXIT_CODE_UNKNOWN  3

// ifdef OPING_ENABLED
enum btype {CPU, MEM, DISK_W, DISK_R_SEQ, DISK_R_RAN, HTTP_GET, PING};
// else  // OPING_ENABLED
// enum btype {CPU, MEM, DISK_W, DISK_R_SEQ, DISK_R_RAN, HTTP_GET};
// endif // OPING_ENABLED

/** ping response */
typedef struct {
  /** latency in miliseconds */
  float latencyMs;
  /** Percent of package loss */
  float lossPerCent;
} pingResponse;

#ifndef OPING_ENABLED
#include <regex.h>
void parsePingOutput (char *source, pingResponse *pr, regex_t *regex1Compiled, regex_t *regex2Compiled);
#endif // OPING_ENABLED

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
  enum btype      type;
  unsigned long  sizeInBytes;
  unsigned long  times;
  char          *targetFileName;
  int            verbose;
  unsigned int   threadNumber;
  unsigned long *blocks;
  double         delta; // return value
} dr_args_struct;


void myAbort(char* msg);

double doCpuTest(unsigned long times, int nThreads, int verbose);


double doMemTest(unsigned long sizeInBytes, unsigned long times, int verbose);


double doDiskWriteTest(unsigned long sizeInBytes, unsigned long times, unsigned int nThreads, char *folderName, int verbose);


// void shuffle(unsigned long *array, size_t n);


double doDiskReadTest(enum btype thisType, unsigned long sizeInBytes, unsigned long times, int nThreads, char *targetFileName, int verbose);


// size_t writeToFile(void *ptr, size_t size, size_t nmemb, FILE *stream);


double httpGet(char *url, char *httpRefFileBasename, int *different, int verbose);

pingResponse doPing(unsigned long sizeInBytes, unsigned long times, char *dest,
             int verbose);

#endif

