#include "lexer.h"

#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <ctype.h>

#include "debug.h"
#include "array.h"
#include "logger.h"

typedef struct {
    Buffer buffer;
    char *fileName;
} FileContext;

typedef struct {
    size_t stackSize;
    FileContext files[16];
} FileContextStack;

typedef struct {
    String name;
    String value;
} Define;

typedef struct {
    size_t numDefines;
    Define defines[4095];
} Defines;

#define TripleCharacterOp(op, tokType) else if (consumeMultiIf(buff, op)) {\
    tok.type = tokType;\
}

#define DoubleCharacterOp(op, tokType) else if (consumeMultiIf(buff, op)) {\
    tok.type = tokType;\
}

#define SingleCharacterOp(op) else if (consumeIf(buff, op )) {\
    tok.type = op;\
}

// Check that next characters are keyword and char after isn't a valid ident
// character
#define Keyword(name, tokType) else if (peekMulti(buff, name) &&\
    (\
        (peekAhead(buff, strlen(name)) != '_') &&\
        (!isalnum(peekAhead(buff, strlen(name))))\
    )\
) {\
    consumeMultiIf(buff, name);\
    tok.type = tokType;\
}

#define printableKeyword(keyword) else if (tok.type == Token_ ## keyword) {\
    printDebug("Keyword: " # keyword "\n");\
}

#define printableOp(op, text) else if (tok.type == op) {\
    printDebug(text "\n");\
}

static bool runPreprocessor(FileContextStack *fileStack);

static bool tryProcessToken(FileContext *context, TokenList *outTokens,
                            LineInfo *outLines);

static bool fileContextStack_pushFile(FileContextStack *stack, FileContext context);

static void fileContextStack_pop(FileContextStack *stack);

static FileContext *fileContextStack_context(FileContextStack *stack);

static bool consumeIntConstSuffix(Buffer *buff);

static bool consumeFloatConstSuffix(Buffer *buff);

static bool consumeHexFloatExponent(Buffer *buff);

static bool consumeDecFloatExponent(Buffer *buff);

static void consumeWhitespace(Buffer *buff);

static void addLineLengthInfo(LineInfo *info, char *fileName, uint64_t line,
                              uint64_t length);

static void appendTok(TokenList *tokens, Token tok);

bool lexFile(Buffer buffer, char *fileName, TokenList *outTokens, LineInfo *outLines) {
    if (NULL == outTokens || NULL == outLines)
        return false;

    buffer.pos = 0;
    buffer.line = 1;
    buffer.col = 1;

    FileContextStack fileStack = {0};

    FileContext newContext =
    {
        .buffer = buffer,
        .fileName = fileName,
    };

    if (!fileContextStack_pushFile(&fileStack, newContext))
        return false;

    while (fileStack.stackSize > 0) {
        FileContext *context = fileContextStack_context(&fileStack);

        Buffer *buff = &(context->buffer);

        // If we're at the end of the file, pop it
        if (buff->pos >= buff->size)
        {
            fileContextStack_pop(&fileStack);
            continue;
        }

        if (peek(buff) == '#') {
            // Run preprocessor command
            if (!runPreprocessor(&fileStack))
                return false;
        }
        else {
            if (!tryProcessToken(context, outTokens, outLines))
                return false;
        }
    }

    return true;
}

