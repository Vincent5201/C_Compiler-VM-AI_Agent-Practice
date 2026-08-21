#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codegen.h"

/* =========================================================
 * Register Information
 * ========================================================= */

/* x86-64 System V ABI argument registers used for passing the first 6 parameters */
static const char *argument_registers[] = {
    "rdi",
    "rsi",
    "rdx",
    "rcx",
    "r8",
    "r9"
};

#define MAX_ARGUMENTS 6
#define MAX_VARIABLES 256

/* =========================================================
 * Stack Variable Struct
 * ========================================================= */

typedef struct {
    char name[64];
    int offset; /* Negative offset from RBP */
} Variable;

static Variable variables[MAX_VARIABLES];
static int variable_count;

static int stack_size; /* Aggregated stack usage of current function */
static int label_count; /* Global counter for unique jump labels */

/* =========================================================
 * Labels
 * ========================================================= */

/* Returns a globally unique branch label index (e.g. .L0, .L1...) */
static int new_label(void)
{
    return label_count++;
}

/* =========================================================
 * Variable Management
 * ========================================================= */

/* Resets state before compiling a new function */
static void reset_variables(void)
{
    variable_count = 0;
    stack_size = 0;
}

/* Returns the RBP-relative stack offset for a variable, or -1 if not declared */
static int find_variable(const char *name)
{
    int i;

    for (i = 0; i < variable_count; i++) {

        if (strcmp(
                variables[i].name,
                name
            ) == 0) {

            return variables[i].offset;
        }
    }

    return -1;
}

/* Allocates a stack offset (8-byte alignment) for a variable */
static void add_variable(const char *name)
{
    if (find_variable(name) != -1)
        return;

    if (variable_count >= MAX_VARIABLES) {
        printf("Too many local variables.\n");
        exit(1);
    }

    /* Allocate exactly 8 bytes (64-bit integer standard in this toy compiler) */
    stack_size += 8;

    strcpy(
        variables[variable_count].name,
        name
    );

    variables[variable_count].offset =
        stack_size;

    variable_count++;
}

/* =========================================================
 * Scan Statements for Local Variables
 * ========================================================= */

/* Traverses the AST nested structures to pre-allocate stack locations for all variables */
static void collect_variables(
    Statement **statements,
    int count)
{
    int i;

    for (i = 0; i < count; i++) {

        Statement *stmt = statements[i];

        if (stmt->type == STMT_VAR_DECL) {

            add_variable(stmt->name);
        }

        /* Collect inside conditional branching structures */
        if (stmt->type == STMT_IF) {

            collect_variables(
                stmt->body,
                stmt->body_count
            );

            collect_variables(
                stmt->else_body,
                stmt->else_count
            );
        }

        /* Collect inside loop structures */
        if (stmt->type == STMT_WHILE) {

            collect_variables(
                stmt->body,
                stmt->body_count
            );
        }
    }
}

/* =========================================================
 * Assembly Memory Operand
 * ========================================================= */

static void print_memory(
    FILE *out,
    int offset)
{
    fprintf(
        out,
        "QWORD PTR [rbp-%d]",
        offset
    );
}

/* =========================================================
 * Expression Generation
 *
 * Implements a classic stack-machine code generator. Evaluated
 * expression results are always produced or left inside the RAX register.
 * ========================================================= */

