#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "decode.h"

void argv_to_bytes(byte_t bytes[], const char *argv[], int argc) {
  for (int i = 1; i < argc; i++) {
    bytes[i - 1] = (byte_t) strtol(argv[i], NULL, 16);
  }
}

int main(int argc, const char *argv[]) {
  byte_t bytes[argc - 1];
  argv_to_bytes(bytes, argv, argc);

  instr_t list[2048];

  // Decode all bblocks
  int count = decode(list, bytes, argc - 1);

  // Xrefs
  proc_labels(list, count);

  // Print
  for (int i = 0; i < count; i++) {
    if (list[i].label[0] != '\0')
      printf(".%s:\n", list[i].label);

    printf("   0x%x:   %-20s    %-5s %-20s %s%s\n",
            list[i].addr,
            list[i].hex_bytes,
            list[i].mnemo_opcode,
            list[i].mnemo_operand,
            list[i].mnemo_notes[0] != '\0' ? "# " : "",
            list[i].mnemo_notes);
  }

  return 0;
}
