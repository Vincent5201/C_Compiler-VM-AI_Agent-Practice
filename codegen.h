#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include "parser.h"

void generate_code(
    Program *program,
    FILE *output
);

#endif