static void generate_expression(
    Expr *expr,
    FILE *out)
{
    int i;

    if (expr == NULL)
        return;

    /* Base Case: Numeric Constant */
    if (expr->type == EXPR_NUMBER) {

        fprintf(
            out,
            "    mov rax, %d\n",
            expr->value
        );

        return;
    }

    /* Base Case: Local Variable Reference */
    if (expr->type == EXPR_VARIABLE) {

        int offset;

        offset = find_variable(
            expr->name
        );

        if (offset == -1) {

            fprintf(
                out,
                "    ; ERROR: unknown variable %s\n",
                expr->name
            );

            fprintf(
                out,
                "    mov rax, 0\n"
            );

            return;
        }

        fprintf(out, "    mov rax, ");

        print_memory(out, offset);

        fprintf(out, "\n");

        return;
    }

    /* Recursive Case: Binary Operator */
    if (expr->type == EXPR_BINARY) {

        /* Evaluate left side first (Result in RAX) */
        generate_expression(
            expr->left,
            out
        );

        /* Push left RAX value onto the stack */
        fprintf(
            out,
            "    push rax\n"
        );

        /* Evaluate right side (Result in RAX) */
        generate_expression(
            expr->right,
            out
        );

        /* Move right result into RCX */
        fprintf(
            out,
            "    mov rcx, rax\n"
        );

        /* Pop left result back into RAX */
        fprintf(
            out,
            "    pop rax\n"
        );

        /* Perform requested arithmetic instruction */
        if (strcmp(expr->op, "+") == 0) {

            fprintf(
                out,
                "    add rax, rcx\n"
            );
        }

        else if (strcmp(expr->op, "-") == 0) {

            fprintf(
                out,
                "    sub rax, rcx\n"
            );
        }

        else if (strcmp(expr->op, "*") == 0) {

            fprintf(
                out,
                "    imul rax, rcx\n"
            );
        }

        else if (strcmp(expr->op, "/") == 0) {

            /* Sign-extend RAX into RDX before executing idiv */
            fprintf(
                out,
                "    cqo\n"
            );

            fprintf(
                out,
                "    idiv rcx\n"
            );
        }

        /* Perform comparative operations using setcc instruction */
        else {

            fprintf(
                out,
                "    cmp rax, rcx\n"
            );

            if (strcmp(expr->op, "==") == 0) {

                fprintf(
                    out,
                    "    sete al\n"
                );
            }

            else if (
                strcmp(expr->op, "!=") == 0
            ) {

                fprintf(
                    out,
                    "    setne al\n"
                );
            }

            else if (
                strcmp(expr->op, "<") == 0
            ) {

                fprintf(
                    out,
                    "    setl al\n"
                );
            }

            else if (
                strcmp(expr->op, ">") == 0
            ) {

                fprintf(
                    out,
                    "    setg al\n"
                );
            }

            else if (
                strcmp(expr->op, "<=") == 0
            ) {

                fprintf(
                    out,
                    "    setle al\n"
                );
            }

            else if (
                strcmp(expr->op, ">=") == 0
            ) {

                fprintf(
                    out,
                    "    setge al\n"
                );
            }

            /* Zero-extend 8-bit AL condition back into 64-bit RAX */
            fprintf(
                out,
                "    movzx rax, al\n"
            );
        }

        return;
    }

    /* Recursive Case: Function Call */
    if (expr->type == EXPR_CALL) {

        if (expr->arg_count > MAX_ARGUMENTS) {

            fprintf(
                out,
                "    ; ERROR: too many arguments\n"
            );

            fprintf(
                out,
                "    mov rax, 0\n"
            );

            return;
        }

        /* Evaluate and load parameters sequentially into System V standard ABI registers */
        for (i = 0;
             i < expr->arg_count;
             i++) {

            generate_expression(
                expr->args[i],
                out
            );

            fprintf(
                out,
                "    mov %s, rax\n",
                argument_registers[i]
            );
        }

        fprintf(
            out,
            "    call %s\n",
            expr->name
        );

        return;
    }
}

/* =========================================================
 * Statement Generation
 * ========================================================= */

