#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codegen.h"

/* =========================================================
 * Register Information
 * ========================================================= */

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
 * Stack Variable
 * ========================================================= */

typedef struct {
    char name[64];
    int offset;
} Variable;

static Variable variables[MAX_VARIABLES];
static int variable_count;

static int stack_size;
static int label_count;

/* =========================================================
 * Labels
 * ========================================================= */

static int new_label(void)
{
    return label_count++;
}

/* =========================================================
 * Variable Management
 * ========================================================= */

static void reset_variables(void)
{
    variable_count = 0;
    stack_size = 0;
}

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

static void add_variable(const char *name)
{
    if (find_variable(name) != -1)
        return;

    if (variable_count >= MAX_VARIABLES) {
        printf("Too many local variables.\n");
        exit(1);
    }

    /*
     * Each int is conceptually 8 bytes here.
     *
     * This is a simplified compiler.
     */
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

        /*
         * Variables inside if
         */
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

        /*
         * Variables inside while
         */
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
 * Result is always stored in RAX.
 * ========================================================= */

static void generate_expression(
    Expr *expr,
    FILE *out)
{
    int i;

    if (expr == NULL)
        return;

    /*
     * Integer
     */
    if (expr->type == EXPR_NUMBER) {

        fprintf(
            out,
            "    mov rax, %d\n",
            expr->value
        );

        return;
    }

    /*
     * Variable
     */
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

    /*
     * Binary expression
     *
     * left OP right
     */
    if (expr->type == EXPR_BINARY) {

        /*
         * Generate left.
         *
         * Result:
         *     RAX
         */
        generate_expression(
            expr->left,
            out
        );

        /*
         * Save left on stack.
         */
        fprintf(
            out,
            "    push rax\n"
        );

        /*
         * Generate right.
         *
         * Result:
         *     RAX
         */
        generate_expression(
            expr->right,
            out
        );

        /*
         * Right → RCX
         */
        fprintf(
            out,
            "    mov rcx, rax\n"
        );

        /*
         * Left → RAX
         */
        fprintf(
            out,
            "    pop rax\n"
        );

        /*
         * Arithmetic
         */
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

            /*
             * RAX / RCX
             */
            fprintf(
                out,
                "    cqo\n"
            );

            fprintf(
                out,
                "    idiv rcx\n"
            );
        }

        /*
         * Comparison
         */
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

            /*
             * Convert AL:
             *
             * 0 / 1
             *
             * into RAX.
             */
            fprintf(
                out,
                "    movzx rax, al\n"
            );
        }

        return;
    }

    /*
     * Function call
     */
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

        /*
         * Evaluate each argument.
         *
         * This simple version assumes
         * arguments are simple expressions.
         */
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

        /*
         * Function return value is already
         * in RAX.
         */

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

        /*
         * Variable declaration
         */
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

        /*
         * Assignment
         *
         * x = expression;
         */
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

        /*
         * return
         */
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

        /*
         * if
         */
        if (stmt->type == STMT_IF) {

            int else_label;
            int end_label;

            else_label = new_label();
            end_label = new_label();

            /*
             * condition → RAX
             */
            generate_expression(
                stmt->condition,
                out
            );

            fprintf(
                out,
                "    cmp rax, 0\n"
            );

            fprintf(
                out,
                "    je .L%d\n",
                else_label
            );

            /*
             * then
             */
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

            /*
             * else
             */
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

        /*
         * while
         */
        if (stmt->type == STMT_WHILE) {

            int begin_label;
            int end_label;

            begin_label = new_label();
            end_label = new_label();

            /*
             * Loop begin
             */
            fprintf(
                out,
                ".L%d:\n",
                begin_label
            );

            /*
             * condition → RAX
             */
            generate_expression(
                stmt->condition,
                out
            );

            fprintf(
                out,
                "    cmp rax, 0\n"
            );

            fprintf(
                out,
                "    je .L%d\n",
                end_label
            );

            /*
             * body
             */
            generate_statements(
                stmt->body,
                stmt->body_count,
                out
            );

            /*
             * repeat
             */
            fprintf(
                out,
                "    jmp .L%d\n",
                begin_label
            );

            /*
             * end
             */
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

static void generate_function(
    Function *function,
    FILE *out)
{
    int i;

    /*
     * Reset function-specific information
     */
    reset_variables();

    /*
     * Parameters occupy stack slots.
     */
    for (i = 0;
         i < function->param_count;
         i++) {

        add_variable(
            function->params[i]
        );
    }

    /*
     * Local variables
     */
    collect_variables(
        function->body,
        function->body_count
    );

    /*
     * Stack alignment.
     *
     * Keep it 16-byte aligned.
     */
    if (stack_size % 16 != 0)
        stack_size += 16 - (stack_size % 16);

    /*
     * Function label
     */
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

    /*
     * Function prologue
     */
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

    /*
     * Save parameters into their stack slots.
     */
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

    /*
     * Function body
     */
    generate_statements(
        function->body,
        function->body_count,
        out
    );

    /*
     * Default return value
     */
    fprintf(
        out,
        "    mov rax, 0\n"
    );

    /*
     * Function epilogue
     */
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

    /*
     * GAS Intel syntax
     */
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