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
  uint8_t min_args;
};
typedef struct InstructionDef InstructionDef;

InstructionDef** getInstructionsDef(void);