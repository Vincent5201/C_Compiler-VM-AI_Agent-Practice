#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

/* =========================================================
 * Parser State
 * ========================================================= */

static TokenList *tokens;
static int current_token;

/* =========================================================
 * Utility
 * ========================================================= */

static Token *current(void)
{
    return &tokens->tokens[current_token];
}

static int check(TokenType type)
{
    return current()->type == type;
}

static Token *advance_token(void)
{
    Token *token = current();

    if (current()->type != TOKEN_EOF)
        current_token++;

    return token;
}

static int match(TokenType type)
{
    if (check(type)) {
        advance_token();
        return 1;
    }

    return 0;
}

static Token *expect(TokenType type)
{
    if (!check(type)) {
        printf(
            "Parser error: expected token type %d, got '%s'\n",
            type,
            current()->text
        );

        return NULL;
    }

    return advance_token();
}

/* =========================================================
 * Memory Helpers
 * ========================================================= */

static Expr *new_expr(void)
{
    Expr *expr = calloc(1, sizeof(Expr));

    if (expr == NULL) {
        printf("Memory allocation error.\n");
        exit(1);
    }

    return expr;
}

static Statement *new_statement(void)
{
    Statement *stmt = calloc(1, sizeof(Statement));

    if (stmt == NULL) {
        printf("Memory allocation error.\n");
        exit(1);
    }

    return stmt;
}

/* =========================================================
 * Expression Parser
 *
 * expression
 *     -> comparison
 *
 * comparison
 *     -> addition (comparison_op addition)*
 *
 * addition
 *     -> multiplication ((+|-) multiplication)*
 *
 * multiplication
 *     -> primary ((*|/) primary)*
 *
 * primary
 *     -> NUMBER
 *     -> IDENTIFIER
 *     -> IDENTIFIER(...)
 *     -> '(' expression ')'
 * ========================================================= */

static Expr *parse_expression(void);

static Expr *parse_primary(void)
{
    Expr *expr;
    Token *token;

    /*
     * Integer
     */
    if (check(TOKEN_NUMBER)) {

        token = advance_token();

        expr = new_expr();

        expr->type = EXPR_NUMBER;
        expr->value = atoi(token->text);

        return expr;
    }

    /*
     * Identifier / function call
     */
    if (check(TOKEN_IDENTIFIER)) {

        token = advance_token();

        /*
         * Function call
         *
         * foo(...)
         */
        if (match(TOKEN_LPAREN)) {

            expr = new_expr();

            expr->type = EXPR_CALL;

            strcpy(expr->name, token->text);

            expr->args = NULL;
            expr->arg_count = 0;

            /*
             * Arguments
             */
            if (!check(TOKEN_RPAREN)) {

                while (1) {

                    Expr *arg;
                    Expr **new_args;

                    arg = parse_expression();

                    new_args = realloc(
                        expr->args,
                        sizeof(Expr *) *
                        (expr->arg_count + 1)
                    );

                    if (new_args == NULL) {
                        printf(
                            "Memory allocation error.\n"
                        );
                        exit(1);
                    }

                    expr->args = new_args;

                    expr->args[
                        expr->arg_count
                    ] = arg;

                    expr->arg_count++;

                    if (!match(TOKEN_COMMA))
                        break;
                }
            }

            expect(TOKEN_RPAREN);

            return expr;
        }

        /*
         * Normal variable
         */
        expr = new_expr();

        expr->type = EXPR_VARIABLE;

        strcpy(expr->name, token->text);

        return expr;
    }

    /*
     * Parenthesized expression
     */
    if (match(TOKEN_LPAREN)) {

        expr = parse_expression();

        expect(TOKEN_RPAREN);

        return expr;
    }

    printf(
        "Parser error: invalid expression near '%s'\n",
        current()->text
    );

    return NULL;
}

static Expr *parse_multiplication(void)
{
    Expr *left;

    left = parse_primary();

    while (
        check(TOKEN_STAR) ||
        check(TOKEN_SLASH)
    ) {
        Token *operator;
        Expr *right;
        Expr *expr;

        operator = advance_token();

        right = parse_primary();

        expr = new_expr();

        expr->type = EXPR_BINARY;

        strcpy(expr->op, operator->text);

        expr->left = left;
        expr->right = right;

        left = expr;
    }

    return left;
}

