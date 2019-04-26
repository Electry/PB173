#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

#include "decode.h"

// GPR mnemonics, index = encoding
static const char * const GPR_64b[] = {
  "rax", "rcx", "rdx", "rbx",
  "rsp", "rbp", "rsi", "rdi",
  "r8",  "r9",  "r10", "r11",
  "r12", "r13", "r14", "r15"
};
static const char * const GPR_32b[] = {
  "eax", "ecx", "edx", "ebx",
  "esp", "ebp", "esi", "edi",
  "r8d",  "r9d",  "r10d", "r11d",
  "r12d", "r13d", "r14d", "r15d"
};

/**
 * Decodes immediate signed value from sequence of bytes
 */
static long long dec_imm(byte_t bytes[], int *pos, int imm_size) {
  long long value = 0;

  switch (imm_size) {
    case 32:
      value = (int32_t)(bytes[*pos] | bytes[*pos + 1] << 8 |
                        bytes[*pos + 2] << 16 | bytes[*pos + 3] << 24);
      (*pos) += 4;
      break;
    case 16:
      value = (int16_t)(bytes[*pos] | bytes[*pos + 1] << 8);
      (*pos) += 2;
      break;
    case 8:
      value = (int8_t)(bytes[*pos]);
      (*pos)++;
      break;
    default:
      break;
  }

  return value;
}

/**
 * Decodes ModRM fields from byte
 */
static bool dec_modrm(byte_t bytes[], int *pos, instr_t *instr) {
  instr->modrm.mod = (bytes[*pos] & 0b11000000) >> 6;
  instr->modrm.reg = (bytes[*pos] & 0b00111000) >> 3;
  instr->modrm.rm  = (bytes[*pos] & 0b00000111);
  (*pos)++;
  return true;
}

/**
 * Checks if next byte is REX byte
 *  If yes, decodes rex fields
 */
static bool dec_rex(byte_t bytes[], int *pos, instr_t *instr) {
  if (bytes[*pos] < 0x40 || bytes[*pos] > 0x4F)
    return false;

  instr->rex.w = bytes[*pos] & 0b1000;
  instr->rex.r = bytes[*pos] & 0b0100;
  instr->rex.x = bytes[*pos] & 0b0010;
  instr->rex.b = bytes[*pos] & 0b0001;
  (*pos)++;
  return true;
}

/**
 * Checks if next byte is escape opcode (0x0F)
 */
static bool dec_twobyte_opcode(byte_t bytes[], int *pos) {
  if (bytes[*pos] == 0x0F) {
    (*pos)++;
    return true;
  }

  return false;
}

/**
 * Returns register mnemonic from ModRM.rm field
 *  REX.b is used as 4th bit
 *  If default_64b is set to false, 32b mnemonics are used unless REX.w is 1
 */
static const char *get_modrm_rm_register(instr_t *instr, bool default_64b) {
  if (instr->modrm.mod == 0b00 && instr->modrm.rm == 0b101) {
    return "rip"; // disp32(%rip)
  }
  byte_t enc = instr->modrm.rm | (instr->has_rex && instr->rex.b ? 0b1000 : 0);
  return default_64b ? GPR_64b[enc] :
            (instr->has_rex && instr->rex.w ? GPR_64b[enc] : GPR_32b[enc]);
}

/**
 * Returns register mnemonic from ModRM.reg field
 *  REX.r is used as 4th bit
 */
static const char *get_modrm_reg_register(instr_t *instr, bool default_64b) {
  byte_t enc = instr->modrm.reg | (instr->has_rex && instr->rex.r ? 0b1000 : 0);
  return default_64b ? GPR_64b[enc] :
            (instr->has_rex && instr->rex.w ? GPR_64b[enc] : GPR_32b[enc]);
}

/**
 * Returns register mnemonic from opcode register bits
 *  REX.b is used as 4th bit
 */
static const char *get_opcode_register(instr_t *instr) {
  return GPR_64b[(instr->opcode & 0b111) |
            ((instr->has_rex && instr->rex.b) ? 0b1000 : 0)];
}

/**
 * Returns expected displacement size in bits based on ModRM byte
 *
 * ModRM.mod =
 *  0b00 : no displacement (disp32 if r/m is 101 => disp32(%rip))
 *  0b01 : disp8
 *  0b10 : disp32
 */
