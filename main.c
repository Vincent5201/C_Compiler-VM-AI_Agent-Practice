#include <stdio.h>
#include "compiler.h"

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Usage: mini_cc <input.c>\n");
        return 1;
    }

    if (compile_file(argv[1]) != 0) {
        printf("Compilation failed.\n");
        return 1;
    }

    printf("Compilation successful.\n");
    return 0;
}