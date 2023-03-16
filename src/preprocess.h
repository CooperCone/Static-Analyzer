#pragma once

#include <stdbool.h>
#include <stdlib.h>

#include "buffer.h"
#include "astring.h"

typedef enum {
    PreprocessToken_ConstNumeric = 256,
    PreprocessToken_ConstString,
    PreprocessToken_ConstChar,

    PreprocessToken_Ident,
    PreprocessToken_Whitespace, // 260
    PreprocessToken_NewLine,
    PreprocessToken_Comment,

    PreprocessToken_Ellipsis,
    PreprocessToken_ShiftRightAssign,
    PreprocessToken_ShiftLeftAssign,
    PreprocessToken_AddAssign,
    PreprocessToken_SubAssign,
    PreprocessToken_MulAssign,
    PreprocessToken_DivAssign,
    PreprocessToken_ModAssign, // 270
    PreprocessToken_AndAssign,
    PreprocessToken_XorAssign,
    PreprocessToken_OrAssign,
    PreprocessToken_ShiftRightOp,
    PreprocessToken_ShiftLeftOp,
    PreprocessToken_IncOp,
    PreprocessToken_DecOp,
    PreprocessToken_PtrOp,
    PreprocessToken_LogAndOp,
    PreprocessToken_LogOrOp, // 280
    PreprocessToken_LEqOp,
    PreprocessToken_GEqOp,
    PreprocessToken_EqOp,
    PreprocessToken_NEqOp,

    PreprocessToken_void,
    PreprocessToken_char,
    PreprocessToken_short,
    PreprocessToken_int,
    PreprocessToken_long,
    PreprocessToken_float, // 290
    PreprocessToken_double,
    PreprocessToken_signed,
    PreprocessToken_unsigned,
    PreprocessToken_bool,
    PreprocessToken_complex,
    PreprocessToken_imaginary,

    PreprocessToken_asm,
    PreprocessToken_auto,
    PreprocessToken_break,
    PreprocessToken_case, // 300
    PreprocessToken_const,
    PreprocessToken_continue,
    PreprocessToken_default,
    PreprocessToken_do,
    PreprocessToken_else,
    PreprocessToken_enum,
    PreprocessToken_extern,
    PreprocessToken_for,
    PreprocessToken_goto,
    PreprocessToken_if, // 310
    PreprocessToken_inline,
    PreprocessToken_register,
    PreprocessToken_restrict,
    PreprocessToken_return,
    PreprocessToken_sizeof,
    PreprocessToken_static,
    PreprocessToken_struct,
    PreprocessToken_switch,
    PreprocessToken_typedef,
    PreprocessToken_union, // 320
    PreprocessToken_volatile,
    PreprocessToken_while,

    PreprocessToken_alignas,
    PreprocessToken_alignof,
    PreprocessToken_atomic,
    PreprocessToken_generic,
    PreprocessToken_noreturn,
    PreprocessToken_staticAssert,
    PreprocessToken_threadLocal,
    PreprocessToken_funcName,
} PreprocessTokenType;

typedef struct {
    PreprocessTokenType type;
    size_t line;
    size_t col;
    union {
        String ident;
        String constNumeric;
        String constString;
    };
} PreprocessToken;

typedef struct {
    size_t numTokens;
    PreprocessToken *tokens;
} PreprocessTokenList;

bool preprocess(Buffer file, PreprocessTokenList *outList);

void printPreprocessTokens(PreprocessTokenList tokens);
