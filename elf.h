#ifndef ELF_H
#define ELF_H

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <elf.h>

#include "decode.h"

#define MAX_SECTIONS 64
#define MAX_SYMBOLS 512

typedef struct {
  uintptr_t elf_offset;
  uintptr_t vaddr;
  int size;
  char *name;
} section_t;

typedef struct {
  uintptr_t value;
  int size;
  byte_t binding;
  char *name;
  uint16_t shndx;
} symbol_t;

int get_elf_sections(byte_t *bin, section_t sections[], int *n_sections);
int get_elf_symtab(byte_t *bin, section_t sections[], int n_sections, symbol_t symbols[], int *n_symbols);
int get_elf_info(byte_t *bin, section_t sections[], int n_sections, uintptr_t *entry, uintptr_t *vaddr, uintptr_t *text_offset, int *text_size);

int load_file(const char *path, int *fd, byte_t **bin, int *fsize);
int close_file(byte_t *bin, int fd, int fsize);

void proc_section_labels(instr_t instr[], int count, section_t sections[], int n_sections);

#endif
