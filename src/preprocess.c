#include "preprocess.h"

#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "array.h"
#include "debug.h"
#include "logger.h"

// TODO: Buffer stack

typedef struct {
    size_t bufferPos;
    size_t numTokens;
} OptState;

typedef struct {
    String name;
    // TODO: Function like macros
    PreprocessTokenList replacementList;
} Macro;

typedef struct {
    size_t numMacros;
    Macro *macros;
} MacroList;

static OptState optSetMark(Buffer *buffer, PreprocessTokenList *list);

static void optRestore(OptState prevState, Buffer *buffer,
                       PreprocessTokenList *list);

static void checkMacroReplacement(PreprocessTokenList *list, MacroList *macros);

static bool parseGroupPart(Buffer *buffer, PreprocessTokenList *list,
                           MacroList *macros);

static bool parseIfDefSection(Buffer *buffer, PreprocessTokenList *list);

// static bool parseElifSection(Buffer *buffer, PreprocessTokenList *list);

static bool parseDefineSection(Buffer *buffer, MacroList *macros);

static bool parseTextLine(Buffer *buffer, PreprocessTokenList *list,
                          MacroList *macros);

static bool parsePPToken(Buffer *buffer, PreprocessTokenList *list);

static bool parseIdentifier(Buffer *buffer, String *outIdent);

static bool parseNumber(Buffer *buffer, PreprocessToken *tok);

static bool consumeIntConstSuffix(Buffer *buffer);

static bool consumeHexFloatExponent(Buffer *buffer);

static bool consumeFloatConstSuffix(Buffer *buff);

static bool consumeDecFloatExponent(Buffer *buffer);

static bool parseNewLine(Buffer *buffer);

static void consumeWhitespaceAndComments(Buffer *buffer);

static bool peekNonNewLineSpace(Buffer *buffer);

bool preprocess(Buffer file, PreprocessTokenList *outList) {
    if (file.bytes == NULL || file.size == 0 || outList == NULL)
        return false;

    MacroList macros = {0};

    PreprocessTokenList list = {0};

    bool result = true;

    file.col = 1;
    file.line = 1;
    file.pos = 0;

    while (file.pos < file.size) {
        if (!parseGroupPart(&file, &list, &macros)) {
            printf("Parsing group part failed: %lu-%d-%c\n", file.pos, *buffCurr(&file),
                *buffCurr(&file));
            result = false;
            break;
        }
    }

    printf("Macros: %lu\n", macros.numMacros);

    if (result) {
        *outList = list;
    }

    return result;
}

#define printableKeyword(keyword) else if (tok.type == PreprocessToken_ ## keyword) {\
    printDebug("Keyword: " # keyword "\n");\
}

#define printableOp(op, text) else if (tok.type == op) {\
    printDebug(text "\n");\
}

