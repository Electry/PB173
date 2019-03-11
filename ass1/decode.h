
#define MSG_UNK_OPCODE "unknown instruction"

typedef unsigned char byte_t;

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