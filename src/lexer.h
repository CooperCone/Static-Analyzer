#pragma once

#include <stdbool.h>

#include "buffer.h"

typedef enum {
    Token_ConstInt = 256,
    Token_ConstNumeric,
    Token_ConstString,
    Token_ConstChar,

    Token_Ident = 260,
    Token_Whitespace,
    Token_NewLine,
    Token_Comment,

    Token_int,
    Token_char,

    Token_include,
    Token_return,
} TokenType;

typedef struct {
    TokenType type;
    uint64_t line;
    uint64_t col;

    union {
        char *ident;
        char *whitespace;
        char *constString;
        struct {
            char *numericWhole;
            char *numericDecimal;
            char *numericSuffix;
            char *numericPrefix;
        };
    };
} Token;

typedef struct {
    size_t numTokens;
    Token *tokens;
} TokenList;

bool lexFile(Buffer buffer, TokenList *outTokens);
void printTokens(TokenList tokens);
