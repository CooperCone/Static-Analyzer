#pragma once

#include "buffer.h"

typedef enum {
    Token_ConstInt = 256,
    Token_ConstFloat,
    Token_ConstString,
    Token_ConstChar,

    Token_Whitespace,
    Token_Comment,

    Token_int,
    Token_char,

    Token_include,
    Token_return,
} TokenType;

typedef struct {
    TokenType type;
    union {
        char *constInt;
        char *constFloat;
        char *constString;
        char *includePayload;
    };
} Token;

typedef struct {
    size_t numTokens;
    Token *tokens;
} TokenList;

int lexFile(Buffer buffer, TokenList *outTokens);
void printTokens(TokenList tokens);
