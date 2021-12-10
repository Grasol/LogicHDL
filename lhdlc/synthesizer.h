#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "instructions.h"


struct LocalCable {
  char *name;
  uint64_t cable_id;
};
typedef struct LocalCable LocalCable;

struct GlobalArgument {
  uint64_t cable_id;
  uint8_t flags;
  uint8_t real_sz;
};
typedef struct GlobalArgument GlobalArgument;

struct InstructionDesc {
  uint64_t args_offset;
  uint16_t opcode;
  uint8_t dst;
  uint8_t src;
};
typedef struct InstructionDesc InstructionDesc;

struct GlobalTable {
  uint64_t global_cable_id;
  uint64_t global_args_offset;
  
  size_t i_instr;
  InstructionDesc **instr_desc;
  
  size_t i_gargs;
  GlobalArgument **global_args;
  
  size_t i_antiloop;
  char **antiloop_names;

  size_t n_mod;
  ModuleData **modules;

  InstructionDef **instr_def;
};
typedef struct GlobalTable GlobalTable;

int synthesizer(GlobalTable *gt, ModuleData *module);
