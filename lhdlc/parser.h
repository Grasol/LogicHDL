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
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lhdlc.h"
#define WHITESPACE " \t\r\x0c\x0b"


struct InstructionData {
  char *operation;
  uint32_t dst;
  uint32_t src;
  char **args;

  size_t ln;
};
typedef struct InstructionData InstructionData;

struct ModuleData {
  char *name;
  InstructionData **instructions;

  char *file_name;
  bool usage;
};
typedef struct ModuleData ModuleData;

char** splitText(InputData *src);

ModuleData** parser(ModuleData *modules[], InputData *src, size_t n_mod);