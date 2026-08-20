#include <stdio.h>
#include <stdlib.h>
#include "compiler.h"
#include "scanner.h"
#include "parser.h"
#include "codegen.h"

int compile_file(const char *filename)
{
    FILE *file;
    long size;
    char *source;

    /*
     * 1. Open source file
     */
    file = fopen(filename, "rb");

    if (file == NULL) {
        printf("Error: cannot open %s\n", filename);
        return 1;
    }

    /*
     * 2. Read entire file
     */
    fseek(file, 0, SEEK_END);
    size = ftell(file);
    rewind(file);

    source = malloc(size + 1);

    if (source == NULL) {
        fclose(file);
        return 1;
    }

    fread(source, 1, size, file);
    source[size] = '\0';

    fclose(file);

    /*
     * 3. Scanner
     */
    TokenList tokens;

    if (scan(source, &tokens) != 0) {
        free(source);
        return 1;
    }

    /*
     * 4. Parser
     */
    Program *program = parse(&tokens);

    if (program == NULL) {
        free_tokens(&tokens);
        free(source);
        return 1;
    }

    /*
     * 5. Output file
     */
    FILE *output = fopen("output.txt", "w");

    if (output == NULL) {
        printf("Error: cannot create output.txt\n");

        free_program(program);
        free_tokens(&tokens);
        free(source);

        return 1;
    }

    /*
     * 6. Generate Assembly
     */
    generate_code(program, output);

    fclose(output);

    /*
     * 7. Cleanup
     */
    free_program(program);
    free_tokens(&tokens);
    free(source);

    return 0;
}