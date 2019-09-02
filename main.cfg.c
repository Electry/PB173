#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "decode.h"
#include "cfg.h"

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
  int count = decode(list, 0, bytes, 0, size, DECODE_LINEAR);

  // Xrefs
  proc_flow_labels(list, count);

  // Print graph
  printf("digraph G {\n");
  print_bblocks(list, count);
  print_arrows(list, count);
  printf("}\n");

  return 0;
}
