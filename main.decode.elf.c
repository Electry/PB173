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

int main(int argc, const char *argv[]) {
  if (argc != 2) {
    printf("Usage: decode.elf <filename>\n");
    return 1;
  }

  int fd, fsize;
  byte_t *bin;
  if (load_file(argv[1], &fd, &bin, &fsize)) {
    return 1;
  }

  section_t sections[64];
  uintptr_t entry, offset, vaddr;
  int size, n_sections;
  bool is_elf = false;

  // Is .elf?
  if (bin[0] == 0x7F && bin[1] == 'E' && bin[2] == 'L' && bin[3] == 'F') {
    is_elf = true;
    if (get_elf_info(bin, &entry, &offset, &vaddr, &size, sections, &n_sections)) {
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

  // Print
  for (int i = 0; i < count; i++) {
    if (list[i].label[0] != '\0')
    printf(".%s:\n", list[i].label);

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