void printPreprocessTokens(PreprocessTokenList tokens) {
    printDebug("Tokens: %lu\n", tokens.numTokens);

    for (size_t i = 0; i < tokens.numTokens; i++) {
        PreprocessToken tok = tokens.tokens[i];

        printDebug("  ");

        // TODO: Replace with filename
        printDebug("%s:%lu:%lu ", "", tok.line, tok.col);

        if (tok.type < 127) {
            printDebug("Character: %c\n", tok.type);
        }

        printableKeyword(asm)
        printableKeyword(auto)
        printableKeyword(break)
        printableKeyword(case)
        printableKeyword(const)
        printableKeyword(continue)
        printableKeyword(default)
        printableKeyword(do)
        printableKeyword(else)
        printableKeyword(enum)
        printableKeyword(extern)
        printableKeyword(for)
        printableKeyword(goto)
        printableKeyword(if)
        printableKeyword(inline)
        printableKeyword(register)
        printableKeyword(restrict)
        printableKeyword(return)
        printableKeyword(sizeof)
        printableKeyword(static)
        printableKeyword(struct)
        printableKeyword(switch)
        printableKeyword(typedef)
        printableKeyword(union)
        printableKeyword(volatile)
        printableKeyword(while)

        printableKeyword(alignas)
        printableKeyword(alignof)
        printableKeyword(atomic)
        printableKeyword(generic)
        printableKeyword(noreturn)
        printableKeyword(staticAssert)
        printableKeyword(threadLocal)
        printableKeyword(funcName)

        printableKeyword(void)
        printableKeyword(char)
        printableKeyword(short)
        printableKeyword(int)
        printableKeyword(long)
        printableKeyword(float)
        printableKeyword(double)
        printableKeyword(signed)
        printableKeyword(unsigned)
        printableKeyword(bool)
        printableKeyword(complex)
        printableKeyword(imaginary)

        printableOp(PreprocessToken_Ellipsis, "...")
        printableOp(PreprocessToken_ShiftRightAssign, ">>=")
        printableOp(PreprocessToken_ShiftLeftAssign, "<<=")
        printableOp(PreprocessToken_AddAssign, "+=")
        printableOp(PreprocessToken_SubAssign, "-=")
        printableOp(PreprocessToken_MulAssign, "*=")
        printableOp(PreprocessToken_DivAssign, "/=")
        printableOp(PreprocessToken_ModAssign, "%%=")
        printableOp(PreprocessToken_AndAssign, "&=")
        printableOp(PreprocessToken_XorAssign, "^=")
        printableOp(PreprocessToken_OrAssign, "|=")
        printableOp(PreprocessToken_ShiftRightOp, ">>")
        printableOp(PreprocessToken_ShiftLeftOp, "<<")
        printableOp(PreprocessToken_IncOp, "++")
        printableOp(PreprocessToken_DecOp, "--")
        printableOp(PreprocessToken_PtrOp, "->")
        printableOp(PreprocessToken_LogAndOp, "&&")
        printableOp(PreprocessToken_LogOrOp, "||")
        printableOp(PreprocessToken_LEqOp, "<=")
        printableOp(PreprocessToken_GEqOp, ">=")
        printableOp(PreprocessToken_EqOp, "==")
        printableOp(PreprocessToken_NEqOp, "!=")

        else if (tok.type == PreprocessToken_Ident) {
            printDebug("Ident: %.*s\n", astr_format(tok.ident));
        }
        else if (tok.type == PreprocessToken_ConstNumeric) {
            printDebug("Number: %.*s\n", astr_format(tok.constNumeric));
        }
        else if (tok.type == PreprocessToken_ConstString) {
            printDebug("String: %.*s\n", astr_format(tok.constString));
        }
        else {
            logFatal("Lexer: Invalid token type when printing: %ld\n", tok.type);
            assert(false);
        }
    }
}

static OptState optSetMark(Buffer *buffer, PreprocessTokenList *list) {
    return (OptState){ .bufferPos = buffer->pos, .numTokens = list->numTokens };
}

static void optRestore(OptState prevState, Buffer *buffer,
                       PreprocessTokenList *list)
{
    buffer->pos = prevState.bufferPos;
    list->numTokens = prevState.numTokens;
}

static void checkMacroReplacement(PreprocessTokenList *list, MacroList *macros) {
    PreprocessToken token = list->tokens[list->numTokens - 1];

    // TODO: Search for Arguments

    if (token.type == PreprocessToken_Ident) {
        for (size_t i = 0; i < macros->numMacros; i++) {
            Macro macro = macros->macros[i];
            if (astr_cmp(token.ident, macro.name)) {
                list->numTokens--;

                PreprocessTokenList replacements = macro.replacementList;

                for (size_t ii = 0; ii < replacements.numTokens; ii++) {
                    PreprocessToken token = replacements.tokens[ii];
                    ArrayAppend(list->tokens, list->numTokens, token);
                }

                break;
            }
        }
    }
}

