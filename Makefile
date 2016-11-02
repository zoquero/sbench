# Makefile for sbench
CC=gcc
## c99 vs gnu99: c99 generates warnings with usleep, gnu99 doesn't
CFLAGS=-Wall -pedantic -std=gnu99
## libcurl:      to compile and test in Ubuntu I insstalled libcurl4-openssl-dev libcurl3
LDFLAGS=-lm -lcurl -loping -lpthread
EXECUTABLE=sbench

all: $(EXECUTABLE)

$(EXECUTABLE): sbench.o
	$(CC) -o $(EXECUTABLE) sbench.c $(LDFLAGS)

clean:
	rm *.o $(EXECUTABLE)
