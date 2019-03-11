#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

#include "decode.h"

// GPR mnemonics, index = encoding
const char * const GPR_64b[] = {
  "rax", "rcx", "rdx", "rbx",
  "rsp", "rbp", "rsi", "rdi",
  "r8",  "r9",  "r10", "r11",
  "r12", "r13", "r14", "r15"
};
const char * const GPR_32b[] = {
  "eax", "ecx", "edx", "ebx",
  "esp", "ebp", "esi", "edi",
  "r8d",  "r9d",  "r10d", "r11d",
  "r12d", "r13d", "r14d", "r15d"
};

/**
 * Decodes immediate signed value from sequence of bytes
 */
long dec_imm(byte_t bytes[], int count, int imm_size) {
  unsigned long value = 0;

  switch (imm_size) {
    case 32:
      value |= bytes[2] << 16 | bytes[3] << 24;
    case 16:
      value |= bytes[1] << 8;
    case 8:
      value |= bytes[0];
    default:
      break;
  }

  return (long) value;
}

/**
 * Decodes ModRM fields from byte
 */
void dec_modrm(byte_t bytes[], int count, modrm_byte_t *modrm) {
  modrm->mod = (bytes[0] & 0b11000000) >> 6;
  modrm->reg = (bytes[0] & 0b00111000) >> 3;
  modrm->rm  = (bytes[0] & 0b00000111);
}

/**
 * Decodes register mnemonic from ModRM.rm field
 *  REX.b is used as 4th bit
 *  If default_64b is set to false, 32b mnemonics are used unless REX.w is 1
 */
const char *dec_modrm_rm_register(modrm_byte_t modrm, rex_byte_t rex, bool default_64b) {
  if (modrm.mod == 0b00 && modrm.rm == 0b101) {
    return "rip"; // %rip+disp32
  }
  byte_t enc = modrm.rm | (rex.b ? 0b1000 : 0);
  return default_64b ? GPR_64b[enc] :
            (rex.w ? GPR_64b[enc] : GPR_32b[enc]);
}

/**
 * Decodes register mnemonic from ModRM.reg field
 *  REX.r is used as 4th bit
 */
const char *dec_modrm_reg_register(modrm_byte_t modrm, rex_byte_t rex, bool default_64b) {
  byte_t enc = modrm.reg | (rex.r ? 0b1000 : 0);
  return default_64b ? GPR_64b[enc] :
            (rex.w ? GPR_64b[enc] : GPR_32b[enc]);
}

/**
 * Decodes register mnemonic from opcode register bits
 *  REX.b is used as 4th bit
 */
const char *dec_opcode_register(byte_t bytes[], int count, rex_byte_t rex) {
  return GPR_64b[(bytes[0] & 0b111) | (rex.b ? 0b1000 : 0)];
}

/**
 * Returns expected displacement size in bits based on ModRM byte
 *
 * ModRM.mod =
 *  0b00 : no displacement (disp32 if r/m is 101 => disp32(%rip))
 *  0b01 : disp8
 *  0b10 : disp32
 */
int get_modrm_disp(modrm_byte_t modrm) {
  switch (modrm.mod) {
    case 0b00: return modrm.rm == 0b101 ? 32 : 0;
    case 0b01: return 8;
    case 0b10: return 32;
    case 0b11: return 0; // invalid / direct
  }
}

/**
 * Checks if next byte is REX byte
 *  If yes, decodes rex fields
 */
bool is_rex(byte_t bytes[], int count, rex_byte_t *rex) {
  if (bytes[0] < 0x40 || bytes[0] > 0x4F)
    return false;

  rex->w = bytes[0] & 0b1000;
  rex->r = bytes[0] & 0b0100;
  rex->x = bytes[0] & 0b0010;
  rex->b = bytes[0] & 0b0001;
  return true;
}

/**
 * Checks if next byte is escape opcode (0x0F)
 */
bool is_twobyte_opcode(byte_t bytes[], int count) {
  return bytes[0] == 0x0F;
}

void format_mnemonic(char *str, const char *opcode, const char *format, ...) {
  va_list args;
  va_start(args, format);
  char tmp[128];

  vsnprintf(tmp, 128, format, args);
  sprintf(str, "%-7s %s", opcode, tmp);

  va_end(args);
}

