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

#include <stdlib.h>       // exit
#include <string.h>       // memcpy, strlen
#include <ctype.h>        // isprint
#include <getopt.h>       // getopt
#include <errno.h>        // errno
#include <curl/curl.h>    // libcurl

#include "sbenchfuncs.h"

void usage() {
  printf("Simple benchmarks, a first approach to performance measuring\n");
  printf("Usage:\n");
  printf("sbench (-v) -t cpu        "
         "(-w warnThreshold -c critThreshold) "
         "-p <times>\n");
  printf("sbench (-v) -t cpu        "
         "(-w warnThreshold -c critThreshold) "
         "-p <times,numThreads>\n");
  printf("sbench (-v) -t mem        "
         "(-w warnThreshold -c critThreshold) "
         "-p <times,sizeInBytes>\n");
  printf("sbench (-v) -t disk_w     "
         "(-w warnThreshold -c critThreshold) "
         "-p <times,sizeInBytes,folderName>\n");
  printf("sbench (-v) -t disk_w     "
         "(-w warnThreshold -c critThreshold) "
         "-p <times,sizeInBytes,numThreads,folderName>\n");
  printf("sbench (-v) -t disk_r_seq "
         "(-w warnThreshold -c critThreshold) "
         "-p <times,sizeInBytes,fileName>\n");
  printf("sbench (-v) -t disk_r_ran "
         "(-w warnThreshold -c critThreshold) "
         "-p <times,sizeInBytes,fileName>\n");
  printf("sbench (-v) -t disk_r_ran "
         "(-w warnThreshold -c critThreshold) "
         "-p <times,sizeInBytes,numThreads,fileName>\n");
// ifdef OPING_ENABLED
  printf("sbench (-v) -t ping       "
         "(-w latencyWarn_lossWarn -c latencyCrit_lossCrit) "
         "-p <times,sizeInBytes,dest>\n");
// endif // OPING_ENABLED
  printf("sbench (-v) -t http_get   "
         "(-w warnThreshold -c critThreshold) "
         "-p <httpRef,url>\n");
  printf("\nExamples:\n");
  printf("* To allocate&commit 10 MiB of RAM and memset it 10 times\n"
         "      and get a response in nagios plugin-like format:\n");
  printf("  sbench -t mem -p 10,104857600 -w 0.3 -c 0.5\n\n");
  printf("* To have 2 threads doing 100E6 silly calculus (2 pows):\n");
  printf("  sbench -t cpu -p 10000000,2\n\n");
  printf("* To create 4 threads each writing 10 MiB in a file in 4k blocks:\n");
  printf("  sbench -t disk_w -p 2560,4096,4,/tmp/_sbench.d\n\n");
  printf("* To read sequentially 100 MiB from a file in 4k blocks:\n");
  printf("  sbench -t disk_r_seq -p 25600,4096,/tmp/_sbench.testfile\n\n");
  printf("* To read by random access 100 MiB from a file\n");
  printf("      by 2 threads in 4k blocks:\n");
  printf("  sbench -t disk_r_ran -p 25600,4096,2,/tmp/_sbench.testfile\n\n");
// ifdef OPING_ENABLED
  printf("* To get the mean round-trip time sending\n");
  printf("      4 ICMP echo request of 56 bytes to www.gnu.org:\n");
  printf("  sbench -t ping -p 4,56,www.gnu.org\n\n");
  printf("* Idem but applying latency warning = 5ms, latency crit = 30ms,\n");
  printf("      packet loss warning = 1%%, packet loss critical = 5%%:\n");
  printf("  sbench -t ping -w 5_1 -c 30_5 -p 4,56,www.gnu.org\n\n");
// endif // OPING_ENABLED
  printf("* To download by HTTP GET http://www.test.com/file ,\n");
  printf("      and to compare it with the reference:\n");
  printf("      file 'my_ref_file' located at %s :\n", CURL_REFS_FOLDER);
  printf("  sbench -t http_get -p my_ref_file,http://www.test.com/file\n\n");
  printf("\nzoquero@gmail.com https://github.com/zoquero/simplebenchmark\n");
  exit(EXIT_CODE_CRITICAL);
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

void parseParams(char *params, enum btype thisType, int verbose, unsigned long *times, unsigned long *sizeInBytes, unsigned int *nThreads, char *folderName, char *targetFileName, char *url, char *httpRefFileBasename, unsigned long *timeoutInMS, char *dest, double warn, double crit) {
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
// ifdef OPING_ENABLED
  else if(thisType == PING) {
    if(sscanf(params, "%lu,%lu,%s", times, sizeInBytes, dest) != 3) {
      fprintf(stderr, "Params must be in \"times,sizeInBytes,dest\" format\n");
      usage();
    }
    if(verbose)
      printf("type=ping, sizeInBytes=%lu, times=%lu, dest=%s, verbose=%d\n", *sizeInBytes, *times, dest, verbose);
  }
// endif // OPING_ENABLED
  else {
    fprintf(stderr, "Unknown o missing type\n");
    usage();
  }
}


void getOpts(int argc, char **argv, char **params, enum btype *thisType, int *verbose, int *nagiosPluginOutput, double *warn, double  *crit, double *warn2, double  *crit2) {
  int c;
  extern char *optarg;
  extern int optind, opterr, optopt;
  opterr = 0;

  if(argc == 2 && strcmp(argv[1], "-h") == 0) {
    usage();
  }
  if(argc < 5) {
    fprintf(stderr, "Missing parameters\n");
    usage();
  }

  while ((c = getopt (argc, argv, ":ht:p:vw:c:")) != -1) {
    switch (c) {
      case 'h':
        usage();
        break;
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
// ifdef OPING_ENABLED
        else if(strcmp(optarg, "ping") == 0) {
          *thisType = PING;
        }
// endif // OPING_ENABLED
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
        if(sscanf(optarg, "%lf_%lf", warn, warn2) != 1) {
          if(sscanf(optarg, "%lf", warn) != 1) {
            fprintf (stderr, "Option -%c requires an argument\n", c);
            usage();
          }
        }
        break;
      case 'c':
        if(sscanf(optarg, "%lf_%lf", crit, crit2) != 1) {
          if(sscanf(optarg, "%lf", crit) != 1) {
            fprintf (stderr, "Option -%c requires an argument\n", c);
            usage();
          }
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
  if( *thisType == PING &&
      ( (*warn == -1) || (*crit == -1) || (*warn2 == -1) || (*crit2 == -1) )) {
    fprintf (stderr, "Ping warning and critical levels must be two arguments"
                     " each (latency in ms and percent of packet loss)"
                     " separated by an underscore \"_\" (eg: -c 10_5 ...)\n");
    usage();
  }
  else
    if(*verbose)
      printf("You prefer nagios-like output\n");
}

/**
  * Main.
  *
  * Some guidelines about developing nagios plugins:
  * https://nagios-plugins.org/doc/guidelines.html
  * http://blog.centreon.com/good-practices-how-to-develop-monitoring-plugin-nagios/
  *
  */
int main (int argc, char *argv[]) {
  int  verbose = 0;
  char *params = 0;
  enum btype thisType;
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
  double warn  = -1., crit  = -1.;
  double warn2 = -1., crit2 = -1.;

  getOpts(argc, argv, &params, &thisType, &verbose, &nagiosPluginOutput, &warn, &crit, &warn2, &crit2);
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
// ifdef OPING_ENABLED
  else if(thisType == PING) {
    pingResponse pr; // pr.latencyMs, pr.lossPerCent
    pr = doPing(sizeInBytes, times, dest, verbose);
    if(verbose) printf("  time_ms=%.1fms, warn=%1.f crit=%1.f\n", pr.latencyMs, warn, crit);
    if(verbose) printf("  loss_percent=%.1f%%, warn=%1.f crit=%1.f\n", pr.lossPerCent, warn2, crit2);

    if(pr.latencyMs == -1 && pr.lossPerCent != 100) {
      printf("PingRTT Unknown = Can't parse ping data = %.1f ms, %.1f %%| time_ms=%.1fms loss_percent=%.1f%%\n", pr.latencyMs, pr.lossPerCent, pr.latencyMs, pr.lossPerCent);
      exit(EXIT_CODE_UNKNOWN);
    }
    if(nagiosPluginOutput) {
      if(pr.latencyMs >= crit || pr.lossPerCent >= crit2) {
        printf("PingRTT Critical = %.1f ms, %.1f %%| time_ms=%.1fms loss_percent=%.1f%%\n", pr.latencyMs, pr.lossPerCent, pr.latencyMs, pr.lossPerCent);
        exit(EXIT_CODE_CRITICAL);
      }
      else if(pr.latencyMs >= warn || pr.lossPerCent >= warn2) {
        printf("PingRTT Warning = %.1f ms, %.1f %%| time_ms=%.1fms loss_percent=%.1f%%\n", pr.latencyMs, pr.lossPerCent, pr.latencyMs, pr.lossPerCent);
        exit(EXIT_CODE_WARNING);
      }
      else {
        printf("PingRTT OK = %.1f ms, %.1f %%| time_ms=%.1fms loss_percent=%.1f%%\n", pr.latencyMs, pr.lossPerCent, pr.latencyMs, pr.lossPerCent);
        exit(EXIT_CODE_OK);
      }
    }
    else {
      printf("%.1f ms;%.1f %%\n", pr.latencyMs, pr.lossPerCent);
      exit(EXIT_CODE_OK);
    }
  }
// endif // OPING_ENABLED
  else {
    myAbort(/* bug */ "Unknown type");
    exit(2);
  }
}
