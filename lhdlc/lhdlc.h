#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct InputData {
  char *data;
  char *name;
  size_t sz;
};
typedef struct InputData InputData;

InputData* readFile(char *name);