static bool parseGroupPart(Buffer *buffer, PreprocessTokenList *list,
                           MacroList *macros)
{
    bool result = true;

    if (consumeMultiIf(buffer, "#ifdef")) {
        result = parseIfDefSection(buffer, list);
    }
    else if (consumeMultiIf(buffer, "#ifndef")) {

    }
    else if (consumeMultiIf(buffer, "#if")) {

    }
    else if (consumeMultiIf(buffer, "#include")) {

    }
    else if (consumeMultiIf(buffer, "#define")) {
        result = parseDefineSection(buffer, macros);
    }
    else if (consumeMultiIf(buffer, "#undef")) {

    }
    else if (consumeMultiIf(buffer, "#line")) {

    }
    else if (consumeMultiIf(buffer, "#error")) {

    }
    else if (consumeMultiIf(buffer, "#pragma")) {

    }
    else if (consumeIf(buffer, '#')) {

    }
    else {
        result = parseTextLine(buffer, list, macros);
    }

    return result;
}

static bool parseIfDefSection(Buffer *buffer, PreprocessTokenList *list) {
    // Identifier
    // if (!parseIdentifier(buffer, list))
    //     return false;

    // New line
    // if (!parseNewLine(buffer))
    //     return false;

    // Optional group
    // OptState groupState = optSetMark(buffer, list);

    // if (!parseGroupPart(buffer, list))
    //     optRestore(groupState, buffer, list);

    // Optional elif groups
    // while (true) {
    //     OptState elifState = optSetMark(buffer, list);
    //     if (!parseElifGroup(buffer, list)) {
    //         optRestore(elifState, buffer, list);
    //         break;
    //     }
    // }

    // Optional else group
    // OptState elseState = optSetMark(buffer, list);

    // if (!parseElseGroup(buffer, list))
    //     optRestore(elseState, buffer, list);

    // End if line
    // if (!parseEndifLine(buffer, list))
    //     return false;

    return true;
}

static bool parseDefineSection(Buffer *buffer, MacroList *macros) {
    // Parse an identifier
    String identifier = {0};

    consumeWhitespaceAndComments(buffer);

    if (!parseIdentifier(buffer, &identifier))
        return false;

    // TODO: Parse optional identifier list

    // Parse a replacement list
    PreprocessTokenList list = {0};

    // TODO: Multi line replacement lists

    while (true) {
        OptState mark = optSetMark(buffer, &list);

        consumeWhitespaceAndComments(buffer);

        if (!parsePPToken(buffer, &list)) {
            optRestore(mark, buffer, &list);
            break;
        }

        consumeWhitespaceAndComments(buffer);
    }

    Macro macro = { .name = identifier, .replacementList = list };

    ArrayAppend(macros->macros, macros->numMacros, macro);

    // Parse a new line
    return parseNewLine(buffer);
}

static bool parseTextLine(Buffer *buffer, PreprocessTokenList *list,
                          MacroList *macros) {
    while (true) {
        OptState mark = optSetMark(buffer, list);

        consumeWhitespaceAndComments(buffer);

        if (!parsePPToken(buffer, list)) {
            optRestore(mark, buffer, list);
            break;
        }

        checkMacroReplacement(list, macros);

        consumeWhitespaceAndComments(buffer);
    }

    return parseNewLine(buffer);
}

// Check that next characters are keyword and char after isn't a valid ident
// character
#define Keyword(name, tokType) else if (peekMulti(buffer, name) &&\
    (\
        (peekAhead(buffer, strlen(name)) != '_') &&\
        (!isalnum(peekAhead(buffer, strlen(name))))\
    )\
) {\
    consumeMultiIf(buffer, name);\
    tok.type = tokType;\
    result = true;\
}

#define TripleCharacterOp(op, tokType) else if (consumeMultiIf(buffer, op)) {\
    tok.type = tokType;\
    result = true;\
}

#define DoubleCharacterOp(op, tokType) else if (consumeMultiIf(buffer, op)) {\
    tok.type = tokType;\
    result = true;\
}

