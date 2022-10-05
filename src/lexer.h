#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "buffer.h"

typedef enum {
    Token_ConstNumeric = 256,
    Token_ConstString,
    Token_ConstChar,

    Token_Ident,
    Token_Whitespace = 260,
    Token_NewLine,
    Token_Comment,

    Token_Ellipsis,
    Token_ShiftRightAssign,
    Token_ShiftLeftAssign,
    Token_AddAssign,
    Token_SubAssign,
    Token_MulAssign,
    Token_DivAssign,
    Token_ModAssign,
    Token_AndAssign,
    Token_XorAssign,
    Token_OrAssign,
    Token_ShiftRightOp,
    Token_ShiftLeftOp,
    Token_IncOp,
    Token_DecOp,
    Token_PtrOp,
    Token_LogAndOp,
    Token_LogOrOp,
    Token_LEqOp,
    Token_GEqOp,
    Token_EqOp,
    Token_NEqOp,

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

    Token_auto,
    Token_break,
    Token_case,
    Token_const,
    Token_continue,
    Token_default,
    Token_do,
    Token_else,
    Token_enum,
    Token_extern,
    Token_for,
    Token_goto,
    Token_if,
    Token_inline,
    Token_register,
    Token_restrict,
    Token_return,
    Token_sizeof,
    Token_static,
    Token_struct,
    Token_switch,
    Token_typedef,
    Token_union,
    Token_volatile,
    Token_while,

    Token_alignas,
    Token_alignof,
    Token_atomic,
    Token_generic,
    Token_noreturn,
    Token_staticAssert,
    Token_threadLocal,
    Token_funcName,
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

typedef struct {
    char *fileName;
    size_t numLines;
    uint64_t *lineLengths;
} FileInfo;

typedef struct {
    size_t numFiles;
    FileInfo *fileInfo;
} LineInfo;

void addLineLengthInfo(LineInfo *info, char *fileName, uint64_t line, uint64_t length);

bool lexFile(Buffer buffer, TokenList *outTokens, LineInfo *outInfo);
void printTokens(TokenList tokens);
