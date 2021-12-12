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

#include "parser.h"

bool checkInString(char c, char *s) {
  for (size_t i = 0; s[i] != 0; i++) {
    if (s[i] == c)
      return true;
  }
  return false;
}

bool stringInString(char *s1, char *s2) { // stringInString is very bad name, I know...
  for (size_t i = 0; s1[i] != 0; i++) {
    if (checkInString(s1[i], s2))
      continue;

    return false;
  }

  return true;
}


char** splitText(InputData *src) {
  char **data = malloc(sizeof(char*));
  if (data == NULL) {
    goto memory_error;
  }
  data[0] = NULL;
  size_t data_sz = 1;
  size_t b_idx = 0, e_idx = 0;
  
  int state = 0; 
  // 0 - start name 
  // 1 - end name 
  // 2 - skip to \n
  for (size_t i = 0; i <= src->sz; i++) {
    char c = src->data[i];
    char *tmp_ptr = NULL;

    if (state == 0) {
      if (checkInString(c, WHITESPACE)) {
        continue;
      }
      else if (checkInString(c, ",=\n")) {
        tmp_ptr = malloc(2);
        if (tmp_ptr == NULL) {
          goto memory_error;
        }

        tmp_ptr[0] = c;
        tmp_ptr[1] = 0;
      }  
      else if (c == ';') {
        state = 2;
      }
      else {
        b_idx = i;
        state = 1;
      }
    }

    else if (state == 1) {
      if (checkInString(c, WHITESPACE)) {
        e_idx = i;
        tmp_ptr = malloc(e_idx - b_idx + 1);
        if (tmp_ptr == NULL) {
          goto memory_error;
        }

        memcpy(tmp_ptr, src->data + b_idx, e_idx - b_idx);
        tmp_ptr[e_idx - b_idx] = 0;
        state = 0;
      }
      else if (checkInString(c, ",=\n")) {
        e_idx = i;
        tmp_ptr = malloc(e_idx - b_idx + 1);
        if (tmp_ptr == NULL) {
          goto memory_error;
        }

        memcpy(tmp_ptr, src->data + b_idx, e_idx - b_idx);
        tmp_ptr[e_idx - b_idx] = 0;
        i--;
        state = 0;
      }
    }

    else if (state == 2) {
      if (c != '\n') {
        continue;
      }

      tmp_ptr = malloc(2);
      tmp_ptr[0] = '\n';
      tmp_ptr[1] = 0;
      state = 0;
    }

    if (tmp_ptr != NULL) {
      data_sz++;
      data = realloc(data, sizeof(char*) * data_sz);
      if (data == NULL) {
        goto memory_error;
      }

      data[data_sz - 2] = tmp_ptr;
      data[data_sz - 1] = NULL;
      //printf("%s\n", tmp_ptr);
    }
  }

  return data;

  memory_error:
  fprintf(stderr, "memory error...\n");
  exit(2);
}



