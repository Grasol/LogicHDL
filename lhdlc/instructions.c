#include "instructions.h"
#include "lhdlc.h"
#include "parser.h"

uint8_t value(char c) {
  uint8_t res = 0;
  switch (c) {
  case 'T': res = 1; break;
  case 'F': res = 0; break;
  case 'N': res = 0; break;
  case 'S': res = 1; break;
  case 'X': res = 2; break;
  }

  return res;
}


InstructionDef** getInstructionsDef(void) {
  char** data = splitText(readFile("instructions.txt"));
  InstructionDef **def = malloc(sizeof(InstructionDef*) * INSTR_NUMBER);
  if (def == NULL) {
    goto memory_error;
  }
  
  size_t j = 0;
  for (size_t i = 0; data[i] != NULL; i++) {
    if (data[i][0] == '\n') {
      continue;
    }

    def[j] = malloc(sizeof(InstructionDef));
    if (def == NULL) {
      goto memory_error;
    }

    def[j]->mnemonic = data[i];
    
    uint16_t opc = (uint16_t)(strtol(data[i + 1], NULL, 16) & 0xffff);
    def[j]->opcode = opc;
    def[j]->dst = (bool)value(data[i + 2][0]);
    def[j]->src = (bool)value(data[i + 3][0]);
    def[j]->src1_arg = value(data[i + 4][0]);
    def[j]->src2_arg = value(data[i + 5][0]);
    def[j]->srcx_arg = (uint8_t)(atoi(data[i + 6]) & 0xff);

    j++;
    i += 7;
  }

  return def;

memory_error:  
  fprintf(stderr, "memory error...\n\n");
  exit(2);
}