static bool runPreprocessor(FileContextStack *fileStack) {
    if (fileStack == NULL)
        return false;

    FileContext *context = fileContextStack_context(fileStack);

    Buffer *buff = &context->buffer;

    // Compiler Commands
    if (consumeMultiIf(buff, "#pragma")) {
        while (peek(buff) != '\n') {
            consume(buff);
        }
    }

    // Header includes
    else if (consumeMultiIf(buff, "#include")) {
        // Get the file
        consumeWhitespace(buff);

        String fileName = {0};

        char c = 0;

        if (peek(buff) == '<') {
            c = '>';
        }
        else if (peek(buff) == '"') {
            c = '"';
        }
        else {
            return false;
        }

        consume(buff);

        fileName.str = buffCurr(buff);

        while (peek(buff) != c) {
            fileName.length++;
            consume(buff);
        }

        // Consume the end character
        consume(buff);

        printf("%.*s\n", astr_format(fileName));

        // Open up the file
        const char *prefix = "/usr/include/";

        char *name = calloc(strlen(prefix) + fileName.length + 1, 1);

        assert(name != NULL);

        memcpy(name, prefix, strlen(prefix));
        memcpy(name + strlen(prefix), fileName.str, fileName.length);

        printf("%s\n", name);

        // Append it to the file stack
        Buffer newBuff = {0};

        if (!openAndReadFileToBuffer(name, &newBuff)) {
            return false;
        }

        FileContext newContext =
        {
            .buffer = newBuff,
            .fileName = name,
        };

        newBuff.pos = 0;
        newBuff.line = 1;
        newBuff.col = 1;

        if (!fileContextStack_pushFile(fileStack, newContext)) {
            return false;
        }
    }

    else {
        logError("Undefined preprocessor directive: %s:%d\n", context->fileName,
                 buff->line);
        printf("%c", peekAhead(buff, 0));
        printf("%c", peekAhead(buff, 1));
        printf("%c", peekAhead(buff, 2));
        printf("%c", peekAhead(buff, 3));
        printf("%c", peekAhead(buff, 4));
        printf("%c", peekAhead(buff, 5));
        printf("%c", peekAhead(buff, 6));
        printf("\n");
        consume(buff);
    }

    return true;
}