static int get_modrm_disp(instr_t *instr) {
  switch (instr->modrm.mod) {
    case 0b00: return instr->modrm.rm == 0b101 ? 32 : 0;
    case 0b01: return 8;
    case 0b10: return 32;
    case 0b11: break; // invalid / direct
  }
  return 0;
}

static void format_mnemonic(instr_t *instr, const char *opcode, const char *format, ...) {
  va_list args;
  va_start(args, format);
  char tmp[128];

  vsnprintf(tmp, 128, format, args);

  snprintf(instr->mnemo_opcode, MNEMO_OPCODE_LEN, "%s", opcode);
  snprintf(instr->mnemo_operand, MNEMO_OPERAND_LEN, "%s", tmp);

  va_end(args);
}

/**
 * Decodes single instruction, starting at bytes[0]
 *  return => number of bytes decoded
 */
static int decode_single(instr_t *instr, byte_t bytes[]) {
  int pos = 0;

  // Check REX byte
  instr->has_rex = dec_rex(bytes, &pos, instr);

  // Check 0F extended opcode
  instr->has_ext_opcode = dec_twobyte_opcode(bytes, &pos);

  // Check opcode
  instr->opcode = bytes[pos++];

  if (!instr->has_ext_opcode) {
    switch (instr->opcode) {
      case OP_ADD: // add eax/rax imm32
        instr->value = dec_imm(bytes, &pos, 32);
        format_mnemonic(instr, "add",
                        instr->rex.w ? "$0x%llx, %%rax" : "$0x%x, %%eax",
                        instr->rex.w ? instr->value : (int)instr->value);
        break;
      case OP_XOR: // xor eax/rax imm32
        instr->value = dec_imm(bytes, &pos, 32);
        format_mnemonic(instr, "xor",
                        instr->rex.w ? "$0x%llx, %%rax" : "$0x%x, %%eax",
                        instr->rex.w ? instr->value : (int)instr->value);
        break;
      case OP_CMP_39: // cmp reg/mem64,reg64
        instr->has_modrm = dec_modrm(bytes, &pos, instr);
        if (instr->modrm.mod == 0b11) {
          format_mnemonic(instr, "cmp", "%%%s, %%%s",
                          get_modrm_reg_register(instr, false),
                          get_modrm_rm_register(instr, false));
        } else {
          instr->value = dec_imm(bytes, &pos, get_modrm_disp(instr));
          format_mnemonic(instr, "cmp", "%%%s, %s0x%x(%%%s)",
                          get_modrm_reg_register(instr, false),
                          (instr->value < 0 ? "-" : ""),
                          (instr->value < 0 ? -instr->value : instr->value),
                          get_modrm_rm_register(instr, true));
        }
        break;
      case OP_CMP_3D: // cmp eax/rax imm32
        instr->value = dec_imm(bytes, &pos, 32);
        format_mnemonic(instr, "cmp",
                        instr->rex.w ? "$0x%llx, %%rax" : "$0x%x, %%eax",
                        instr->rex.w ? instr->value : (int)instr->value);
        break;
      case OP_PUSH_50:
      case OP_PUSH_51:
      case OP_PUSH_52:
      case OP_PUSH_53:
      case OP_PUSH_54:
      case OP_PUSH_55:
      case OP_PUSH_56:
      case OP_PUSH_57: // push reg64
        format_mnemonic(instr, "push", "%%%s", get_opcode_register(instr));
        break;
      case OP_POP_58:
      case OP_POP_59:
      case OP_POP_5A:
      case OP_POP_5B:
      case OP_POP_5C:
      case OP_POP_5D:
      case OP_POP_5E:
      case OP_POP_5F: // pop reg64
        format_mnemonic(instr, "pop", "%%%s", get_opcode_register(instr));
        break;
      case OP_PUSH_68: // push imm64 (sign-extended 32b imm)
        instr->value = dec_imm(bytes, &pos, 32);
        format_mnemonic(instr, "push", "$0x%llx", instr->value);
        break;
      case OP_PUSH_6A: // push imm8
        instr->value = dec_imm(bytes, &pos, 8);
        format_mnemonic(instr, "push", "$0x%llx", instr->value);
        break;
      case OP_JB: // jb rel8off
        instr->value = dec_imm(bytes, &pos, 8);
        format_mnemonic(instr, "jb", "$rip%s0x%x",
                        (instr->value < 0 ? "-" : "+"),
                        (instr->value < 0 ? -instr->value : instr->value));
        break;
      case OP_JE: // je rel8off
        instr->value = dec_imm(bytes, &pos, 8);
        format_mnemonic(instr, "je", "$rip%s0x%x",
                        (instr->value < 0 ? "-" : "+"),
                        (instr->value < 0 ? -instr->value : instr->value));
        break;
      case OP_JNE: // jne rel8off
        instr->value = dec_imm(bytes, &pos, 8);
        format_mnemonic(instr, "jne", "$rip%s0x%x",
                        (instr->value < 0 ? "-" : "+"),
                        (instr->value < 0 ? -instr->value : instr->value));
        break;
      case OP_MOV_89: // mov reg/mem64,reg64
        instr->has_modrm = dec_modrm(bytes, &pos, instr);
        if (instr->modrm.mod == 0b11) {
          format_mnemonic(instr, "mov", "%%%s, %%%s",
                          get_modrm_reg_register(instr, false),
                          get_modrm_rm_register(instr, false));
        } else {
          instr->value = dec_imm(bytes, &pos, get_modrm_disp(instr));
          format_mnemonic(instr, "mov", "%%%s, %s0x%x(%%%s)",
                          get_modrm_reg_register(instr, false),
                          (instr->value < 0 ? "-" : ""),
                          (instr->value < 0 ? -instr->value : instr->value),
                          get_modrm_rm_register(instr, true));
        }
        break;
      case OP_MOV_8B: // mov reg64,reg/mem64
        instr->has_modrm = dec_modrm(bytes, &pos, instr);
        if (instr->modrm.mod == 0b11) {
          format_mnemonic(instr, "mov", "%%%s, %%%s",
                          get_modrm_rm_register(instr, false),
                          get_modrm_reg_register(instr, false));
        } else {
          instr->value = dec_imm(bytes, &pos, get_modrm_disp(instr));
          format_mnemonic(instr, "mov", "%s0x%x(%%%s), %%%s",
                          (instr->value < 0 ? "-" : ""),
                          (instr->value < 0 ? -instr->value : instr->value),
                          get_modrm_rm_register(instr, true),
                          get_modrm_reg_register(instr, false));
        }
        break;
      case OP_8F:
        instr->has_modrm = dec_modrm(bytes, &pos, instr);
        switch (instr->modrm.reg) {
          case 0: // pop reg/mem64
            if (instr->modrm.mod == 0b11) {
              format_mnemonic(instr, "pop", "%%%s", get_modrm_rm_register(instr, true));
            } else {
              instr->value = dec_imm(bytes, &pos, get_modrm_disp(instr));
              format_mnemonic(instr, "pop", "%s0x%x(%%%s)",
                              (instr->value < 0 ? "-" : ""),
                              (instr->value < 0 ? -instr->value : instr->value),
                              get_modrm_rm_register(instr, true));
            }
            break;
          default:
            goto UNK_OPCODE;
        }
        break;
      case OP_NOP: // nop
        format_mnemonic(instr, "nop", "");
        break;
      case OP_RET_C2: // ret imm16 (ALWAYS >=0)
        instr->value = dec_imm(bytes, &pos, 16);
        format_mnemonic(instr, "ret", "$0x%hx", (short)instr->value);
        break;
      case OP_RET_C3: // ret
        format_mnemonic(instr, "ret", "");
        break;
      case OP_INT3: // int 3
        format_mnemonic(instr, "int 3", "");
        break;
      case OP_CALL: // call rel32off
        instr->value = dec_imm(bytes, &pos, 32);
        format_mnemonic(instr, "call", "$rip%s0x%x",
                              (instr->value < 0 ? "-" : "+"),
                              (instr->value < 0 ? -instr->value : instr->value));
        break;
      case OP_JMP_E9: // jmp rel32off
        instr->value = dec_imm(bytes, &pos, 32);
        format_mnemonic(instr, "jmp", "$rip%s0x%x",
                              (instr->value < 0 ? "-" : "+"),
                              (instr->value < 0 ? -instr->value : instr->value));
        break;
      case OP_JMP_EB: // jmp rel8off
        instr->value = dec_imm(bytes, &pos, 8);
        format_mnemonic(instr, "jmp", "$rip%s0x%x",
                              (instr->value < 0 ? "-" : "+"),
                              (instr->value < 0 ? -instr->value : instr->value));
        break;
      case OP_F7:
        instr->has_modrm = dec_modrm(bytes, &pos, instr);
        switch (instr->modrm.reg) {
          case 4: // mul reg/mem64
            if (instr->modrm.mod == 0b11) {
              format_mnemonic(instr, "mul", "%%%s", get_modrm_rm_register(instr, false));
            } else {
              instr->value = dec_imm(bytes, &pos, get_modrm_disp(instr));
              format_mnemonic(instr, "mul", "%s0x%x(%%%s)",
                              (instr->value < 0 ? "-" : ""),
                              (instr->value < 0 ? -instr->value : instr->value),
                              get_modrm_rm_register(instr, true));
            }
            break;
          default:
            goto UNK_OPCODE;
        }
        break;
      case OP_FF:
        instr->has_modrm = dec_modrm(bytes, &pos, instr);
        switch (instr->modrm.reg) {
          case 2: // call reg/mem64
            if (instr->modrm.mod == 0b11) {
              format_mnemonic(instr, "call", "*%%%s", get_modrm_rm_register(instr, true));
            } else {
              instr->value = dec_imm(bytes, &pos, get_modrm_disp(instr));
              format_mnemonic(instr, "call", "*%s0x%x(%%%s)",
                              (instr->value < 0 ? "-" : ""),
                              (instr->value < 0 ? -instr->value : instr->value),
                              get_modrm_rm_register(instr, true));
            }
            break;
          case 6: // push reg/mem64
            if (instr->modrm.mod == 0b11) {
              format_mnemonic(instr, "push", "%%%s", get_modrm_rm_register(instr, true));
            } else {
              instr->value = dec_imm(bytes, &pos, get_modrm_disp(instr));
              format_mnemonic(instr, "push", "%s0x%x(%%%s)",
                              (instr->value < 0 ? "-" : ""),
                              (instr->value < 0 ? -instr->value : instr->value),
                              get_modrm_rm_register(instr, true));
            }
            break;
          default:
            goto UNK_OPCODE;
        }
        break;
      default:
        goto UNK_OPCODE;
    }
  } else {
    switch (instr->opcode) {
      case OP_EXT_JB: // jb rel32off
        instr->value = dec_imm(bytes, &pos, 32);
        format_mnemonic(instr, "jb", "$rip%s0x%x",
                          (instr->value < 0 ? "-" : "+"),
                          (instr->value < 0 ? -instr->value : instr->value));
        break;
      case OP_EXT_JE: // je/jz rel32off
        instr->value = dec_imm(bytes, &pos, 32);
        format_mnemonic(instr, "je", "$rip%s0x%x",
                          (instr->value < 0 ? "-" : "+"),
                          (instr->value < 0 ? -instr->value : instr->value));
        break;
      case OP_EXT_JNE: // jne/jnz rel32off
        instr->value = dec_imm(bytes, &pos, 32);
        format_mnemonic(instr, "jne", "$rip%s0x%x",
                          (instr->value < 0 ? "-" : "+"),
                          (instr->value < 0 ? -instr->value : instr->value));
        break;
      case OP_EXT_NOP: // nop reg/mem64
        instr->has_modrm = dec_modrm(bytes, &pos, instr);
        if (instr->modrm.mod == 0b11) {
          format_mnemonic(instr, "nop", "%%%s", get_modrm_rm_register(instr, false));
        } else {
          instr->value = dec_imm(bytes, &pos, get_modrm_disp(instr));
          format_mnemonic(instr, "nop", "%s0x%x(%%%s)",
                          (instr->value < 0 ? "-" : ""),
                          (instr->value < 0 ? -instr->value : instr->value),
                          get_modrm_rm_register(instr, true));
        }
        break;
      case OP_EXT_PUSH_FS: // push fs
        format_mnemonic(instr, "push", "%%fs");
        break;
      case OP_EXT_POP_FS: // pop fs
        format_mnemonic(instr, "pop", "%%fs");
        break;
      case OP_EXT_PUSH_GS: // push gs
        format_mnemonic(instr, "push", "%%gs");
        break;
      case OP_EXT_POP_GS: // pop gs
        format_mnemonic(instr, "pop", "%%gs");
        break;
      default:
        goto UNK_OPCODE;
    }
  }

  return pos;

UNK_OPCODE:
  format_mnemonic(instr, "unknown", "opcode");
  return pos; // skip bytes
}

