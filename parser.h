#ifndef PARSER_H
#define PARSER_H

#include "scanner.h"

typedef enum {
    EXPR_NUMBER,
    EXPR_VARIABLE,
    EXPR_BINARY,
    EXPR_CALL
} ExprType;

typedef struct Expr Expr;

struct Expr {
    ExprType type;

    int value;

    char name[64];
    char op[4];

    Expr *left;
    Expr *right;

    Expr **args;
    int arg_count;
};

typedef enum {
    STMT_VAR_DECL,
    STMT_ASSIGN,
    STMT_RETURN,
    STMT_IF,
    STMT_WHILE
} StatementType;

typedef struct Statement Statement;

struct Statement {
    StatementType type;

    char name[64];

    Expr *expr;

    Expr *condition;

    Statement **body;
    int body_count;

    Statement **else_body;
    int else_count;
};

typedef struct {
    char name[64];

    char **params;
    int param_count;

    Statement **body;
    int body_count;

} Function;

typedef struct {
    Function **functions;
    int function_count;
} Program;

Program *parse(TokenList *tokens);

void free_program(Program *program);

#endif