static bool tryProcessToken(FileContext *context, TokenList *outTokens,
                            LineInfo *outLines)
{
    if (context == NULL)
        return false;

    Buffer *buff = &context->buffer;

    Token tok = {0};
    tok.line = buff->line;
    tok.col = buff->col;
    tok.fileName = context->fileName;
    tok.fileIndex = context->buffer.pos;

    // Look for keywords to ignore
    if (consumeMultiIf(buff, "__extension__")) {
        return true;
    }

    // TODO: Determine if we need to keep this keyword
    else if (consumeMultiIf(buff, "__attribute__")) {
        uint64_t numParens = 0;

        // consume all whitespace
        while (isspace(peek(buff)))
            consume(buff);

        if (consumeIf(buff, '('))
            numParens++;

        while (numParens > 0) {
            if (peek(buff) == '(')
                numParens++;
            if (peek(buff) == ')')
                numParens--;
            consume(buff);
        }

        return true;
    }

    // Comments
    else if (peekMulti(buff, "//")) {
        uint8_t *bytes = buffCurr(buff);

        while (peek(buff) != '\r' && peek(buff) != '\n')
        {
            consume(buff);
        }

        tok.type = Token_Comment;
        tok.comment = (String) {
            .str = bytes,
            .length = buffCurr(buff) - bytes
        };

        return true;
    }

    else if (peekMulti(buff, "/*")) {
        while (!peekMulti(buff, "*/")) {
            consume(buff);
        }
        consume(buff);
        consume(buff);
        return true;
    }

    // Whitespace
    else if (peek(buff) == '\r' || peek(buff) == '\n') {
        tok.type = Token_NewLine;

        if (peek(buff) == '\r')
            consume(buff);

        // Add line info
        addLineLengthInfo(outLines, context->fileName, buff->line, buff->col);

        consume(buff);

        return true;
    }
    else if (isspace(peek(buff))) {
        uint8_t *bytes = buffCurr(buff);

        while (isspace(peek(buff)) && (peek(buff) != '\n') &&
               (peek(buff) != '\r'))
        {
            consume(buff);
        }

        tok.type = Token_Whitespace;
        tok.whitespace = (String) {
            .str = bytes,
            .length = buffCurr(buff) - bytes
        };

        // FIXME: How should we process whitespace?
        return true;
    }

    // Types
    Keyword("void", Token_void)
    Keyword("char", Token_char)
    Keyword("short", Token_short)
    Keyword("int", Token_int)
    Keyword("long", Token_long)
    Keyword("float", Token_float)
    Keyword("double", Token_double)
    Keyword("signed", Token_signed)
    Keyword("unsigned", Token_unsigned)
    Keyword("_Bool", Token_bool)
    Keyword("_Complex", Token_complex)
    Keyword("_Imaginary", Token_imaginary)

    // Keywords
    Keyword("asm", Token_asm)
    Keyword("__asm__", Token_asm) // GCC uses __asm__ same as asm
    Keyword("auto", Token_auto)
    Keyword("break", Token_break)
    Keyword("case", Token_case)
    Keyword("const", Token_const)
    Keyword("continue", Token_continue)
    Keyword("default", Token_default)
    Keyword("do", Token_do)
    Keyword("else", Token_else)
    Keyword("enum", Token_enum)
    Keyword("extern", Token_extern)
    Keyword("for", Token_for)
    Keyword("goto", Token_goto)
    Keyword("if", Token_if)
    Keyword("inline", Token_inline)
    Keyword("__inline", Token_inline) // GCC uses __inline same as inline
    Keyword("__inline__", Token_inline) // GCC uses __inline__ same as inline
    Keyword("register", Token_register)
    Keyword("restrict", Token_restrict)
    Keyword("__restrict", Token_restrict) // GCC uses __restrict the same as restrict
    Keyword("__restrict__", Token_restrict) // GCC uses __restrict__ the same as restrict
    Keyword("return", Token_return)
    Keyword("sizeof", Token_sizeof)
    Keyword("static", Token_static)
    Keyword("struct", Token_struct)
    Keyword("switch", Token_switch)
    Keyword("typedef", Token_typedef)
    Keyword("union", Token_union)
    Keyword("volatile", Token_volatile)
    Keyword("while", Token_while)

    Keyword("_Alignas", Token_alignas)
    Keyword("_Alignof", Token_alignof)
    Keyword("_Atomic", Token_atomic)
    Keyword("_Generic", Token_generic)
    Keyword("_Noreturn", Token_noreturn)
    Keyword("_Static_assert", Token_staticAssert)
    Keyword("_Thread_local", Token_threadLocal)
    Keyword("__func__", Token_funcName)

    // Identifier
    else if (peek(buff) == '_' || isalpha(peek(buff))) {
        uint8_t *bytes = buffCurr(buff);

        while (peek(buff) == '_' || isalnum(peek(buff)))
        {
            consume(buff);
        }

        tok.type = Token_Ident;
        tok.ident = (String) {
            .str = bytes,
            .length = buffCurr(buff) - bytes
        };
    }

    // Operators
    TripleCharacterOp("...", Token_Ellipsis)
    TripleCharacterOp(">>=", Token_ShiftRightAssign)
    TripleCharacterOp("<<=", Token_ShiftLeftAssign)

    DoubleCharacterOp("+=", Token_AddAssign)
    DoubleCharacterOp("-=", Token_SubAssign)
    DoubleCharacterOp("*=", Token_MulAssign)
    DoubleCharacterOp("/=", Token_DivAssign)
    DoubleCharacterOp("%=", Token_ModAssign)
    DoubleCharacterOp("&=", Token_AndAssign)
    DoubleCharacterOp("^=", Token_XorAssign)
    DoubleCharacterOp("|=", Token_OrAssign)
    DoubleCharacterOp(">>", Token_ShiftRightOp)
    DoubleCharacterOp("<<", Token_ShiftLeftOp)
    DoubleCharacterOp("++", Token_IncOp)
    DoubleCharacterOp("--", Token_DecOp)
    DoubleCharacterOp("->", Token_PtrOp)
    DoubleCharacterOp("&&", Token_LogAndOp)
    DoubleCharacterOp("||", Token_LogOrOp)
    DoubleCharacterOp("<=", Token_LEqOp)
    DoubleCharacterOp(">=", Token_GEqOp)
    DoubleCharacterOp("==", Token_EqOp)
    DoubleCharacterOp("!=", Token_NEqOp)

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

    // Constants
    else if (consumeIf(buff, '"')) {
        uint8_t *bytes = buffCurr(buff);

        while (peek(buff) != '"') {
            if (peek(buff) == '\\')
                consume(buff);

            consume(buff);
        }

        tok.type = Token_ConstString;
        tok.constString = (String) {
            .str = bytes,
            .length = buffCurr(buff) - bytes
        };

        // Get the last "
        consume(buff);

        // TODO: Should we handle strings next to each other here?
        // We currently do it in the parser
    }
    else if (isdigit(peek(buff))) {
        uint8_t *numeric = buffCurr(buff);

        bool lookForFloat = false;
        bool isHex = false;

        if (consumeMultiIf(buff, "0x") || consumeMultiIf(buff, "0X")) {
            // Hex
            isHex = true;

            while (isxdigit(peek(buff))) {
                consume(buff);
            }

            if (!consumeIntConstSuffix(buff)) {
                // Try to look for floats
                lookForFloat = true;
            }
        }
        // We explicitly ignore octal numbers because it makes it easier to
        // parse. We assume that the user has compiled the code, and that
        // it works.
        else {
            // Decimal
            while (isdigit(peek(buff))) {
                consume(buff);
            }

            if (!consumeIntConstSuffix(buff)) {
                // Try to look for floats
                lookForFloat = true;
            }
        }

        if (lookForFloat && isHex) {
            if (peek(buff) == '.') {
                consume(buff);

                while (isxdigit(peek(buff))) {
                    consume(buff);
                }

                if (!consumeHexFloatExponent(buff)) {
                    printf("%s:%ld\n", context->fileName, buff->line);
                    assert(false);
                }
                consumeFloatConstSuffix(buff);
            }
        }
        else if (lookForFloat) {
            if (peek(buff) == '.') {
                consume(buff);

                while (isdigit(peek(buff))) {
                    consume(buff);
                }

                consumeDecFloatExponent(buff);

                consumeFloatConstSuffix(buff);
            }
        }

        size_t length = buffCurr(buff) - numeric;

        tok.type = Token_ConstNumeric;
        tok.numeric = (String) {
            .str = numeric,
            .length = length
        };
    }
    else if (peek(buff) == '\'') {
        uint8_t *bytes = buff->bytes;

        consume(buff);

        while (peek(buff) != '\'') {
            if (peek(buff) == '\\')
                consume(buff);

            consume(buff);
        }

        consume(buff);

        tok.type = Token_ConstNumeric;
        tok.numeric = (String) {
            .str = bytes,
            .length = buff->bytes - bytes
        };
    }

    else {
        logError("Lexer: Discarding invalid token start character: %c at %s:%lu\n",
            *buffCurr(buff), context->fileName, buff->line);

        consume(buff);
    }

    appendTok(outTokens, tok);

    return true;
}