// Assuming 64b mode
void decode(char *str, byte_t bytes[], int count) {
  if (!count)
    return; // Nothing to decode

  rex_byte_t rex = {0};
  modrm_byte_t modrm = {0};

  // Check REX byte
  bool has_rex = is_rex(bytes, count, &rex);
  if (has_rex)
    bytes++; // advance

  // Check 0F extended opcode
  bool has_twobyte_opcode = is_twobyte_opcode(bytes, count);
  if (has_twobyte_opcode)
    bytes++; // advance

  // Check opcode
  if (!has_twobyte_opcode) {
    switch (bytes[0]) {
      case 0x05: // add eax/rax imm32
        format_mnemonic(str, "add",
                        rex.w ? "$0x%X, %%rax" : "$0x%X, %%eax",
                        dec_imm(&bytes[1], count - 1, 32));
        break;
      case 0x35: // xor eax/rax imm32
        format_mnemonic(str, "xor",
                        rex.w ? "$0x%X, %%rax" : "$0x%X, %%eax",
                        dec_imm(&bytes[1], count - 1, 32));
        break;
      case 0x39: // cmp reg/mem64,reg64
        dec_modrm(&bytes[1], count - 1, &modrm);
        if (modrm.mod == 0b11) { // direct
          format_mnemonic(str, "cmp", "%%%s, %%%s",
                          dec_modrm_reg_register(modrm, rex, false),
                          dec_modrm_rm_register(modrm, rex, false));
        } else {                 // indirect
          format_mnemonic(str, "cmp", "%%%s, 0x%X(%%%s)",
                          dec_modrm_reg_register(modrm, rex, false),
                          dec_imm(&bytes[2], count - 2, get_modrm_disp(modrm)),
                          dec_modrm_rm_register(modrm, rex, true));
        }
        break;
      case 0x3D: // cmp eax/rax imm32
        format_mnemonic(str, "cmp",
                        rex.w ? "$0x%X, %%rax" : "$0x%X, %%eax",
                        dec_imm(&bytes[1], count - 1, 32));
        break;
      case 0x50:
      case 0x51:
      case 0x52:
      case 0x53:
      case 0x54:
      case 0x55:
      case 0x56:
      case 0x57: // push reg64
        format_mnemonic(str, "push", "%%%s", dec_opcode_register(&bytes[0], count - 1, rex));
        break;
      case 0x58:
      case 0x59:
      case 0x5A:
      case 0x5B:
      case 0x5C:
      case 0x5D:
      case 0x5E:
      case 0x5F: // pop reg64
        format_mnemonic(str, "pop", "%%%s", dec_opcode_register(&bytes[0], count - 1, rex));
        break;
      case 0x68: // push imm64 (sign-extended 32b imm)
        format_mnemonic(str, "push", "$0x%X", dec_imm(&bytes[1], count - 1, 32));
        break;
      case 0x6A: // push imm8
        format_mnemonic(str, "push", "$0x%X", dec_imm(&bytes[1], count - 1, 8));
        break;
      case 0x72: // jb imm8off
        format_mnemonic(str, "jb", "$rip+0x%X", dec_imm(&bytes[1], count - 1, 8));
        break;
      case 0x74: // je imm8off
        format_mnemonic(str, "je", "$rip+0x%X", dec_imm(&bytes[1], count - 1, 8));
        break;
      case 0x75: // jne imm8off
        format_mnemonic(str, "jne", "$rip+0x%X", dec_imm(&bytes[1], count - 1, 8));
        break;
      case 0x89: // mov reg/mem64,reg64
        dec_modrm(&bytes[1], count - 1, &modrm);
        if (modrm.mod == 0b11) { // direct
          format_mnemonic(str, "mov", "%%%s, %%%s",
                          dec_modrm_reg_register(modrm, rex, false),
                          dec_modrm_rm_register(modrm, rex, false));
        } else {                 // indirect
          format_mnemonic(str, "mov", "%%%s, 0x%X(%%%s)",
                          dec_modrm_reg_register(modrm, rex, false),
                          dec_imm(&bytes[2], count - 2, get_modrm_disp(modrm)),
                          dec_modrm_rm_register(modrm, rex, true));
        }
        break;
      case 0x8B: // mov reg64,reg/mem64
        dec_modrm(&bytes[1], count - 1, &modrm);
        if (modrm.mod == 0b11) { // direct
          format_mnemonic(str, "mov", "%%%s, %%%s",
                          dec_modrm_rm_register(modrm, rex, false),
                          dec_modrm_reg_register(modrm, rex, false));
        } else {                 // indirect
          format_mnemonic(str, "mov", "0x%X(%%%s), %%%s",
                          dec_imm(&bytes[2], count - 2, get_modrm_disp(modrm)),
                          dec_modrm_rm_register(modrm, rex, true),
                          dec_modrm_reg_register(modrm, rex, false));
        }
        break;
      case 0x8F:
        dec_modrm(&bytes[1], count - 1, &modrm);
        switch (modrm.reg) {
          case 0: // pop reg/mem64
            if (modrm.mod == 0b11) { // direct
              format_mnemonic(str, "pop", "%%%s", dec_modrm_rm_register(modrm, rex, true));
            } else {                 // indirect
              format_mnemonic(str, "pop", "0x%X(%%%s)",
                              dec_imm(&bytes[2], count - 2, get_modrm_disp(modrm)),
                              dec_modrm_rm_register(modrm, rex, true));
            }
            break;
          default:
            goto UNK_OPCODE;
        }
        break;
      case 0x90: // nop
        format_mnemonic(str, "nop", "");
        break;
      case 0xC2: // ret imm16
        format_mnemonic(str, "ret", "$0x%X", dec_imm(&bytes[1], count - 1, 16));
        break;
      case 0xC3: // ret
        format_mnemonic(str, "ret", "");
        break;
      case 0xCC: // int 3
        format_mnemonic(str, "int 3", "");
        break;
      case 0xE8: // call rel32off
        format_mnemonic(str, "call", "$rip+0x%X", dec_imm(&bytes[1], count - 1, 32));
        break;
      case 0xE9: // jmp imm32off
        format_mnemonic(str, "jmp", "$rip+0x%X", dec_imm(&bytes[1], count - 1, 32));
        break;
      case 0xEB: // jmp imm8off
        format_mnemonic(str, "jmp", "$rip+0x%X", dec_imm(&bytes[1], count - 1, 8));
        break;
      case 0xF7:
        dec_modrm(&bytes[1], count - 1, &modrm);
        switch (modrm.reg) {
          case 4: // mul reg/mem64
            if (modrm.mod == 0b11) { // direct
              format_mnemonic(str, "mul", "%%%s", dec_modrm_rm_register(modrm, rex, false));
            } else {                 // indirect
              format_mnemonic(str, "mul", "0x%X(%%%s)",
                              dec_imm(&bytes[2], count - 2, get_modrm_disp(modrm)),
                              dec_modrm_rm_register(modrm, rex, true));
            }
            break;
          default:
            goto UNK_OPCODE;
        }
        break;
      case 0xFF:
        dec_modrm(&bytes[1], count - 1, &modrm);
        // opcode extension
        switch (modrm.reg) {
          case 2: // call reg/mem64
            if (modrm.mod == 0b11) { // direct
              format_mnemonic(str, "call", "*%%%s", dec_modrm_rm_register(modrm, rex, true));
            } else {                 // indirect
              format_mnemonic(str, "call", "*0x%X(%%%s)",
                              dec_imm(&bytes[2], count - 2, get_modrm_disp(modrm)),
                              dec_modrm_rm_register(modrm, rex, true));
            }
            break;
          case 6: // push reg/mem64
            if (modrm.mod == 0b11) { // direct
              format_mnemonic(str, "push", "%%%s", dec_modrm_rm_register(modrm, rex, true));
            } else {                 // indirect
              format_mnemonic(str, "push", "0x%X(%%%s)",
                              dec_imm(&bytes[2], count - 2, get_modrm_disp(modrm)),
                              dec_modrm_rm_register(modrm, rex, true));
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
    switch (bytes[0]) {
      case 0x82: // jb imm32off
        format_mnemonic(str, "jb", "$rip+0x%X", dec_imm(&bytes[1], count - 1, 32));
        break;
      case 0x84: // je/jz imm32off
        format_mnemonic(str, "je", "$rip+0x%X", dec_imm(&bytes[1], count - 1, 32));
        break;
      case 0x85: // jne/jnz imm32off
        format_mnemonic(str, "jne", "$rip+0x%X", dec_imm(&bytes[1], count - 1, 32));
        break;
      case 0x1F: // nop reg/mem64
        dec_modrm(&bytes[1], count - 1, &modrm);
        if (modrm.mod == 0b11) { // direct
          format_mnemonic(str, "nop", "%%%s", dec_modrm_rm_register(modrm, rex, false));
        } else {                 // indirect
          format_mnemonic(str, "nop", "0x%X(%%%s)",
                          dec_imm(&bytes[2], count - 2, get_modrm_disp(modrm)),
                          dec_modrm_rm_register(modrm, rex, true));
        }
        break;
      case 0xA0: // push fs
        format_mnemonic(str, "push", "%%fs");
        break;
      case 0xA1: // pop fs
        format_mnemonic(str, "pop", "%%fs");
        break;
      case 0xA8: // push gs
        format_mnemonic(str, "push", "%%gs");
        break;
      case 0xA9: // pop gs
        format_mnemonic(str, "pop", "%%gs");
        break;
      default:
        goto UNK_OPCODE;
    }
  }

  return;

UNK_OPCODE:
  sprintf(str, MSG_UNK_OPCODE);
}

void print_bytes(byte_t bytes[], int count) {
  for (int i = 0; i < count; i++) {
    printf("%02X ", bytes[i]);
  }
  printf("\n");
}

void argv_to_bytes(byte_t bytes[], const char *argv[], int argc) {
  for (int i = 1; i < argc; i++) {
    bytes[i - 1] = (byte_t) strtol(argv[i], NULL, 16);
  }
}

int main(int argc, const char *argv[]) {

  char str[128];
  byte_t bytes[argc - 1];

  argv_to_bytes(bytes, argv, argc);
  //print_bytes(bytes, argc - 1);
  decode(str, bytes, argc - 1);

  printf("%s\n", str);
  return 0;
}