static Expr *parse_addition(void)
{
    Expr *left;

    left = parse_multiplication();

    while (
        check(TOKEN_PLUS) ||
        check(TOKEN_MINUS)
    ) {
        Token *operator;
        Expr *right;
        Expr *expr;

        operator = advance_token();

        right = parse_multiplication();

        expr = new_expr();

        expr->type = EXPR_BINARY;

        strcpy(expr->op, operator->text);

        expr->left = left;
        expr->right = right;

        left = expr;
    }

    return left;
}

static Expr *parse_comparison(void)
{
    Expr *left;

    left = parse_addition();

    while (
        check(TOKEN_EQ) ||
        check(TOKEN_NEQ) ||
        check(TOKEN_LT) ||
        check(TOKEN_GT) ||
        check(TOKEN_LE) ||
        check(TOKEN_GE)
    ) {
        Token *operator;
        Expr *right;
        Expr *expr;

        operator = advance_token();

        right = parse_addition();

        expr = new_expr();

        expr->type = EXPR_BINARY;

        strcpy(expr->op, operator->text);

        expr->left = left;
        expr->right = right;

        left = expr;
    }

    return left;
}

static Expr *parse_expression(void)
{
    return parse_comparison();
}

/* =========================================================
 * Statement List
 * ========================================================= */

static void add_statement(
    Statement ***array,
    int *count,
    Statement *statement)
{
    Statement **new_array;

    new_array = realloc(
        *array,
        sizeof(Statement *) * (*count + 1)
    );

    if (new_array == NULL) {
        printf("Memory allocation error.\n");
        exit(1);
    }

    *array = new_array;

    (*array)[*count] = statement;

    (*count)++;
}

/* =========================================================
 * Block
 * ========================================================= */

static void parse_block_contents(
    Statement ***statements,
    int *count)
{
    expect(TOKEN_LBRACE);

    while (
        !check(TOKEN_RBRACE) &&
        !check(TOKEN_EOF)
    ) {
        Statement *stmt;

        /*
         * Statement parser is defined below.
         */
        extern Statement *parse_statement_for_block(void);

        stmt = parse_statement_for_block();

        if (stmt != NULL)
            add_statement(
                statements,
                count,
                stmt
            );
    }

    expect(TOKEN_RBRACE);
}

/* =========================================================
 * Statements
 * ========================================================= */

Statement *parse_statement_for_block(void)
{
    Statement *stmt;

    /*
     * int x;
     */
    if (match(TOKEN_INT)) {

        Token *name;

        name = expect(TOKEN_IDENTIFIER);

        if (name == NULL)
            return NULL;

        expect(TOKEN_SEMICOLON);

        stmt = new_statement();

        stmt->type = STMT_VAR_DECL;

        strcpy(stmt->name, name->text);

        return stmt;
    }

    /*
     * return expression;
     */
    if (match(TOKEN_RETURN)) {

        Expr *expr;

        expr = parse_expression();

        expect(TOKEN_SEMICOLON);

        stmt = new_statement();

        stmt->type = STMT_RETURN;
        stmt->expr = expr;

        return stmt;
    }

    /*
     * if (condition) { ... }
     */
    if (match(TOKEN_IF)) {

        stmt = new_statement();

        stmt->type = STMT_IF;

        expect(TOKEN_LPAREN);

        stmt->condition = parse_expression();

        expect(TOKEN_RPAREN);

        stmt->body = NULL;
        stmt->body_count = 0;

        stmt->else_body = NULL;
        stmt->else_count = 0;

        parse_block_contents(
            &stmt->body,
            &stmt->body_count
        );

        /*
         * else
         */
        if (match(TOKEN_ELSE)) {

            parse_block_contents(
                &stmt->else_body,
                &stmt->else_count
            );
        }

        return stmt;
    }

    /*
     * while (condition) { ... }
     */
    if (match(TOKEN_WHILE)) {

        stmt = new_statement();

        stmt->type = STMT_WHILE;

        expect(TOKEN_LPAREN);

        stmt->condition = parse_expression();

        expect(TOKEN_RPAREN);

        stmt->body = NULL;
        stmt->body_count = 0;

        parse_block_contents(
            &stmt->body,
            &stmt->body_count
        );

        return stmt;
    }

    /*
     * Assignment
     *
     * x = expression;
     */
    if (check(TOKEN_IDENTIFIER)) {

        Token *name;

        name = advance_token();

        if (!expect(TOKEN_ASSIGN))
            return NULL;

        stmt = new_statement();

        stmt->type = STMT_ASSIGN;

        strcpy(stmt->name, name->text);

        stmt->expr = parse_expression();

        expect(TOKEN_SEMICOLON);

        return stmt;
    }

    printf(
        "Parser error: unknown statement near '%s'\n",
        current()->text
    );

    return NULL;
}