static bool fileContextStack_pushFile(FileContextStack *stack, FileContext context) {
    if (stack == NULL || stack->stackSize == 16) {
        return false;
    }

    stack->files[stack->stackSize] = context;
    stack->stackSize++;

    return true;
}

// TODO: Error handling
static void fileContextStack_pop(FileContextStack *stack) {
    stack->stackSize--;
}

static FileContext *fileContextStack_context(FileContextStack *stack) {
    assert(stack != NULL);
    assert(stack->stackSize > 0);

    return stack->files + stack->stackSize - 1;
}

static bool consumeIntConstSuffix(Buffer *buff) {
    if (tolower(peek(buff)) == 'u') {
        consume(buff);

        if (tolower(peek(buff)) == 'l')
            consume(buff);

        if (tolower(peek(buff)) == 'l')
            consume(buff);

        return true;
    }
    else if (tolower(peek(buff)) == 'l') {
        consume(buff);

        if (tolower(peek(buff)) == 'l')
            consume(buff);

        if (tolower(peek(buff)) == 'u')
            consume(buff);

        return true;
    }

    return false;
}

static bool consumeFloatConstSuffix(Buffer *buff) {
    if (tolower(peek(buff)) == 'f') {
        consume(buff);
        return true;
    }

    if (tolower(peek(buff)) == 'l') {
        consume(buff);
        return true;
    }

    return false;
}

static bool consumeHexFloatExponent(Buffer *buff) {
    if (tolower(peek(buff)) == 'p') {
        consume(buff);

        consumeIf(buff, '-');
        consumeIf(buff, '+');

        while (isdigit(buff)) {
            consume(buff);
        }

        return true;
    }

    return false;
}

