#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <stdint.h>

/*
 * Executes the assembly program specified by filename.
 * Returns the return value of main() as a 64-bit signed integer.
 */
int64_t run_interpreter(const char *filename);

#endif /* INTERPRETER_H */
