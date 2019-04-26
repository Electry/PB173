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


typedef struct {
  uintptr_t addr;
  int size;
  char *name;
} section_t;

int get_elf_info(byte_t *bin, uintptr_t *entry, uintptr_t *text_offset, uintptr_t *vaddr, int *size, section_t sections[], int *n_sections);

int load_file(const char *path, int *fd, byte_t **bin, int *fsize);
int close_file(byte_t *bin, int fd, int fsize);

void proc_section_labels(instr_t instr[], int count, section_t sections[], int n_sections);

#endif
