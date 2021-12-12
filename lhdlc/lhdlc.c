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

#include "lhdlc.h"
#include "parser.h"
#include "synthesizer.h"
#include "instructions.h"

char* genOutputFileName(char *name) {
  size_t sz = strlen(name);
  char *out = malloc(sz + 4);
  if (out == NULL) {
    fprintf(stderr, "memory error...\n");
    exit(2);
  }

  size_t dot = 0;
  for (size_t i = 0; i < sz; i++) {
    out[i] = name[i];
    if (name[i] == '.') {
      dot = i;
    }
  }

  out[dot + 1] = 'b';
  out[dot + 2] = 'i';
  out[dot + 3] = 'n';
  out[dot + 4] = 0;

  return out;
}


void u64tochar(char *data, uint64_t num, int off) {
  for (int i = 1; i <= off; i++) {
    data[i - 1] = (num >> ((off - i) * 8)) & 0xff;
  }

  return;
}

InputData* readFile(char *name) {
  FILE *f = fopen(name, "rb");
  if (f == NULL) {
    goto file_open_error;
  }

  fseek(f, 0, SEEK_END);
  size_t file_sz = ftell(f) + 2;
  fseek(f, 0, SEEK_SET);

  char *data = malloc(file_sz);
  if (data == NULL) {
    goto memory_error; 
  }

  fread(data, 1, file_sz, f);
  fclose(f);
  data[file_sz - 2] = '\n';
  data[file_sz - 1] = 0;
  //printf(">>> %I64u\n\n", file_sz);

  InputData *res = malloc(sizeof(InputData));
  if (data == NULL) {
    goto memory_error;
  }

  res->data = data;
  res->name = name;
  res->sz = file_sz;

  return res;

  file_open_error:
  fprintf(stderr, "can't open file '%s'", name);
  exit(2);

  memory_error:
  fprintf(stderr, "memory error...\n");
  exit(2);
}


int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "usage: lgdlc <files>\n");
    return 1;
  }

  InputData *file;
  ModuleData **modules = malloc(sizeof(ModuleData*));
  modules[0] = NULL;
  size_t n_mod = 1; // number of modules
  for (int i = 1; i < argc; i++) {
    file = readFile(argv[i]);

    modules = parser(modules, file, n_mod);
    if (modules == NULL) {
      goto compilation_terminate;
    }

    while (modules[n_mod - 1] != NULL) {
      n_mod++;
    }
  }

  for (int i = 0; modules[i] != NULL; i++) {
    printf("(%s)\n", modules[i]->name);
  }

  GlobalTable *gt = malloc(sizeof(GlobalTable));
  if (gt == NULL) {
    goto memory_error;
  }

  gt->global_cable_id = 1;
  gt->global_args_offset = 0;  
  gt->i_instr = 0;
  gt->instr_desc = malloc(sizeof(InstructionDef*));
  if (gt->instr_desc == NULL) {
    goto memory_error;
  }
  
  gt->i_gargs = 0;
  gt->global_args = malloc(sizeof(GlobalArgument*));
  if (gt->global_args == NULL) {
    goto memory_error;
  }

  gt->i_antiloop = 0;
  gt->antiloop_names = malloc(sizeof(char*) * n_mod);
  if (gt->antiloop_names == NULL) {
    goto memory_error;
  }
  
  gt->modules = modules;
  gt->n_mod = n_mod;
  gt->instr_desc[0] = NULL;
  gt->global_args[0] = NULL;
  memset(gt->antiloop_names, 0, sizeof(char*) * n_mod);
  gt->instr_def = getInstructionsDef();
  
  printf("11111111\n");
  // search top_module
  size_t tm = 0;
  for (; tm < n_mod - 1; tm++) {
    if (memcmp(modules[tm]->name, "top_module\0", 11) == 0) {
      break;
    }
  }

  if (tm == n_mod - 1) {
    fprintf(stderr, "error: not found 'top_module'\n\n");
    goto compilation_terminate;
  }

  int err = synthesizer(gt, modules[tm], NULL);
  if (err) {
    goto compilation_terminate;
  }

  // store to bin file
  // 
  // file construction:
  // 0: INSTRUCTION DESCRIPTORS TABLE PTR
  // 8: GLOBAL ARGUMENTS TABLE PTR
  // IDTP: IDT
  // GATP: GAT
  size_t all_real_sz = 16; // IDTP + GATP
  all_real_sz += (gt->i_instr + 1) * 16;
  all_real_sz += gt->global_args_offset;

  size_t i_bin = 16;
  char *bin_data = malloc(all_real_sz);
  if (bin_data == NULL) {
    goto memory_error;
  }

  u64tochar(bin_data, 16, 8); // IDTP
  u64tochar(bin_data + 8, (gt->i_instr + 2) * 16, 8); // GATP

  for (size_t i = 0; i < gt->i_instr; i++) {
    u64tochar(bin_data + i_bin, (uint64_t)(gt->instr_desc[i]->opcode), 2);
    bin_data[i_bin + 2] = gt->instr_desc[i]->dst;
    bin_data[i_bin + 3] = gt->instr_desc[i]->src;
    u64tochar(bin_data + i_bin + 8, gt->instr_desc[i]->args_offset, 8);
    i_bin += 16;
  }

  memset(bin_data + i_bin, 0xff, 16);
  i_bin += 16;

  for (size_t i = 0; i < gt->i_gargs; i++) {
    uint8_t tmp_sz = gt->global_args[i]->real_sz;
    u64tochar(bin_data + i_bin, gt->global_args[i]->cable_id, tmp_sz);
    bin_data[i_bin] |= gt->global_args[i]->flags << 5;
    i_bin += tmp_sz;
  }

  FILE *f = fopen(genOutputFileName(argv[1]), "wb");
  if (f == NULL) {
    fprintf(stderr, "error save data...\n");
    return 2;
  }

  fwrite(bin_data, 1, all_real_sz, f);
  fclose(f);

  return 0;

  compilation_terminate:
  fprintf(stderr, "compilation terminate...\n");
  return 1;

  memory_error:
  fprintf(stderr, "memory error...\n");
  return 2;
}