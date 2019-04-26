#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "decode.h"

static void argv_to_bytes(byte_t bytes[], const char *argv[], int argc) {
  for (int i = 1; i < argc; i++) {
    bytes[i - 1] = (byte_t) strtol(argv[i], NULL, 16);
  }
}

int main(int argc, const char *argv[]) {
  byte_t bytes[2048]; // 2kB max
  instr_t list[2048];
  size_t size = 0;

  if (argc > 1) { // argv
      argv_to_bytes(bytes, argv, argc);
      size = argc - 1;
  } else { // stdin
      freopen(NULL, "rb", stdin);
      size = fread(bytes, sizeof(byte_t), 2048, stdin);
  }

  // Decode all bblocks
  int count = decode(list, bytes, 0, size);

  // Xrefs
  proc_flow_labels(list, count);

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

  return 0;
}
