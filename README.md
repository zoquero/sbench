# simplebenchmark
Lightweight program written in C to do simple benchmarks

Measures how systems perform doing these tests:

* Memory: allocate, commit and set
* CPU: multi-threaded floating-point operations (simply sum, multiply and divide)
* Disk:
    * Sequential read
    * Sequential write
    * Multi-threaded random write
* Network:
    * Latency: round-trip time by ICMP echo request 
    * Throughput: HTTP GET

Tested on Ubuntu Linux and can be portable to other platforms because just uses POSIX API, liboping and libcurl.

Source on GitHub: [https://github.com/zoquero/simplebenchmark/](https://github.com/zoquero/simplebenchmark/).

Angel ( zoquero@gmail.com )

Since November of 2016
