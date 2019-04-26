CC=gcc
CFLAGS=-Wall

all: decode cfg decode.elf cfg.elf
default: decode


decode.o: decode.c decode.h
	$(CC) $(CFLAGS) -c decode.c

cfg.o: cfg.c cfg.h
	$(CC) $(CFLAGS) -c cfg.c

elf.o: elf.c elf.h
	$(CC) $(CFLAGS) -c elf.c


decode: decode.o
	$(CC) $(CFLAGS) main.decode.c decode.o -o decode

cfg: decode.o cfg.o
	$(CC) $(CFLAGS) main.cfg.c decode.o cfg.o -o cfg

decode.elf: decode.o elf.o
	$(CC) $(CFLAGS) main.decode.elf.c decode.o elf.o -o decode.elf

cfg.elf: decode.o cfg.o elf.o
	$(CC) $(CFLAGS) main.cfg.elf.c decode.o cfg.o elf.o -o cfg.elf


clean:
	rm decode cfg decode.elf cfg.elf *.o
