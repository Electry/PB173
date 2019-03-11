CC=gcc
CFLAGS=-Wall

all: decode
default: decode

decode.o: decode.c decode.h
	$(CC) -c decode.c

decode: decode.o
	$(CC) decode.o -o decode

clean:
	rm decode *.o
