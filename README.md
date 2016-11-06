# Summary
Lightweight program written in C to do simple benchmarks

Angel ( zoquero@gmail.com ), november of 2016

# Features

Measures how systems perform doing these tests:

* Memory: allocate, commit and set
* CPU: multi-threaded floating-point operations (simply sums, substractions, powers and divisions)
* Disk (filesystem):
    * Sequential read
    * Sequential write
    * Multi-threaded random write
* Network:
    * Latency: round-trip time by ICMP echo request 
    * Throughput: HTTP GET

# Motivation
You can find suites for benchmarking like [hardinfo](https://github.com/lpereira/hardinfo/) or [Phoronix Test Suite](http://www.phoronix-test-suite.com/) but I just need some simple and reproducible tests that can be easily understood. For example, when testing CPU it just performs some simple floating-point operations that any i386 could do (sums, substractions, powers and divisions). If you are looking for a more complex CPU tests then you may be looking for specs like the ones from [SPEC CPU Benchmark](https://www.spec.org/cpu/).

Ok, but *why can I need it*? For **troubleshooting** if you are **moving** your virtual machines from your own IaaS **to a public cloud**. When you manage the IaaS under your VMs you always can read some data on your hypervisors, your network appliances, your SAN and your storage arrays (like for example *CPU Ready* and *Co-Stop* or network and storage latencies) that you will not be able to read on a public cloud.

Ok, but *I already use a benchmark suite to test it all*? Nice, keep on using it. Mmmmmm, but, how long does it takes for you to run those tests? Do you know exactly what are those tests doing? And, can you adjust that suite so that it doesn't induce a heavy load-test that could affect your other systems?

## Baseline
You can use it to extract a simple baseline of your systems (on idle!), so that you if you have performance problems on your *pets* you will be able to (stop services first! sig) extract it again to realize if it's your application or the IaaS who is inducing the problem.

## Use wisely on production environments
Take care when using it. Having several threads writing on a file can generate hundreds of IOPS that can affect negatively the performance of other systems. Fortunately you can adjust finely the tests.

# Command line interface 
You'll find help simply running the executable without arguments. It's esasy to use.

## Nagios plugin
Using the command line interface you can set threshold parameters and the output will then be nagios plugin-like, so that you will be able to integrate it with your nagios-compatible monitoring system. Remember that with great power there must also come great responsibility :) be awared again that you may induce heavy load and you could affect the performance of your systems.

# Platforms
It has been tested on Ubuntu Linux 16.04, but surely is portable to other platforms because just uses POSIX API, liboping and libcurl.

# Disclaimer
Let me add a logical notice that I release myself from responsibility for any problem in the performance of your applications that it could lead.

# Source
It's source can be found at GitHub: [https://github.com/zoquero/simplebenchmark/](https://github.com/zoquero/simplebenchmark/).

I hope you'll find it useful.

