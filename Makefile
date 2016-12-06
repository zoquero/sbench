# Makefile for sbench
#
# To build with    OPING:   (Ubuntu)
# $ make all OPING_ENABLED=y
#
# To build without OPING:   (SLES)
# $ make all
#

ifdef OPING_ENABLED
PING_ENABLE_COMPILE=-DOPING_ENABLED
PING_ENABLE_LINK=-loping -DOPING_ENABLED
endif

CC=gcc
# c99 vs gnu99: c99 generates warnings with usleep, gnu99 doesn't
CFLAGS=-Wall -pedantic -std=gnu99 $(PING_ENABLE_COMPILE)
#
# libcurl: to compile and test in Ubuntu I insstalled libcurl4-openssl-dev libcurl3
#          SLES doesn't provide libcurl 
#
LDFLAGS=-lm -lcurl -lpthread -std=gnu99 $(PING_ENABLE_LINK)
EXECUTABLE=sbench

all: $(EXECUTABLE)

$(EXECUTABLE): sbench.o
	$(CC) -o $(EXECUTABLE) sbenchfuncs.c sbench.c $(LDFLAGS)

clean:
	rm *.o $(EXECUTABLE)

install:
	mkdir -p $(DESTDIR)
	install -m 0755 $(EXECUTABLE) $(DESTDIR)

#	It doesn't work : "/bin/sh: -c: line 1: syntax error: unexpected end of file"
#	if [ ! -z "$(MANDIR)" -a -d "$(MANDIR)"  ]; then
#	  install -m 0644 doc/sbench.1.gz $(MANDIR)
#	fi
	install -m 0644 doc/sbench.1.gz $(MANDIR)

#	Must work on it, SLES works this way:
#	# install -m 0644 doc/sbench.1.gz $(MANDIR)
#	but ubuntu doesn't. Already tried this:
#	# install -m 0644 doc/sbench.1.gz /usr/share/man/man1/
#       but then it fails on ubuntu: "install: cannot create regular file `/usr/share/man/man1/sbench.1.gz': Permission denied"