static void generate_statements(
    Statement **statements,
    int count,
    FILE *out)
{
    int i;

    for (i = 0; i < count; i++) {

        Statement *stmt = statements[i];

        /* Local variable block offset log */
        if (stmt->type == STMT_VAR_DECL) {

            int offset;

            offset = find_variable(
                stmt->name
            );

            fprintf(
                out,
                "    ; int %s at [rbp-%d]\n",
                stmt->name,
                offset
            );

            continue;
        }

        /* Assignment operation */
        if (stmt->type == STMT_ASSIGN) {

            int offset;

            generate_expression(
                stmt->expr,
                out
            );

            offset = find_variable(
                stmt->name
            );

            if (offset == -1) {

                fprintf(
                    out,
                    "    ; ERROR: unknown variable %s\n",
                    stmt->name
                );

                continue;
            }

            fprintf(
                out,
                "    mov "
            );

            print_memory(out, offset);

            fprintf(
                out,
                ", rax\n"
            );

            continue;
        }

        /* Return from function (epilogue translation) */
        if (stmt->type == STMT_RETURN) {

            generate_expression(
                stmt->expr,
                out
            );

            fprintf(
                out,
                "    mov rsp, rbp\n"
            );

            fprintf(
                out,
                "    pop rbp\n"
            );

            fprintf(
                out,
                "    ret\n"
            );

            continue;
        }

        /* Conditional Branch: If Statement */
        if (stmt->type == STMT_IF) {

            int else_label;
            int end_label;

            else_label = new_label();
            end_label = new_label();

            /* Evaluate boolean condition */
            generate_expression(
                stmt->condition,
                out
            );

            fprintf(
                out,
                "    cmp rax, 0\n"
            );

            /* Branch to else block if condition is false */
            fprintf(
                out,
                "    je .L%d\n",
                else_label
            );

            /* Generate statements for "then" block */
            generate_statements(
                stmt->body,
                stmt->body_count,
                out
            );

            fprintf(
                out,
                "    jmp .L%d\n",
                end_label
            );

            /* Generate statements for "else" block */
            fprintf(
                out,
                ".L%d:\n",
                else_label
            );

            generate_statements(
                stmt->else_body,
                stmt->else_count,
                out
            );

            fprintf(
                out,
                ".L%d:\n",
                end_label
            );

            continue;
        }

        /* Iterative Loop: While Statement */
        if (stmt->type == STMT_WHILE) {

            int begin_label;
            int end_label;

            begin_label = new_label();
            end_label = new_label();

            /* Mark the head of loop */
            fprintf(
                out,
                ".L%d:\n",
                begin_label
            );

            /* Evaluate loop condition */
            generate_expression(
                stmt->condition,
                out
            );

            fprintf(
                out,
                "    cmp rax, 0\n"
            );

            /* Break loop if false */
            fprintf(
                out,
                "    je .L%d\n",
                end_label
            );

            /* Evaluate loop body statements */
            generate_statements(
                stmt->body,
                stmt->body_count,
                out
            );

            /* Re-test condition */
            fprintf(
                out,
                "    jmp .L%d\n",
                begin_label
            );

            fprintf(
                out,
                ".L%d:\n",
                end_label
            );

            continue;
        }
    }
}

/* =========================================================
 * Function Generation
 * ========================================================= */

/* Synthesizes assembly code, stack prologue, and epilogue for a single C function */
static void generate_function(
    Function *function,
    FILE *out)
{
    int i;

    reset_variables();

    /* Load function parameters first so they occupy early stack slots */
    for (i = 0;
         i < function->param_count;
         i++) {

        add_variable(
            function->params[i]
        );
    }

    /* Collect all variables declared in the body */
    collect_variables(
        function->body,
        function->body_count
    );

    /* Enforce x86-64 standard 16-byte stack frame alignment requirement */
    if (stack_size % 16 != 0)
        stack_size += 16 - (stack_size % 16);

    fprintf(
        out,
        "\n.globl %s\n",
        function->name
    );

    fprintf(
        out,
        "%s:\n",
        function->name
    );

    /* Prologue: setup standard base pointer */
    fprintf(
        out,
        "    push rbp\n"
    );

    fprintf(
        out,
        "    mov rbp, rsp\n"
    );

    if (stack_size > 0) {

        fprintf(
            out,
            "    sub rsp, %d\n",
            stack_size
        );
    }

    /* Save parameter values passed from registers into stack-based variables */
    for (i = 0;
         i < function->param_count;
         i++) {

        int offset;

        offset = find_variable(
            function->params[i]
        );

        if (i >= MAX_ARGUMENTS)
            break;

        fprintf(
            out,
            "    mov "
        );

        print_memory(out, offset);

        fprintf(
            out,
            ", %s\n",
            argument_registers[i]
        );
    }

    /* Execute the statements inside function body */
    generate_statements(
        function->body,
        function->body_count,
        out
    );

    /* Implicit fallback return value */
    fprintf(
        out,
        "    mov rax, 0\n"
    );

    /* Epilogue: restore base pointer and return */
    fprintf(
        out,
        "    mov rsp, rbp\n"
    );

    fprintf(
        out,
        "    pop rbp\n"
    );

    fprintf(
        out,
        "    ret\n"
    );
}

/* =========================================================
 * Program Generation
 * ========================================================= */

/* Generates GAS-Intel formatted x86-64 assembly instructions for the entire Program AST */
void generate_code(
    Program *program,
    FILE *output)
{
    int i;

    fprintf(
        output,
        "# Mini C Compiler\n"
    );

    fprintf(
        output,
        "# x86-64 Assembly\n"
    );

    fprintf(
        output,
        "# Generated automatically\n\n"
    );

    fprintf(
        output,
        ".intel_syntax noprefix\n\n"
    );

    label_count = 0;

    for (i = 0;
         i < program->function_count;
         i++) {

        generate_function(
            program->functions[i],
            output
        );
    }
}