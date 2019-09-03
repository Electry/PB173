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
#include "decode.h"

int compare_instr_vaddr(const void *a, const void *b) {
  return ((instr_t *)a)->addr - ((instr_t *)b)->addr;
}

int main(int argc, const char *argv[]) {
  if (argc != 2) {
    printf("Usage: recfun <filename>\n");
    return 1;
  }

  int fd, fsize;
  byte_t *bin;
  if (load_file(argv[1], &fd, &bin, &fsize)) {
    return 1;
  }

  section_t sections[MAX_SECTIONS];
  symbol_t symbols[MAX_SYMBOLS];
  uintptr_t entry, offset, vaddr;
  int size, n_sections, n_symbols;

  // Is .elf?
  if (bin[0] != 0x7F || bin[1] != 'E' || bin[2] != 'L' || bin[3] != 'F') {
    goto ERR_CLOSE_FILE;
  }

  // Get .elf info, parse symtab
  if (get_elf_sections(bin, sections, &n_sections)
      || get_elf_info(bin, sections, n_sections, &entry, &vaddr, &offset, &size)
      || get_elf_symtab(bin, sections, n_sections, symbols, &n_symbols)) {
    goto ERR_CLOSE_FILE;
  }

  // Make room for decoded instr_t
  instr_t *list = (instr_t *)malloc(size * sizeof(instr_t));
  if (list == NULL) {
    printf("Could not allocate memory for decoder!\n");
    goto ERR_CLOSE_FILE;
  }

  // Decode all bblocks
  int count = decode(list, 0, bin + offset + (entry - vaddr), entry, size, DECODE_RECURSIVE, 0);

  // Sort by vaddr
  //printf("Total count = %d\n", count);
  qsort(list, count, sizeof(instr_t), compare_instr_vaddr);

  // Xrefs
  proc_symtab_labels(list, count, symbols, n_symbols);
  proc_flow_labels(list, count);
  proc_section_labels(list, count, sections, n_sections);

  // Print
  for (int i = 0; i < count; i++) {
    if (list[i].label[0] != '\0')
    printf("%s:\n", list[i].label);

    char addr_buf[16];
    snprintf(addr_buf, 16, "0x%x", list[i].addr);

    printf("   %s:%*s %-20s    %-5s %-20s %s%s\n",
            addr_buf,
            (int)(7 - strlen(addr_buf)), "", // padding
            list[i].hex_bytes,
            list[i].mnemo_opcode,
            list[i].mnemo_operand,
            list[i].mnemo_notes[0] != '\0' ? "# " : "",
            list[i].mnemo_notes);
  }

  // Unmap file & free mem
  free(list);
ERR_CLOSE_FILE:
  close_file(bin, fd, fsize);

  return 0;
}
