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
};
typedef struct InstructionData InstructionData;

struct ModuleData {
  char *name;
  InstructionData **instructions;
};
typedef struct ModuleData ModuleData;

char** splitText(InputData *src);

ModuleData** parser(ModuleData *modules[], InputData *src, size_t n_mod);