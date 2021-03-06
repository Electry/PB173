#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "decode.h"

int print_bblocks(instr_t list[], int count) {
  int bblock_count = 0;

  for (int i = 0; i < count; i++) {
    if (list[i].label[0] != '\0') {
      if (bblock_count > 0)
        printf("\"]\n"); // Closing
      bblock_count++;
      printf("  %s [width=4 shape=rectangle fontname=Monospace fontsize=11 label=\"",
              list[i].label); // Opening
      printf("%s:\\l", list[i].label); // Label
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

void print_arrows(instr_t list[], int count) {
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
          if (list[i].mnemo_cf_label[0] != '\0') {
            printf("  %s -> %s [fontname=Monospace fontsize=10 label=\"%s\\l\"]\n",
                    list[prev_bblock_i].label,
                    list[i].mnemo_cf_label,
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
