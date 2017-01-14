# Summary

Simple, tunable and lightweight benchmarking tool written in C for testing infrastructure (CPU, memory, disk and network).

Angel Galindo ( zoquero@gmail.com ), november of 2016

![Illustration](https://github.com/zoquero/sbench/blob/master/aux/logo.200x260.png)

# Features

It measures system's performnce by testing:

* Memory:
    * allocate, commit and set
* CPU:
    * multi-threaded floating-point operations (simply sums, substractions, powers and divisions)
* Disk (well... filesystem):
    * Sequential read
    * Sequential write
    * Multi-threaded random write
* Network:
    * Latency: RTT by ICMP echo request 
    * Packet loss: by ICMP echo request 
    * Throughput: HTTP GET

# Motivation

I (the author) just need some simple, easily understandable, tunable and reproducible tests to measure how my IaaS is performing. You can find nice suites for benchmarking like [hardinfo](https://github.com/lpereira/hardinfo/) or [Phoronix Test Suite](http://www.phoronix-test-suite.com/) but they are complex, they have some dependencies, sometimes they are difficult to be used and it's difficult to imagine what will be their exact impact on the IaaS when launching tests.

An example of the simplicity of this tool (sbench) are its CPU tests: it just performs some simple floating-point operations that any CPU can do (sums, substractions, powers and divisions). The only thing I need is having the CPU doing a concrete amount of calculus. If you are looking for complete CPU tests to benchmark it's instruction set then you may be looking for something like [SPEC CPU Benchmark](https://www.spec.org/cpu/).

These simple benchmarks can be used intensively on farm for stresstesting (it will impact on all system's performance) but they can also be used softly just as a measurement, a charaterization of your IaaS. Take a look at [bgrunner](https://github.com/zoquero/bgrunner) for this purpose.

Ok, but *why can I need it*? For example as an enablement for **troubleshooting** if you are **moving** your virtual machines from your own IaaS **to a public cloud**. When you manage the IaaS under your VMs on your private cloud you always can read performance indicators on your hypervisors, your network appliances, your SAN and your storage arrays (like for example *CPU Ready*, *Co-Stop*, balooning, hypervisor swapping or network and storage latencies) that you will not be able to get when running on a public cloud.

## Baseline

You can use it to extract a simple baseline of your systems (on idle!), so that you if later you have performance problems on your *pets* you will be able to (stop services first! sig) extract it again to realize if it's your application or the IaaS who is inducing the problem.

## Use wisely on production environments

You can simply use it on your production environments... but can you assert that you have an idle CPU, hundreds of MB of unused RAM, some spare network bandwith and IO channels? If your tests require resources that are busy then it not only will harm your applications but also your measurement will be untrue.

In other hand, testing one of your systems can affect negatively on other systems' performance. Fortunately you can adjust finely the tests.

# Command line interface 

It's very easy to be used. You'll find help:
* running the executable without arguments or adding "`-h`".
* `man 1 sbench`

`$ sbench -h`

`Simple, tunable and lightweight benchmarking tool for testing infrastructure`

`Usage:`

`sbench (-v) (-r) -t cpu        (-w warnThreshold -c critThreshold) -p <times>`

`sbench (-v) (-r) -t cpu        (-w warnThreshold -c critThreshold) -p <times,numThreads>`

`sbench (-v) (-r) -t mem        (-w warnThreshold -c critThreshold) -p <times,sizeInBytes>`

`sbench (-v) (-r) -t disk_w     (-w warnThreshold -c critThreshold) -p <times,sizeInBytes,folderName>`

`sbench (-v) (-r) -t disk_w     (-w warnThreshold -c critThreshold) -p <times,sizeInBytes,numThreads,folderName>`

`sbench (-v) (-r) -t disk_r_seq (-w warnThreshold -c critThreshold) -p <times,sizeInBytes,fileName>`

`sbench (-v) (-r) -t disk_r_ran (-w warnThreshold -c critThreshold) -p <times,sizeInBytes,fileName>`

`sbench (-v) (-r) -t disk_r_ran (-w warnThreshold -c critThreshold) -p <times,sizeInBytes,numThreads,fileName>`

`sbench (-v) (-r) -t ping       (-w latencyWarn_lossWarn -c latencyCrit_lossCrit) -p <times,sizeInBytes,dest>`

`sbench (-v) (-r) -t http_get   (-w warnThreshold -c critThreshold) -p <httpRef,url>`

``

` * -v == verbose:`

` * -r == RealTime:`

``

`Examples:`

`* To allocate&commit 10 MiB of RAM and memset it 10 times`

`      and get a response in nagios plugin-like format:`

`  sbench -t mem -p 10,104857600 -w 0.3 -c 0.5`

``

`* To have 2 threads doing 100E6 flotating point calculus (+-/^):`

`  sbench -t cpu -p 10000000,2`

``

`* To create 4 threads each writing 10 MiB in a file in 4k blocks:`

`  sbench -t disk_w -p 2560,4096,4,/tmp/_sbench.d`

``

`* To read sequentially 100 MiB from a file in 4k blocks:`

`  sbench -t disk_r_seq -p 25600,4096,/tmp/_sbench.testfile`

``

`* To random access read 100 MiB from a file`

`      concurrently by 2 threads reading 4k blocks:`

`  sbench -t disk_r_ran -p 25600,4096,2,/tmp/_sbench.testfile`

``

`* To get the mean round-trip time sending`

`      4 ICMP echo request of 56 bytes to www.gnu.org:`

`  sbench -t ping -p 4,56,www.gnu.org`

``

`* Idem but applying latency warning = 5ms, latency crit = 30ms,`

`      packet loss warning = 1%, packet loss critical = 5%:`

`  sbench -t ping -w 5_1 -c 30_5 -p 4,56,www.gnu.org`

``

`* To download by HTTP GET http://www.test.com/file ,`

`      and to compare it with the reference:`

`      file 'my_ref_file' located at /var/lib/sbench/http_refs :`

`  sbench -t http_get -p my_ref_file,http://www.test.com/file

# Nagios plugin

If you pass warning and critical thresholds to this program, then the output will be nagios plugin-like, so that you will be able to integrate it with your nagios-compatible monitoring system:

Normal output, usefull to output to CSV, for example:

`$ ./sbench -t mem -p 1,104857600`

`0.56 s`

Nagios-like output, usefull to integrate it with your monitoring tool:

`$ ./sbench -t mem -p 1,104857600 -w 0.6 -c 1.2`

`Mem Warning = 0.85 s| time=0.85`

`$ echo $?`

`1`

## NRPE configuration

To be able to launch it through NRPE you must:

* Enable command arguments adding this to your `nrpe.cfg` (such a risky parameter):

`dont_blame_nrpe=1`

* Configure this command, adding this to your `nrpe.cfg`:

`command[check_sbench]=/usr/lib/nagios/plugins/check_sbench -w $ARG1$ -c $ARG2$ -t $ARG3$ -p $ARG4$`

* copy the program to `/usr/lib/nagios/plugins/check_sbench`

* Test it:

`$ /usr/lib/nagios/plugins/check_nrpe -H 127.0.0.1 -c check_sbench -a 0.6 1.2 mem 1,104857600`

`Mem Warning = 0.85 s| time=0.85`


Note: If you get this error:

`CHECK_NRPE: Received 0 bytes from daemon.  Check the remote server logs for error messages.`

and something like this in your syslog:

`Error: Request contained command arguments!`

`Client request was invalid, bailing out...`

then probably your NRPE service has been compiled
without "`--enable-command-args`" and you should rebuild it.
For examle, you can follow up [these instructions](http://sysadmin.compxtreme.ro/nagios-nrpe-server-ignores-dont_blame_nrpe1/).

## Remote Batch execution

The original intention of this project is being able to launch a load test on a IaaS. It's a collection of virtual machines with their resources and the test should be launched simultaneously and remotely by a master, like on a distributed JMeter test. If you have set NRPE configuration then you can use [bgrunner](https://github.com/zoquero/bgrunner) to launch an orchestrated stress test:

![Illustration](https://github.com/zoquero/bgrunner/blob/master/doc/bgrunner.illustration.png)

# RealTime checks

With the "`-r`" parameter you can use RealTime schedulling for your checks. It will make your checks more precise but may impact in your applications (stop services first!) and may need root permisions or capabilities:

To enable RealTime schedulling you will need to run it as root or, if the kernel enables it, you can use **capabilities** like this:

`sudo setcap cap_sys_nice+ep ./sbench`

`sudo setcap cap_ipc_lock+ep ./sbench`

Then you may also want to set permisions like this:

`chown root:nagios ./sbench`

`chmod 0750 ./sbench`

or use "`setfacl`":

`setfacl -m user:nagios:r-x ./sbench`

## Dead-lock guard

If you want to launch RealTime checks then you may want to read about *System wide settings* described in chapter 2.1 of [Real-Time group scheduling](https://www.kernel.org/doc/Documentation/scheduler/sched-rt-group.txt) doc from the Linux kernel. So you may want to do as root something like:

`echo "-1" > /proc/sys/kernel/sched_rt_runtime_us`

it's risky but it will ensure that any non-realtime processes won't steal CPU cycles to your RT tests.

# Build and install

## Quick guide

How to build:
* ( `make clean` )
* `make`

How to install:
* `sudo install -o root -g root -m 0755 ./sbench /usr/bin/`
* `sudo install -o root -g root -m 0644 ./doc/sbench.1.gz /usr/share/man/man1/`

## Open Build Service
If you prefer you can install my already built packages available in [Open Build Service](http://software.opensuse.org/download.html?project=home%3Azoquero%3Asbench&package=sbench).

## Dependencies

It just requires `libcurl`, but if it's a problem then you can easily strip the code removing functions like `sbenchfuncs.c:httpGet`, you will just miss the bandwith througput tests.

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

## Alternate ping with liboping (optional)

It pings running "`ping`" external command, but if you prefer you can use [Octo's ping library](http://noping.cc/) this way:

`make OPING_ENABLED=y`

### Packages for liboping on Ubuntu (optional):

* building: 
    * liboping-dev
* running: 
    * liboping0

### Packages for liboping on SLES|OpenSUSE (optional):

* building: 
    * liboping-devel | liboping-devel-32bit
* running: 
    * liboping0 | liboping0-32bit

# Platforms

It has been tested on *Ubuntu Linux 16.04* on `x86_64` and *SLES 11 SP4* on `x86_32` and `x86_64`, but surely it's portable to other platforms because just it uses POSIX API and libcurl (and optionaly liboping ).

# Disclaimer

I release myself from responsibility for any problem in the performance of your applications that it could lead.

# Source
It's source can be found at GitHub: [https://github.com/zoquero/sbench/](https://github.com/zoquero/sbench/).

I hope you'll find it useful!
/Ángel

