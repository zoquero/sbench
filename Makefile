# Makefile for sbench
CC=gcc
## c99 generates warnings with usleep, gnu99 doesn't
CFLAGS=-Wall -pedantic -std=gnu99
LDFLAGS=-lm
EXECUTABLE=sbench

all: $(EXECUTABLE)

$(EXECUTABLE): sbench.o
	$(CC) -o $(EXECUTABLE) sbench.c $(LDFLAGS)

clean:
	rm *.o $(EXECUTABLE)
