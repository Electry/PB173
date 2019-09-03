#ifndef DECODE_H
#define DECODE_H

#define MSG_UNK_OPCODE "unknown instruction"

#define MNEMO_OPCODE_LEN   8
#define MNEMO_OPERAND_LEN  32
#define MNEMO_NOTES_LEN    48
#define MNEMO_CF_LABEL_LEN 32

#define HEX_BYTES_LEN      32
#define LABEL_LEN          32

typedef unsigned char byte_t;

typedef enum {
  OP_ADD     = 0x05,
  OP_XOR     = 0x35,
  OP_CMP_39  = 0x39,
  OP_CMP_3D  = 0x3D,
  OP_PUSH_50 = 0x50,
  OP_PUSH_51 = 0x51,
  OP_PUSH_52 = 0x52,
  OP_PUSH_53 = 0x53,
  OP_PUSH_54 = 0x54,
  OP_PUSH_55 = 0x55,
  OP_PUSH_56 = 0x56,
  OP_PUSH_57 = 0x57,
  OP_POP_58  = 0x58,
  OP_POP_59  = 0x59,
  OP_POP_5A  = 0x5A,
  OP_POP_5B  = 0x5B,
  OP_POP_5C  = 0x5C,
  OP_POP_5D  = 0x5D,
  OP_POP_5E  = 0x5E,
  OP_POP_5F  = 0x5F,
  OP_PUSH_68 = 0x68,
  OP_PUSH_6A = 0x6A,
  OP_JB      = 0x72,
  OP_JE      = 0x74,
  OP_JNE     = 0x75,
  OP_MOV_89  = 0x89,
  OP_MOV_8B  = 0x8B,
  OP_8F      = 0x8F, // pop reg/mem64 /0
  OP_NOP     = 0x90,
  OP_RET_C2  = 0xC2,
  OP_RET_C3  = 0xC3,
  OP_INT3    = 0xCC,
  OP_CALL    = 0xE8,
  OP_JMP_E9  = 0xE9,
  OP_JMP_EB  = 0xEB,
  OP_F7      = 0xF7, // mul reg/mem64 /4
  OP_FF      = 0xFF  // call reg/mem64 /2, push reg/mem64 /6
} opcode_t;

typedef enum {
  OP_EXT_JB      = 0x82,
  OP_EXT_JE      = 0x84,
  OP_EXT_JNE     = 0x85,
  OP_EXT_NOP     = 0x1F,
  OP_EXT_PUSH_FS = 0xA0,
  OP_EXT_POP_FS  = 0xA1,
  OP_EXT_PUSH_GS = 0xA8,
  OP_EXT_POP_GS  = 0xA9,
} ext_opcode_t;

typedef struct {
  bool w;
  bool r;
  bool x;
  bool b;
} rex_byte_t;

typedef struct {
  byte_t mod;
  byte_t reg;
  byte_t rm;
} modrm_byte_t;

typedef struct {
  unsigned int addr;
  size_t len;

  char mnemo_opcode[MNEMO_OPCODE_LEN];
  char mnemo_operand[MNEMO_OPERAND_LEN];
  char mnemo_notes[MNEMO_NOTES_LEN];
  char mnemo_cf_label[MNEMO_CF_LABEL_LEN];

  char hex_bytes[HEX_BYTES_LEN];
  char label[LABEL_LEN];
  unsigned int sub_addr;

  bool has_ext_opcode;
  byte_t opcode;
  long long value;

  bool has_rex;
  rex_byte_t rex;

  bool has_modrm;
  modrm_byte_t modrm;
} instr_t;

typedef enum {
  DECODE_LINEAR,
  DECODE_RECURSIVE
} decode_mode_t;

int decode(instr_t instr[], int instr_pos, byte_t bytes[], unsigned int vaddr, int len, decode_mode_t mode, unsigned int sub_addr);
void proc_flow_labels(instr_t instr[], int count);

#endif
