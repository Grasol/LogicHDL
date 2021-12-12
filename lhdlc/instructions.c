// LogicHDL Compiler v0.1
//
// Copyright 2021 Grasol
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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
  // path to instructions.txt
  /*printf("ZZZZZZZZZZZZZZZZZZZZZ\n");
  size_t sz_path = strlen(_pgmptr) + 8;
  char *path = malloc(sz_path);
  memcpy(path, _pgmptr, sz_path);
  memcpy(path + sz_path - 17, "instructions.txt\0", 17);
  */

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

    def[j]->min_args = 0;
    if (def[j]->src1_arg) {
      def[j]->min_args++;
    }

    if (def[j]->src2_arg) {
      def[j]->min_args++;
    }

    j++;
    i += 7;
  }

  return def;

memory_error:  
  fprintf(stderr, "memory error...\n\n");
  exit(2);
}