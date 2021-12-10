#pragma once

#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INSTR_NUMBER 28


struct InstructionDef {
  char *mnemonic;
  uint16_t opcode;
  bool dst;
  bool src;
  uint8_t src1_arg; // 0 - False/Null, 1 - True/Signal, 2 - Xstring
  uint8_t src2_arg;
  uint8_t srcx_arg;
};
typedef struct InstructionDef InstructionDef;

InstructionDef** getInstructionsDef(void);