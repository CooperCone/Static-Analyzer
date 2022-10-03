#pragma once

#include <stdbool.h>

#include "buffer.h"

typedef enum {
    Token_ConstNumeric = 256,
    Token_ConstString,
    Token_ConstChar,

    Token_Ident,
    Token_Whitespace = 260,
    Token_NewLine,
    Token_Comment,

    Token_IncOp,
    Token_DecOp,
    Token_PtrOp,
    Token_LogOrOp,
    Token_LogAndOp,
    Token_EqOp,
    Token_NEqOp,
    Token_LEqOp,
    Token_GEqOp,
    Token_ShiftLeftOp,
    Token_ShiftRightOp,
    Token_MulEqOp,
    Token_DivEqOp,
    Token_ModEqOp,
    Token_AddEqOp,
    Token_SubEqOp,
    Token_ShiftLeftEqOp,
    Token_ShiftRightEqOp,
    Token_AndEqOp,
    Token_XorEqOp,
    Token_OrEqOp,

    Token_void,
    Token_char,
    Token_short,
    Token_int,
    Token_long,
    Token_float,
    Token_double,
    Token_signed,
    Token_unsigned,
    Token_bool,
    Token_complex,
    Token_imaginary,

    Token_include,
    Token_return,
    Token_auto,
    Token_register,
    Token_typedef,
    Token_extern,
    Token_static,
    Token_threadLocal,
    Token_atomic,
    Token_const,
    Token_restrict,
    Token_volatile,
    Token_sizeof,
    Token_alignof,
    Token_funcName,
    Token_generic,
    Token_default,
} TokenType;

typedef struct {
    TokenType type;
    uint64_t line;
    uint64_t col;

    union {
        char *ident;
        char *whitespace;
        char *comment;
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
    size_t pos;
    Token *tokens;
} TokenList;

bool lexFile(Buffer buffer, TokenList *outTokens);
void printTokens(TokenList tokens);