/**
 * Decodes all instructions
 *  return => num of instr. decoded
 */
int decode(instr_t instr[], byte_t bytes[], int vaddr, int len) {
  int count = 0, pos = 0;
  bool label_pending = true;
  int label_count = 0;

  // Decode all, one by one
  while (pos < len) {
    memset(&instr[count], 0, sizeof(instr_t));

    // Decode single instr.
    instr[count].addr = vaddr + pos; // store begin rel. addr
    pos += decode_single(&instr[count], &bytes[pos]); // decode
    instr[count].len = (vaddr + pos) - instr[count].addr; // store byte size

    // Set hex bytes string
    int str_pos = 0;
    for (int i = instr[count].addr; i < pos; i++) {
      str_pos += snprintf(instr[count].hex_bytes + str_pos,
                          HEX_BYTES_LEN - str_pos, "%s%02x",
                          (i > instr[count].addr ? " " : ""), bytes[i]);
    }

    // Set label to new block
    if (label_pending) {
      snprintf(instr[count].label, LABEL_LEN, "L%d", label_count++);
      label_pending = false;
    }

    // Mark end of block, if any
    if (!instr[count].has_ext_opcode) {
      switch (instr[count].opcode) {
        case OP_JB:
        case OP_JE:
        case OP_JNE:
        case OP_JMP_EB:
        case OP_JMP_E9:
        case OP_RET_C2:
        case OP_RET_C3:
          label_pending = true;
          break;
      }
    } else {
      switch (instr[count].opcode) {
        case OP_EXT_JB:
        case OP_EXT_JE:
        case OP_EXT_JNE:
          label_pending = true;
          break;
      }
    }

    count++;
  }

  return count;
}

