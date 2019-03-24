CC=gcc
CFLAGS=-Wall

all: decode
default: decode

main.o: main.c decode.h
	$(CC) $(CFLAGS) -c main.c

decode.o: decode.c decode.h
	$(CC) $(CFLAGS) -c decode.c

decode: main.o decode.o
	$(CC) $(CFLAGS) main.o decode.o -o decode

clean:
	rm decode *.o
