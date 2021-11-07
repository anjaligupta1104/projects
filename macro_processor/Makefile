CC=gcc
CFLAGS=-g3 -Wall

PROGRAMS=proj1

all: $(PROGRAMS)

stringBuf.o: stringBuf.h

macro.o: macro.h stringBuf.h

proj1.o: proj1.c macro.h stringBuf.h

proj1: proj1.o macro.o stringBuf.o

clean:
	$(RM) *.o $(PROGRAMS)
