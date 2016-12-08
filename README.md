# Summary

Lightweight program written in C to do simple benchmarks

Angel Galindo ( zoquero@gmail.com ), november of 2016

# Features

It measures how systems perform doing these tests:

* Memory:
    * allocate, commit and set
* CPU:
    * multi-threaded floating-point operations (simply sums, substractions, powers and divisions)
* Disk (filesystem):
    * Sequential read
    * Sequential write
    * Multi-threaded random write
* Network:
    * Latency: RTT by ICMP echo request 
    * Packet loss: by ICMP echo request 
    * Throughput: HTTP GET

# Motivation

I just need some simple, easily understood and reproducible tests to measure how my IaaS is performing. You can find nice suites for benchmarking like [hardinfo](https://github.com/lpereira/hardinfo/) or [Phoronix Test Suite](http://www.phoronix-test-suite.com/) but they are complex, they have some dependencies, sometimes they are difficult to be used and it's difficult to imagine what will be their exact impact on the IaaS when launching tests.

An example of it's simplicity are the CPU tests: it just performs some simple floating-point operations that any CPU can do (sums, substractions, powers and divisions). The only thing I need is having the CPU doing a concrete amount of calculus. If you are looking for a complete CPU tests to test it's Instruction Set then you may be looking for specs like the ones from [SPEC CPU Benchmark](https://www.spec.org/cpu/).

These simple benchmarks can be used intensively on farm for stresstesting (will impact in all system's performance) but they can also be used softly just as a measurement, a charaterization of your IaaS.

Ok, but *why can I need it*? For example for **troubleshooting** if you are **moving** your virtual machines from your own IaaS **to a public cloud**. When you manage the IaaS under your VMs you always can read performance indicators on your hypervisors, your network appliances, your SAN and your storage arrays (like for example *CPU Ready*, *Co-Stop*, balooning, hypervisor swapping or network and storage latencies) that you will not be able to get when running on a public cloud.

Ok, but *I am already using a benchmark suite to test it all*. Nice, go ahead and use that suite. Mmmmmm, but, how long does it takes for you to run those tests? Do you know exactly what are those tests doing? And, can you adjust that suite so that it doesn't induce a heavy load-test that could affect your other systems?

## Baseline

You can use it to extract a simple baseline of your systems (on idle!), so that you if later you have performance problems on your *pets* you will be able to (stop services first! sig) extract it again to realize if it's your application or the IaaS who is inducing the problem.

## Use wisely on production environments

You can simply use it on your production environments... but can you assert that you have an idle CPU, hundreds of MB of unused RAM, some spare network bandwith and IO channels? If your tests require resources that are busy then it not only will harm your applications but also your measurement will be untrue.

In other hand, testing one of your systems can affect negatively on other systems' performance. Fortunately you can adjust finely the tests.

# Command line interface 

It's very easy to be used. You'll find help:
* running the executable without arguments or adding "-h".
* man 1 sbench

## Nagios plugin

Using the command line interface you can set threshold parameters and the output will then be nagios plugin-like, so that you will be able to integrate it with your nagios-compatible monitoring system. Remember that with great power there must also come great responsibility :) be awared again that you may induce heavy load and you could affect the performance of your systems.

# Build and install

## Quick guide

How to build:
* `make`
* copy executable and man page where needed

## Dependencies

It just requires *libcurl*, but if it's a problem then you can easily strip the code removing functions like "*sbenchfuncs.c:httpGet*", you will just miss the bandwith througput tests.

## Packages for dependencies on Ubuntu:

* building: 
    * libcurl4-openssl-dev
* running: 
    * libcurl3
    * libcurl3-gnutls

## Packages for dependencies on SLES|OpenSUSE:

* building: 
    * libcurl-devel
* running: 
    * libcurl4 | libcurl4-32bit

## Alternate ping with liboping

It pings running "`ping`" external command, but if you prefer you can use [Octo's ping library](http://noping.cc/) this way:

`make OPING_ENABLED=y`

### Packages for liboping on Ubuntu:

* building: 
    * liboping-dev
* running: 
    * liboping0

### Packages for liboping on SLES|OpenSUSE:

* building: 
    * liboping-devel | liboping-devel-32bit
* running: 
    * liboping0 | liboping0-32bit

# Platforms

It has been tested on *Ubuntu Linux 16.04* on `x86_64` and *SLES 11 SP4* on x86_32 and x86_64, but surely it's portable to other platforms because just it uses POSIX API and libcurl (and optionaly liboping ).

# Disclaimer

I release myself from responsibility for any problem in the performance of your applications that it could lead.

# Source
It's source can be found at GitHub: [https://github.com/zoquero/simplebenchmark/](https://github.com/zoquero/simplebenchmark/).

I hope you'll find it useful!
/Ángel

