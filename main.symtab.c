#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <elf.h>

#include "elf.h"

char get_symbol_type(symbol_t sym) {
  if (sym.shndx == 0) {
    if (sym.binding == STB_WEAK)
      return 'w';
    if (sym.binding == STB_GLOBAL)
      return 'U';
    else
      return 'u';
  } else {
    if (sym.binding == STB_WEAK)
      return 'W';
    if (sym.binding == STB_GLOBAL)
      return 'T';
    else
      return 't';
  }

  return '?';
}

int main(int argc, const char *argv[]) {
  if (argc != 2) {
    printf("Usage: symtab <filename>\n");
    return 1;
  }

  int fd, fsize;
  byte_t *bin;
  if (load_file(argv[1], &fd, &bin, &fsize)) {
    return 1;
  }

  // Is .elf?
  if (bin[0] != 0x7F || bin[1] != 'E' || bin[2] != 'L' || bin[3] != 'F') {
    printf("File is not an .elf file!\n");
    goto ERR_CLOSE_FILE;
  }

  // Parse section headers
  section_t sections[MAX_SECTIONS];
  int n_sections;
  if (get_elf_sections(bin, sections, &n_sections))
    goto ERR_CLOSE_FILE;

  // Parse symtab
  symbol_t symbols[MAX_SYMBOLS];
  int n_symbols;
  if (get_elf_symtab(bin, sections, n_sections, symbols, &n_symbols))
    goto ERR_CLOSE_FILE;

  // Print
  for (int i = 0; i < n_symbols; i++) {
    if (symbols[i].type != STT_FUNC && symbols[i].type != STT_NOTYPE)
      continue;

    if (symbols[i].value == 0) {
      printf("%-16s %c %s\n", " ", get_symbol_type(symbols[i]), symbols[i].name);
    } else {
      printf("%016lx %c %s\n", symbols[i].value, get_symbol_type(symbols[i]), symbols[i].name);
    }
  }

ERR_CLOSE_FILE:
  close_file(bin, fd, fsize);

  return 0;
}