static bool consumeDecFloatExponent(Buffer *buff) {
    if (tolower(peek(buff)) == 'e') {
        consume(buff);

        consumeIf(buff, '-');
        consumeIf(buff, '+');

        while (isdigit(buff)) {
            consume(buff);
        }

        return true;
    }

    return false;
}

static void consumeWhitespace(Buffer *buff) {
    while (isspace(peek(buff))) {
        consume(buff);
    }
}

static void addLineLengthInfo(LineInfo *info, char *fileName, uint64_t line,
                              uint64_t length)
{
    if (NULL == fileName) {
        assert(false);
    }

    FileInfo *fileInfo = NULL;
    // Check to see if fileName exists
    for (size_t i = 0; i < info->numFiles; i++) {
        if (strcmp(info->fileInfo[i].fileName, fileName) == 0) {
            fileInfo = info->fileInfo + i;
            break;
        }
    }

    // If not, create info and add it to line length info
    if (fileInfo == NULL) {
        FileInfo newInfo = {
            .fileName = fileName
        };
        ArrayAppend(info->fileInfo, info->numFiles, newInfo);

        fileInfo = info->fileInfo + info->numFiles - 1;
    }

    // Check if num lines are lower than current line
    while (line > fileInfo->numLines) {
        ArrayAppend(fileInfo->lineLengths, fileInfo->numLines, (uint64_t)0);
    }

    // Add line length
    fileInfo->lineLengths[line - 1] = length;
}

static void appendTok(TokenList *tokens, Token tok) {
    tokens->numTokens++;
    tokens->tokens = realloc(tokens->tokens, tokens->numTokens * sizeof(Token));
    tokens->tokens[tokens->numTokens - 1] = tok;
}

void printTokens(TokenList tokens) {
    printDebug("Tokens: %lu\n", tokens.numTokens);

    for (uint64_t i = 0; i < tokens.numTokens; i++) {
        Token tok = tokens.tokens[i];

        printDebug("%s:%ld:%ld ", tok.fileName, tok.line, tok.col);

        // Assuming this is a character token
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

        printableOp(Token_Ellipsis, "...")
        printableOp(Token_ShiftRightAssign, ">>=")
        printableOp(Token_ShiftLeftAssign, "<<=")
        printableOp(Token_AddAssign, "+=")
        printableOp(Token_SubAssign, "-=")
        printableOp(Token_MulAssign, "*=")
        printableOp(Token_DivAssign, "/=")
        printableOp(Token_ModAssign, "%%=")
        printableOp(Token_AndAssign, "&=")
        printableOp(Token_XorAssign, "^=")
        printableOp(Token_OrAssign, "|=")
        printableOp(Token_ShiftRightOp, ">>")
        printableOp(Token_ShiftLeftOp, "<<")
        printableOp(Token_IncOp, "++")
        printableOp(Token_DecOp, "--")
        printableOp(Token_PtrOp, "->")
        printableOp(Token_LogAndOp, "&&")
        printableOp(Token_LogOrOp, "||")
        printableOp(Token_LEqOp, "<=")
        printableOp(Token_GEqOp, ">=")
        printableOp(Token_EqOp, "==")
        printableOp(Token_NEqOp, "!=")

        else if (tok.type == Token_Ident) {
            printDebug("Ident: %.*s\n", astr_format(tok.ident));
        }
        else if (tok.type == Token_Comment) {
            printDebug("%.*s\n", astr_format(tok.comment));
        }
        else if (tok.type == Token_ConstString) {
            printDebug("String: %.*s\n", astr_format(tok.constString));
        }
        else if (tok.type == Token_ConstNumeric) {
            printDebug("Number: %.*s\n", astr_format(tok.numeric));
        }
        else if (tok.type == Token_Whitespace) {
            printDebug("Whitespace\n");
        }
        else if (tok.type == Token_NewLine) {
            printDebug("\\n\n");
        }
        else {
            logFatal("Lexer: Invalid token type when printing: %ld\n", tok.type);
            assert(false);
        }
    }
}
