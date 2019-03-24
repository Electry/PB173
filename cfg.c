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

static int print_bblocks(instr_t list[], int count) {
  int bblock_count = 0;

  for (int i = 0; i < count; i++) {
    if (list[i].label[0] != '\0') {
      if (bblock_count > 0)
        printf("\"]\n"); // Closing
      bblock_count++;
      printf("  %s [width=4 shape=rectangle fontname=Monospace fontsize=11 label=\"",
              list[i].label); // Opening
      printf(".%s:\\l", list[i].label); // Label
    }

    char addr_buf[16];
    snprintf(addr_buf, 16, "0x%x", list[i].addr);

    printf("   %s:%*s %-5s %-20s %s%s\\l",
            addr_buf,
            (int)(7 - strlen(addr_buf)), "", // padding
            list[i].mnemo_opcode,
            list[i].mnemo_operand,
            list[i].mnemo_notes[0] != '\0' ? "# " : "",
            list[i].mnemo_notes);
  }

  if (bblock_count > 0)
    printf("\"]\n"); // Closing

  return bblock_count;
}

static void print_arrows(instr_t list[], int count) {
  int prev_bblock_i = 0;

  for (int i = 0; i < count; i++) {
    if (list[i].label[0] != '\0')
      prev_bblock_i = i; // save last bblock beg. index

    // this bblock -> some other bblock
    if (!list[i].has_ext_opcode) {
      switch (list[i].opcode) {
        case OP_JB:
        case OP_JE:
        case OP_JNE:
        case OP_JMP_E9:
        case OP_JMP_EB:
        case OP_CALL:
PRINT_JUMP_ARROW:
          if (list[i].mnemo_notes[0] == '.') {
            // TODO: Don't use formatted mnemo for this
            printf("  %s -> %s [fontname=Monospace fontsize=10 label=\"%s\\l\"]\n",
                    list[prev_bblock_i].label,
                    &list[i].mnemo_notes[1],
                    list[i].mnemo_opcode);
          }
          break;
      }
    } else {
      switch (list[i].opcode) {
        case OP_EXT_JB:
        case OP_EXT_JE:
        case OP_EXT_JNE:
          goto PRINT_JUMP_ARROW;
      }
    }

    // this bblock -> next
    if (i < count - 1
        && list[i + 1].label[0] != '\0'
        && list[i].opcode != OP_JMP_E9    // Ignore prev bblocks ending with JMP/RET,
        && list[i].opcode != OP_JMP_EB    // those can't fall to this bblock
        && list[i].opcode != OP_RET_C2
        && list[i].opcode != OP_RET_C3) {
      printf("  %s -> %s\n", list[prev_bblock_i].label, list[i + 1].label);
    }
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
  int count = decode(list, bytes, size);

  // Xrefs
  proc_labels(list, count);

  // Print graph
  printf("digraph G {\n");
  print_bblocks(list, count);
  print_arrows(list, count);
  printf("}\n");

  return 0;
}