/* =========================================================
 * Function
 * ========================================================= */

static Function *parse_function(void)
{
    Function *function;

    Token *name;

    /*
     * int
     */
    if (!expect(TOKEN_INT))
        return NULL;

    /*
     * function name
     */
    name = expect(TOKEN_IDENTIFIER);

    if (name == NULL)
        return NULL;

    function = calloc(1, sizeof(Function));

    if (function == NULL) {
        printf("Memory allocation error.\n");
        exit(1);
    }

    strcpy(function->name, name->text);

    /*
     * (
     */
    expect(TOKEN_LPAREN);

    /*
     * parameters
     *
     * int a, int b
     */
    if (!check(TOKEN_RPAREN)) {

        while (1) {

            Token *parameter;
            char **new_params;

            expect(TOKEN_INT);

            parameter =
                expect(TOKEN_IDENTIFIER);

            if (parameter == NULL) {
                free(function);
                return NULL;
            }

            new_params = realloc(
                function->params,
                sizeof(char *) *
                (function->param_count + 1)
            );

            if (new_params == NULL) {
                printf(
                    "Memory allocation error.\n"
                );
                exit(1);
            }

            function->params = new_params;

            function->params[
                function->param_count
            ] = malloc(
                strlen(parameter->text) + 1
            );

            strcpy(
                function->params[
                    function->param_count
                ],
                parameter->text
            );

            function->param_count++;

            if (!match(TOKEN_COMMA))
                break;
        }
    }

    expect(TOKEN_RPAREN);

    /*
     * Function body
     */
    function->body = NULL;
    function->body_count = 0;

    parse_block_contents(
        &function->body,
        &function->body_count
    );

    return function;
}

/* =========================================================
 * Program
 * ========================================================= */

Program *parse(TokenList *input_tokens)
{
    Program *program;

    tokens = input_tokens;
    current_token = 0;

    program = calloc(1, sizeof(Program));

    if (program == NULL) {
        printf("Memory allocation error.\n");
        exit(1);
    }

    while (!check(TOKEN_EOF)) {

        Function *function;
        Function **new_functions;

        function = parse_function();

        if (function == NULL) {
            free_program(program);
            return NULL;
        }

        new_functions = realloc(
            program->functions,
            sizeof(Function *) *
            (program->function_count + 1)
        );

        if (new_functions == NULL) {
            printf("Memory allocation error.\n");
            exit(1);
        }

        program->functions = new_functions;

        program->functions[
            program->function_count
        ] = function;

        program->function_count++;
    }

    return program;
}

/* =========================================================
 * Free AST
 * ========================================================= */

static void free_expr(Expr *expr)
{
    int i;

    if (expr == NULL)
        return;

    free_expr(expr->left);
    free_expr(expr->right);

    for (i = 0; i < expr->arg_count; i++)
        free_expr(expr->args[i]);

    free(expr->args);

    free(expr);
}

static void free_statement(Statement *stmt)
{
    int i;

    if (stmt == NULL)
        return;

    free_expr(stmt->expr);
    free_expr(stmt->condition);

    for (i = 0; i < stmt->body_count; i++)
        free_statement(stmt->body[i]);

    for (i = 0; i < stmt->else_count; i++)
        free_statement(stmt->else_body[i]);

    free(stmt->body);
    free(stmt->else_body);

    free(stmt);
}

static void free_function(Function *function)
{
    int i;

    if (function == NULL)
        return;

    for (i = 0; i < function->param_count; i++)
        free(function->params[i]);

    for (i = 0; i < function->body_count; i++)
        free_statement(function->body[i]);

    free(function->params);
    free(function->body);

    free(function);
}

void free_program(Program *program)
{
    int i;

    if (program == NULL)
        return;

    for (i = 0; i < program->function_count; i++)
        free_function(program->functions[i]);

    free(program->functions);

    free(program);
}