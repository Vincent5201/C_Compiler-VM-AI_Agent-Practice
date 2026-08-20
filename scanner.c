#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "scanner.h"

static void add_token(
    TokenList *list,
    TokenType type,
    const char *text)
{
    if (list->count >= list->capacity) {

        list->capacity *= 2;

        list->tokens = realloc(
            list->tokens,
            sizeof(Token) * list->capacity
        );
    }

    list->tokens[list->count].type = type;

    strncpy(
        list->tokens[list->count].text,
        text,
        63
    );

    list->tokens[list->count].text[63] = '\0';

    list->count++;
}

static TokenType keyword_type(const char *word)
{
    if (strcmp(word, "int") == 0)
        return TOKEN_INT;

    if (strcmp(word, "return") == 0)
        return TOKEN_RETURN;

    if (strcmp(word, "if") == 0)
        return TOKEN_IF;

    if (strcmp(word, "else") == 0)
        return TOKEN_ELSE;

    if (strcmp(word, "while") == 0)
        return TOKEN_WHILE;

    return TOKEN_IDENTIFIER;
}

int scan(const char *source, TokenList *list)
{
    int i = 0;

    list->count = 0;
    list->capacity = 128;

    list->tokens =
        malloc(sizeof(Token) * list->capacity);

    while (source[i] != '\0') {

        /*
         * whitespace
         */
        if (isspace(source[i])) {
            i++;
            continue;
        }

        /*
         * identifier / keyword
         */
        if (isalpha(source[i]) || source[i] == '_') {

            char buffer[64];
            int j = 0;

            while (
                isalnum(source[i]) ||
                source[i] == '_'
            ) {
                if (j < 63)
                    buffer[j++] = source[i];

                i++;
            }

            buffer[j] = '\0';

            add_token(
                list,
                keyword_type(buffer),
                buffer
            );

            continue;
        }

        /*
         * number
         */
        if (isdigit(source[i])) {

            char buffer[64];
            int j = 0;

            while (isdigit(source[i])) {

                if (j < 63)
                    buffer[j++] = source[i];

                i++;
            }

            buffer[j] = '\0';

            add_token(
                list,
                TOKEN_NUMBER,
                buffer
            );

            continue;
        }

        /*
         * Two-character operators
         */

        if (source[i] == '=' &&
            source[i + 1] == '=') {

            add_token(list, TOKEN_EQ, "==");
            i += 2;
            continue;
        }

        if (source[i] == '!' &&
            source[i + 1] == '=') {

            add_token(list, TOKEN_NEQ, "!=");
            i += 2;
            continue;
        }

        if (source[i] == '<' &&
            source[i + 1] == '=') {

            add_token(list, TOKEN_LE, "<=");
            i += 2;
            continue;
        }

        if (source[i] == '>' &&
            source[i + 1] == '=') {

            add_token(list, TOKEN_GE, ">=");
            i += 2;
            continue;
        }

        /*
         * One-character tokens
         */

        switch (source[i]) {

        case '+':
            add_token(list, TOKEN_PLUS, "+");
            break;

        case '-':
            add_token(list, TOKEN_MINUS, "-");
            break;

        case '*':
            add_token(list, TOKEN_STAR, "*");
            break;

        case '/':
            add_token(list, TOKEN_SLASH, "/");
            break;

        case '=':
            add_token(list, TOKEN_ASSIGN, "=");
            break;

        case '<':
            add_token(list, TOKEN_LT, "<");
            break;

        case '>':
            add_token(list, TOKEN_GT, ">");
            break;

        case '(':
            add_token(list, TOKEN_LPAREN, "(");
            break;

        case ')':
            add_token(list, TOKEN_RPAREN, ")");
            break;

        case '{':
            add_token(list, TOKEN_LBRACE, "{");
            break;

        case '}':
            add_token(list, TOKEN_RBRACE, "}");
            break;

        case ',':
            add_token(list, TOKEN_COMMA, ",");
            break;

        case ';':
            add_token(
                list,
                TOKEN_SEMICOLON,
                ";"
            );
            break;

        default:
            printf(
                "Scanner error: unknown character '%c'\n",
                source[i]
            );

            free(list->tokens);

            return 1;
        }

        i++;
    }

    add_token(list, TOKEN_EOF, "");

    return 0;
}

void free_tokens(TokenList *list)
{
    free(list->tokens);
}