#define SingleCharacterOp(op) else if (consumeIf(buffer, op )) {\
    tok.type = op;\
    result = true;\
}

static bool parsePPToken(Buffer *buffer, PreprocessTokenList *list) {
    bool result = false;

    PreprocessToken tok = {0};

    tok.line = buffer->line;
    tok.col = buffer->col;

    // Constant
    if (isdigit(peek(buffer))) {
        result = parseNumber(buffer, &tok);
    }

    // Types
    Keyword("void", PreprocessToken_void)
    Keyword("char", PreprocessToken_char)
    Keyword("short", PreprocessToken_short)
    Keyword("int", PreprocessToken_int)
    Keyword("long", PreprocessToken_long)
    Keyword("float", PreprocessToken_float)
    Keyword("double", PreprocessToken_double)
    Keyword("signed", PreprocessToken_signed)
    Keyword("unsigned", PreprocessToken_unsigned)
    Keyword("_Bool", PreprocessToken_bool)
    Keyword("_Complex", PreprocessToken_complex)
    Keyword("_Imaginary", PreprocessToken_imaginary)

    // Keywords
    Keyword("asm", PreprocessToken_asm)
    Keyword("__asm__", PreprocessToken_asm) // GCC uses __asm__ same as asm
    Keyword("auto", PreprocessToken_auto)
    Keyword("break", PreprocessToken_break)
    Keyword("case", PreprocessToken_case)
    Keyword("const", PreprocessToken_const)
    Keyword("continue", PreprocessToken_continue)
    Keyword("default", PreprocessToken_default)
    Keyword("do", PreprocessToken_do)
    Keyword("else", PreprocessToken_else)
    Keyword("enum", PreprocessToken_enum)
    Keyword("extern", PreprocessToken_extern)
    Keyword("for", PreprocessToken_for)
    Keyword("goto", PreprocessToken_goto)
    Keyword("if", PreprocessToken_if)
    Keyword("inline", PreprocessToken_inline)
    Keyword("__inline", PreprocessToken_inline) // GCC uses __inline same as inline
    Keyword("__inline__", PreprocessToken_inline) // GCC uses __inline__ same as inline
    Keyword("register", PreprocessToken_register)
    Keyword("restrict", PreprocessToken_restrict)
    Keyword("__restrict", PreprocessToken_restrict) // GCC uses __restrict the same as restrict
    Keyword("__restrict__", PreprocessToken_restrict) // GCC uses __restrict__ the same as restrict
    Keyword("return", PreprocessToken_return)
    Keyword("sizeof", PreprocessToken_sizeof)
    Keyword("static", PreprocessToken_static)
    Keyword("struct", PreprocessToken_struct)
    Keyword("switch", PreprocessToken_switch)
    Keyword("typedef", PreprocessToken_typedef)
    Keyword("union", PreprocessToken_union)
    Keyword("volatile", PreprocessToken_volatile)
    Keyword("while", PreprocessToken_while)

    Keyword("_Alignas", PreprocessToken_alignas)
    Keyword("_Alignof", PreprocessToken_alignof)
    Keyword("_Atomic", PreprocessToken_atomic)
    Keyword("_Generic", PreprocessToken_generic)
    Keyword("_Noreturn", PreprocessToken_noreturn)
    Keyword("_Static_assert", PreprocessToken_staticAssert)
    Keyword("_Thread_local", PreprocessToken_threadLocal)
    Keyword("__func__", PreprocessToken_funcName)

    // Identifier
    else if (peek(buffer) == '_' || isalpha(peek(buffer))) {
        result = parseIdentifier(buffer, &tok.ident);

        if (result)
            tok.type = PreprocessToken_Ident;
    }

    // String Literal
    else if (consumeIf(buffer, '"')) {
        uint8_t *bytes = buffCurr(buffer);

        while (peek(buffer) != '"') {
            if (peek(buffer) == '\\')
                consume(buffer);

            consume(buffer);
        }

        tok.type = PreprocessToken_ConstString;
        tok.constString = (String) {
            .str = bytes,
            .length = buffCurr(buffer) - bytes
        };

        // Get the last "
        consume(buffer);

        // TODO: Should we handle strings next to each other here?
        // We currently do it in the parser
        result = true;
    }

    // Punctuator
    TripleCharacterOp("...", PreprocessToken_Ellipsis)
    TripleCharacterOp(">>=", PreprocessToken_ShiftRightAssign)
    TripleCharacterOp("<<=", PreprocessToken_ShiftLeftAssign)

    DoubleCharacterOp("+=", PreprocessToken_AddAssign)
    DoubleCharacterOp("-=", PreprocessToken_SubAssign)
    DoubleCharacterOp("*=", PreprocessToken_MulAssign)
    DoubleCharacterOp("/=", PreprocessToken_DivAssign)
    DoubleCharacterOp("%=", PreprocessToken_ModAssign)
    DoubleCharacterOp("&=", PreprocessToken_AndAssign)
    DoubleCharacterOp("^=", PreprocessToken_XorAssign)
    DoubleCharacterOp("|=", PreprocessToken_OrAssign)
    DoubleCharacterOp(">>", PreprocessToken_ShiftRightOp)
    DoubleCharacterOp("<<", PreprocessToken_ShiftLeftOp)
    DoubleCharacterOp("++", PreprocessToken_IncOp)
    DoubleCharacterOp("--", PreprocessToken_DecOp)
    DoubleCharacterOp("->", PreprocessToken_PtrOp)
    DoubleCharacterOp("&&", PreprocessToken_LogAndOp)
    DoubleCharacterOp("||", PreprocessToken_LogOrOp)
    DoubleCharacterOp("<=", PreprocessToken_LEqOp)
    DoubleCharacterOp(">=", PreprocessToken_GEqOp)
    DoubleCharacterOp("==", PreprocessToken_EqOp)
    DoubleCharacterOp("!=", PreprocessToken_NEqOp)

    SingleCharacterOp(';')
    SingleCharacterOp('{')
    SingleCharacterOp('}')
    SingleCharacterOp(',')
    SingleCharacterOp(':')
    SingleCharacterOp('=')
    SingleCharacterOp('(')
    SingleCharacterOp(')')
    SingleCharacterOp('[')
    SingleCharacterOp(']')
    SingleCharacterOp('.')
    SingleCharacterOp('&')
    SingleCharacterOp('!')
    SingleCharacterOp('~')
    SingleCharacterOp('-')
    SingleCharacterOp('+')
    SingleCharacterOp('*')
    SingleCharacterOp('/')
    SingleCharacterOp('%')
    SingleCharacterOp('<')
    SingleCharacterOp('>')
    SingleCharacterOp('^')
    SingleCharacterOp('|')
    SingleCharacterOp('?')

    ArrayAppend(list->tokens, list->numTokens, tok);

    return result;
}

