#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "decode.h"
#include "cfg.h"
#include "elf.h"

int main(int argc, const char *argv[]) {
  if (argc != 2) {
    printf("Usage: cfg.elf <filename>\n");
    return 1;
  }

  int fd, fsize;
  byte_t *bin;
  if (load_file(argv[1], &fd, &bin, &fsize)) {
    return 1;
  }

  section_t sections[MAX_SECTIONS];
  uintptr_t entry, offset, vaddr;
  int size, n_sections;
  bool is_elf = false;

  // Is .elf?
  if (bin[0] == 0x7F && bin[1] == 'E' && bin[2] == 'L' && bin[3] == 'F') {
    is_elf = true;
    if (get_elf_sections(bin, sections, &n_sections)
        || get_elf_info(bin, sections, n_sections, &entry, &vaddr, &offset, &size)) {
      goto ERR_CLOSE_FILE;
    }
  }
  else {
    entry = offset = vaddr = 0;
    size = fsize;
  }

  // Make room for decoded instr_t
  instr_t *list = (instr_t *)malloc(size * sizeof(instr_t));
  if (list == NULL) {
    printf("Could not allocate memory for decoder!\n");
    goto ERR_CLOSE_FILE;
  }

  // Decode all bblocks
  int count = decode(list, bin + offset, vaddr, size);

  // Xrefs
  proc_flow_labels(list, count);
  if (is_elf)
    proc_section_labels(list, count, sections, n_sections);

  // Print graph
  printf("digraph G {\n");
  print_bblocks(list, count);
  print_arrows(list, count);
  printf("}\n");

  // Unmap file & free mem
  free(list);
ERR_CLOSE_FILE:
  close_file(bin, fd, fsize);

  return 0;
}
