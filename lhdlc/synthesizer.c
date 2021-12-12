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

#include "synthesizer.h"

GlobalArgument* updataGlobalArguments(GlobalTable *gt, uint64_t cable_id, uint8_t flags) {
  GlobalArgument *ga = malloc(sizeof(GlobalArgument));
  if (ga == NULL) {
    goto memory_error;
  } 

  gt->global_args[gt->i_gargs] = ga; 
  gt->i_gargs++;
  gt->global_args = realloc(gt->global_args, sizeof(GlobalArgument*) * (gt->i_gargs + 1));
  if (gt->global_args == NULL) {
    goto memory_error;
  }

  ga->cable_id = cable_id;

  uint8_t real_sz = 0;;
  switch (flags & 0b110) {
  case 0b000: real_sz = 2; break;
  case 0b010: real_sz = 2; break;
  case 0b100: real_sz = 4; break;
  case 0b110: real_sz = 8; break;
  }
  
  ga->flags = flags;
  ga->real_sz = real_sz;

  gt->global_args_offset += real_sz;
  gt->global_args[gt->i_gargs] = NULL;
  return ga;

  memory_error:
  fprintf(stderr, "memory error...\n");
  exit(2);
}

LocalCable** updataLocalCables(GlobalTable *gt, LocalCable **lc, char *cable) {
  size_t i = 0;
  while (lc[i] != NULL) {
    if (strcmp(lc[i]->name, cable) == 0) {
      break;
    }
    i++;
  }

  if (lc[i] != NULL) {
    return lc;
  }

  lc = realloc(lc, sizeof(LocalCable*) * (i + 2));
  if (lc == NULL) {
    goto memory_error;
  }

  lc[i] = malloc(sizeof(LocalCable));
  if (lc[i] == NULL) {
    goto memory_error;
  }

  lc[i + 1] = NULL;
  lc[i]->name = cable;
  lc[i]->cable_id = gt->global_cable_id;
  gt->global_cable_id++;

  return lc;

  memory_error:
  fprintf(stderr, "memory error...\n");
  exit(2);
}

uint64_t getCableID(LocalCable **lc, char *cable) {
  for (size_t i = 0; lc[i] != NULL; i++) {
    if (strcmp(lc[i]->name, cable) == 0) {
      return lc[i]->cable_id;
    }
  }

  return -1;
}

uint8_t setFlagsCableID(uint64_t cable_id) {
  uint8_t res;
  if ((cable_id & 0xffffffffffffe000) == 0)
    res = 0b010;
 
  else if ((cable_id & 0xffffffffe0000000) == 0) 
    res = 0b100;

  else if ((cable_id & 0xe000000000000000) == 0) 
    res = 0b110;

  else 
    res = -1;

  return res; 
}

// 0-3 - CONST SIG
// 4 - SIG
// 5 - XSTR
// 6 - NOT
// FF - ERR
uint8_t argParse(char *arg, uint8_t def_arg) {
  //printf("ARGPARSE: %s %x\n", arg, def_arg);
  if (arg == NULL) {
    return -1;
  }

  if (strcmp(arg, "NOT") == 0) {
    return 6;
  }

  if (def_arg == 0) { // FALSE
    if (strcmp(arg, "NULL") == 0) {
      return 0;
    }

    return -1;
  }
  else if (def_arg == 1) { // SIG
    if (arg[0] == '#') {
      switch (arg[1]) {
      case '0': return 1;
      case '1': return 2;
      case 'Z': return 0;
      case 'X': return 3;
      }
    }
    
    if (strcmp(arg, "NULL") == 0) {
      return 0; // NULL is alias Z signal
    }

    return 4;
  }
  else if (def_arg == 2) { // XSTR
    return 5;
  }

  return -1;
}