static bool parseIdentifier(Buffer *buffer, String *outIdent) {
    String string = { .str =buffCurr(buffer) };

    if (peek(buffer) != '_' && !isalpha(peek(buffer)))
        return false;

    consume(buffer);

    while (peek(buffer) == '_' || isalnum(peek(buffer))) {
        consume(buffer);
    }

    string.length = buffCurr(buffer) - string.str;

    *outIdent = string;

    return true;
}

// TODO: Refactor this function to make it cleaner
static bool parseNumber(Buffer *buffer, PreprocessToken *tok) {
    uint8_t *numeric = buffCurr(buffer);

    bool lookForFloat = false;
    bool isHex = false;

    if (consumeMultiIf(buffer, "0x") || consumeMultiIf(buffer, "0X")) {
        // Hex
        isHex = true;

        while (isxdigit(peek(buffer))) {
            consume(buffer);
        }

        if (!consumeIntConstSuffix(buffer)) {
            // Try to look for floats
            lookForFloat = true;
        }
    }
    // We explicitly ignore octal numbers because it makes it easier to
    // parse. We assume that the user has compiled the code, and that
    // it works.
    else {
        // Decimal
        while (isdigit(peek(buffer))) {
            consume(buffer);
        }

        if (!consumeIntConstSuffix(buffer)) {
            // Try to look for floats
            lookForFloat = true;
        }
    }

    if (lookForFloat && isHex) {
        if (peek(buffer) == '.') {
            consume(buffer);

            while (isxdigit(peek(buffer))) {
                consume(buffer);
            }

            if (!consumeHexFloatExponent(buffer)) {
                // TODO: Add in the file name
                printf("%s:%ld\n", "", buffer->line);
                assert(false);
            }
            consumeFloatConstSuffix(buffer);
        }
    }
    else if (lookForFloat) {
        if (peek(buffer) == '.') {
            consume(buffer);

            while (isdigit(peek(buffer))) {
                consume(buffer);
            }

            consumeDecFloatExponent(buffer);

            consumeFloatConstSuffix(buffer);
        }
    }

    size_t length = buffCurr(buffer) - numeric;

    tok->type = PreprocessToken_ConstNumeric;
    tok->constNumeric = (String) {
        .str = numeric,
        .length = length
    };

    return true;
}