ModuleData** parser(ModuleData *modules[], InputData *src, size_t n_mod) {
  char **data = splitText(src);
  char *module_name;
  bool name_argument, in_module;
  InstructionData *ip;

  size_t i = 0, ln = 1, m_idx, ins_itx, n_ins; 
  int parser_state = 0, err = 0;
  // 0 - out module
  // 1 - in modulule
  // 2 - new instruction
  // 3 - destination
  // 4 - operation 
  // 5 - source
  // 8 - out_module_skip_line
  while (data[i] != NULL) {
    switch (parser_state) {
    case 0x00: { // out module
      if (data[i][0] == '\n' || stringInString(data[i], WHITESPACE)) {
        i++;
        ln++;
        break;
      }

      if (memcmp(data[i], "module\0", 7) == 0) {
        parser_state = 1;
        module_name = NULL;
      }
      else {
        parser_state = 8;
        fprintf(stderr, "%s:%I64u warning: line is not in any module and will be skip\n", 
                src->name, ln);
      }

      i++;
      break;
    }

    case 0x01: { // in modulule
      if (checkInString(data[i][0], ",=\n")) {
        if (module_name == NULL) {
          parser_state = 8;
          fprintf(stderr, "%s:%I64u error: invalid syntax\n", src->name, ln);
          err |= 1;
        }
        else { 
          // change state
          parser_state = 2;
          in_module = true;
          m_idx = n_mod - 1;
          ins_itx = 0;
          n_ins = 1;;
          
          ModuleData *p = malloc(sizeof(ModuleData));
          if (p == NULL) {
            goto memory_error;
          }

          p->name = module_name;
          p->file_name = src->name;
          p->usage = false;
          p->instructions = malloc(sizeof(InstructionData*));
          if (p->instructions == NULL) {
            goto memory_error;
          }

          p->instructions[ins_itx] = NULL;
          modules[m_idx] = p;
          n_mod++;
          modules = realloc(modules, sizeof(ModuleData*) * n_mod);
          if (modules == NULL) {
            goto memory_error;
          }

          modules[n_mod - 1] = NULL;
          //printf("--> to 2\n");
        }

        ln++;
      }
      else if (module_name == NULL) {
        module_name = data[i];
      }
      else {
        parser_state = 8;
        fprintf(stderr, "%s:%I64u error: invalid name '%s'\n", src->name, ln, data[i]);
        err |= 1;
      }

      i++;
      break;
    }

    case 0x02: { // new instruction
      //printf("AAA %i\n", i);
      if (data[i - 1][0] != '\n' && data[i][0] != '\n') {
        i++;
        break;
      }
      else if (data[i][0] == '\n') {
        ln++;
        i++;
        break;
      }

      parser_state = 3;
      name_argument = true;
      ins_itx = n_ins - 1;
      
      ip = malloc(sizeof(InstructionData)); // instruction pointer
      if (ip == NULL) {
        goto memory_error;
      }

      ip->operation = NULL;
      ip->dst = 0;
      ip->src = 0;
      ip->ln = ln;
      ip->args = malloc(sizeof(char*));
      if (ip->args == NULL) {
        goto memory_error;
      }

      ip->args[0] = NULL;
      modules[m_idx]->instructions[ins_itx] = ip;
      n_ins++;
      
      InstructionData **p = realloc(modules[m_idx]->instructions, 
                                    sizeof(InstructionData*) * n_ins);
      if (p == NULL) {
        goto memory_error;
      }

      p[n_ins - 1] = NULL;
      modules[m_idx]->instructions = p;
      break;
    }

    case 0x03: { // destination
      if (ip->dst == 0) {
        if (memcmp(data[i], "endmodule\0", 10) == 0) {
          //printf("AAA %i\n", i);
          parser_state = 8;
          in_module = false;
          i++;
          break;
        }
      }

      if (data[i][0] == '\n') {
        parser_state = 2;
        fprintf(stderr, "%s:%I64u error: invalid syntax\n", src->name, ln);
        err |= 1;
        ln++;
      }
      else if (data[i][0] == ',') {
        if (name_argument) {
          parser_state = 2;
          fprintf(stderr, "%s:%I64u error: invalid syntax ','\n", src->name, ln);          
          err |= 1;
        }
        else {
          name_argument = true;
        }
      }
      else if (data[i][0] == '=') {
        if (name_argument) {
          parser_state = 2;
          fprintf(stderr, "%s:%I64u error: invalid syntax '='\n", src->name, ln);  
          err |= 1;
        }
        else {
          parser_state = 4;
        }
      }
      else {
        ip->args[ip->dst] = data[i];
        ip->dst++;
        ip->args = realloc(ip->args, sizeof(char*) * (ip->dst + 1));
        if (ip->args == NULL) {
          goto memory_error;
        }

        ip->args[ip->dst] = NULL;
        name_argument = false;
      }

      i++;
      break;
    }

    case 0x04: { // operation 
      if (checkInString(data[i][0], ",=\n")) {
        parser_state = 2;
        fprintf(stderr, "%s:%I64u error: invalid syntax '%c'\n", src->name, ln, data[i][0]); 
        err |= 1;
        if (data[i][0] == '\n') {
          ln++;
        }
      }
      else {
        parser_state = 5;
        ip->operation = data[i];
      }

      name_argument = true;
      i++;
      break;
    }

    case 0x05: { // source
      if (data[i][0] == '\n') {
        parser_state = 2;
        if (name_argument) {
          fprintf(stderr, "%s:%I64u error: invalid syntax ','\n", src->name, ln); 
          err |= 1;          
        }
        ln++;
      }
      else if (data[i][0] == ',') {
        if (name_argument) {
          parser_state = 2;
          fprintf(stderr, "%s:%I64u error: invalid syntax ','\n", src->name, ln); 
          err |= 1;
        }
        else {
          name_argument = true;
        }
      }
      else if (data[i][0] == '=') {
        parser_state = 2;
        fprintf(stderr, "%s:%I64u error: invalid syntax '='\n", src->name, ln); 
        err |= 1;
      }
      else {
        ip->args[ip->dst + ip->src] = data[i];
        ip->src++;
        ip->args = realloc(ip->args, sizeof(char*) * (ip->dst + ip->src + 1));
        if (ip->args == NULL) {
          goto memory_error;
        }

        ip->args[ip->dst + ip->src] = NULL;
        name_argument = false;
      }

      i++;
      break;
    }

    case 0x08: { // out_module_skip_line
      if (data[i][0] == '\n') {
        parser_state = 0;
        in_module = false;
        ln++;
      }

      i++;
      break;
    }
    }
  }

  if (in_module) {
    fprintf(stderr, "%s:%I64u error: module '%s' is not close\n", 
            src->name, ln, modules[m_idx]->name);
    err |= 1;
  }

  /*printf("=====");
  for (size_t j = 0; modules[j] != NULL; j++) {
    printf("\nIN MODULE = %s\n", modules[j]->name);
    for (size_t k = 0; modules[j]->instructions[k] != NULL; k++) {
      printf("  OP: %s, DST: %i SRC: %i\n",
        modules[j]->instructions[k]->operation,
        modules[j]->instructions[k]->dst,
        modules[j]->instructions[k]->src
        );
    }
  }*/


  if (err) {
    return NULL;
  }
  return modules;

  memory_error:
  fprintf(stderr, "memory error...\n");
  exit(2);

}