int synthesizer(GlobalTable *gt, ModuleData *module, CablesLink *clink) {
  // init and check module loop
  for (size_t i = 0; i < gt->i_antiloop; i++) {
    if (strcmp(module->name, gt->antiloop_names[i]) == 0) {
        fprintf(stderr, "error: Module '%s' references module '%s'. This is a non executable operation\n", 
                gt->antiloop_names[gt->i_antiloop - 1], module->name);
      return 1;
    }
  }
  printf("AAAAAAAAA\n");
  gt->antiloop_names[gt->i_antiloop] = module->name;
  gt->i_antiloop++;

  LocalCable **lc = malloc(sizeof(LocalCable*));
  if (lc == NULL) {
    goto memory_error;
  }

  module->usage = true;
  lc[0] = NULL;
  size_t ii = 0;
  while (module->instructions[ii] != NULL) {
    InstructionData *ip = module->instructions[ii];
    if (ip->operation == NULL) {
      break;
    }

    int oper_state = 0;
    // 1 - simple
    // 2 - module
    // 3 - io

    // recognize operation
    size_t i_id = 0, i_md = 0; // index instr_def, index module
    if (strcmp(ip->operation, "IO") == 0) { 
      oper_state = 3; // IO
    }
    else {
      for (i_id = 0; i_id < INSTR_NUMBER; i_id++) {
        //printf("'%s' '%s'\n", gt->instr_def[i_id]->mnemonic, ip->operation);
        if (strcmp(gt->instr_def[i_id]->mnemonic, ip->operation) == 0) {
          oper_state = 1; // SIMPLE INSTRUCTION
          break;
        }
      }

      if (oper_state == 0) {
        for (i_md = 0; i_md < gt->n_mod - 1; i_md++) {
          //printf("'%s' '%s' %I64u\n", gt->modules[i_md]->name, ip->operation, gt->n_mod);
          if (strcmp(gt->modules[i_md]->name, ip->operation) == 0) {
            oper_state = 2; // MODULE
            break;
          }
        }
      }
    }
    switch (oper_state) {
    case 0: {
      fprintf(stderr, "%s:%I64u error: Invalid operation '%s'\n", 
              module->file_name, ip->ln, ip->operation);
      return 1;
    }

    case 1: { // simple
      // new instruction descriptor
      InstructionDesc *idesc = malloc(sizeof(InstructionDesc));
      if (idesc == NULL) {
        goto memory_error;
      } 

      gt->instr_desc[gt->i_instr] = idesc;
      gt->i_instr++;
      gt->instr_desc = realloc(gt->instr_desc, sizeof(InstructionDesc*) * (gt->i_instr + 1));
      if (gt->instr_desc == NULL) {
        goto memory_error;
      }

      // set descriptor
      idesc->opcode = gt->instr_def[i_id]->opcode;
      idesc->args_offset = gt->global_args_offset;
      
      idesc->dst = 0; idesc->src = 0;
      uint8_t flags = 0, temp_info = 0;
      uint64_t cable_id = 0;
      for (uint32_t i = 0; i < ip->dst; i++) { // dst arguments loop
        temp_info = argParse(ip->args[i], gt->instr_def[i_id]->dst);
        if (temp_info == 255) {
          fprintf(stderr, "%s:%I64u error: invalid cable name: '%s'\n", 
                  module->file_name, ip->ln, ip->args[i]);
          return 1;
        }
        else if (temp_info == 6) { // NOT
          flags |= 1;
          continue;
        }
        else if (temp_info <= 4) { // SIG 
          if (temp_info == 4) {
            lc = updataLocalCables(gt, lc, ip->args[i]); // cable
            cable_id = getCableID(lc, ip->args[i]);
            flags |= setFlagsCableID(cable_id);
            if (flags == 255) {
              fprintf(stderr, "%s:%I64u error: cable ID overflow. Your project is too big :(\n", 
                      module->file_name, ip->ln);
              return 1;
            }
          }
          else { // const signal
            if (gt->instr_def[i_id]->dst == 0 && temp_info == 0) {
              flags = 0;
              temp_info = 0;
              cable_id = 0;
              continue;           
            }

            cable_id = temp_info;
            flags &= 1;
          }

          updataGlobalArguments(gt, cable_id, flags);
          flags = 0;
          temp_info = 0;
          cable_id = 0;
          idesc->dst++;
        }
        else {
          fprintf(stderr, "%s:%I64u error: invalid name: '%s'\n", 
                  module->file_name, ip->ln, ip->args[i]);
          return 1;
        }
      }

      uint8_t def_arg;
      for (uint32_t i = ip->dst; i < ip->dst + ip->src; i++) { // src arguments loop 
        if (idesc->src == 0) {
          def_arg = gt->instr_def[i_id]->src1_arg;
        }
        else if (idesc->src == 1) {
          def_arg = gt->instr_def[i_id]->src2_arg;
        }
        else {
          def_arg = 1;
          if (gt->instr_def[i_id]->srcx_arg < idesc->src) {
            fprintf(stderr, "%s:%I64u error: invalid cable name: '%s'\n", 
                    module->file_name, ip->ln, ip->args[i]);
            return 1;
          }
        }

        temp_info = argParse(ip->args[i], def_arg);
        if (temp_info == 255) {
          fprintf(stderr, "%s:%I64u error: invalid cable name: '%s'\n", 
                  module->file_name, ip->ln, ip->args[i]);
          return 1;
        }
        else if (temp_info == 6) { // NOT
          flags |= 1;
          continue;
        }
        else if (temp_info <= 4) { // SIG 
          if (temp_info == 4) {
            lc = updataLocalCables(gt, lc, ip->args[i]); // cable
            cable_id = getCableID(lc, ip->args[i]);
            flags |= setFlagsCableID(cable_id);
            if (flags == 255) {
              fprintf(stderr, "%s:%I64u error: cable ID overflow. Your project is too big :(\n", 
                      module->file_name, ip->ln);
              return 1;
            }
          }
          else { // const signal
            if (def_arg == 0 && temp_info == 0) {
              flags = 0;
              temp_info = 0;
              cable_id = 0;
              continue;           
            }

            cable_id = temp_info;
            flags &= 1;
          }

          updataGlobalArguments(gt, cable_id, flags);
          flags = 0;
          temp_info = 0;
          cable_id = 0;
          idesc->src++;
        }
        else if (temp_info == 5) { // XSTR
          size_t xstr_sz = strlen(ip->args[i]);
          printf("XSTR %s %I64u ->", ip->args[i], xstr_sz);
          if (xstr_sz > 7) {
            fprintf(stderr, "%s:%I64u warning: name '%s' will be cut to 7 characters\n", 
                    module->file_name, ip->ln, ip->args[i]);
            xstr_sz = 7;
          }
          for (size_t k = 0; k < xstr_sz; k++) {
            cable_id <<= 8;
            cable_id |= (uint64_t)ip->args[i][k];
          }

          printf(" %I64u\n", cable_id);
          flags = setFlagsCableID(cable_id);
          updataGlobalArguments(gt, cable_id, flags);
          flags = 0;
          temp_info = 0;
          cable_id = 0;
          idesc->src++;          
        }
        else {
          fprintf(stderr, "%s:%I64u error: invalid name: '%s'\n", 
                  module->file_name, ip->ln, ip->args[i]);
          return 1;
        }
      }

      if (idesc->src < gt->instr_def[i_id]->min_args) {
        fprintf(stderr, "%s:%I64u warning: too few arguments to instruction '%s'\n", 
                module->file_name, ip->ln, ip->operation);
      }

      break;
    }
    
    case 2: { // module
      // create list linked cables
      CablesLink *new_clink = malloc(sizeof(CablesLink)); 
      if (new_clink == NULL) {
        goto memory_error;
      }

      new_clink->cable_id = malloc(sizeof(uint64_t*));
      if (new_clink->cable_id == NULL) {
        goto memory_error;
      }

      new_clink->flags = malloc(sizeof(uint8_t*));
      if (new_clink->flags == NULL) {
        goto memory_error;
      }

      new_clink->cable_id[0] = 0;
      new_clink->flags[0] = 0;
      new_clink->dst = 0;
      new_clink->src = 0;

      uint8_t flags = 0, temp_info = 0;
      uint64_t cable_id = 0;
      for (uint32_t i = 0; i < ip->dst + ip->src; i++) { // arguments loop
        temp_info = argParse(ip->args[i], 1);
        if (temp_info == 255) {
          fprintf(stderr, "%s:%I64u error: invalid cable name: '%s'\n", 
                  module->file_name, ip->ln, ip->args[i]);
          return 1;
        }
        else if (temp_info == 6) { // NOT
          flags |= 1;
          continue;
        }
        else if (temp_info <= 4) { // SIG 
          if (temp_info == 4) { // cable
            lc = updataLocalCables(gt, lc, ip->args[i]);
            cable_id = getCableID(lc, ip->args[i]);
            flags |= setFlagsCableID(cable_id);
            if (flags == 255) {
              fprintf(stderr, "%s:%I64u error: cable ID overflow. Your project is too big :(\n", 
                      module->file_name, ip->ln);
              return 1;
            }
          }
          else { // const signal
            if (gt->instr_def[i_id]->dst == 0 && temp_info == 0) {
              flags = 0;
              temp_info = 0;
              cable_id = 0;
              continue;           
            }

            cable_id = temp_info;
            flags &= 1;
          }

          size_t i_ncl = new_clink->dst + new_clink->src;
          new_clink->cable_id[i_ncl] = cable_id;
          new_clink->flags[i_ncl] = flags;
          new_clink->cable_id = realloc(new_clink->cable_id, sizeof(uint64_t*) * (i_ncl + 2));
          if (new_clink->flags == NULL) {
            goto memory_error;
          }

          new_clink->flags = realloc(new_clink->flags, sizeof(uint8_t*) * (i_ncl + 2));
          if (new_clink->flags == NULL) {
            goto memory_error;
          }

          new_clink->cable_id[i_ncl + 1] = 0;
          new_clink->flags[i_ncl + 1] = 0;
          flags = 0;
          temp_info = 0;
          cable_id = 0;
          if (i < ip->dst) {
            new_clink->dst++;
          }
          else {
            new_clink->src++;
          }

        }
        else {
          fprintf(stderr, "%s:%I64u error: invalid name: '%s'\n", 
                  module->file_name, ip->ln, ip->args[i]);
          return 1;       
        }
      }

      // synthesize called module
      int err = synthesizer(gt, gt->modules[i_md], new_clink);
      if (err) {
        return 1;
      }

      gt->i_antiloop--;
      break;
    }
    
    case 3: { // IO psuedoop
      if (clink == NULL) {
        fprintf(stderr, "%s:%I64u error: module '%s' has not intermodule io connections\n",
                module->file_name, ip->ln, module->name);
        return 1;
      }

      i_id = 0;
      uint8_t flags = 0, temp_info = 0, io_dst = 0, io_src = 0;
      uint64_t cable_id = 0;
      for (uint32_t i = 0; i < ip->dst + ip->src; i++) { // io arguments loop
        temp_info = argParse(ip->args[i], gt->instr_def[i_id]->dst);
        if (temp_info == 255) {
          fprintf(stderr, "%s:%I64u error: invalid cable name: '%s'\n", 
                  module->file_name, ip->ln, ip->args[i]);
          return 1;
        }
        else if (temp_info == 6) { // NOT
          flags |= 1;
          continue;
        }
        else if (temp_info <= 4) { // SIG 
          if (temp_info == 4) {
            lc = updataLocalCables(gt, lc, ip->args[i]); // cable
            cable_id = getCableID(lc, ip->args[i]);
            flags |= setFlagsCableID(cable_id);
            if (flags == 255) {
              fprintf(stderr, "%s:%I64u error: cable ID overflow. Your project is too big :(\n", 
                      module->file_name, ip->ln);
              return 1;
            }
          }
          else { // const signal
            if (gt->instr_def[i_id]->dst == 0 && temp_info == 0) {
              flags = 0;
              temp_info = 0;
              cable_id = 0;
              continue;           
            }

            cable_id = temp_info;
            flags &= 1;
          }

          // new instruction (BUF) descriptor
          InstructionDesc *idesc = malloc(sizeof(InstructionDesc));
          if (idesc == NULL) {
            goto memory_error;
          } 
    
          gt->instr_desc[gt->i_instr] = idesc;
          gt->i_instr++;
          gt->instr_desc = realloc(gt->instr_desc, sizeof(InstructionDesc*) * (gt->i_instr + 1));
          if (gt->instr_desc == NULL) {
            goto memory_error;
          }
    
          // set descriptor
          idesc->opcode = gt->instr_def[i_id]->opcode;
          idesc->args_offset = gt->global_args_offset;
          idesc->dst = 1; idesc->src = 1;
          
          uint64_t clink_cable_id = 0;
          uint8_t clink_flags = 0;
          if (i < ip->dst) {
            updataGlobalArguments(gt, cable_id, flags);

            clink_cable_id = clink->cable_id[clink->dst + io_dst];
            clink_flags = clink->flags[clink->dst + io_dst];
            updataGlobalArguments(gt, clink_cable_id, clink_flags);
            io_dst++;
          }
          else {
            clink_cable_id = clink->cable_id[io_src];
            clink_flags = clink->flags[io_src];
            updataGlobalArguments(gt, clink_cable_id, clink_flags);

            updataGlobalArguments(gt, cable_id, flags);
            io_src++;
          }

          printf("io_dst:%i | clink->dst:%i | io_src:%i | clink->src:%i\n", io_dst - 1, clink->dst, io_src - 1, clink->src);
          printf("clink_cable_id:%I64i clink_flags:%i\n", clink_cable_id, clink_flags);

          flags = 0;
          temp_info = 0;
          cable_id = 0;
        }
        else {
          fprintf(stderr, "%s:%I64u error: invalid name: '%s'\n", 
                  module->file_name, ip->ln, ip->args[i]);
          return 1;
        
        }
      }

      if (io_dst != clink->src || io_src != clink->dst) {
        fprintf(stderr, "%s:%I64u warning: number of intermodule io connections is invalid\n", 
                module->file_name, ip->ln);
      }  
    }
    }

    ii++;
  }
  
  printf("global_cable_id:%I64u\nglobal_args_offset:%I64u\n", gt->global_cable_id, gt->global_args_offset);
  printf("i_instr:%I64u\ni_gargs:%I64u\ni_antiloop:%I64u\n", gt->i_instr, gt->i_gargs, gt->i_antiloop);



  return 0;

  memory_error:
  fprintf(stderr, "memory error...\n");
  exit(2);
}