void proc_flow_labels(instr_t instr[], int count) {
  int new_label_count = 0;

  for (int i = 0; i < count; i++) {
    bool done = false;
    long long dest = 0;

    // Should print note?
    if (!instr[i].has_ext_opcode) {
      switch (instr[i].opcode) {
        case OP_JB:
        case OP_JE:
        case OP_JNE:
        case OP_JMP_EB: // rel8off
        case OP_JMP_E9: // rel32off
        case OP_CALL:
          dest = instr[i].addr + instr[i].len + instr[i].value;
          goto PROC_JUMP;
        case OP_PUSH_68:
        case OP_PUSH_6A:
        case OP_XOR:
        case OP_ADD:
        case OP_CMP_3D: // print signed value
          if (instr[i].value < 0) {
            snprintf(instr[i].mnemo_notes, MNEMO_NOTES_LEN,
                    "-0x%llx", -instr[i].value);
          }
          break;
      }
    } else {
      switch (instr[i].opcode) {
        case OP_EXT_JB:
        case OP_EXT_JE:
        case OP_EXT_JNE: // rel32off
          dest = instr[i].addr + instr[i].len + instr[i].value;
          goto PROC_JUMP;
      }
    }

    continue;

PROC_JUMP: ;
    // Print dest. label to instr. note
    bool search_back = dest < instr[i].addr;
    for (int j = (search_back ? 0 : i); j < (search_back ? i : count); j++) {
      if (instr[j].addr == dest) { // find dest. instr.
        if (instr[j].label[0] == '\0') // no label?
          snprintf(instr[j].label, LABEL_LEN, "LL%d", new_label_count++);

        snprintf(instr[i].mnemo_notes, MNEMO_NOTES_LEN, ".%s", instr[j].label);
        done = true;
        break;
      }
    }

    if (!done) { // out of scope / inside of instr.
      snprintf(instr[i].mnemo_notes, MNEMO_NOTES_LEN, "%s0x%llx",
                (dest < 0 ? "-" : ""), (dest < 0 ? -dest : dest));
    }
  }
}
