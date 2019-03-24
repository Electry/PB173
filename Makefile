CC=gcc
CFLAGS=-Wall

all: decode
default: decode


main.o: main.c decode.h
	$(CC) $(CFLAGS) -c main.c

decode.o: decode.c decode.h
	$(CC) $(CFLAGS) -c decode.c

cfg.o: cfg.c decode.h
	$(CC) $(CFLAGS) -c cfg.c


decode: main.o decode.o
	$(CC) $(CFLAGS) main.o decode.o -o decode

cfg: cfg.o decode.o
	$(CC) $(CFLAGS) cfg.o decode.o -o cfg


clean:
	rm decode *.o