static bool consumeIntConstSuffix(Buffer *buffer) {
    if (tolower(peek(buffer)) == 'u') {
        consume(buffer);

        if (tolower(peek(buffer)) == 'l')
            consume(buffer);

        if (tolower(peek(buffer)) == 'l')
            consume(buffer);

        return true;
    }
    else if (tolower(peek(buffer)) == 'l') {
        consume(buffer);

        if (tolower(peek(buffer)) == 'l')
            consume(buffer);

        if (tolower(peek(buffer)) == 'u')
            consume(buffer);

        return true;
    }

    return false;
}

static bool consumeFloatConstSuffix(Buffer *buffer) {
    if (tolower(peek(buffer)) == 'f') {
        consume(buffer);
        return true;
    }

    if (tolower(peek(buffer)) == 'l') {
        consume(buffer);
        return true;
    }

    return false;
}

static bool consumeHexFloatExponent(Buffer *buffer) {
    if (tolower(peek(buffer)) == 'p') {
        consume(buffer);

        consumeIf(buffer, '-');
        consumeIf(buffer, '+');

        while (isdigit(buffer)) {
            consume(buffer);
        }

        return true;
    }

    return false;
}

static bool consumeDecFloatExponent(Buffer *buffer) {
    if (tolower(peek(buffer)) == 'e') {
        consume(buffer);

        consumeIf(buffer, '-');
        consumeIf(buffer, '+');

        while (isdigit(buffer)) {
            consume(buffer);
        }

        return true;
    }

    return false;
}

static bool parseNewLine(Buffer *buffer) {
    consumeIf(buffer, '\r');

    return consumeIf(buffer, '\n');
}

static void consumeWhitespaceAndComments(Buffer *buffer) {
    bool foundConsumable = false;

    do {
        foundConsumable = true;

        if (peekMulti(buffer, "/*")) {
            while (!peekMulti(buffer, "*/"))
                consume(buffer);

            consumeMulti(buffer, 2);
        }
        else if (peekMulti(buffer, "//")) {
            while (peek(buffer) != '\r' && peek(buffer) != '\n')
                consume(buffer);
        }
        else if (peekNonNewLineSpace(buffer)) {
            while (peekNonNewLineSpace(buffer))
                consume(buffer);
        }
        else {
            foundConsumable = false;
        }

    } while (foundConsumable);
}

static bool peekNonNewLineSpace(Buffer *buffer) {
    return isspace(peek(buffer)) && peek(buffer) != '\r' &&
        peek(buffer) != '\n';
}
