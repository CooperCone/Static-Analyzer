#include "lexer.h"

#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <ctype.h>

#include "debug.h"

void appendTok(TokenList *tokens, Token tok) {
    tokens->numTokens++;
    tokens->tokens = realloc(tokens->tokens, tokens->numTokens * sizeof(Token));
    tokens->tokens[tokens->numTokens - 1] = tok;
}

#define TripleCharacterOp(op, tokType) else if (consumeMultiIf(buff, op)) {\
    tok.type = tokType;\
    col += 3;\
}

#define DoubleCharacterOp(op, tokType) else if (consumeMultiIf(buff, op)) {\
    tok.type = tokType;\
    col += 2;\
}

#define SingleCharacterOp(op) else if (consumeIf(buff, op )) {\
    tok.type = op;\
    col++;\
}

#define Keyword(name, tokType) else if (consumeMultiIf(buff, name)) {\
    tok.type = tokType;\
    col += strlen(name);\
}

bool lexFile(Buffer buffer, TokenList *outTokens) {
    if (NULL == outTokens)
        return -1;

    buffer.pos = 0;

    Buffer *buff = &buffer;

    uint64_t line = 1;
    uint64_t col = 1;

    while (buffer.pos < buffer.size) {

        Token tok = {0};
        tok.line = line;
        tok.col = col;
        // TODO: Should we update line and col in a better way?

        // Comments
        if (peekMulti(buff, "//")) {
            uint64_t commentLength = 0;

            while (peekAhead(buff, commentLength) != '\r' &&
                   peekAhead(buff, commentLength) != '\n')
            {
                commentLength++;
            }

            char *commentStr = NULL;
            consumeAndCopyOut(buff, commentLength, &commentStr);
            
            tok.type = Token_Comment;
            tok.comment = commentStr;

            col += commentLength;

            // FIXME: Do we need to keep comments?
            continue;
        }

        // Whitespace
        else if (peek(buff) == '\r' || peek(buff) == '\n') {
            tok.type = Token_NewLine;

            if (peek(buff) == '\r')
                buffer.pos++;
            buffer.pos++;

            line++;
            col = 1;

            // FIXME: We still need a way to keep track of new lines
            continue;
        }
        else if (isspace(peek(buff))) {
            size_t whitespaceLen = 1;

            while (isspace(peekAhead(buff, whitespaceLen)) &&
                (peekAhead(buff, whitespaceLen) != '\n') &&
                (peekAhead(buff, whitespaceLen) != '\r'))
            {
                whitespaceLen++;
            }

            char *whitespace = NULL;
            consumeAndCopyOut(buff, whitespaceLen, &whitespace);

            tok.type = Token_Whitespace;
            tok.whitespace = whitespace;

            col += whitespaceLen;

            // FIXME: How should we process whitespace?
            continue;
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
        Keyword("register", Token_register)
        Keyword("restrict", Token_restrict)
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
            size_t identLength = 1;

            while (peekAhead(buff, identLength) == '_' ||
                   isalnum(peekAhead(buff, identLength)))
            {
                identLength++;
            }

            char *ident = NULL;
            consumeAndCopyOut(buff, identLength, &ident);

            tok.type = Token_Ident;
            tok.ident = ident;

            col += identLength;
        }

        // Operators
        TripleCharacterOp("...", Token_Ellipsis)

        DoubleCharacterOp(">>=", Token_ShiftRightAssign)
        DoubleCharacterOp("<<=", Token_ShiftLeftAssign)
        DoubleCharacterOp("+=", Token_AddAssign)
        DoubleCharacterOp("-=", Token_SubAssign)
        DoubleCharacterOp("*=", Token_MulAssign)
        DoubleCharacterOp("/=", Token_DivAssign)
        DoubleCharacterOp("%%=", Token_ModAssign)
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
            size_t stringLength = 0;

            while (peekAhead(buff, stringLength) != '"') {
                stringLength++;
            }

            char *str = NULL;
            consumeAndCopyOut(buff, stringLength, &str);

            tok.type = Token_ConstString;
            tok.constString = str;

            buffer.pos += 1; // Get the last "
            col += stringLength + 2; // 2 so we also get the first one
        }
        // TODO: Prefix
        // TODO: Suffix
        else if (isdigit(peek(buff))) {
            size_t wholeLen = 1;

            while (isdigit(peekAhead(buff, wholeLen))) {
                wholeLen++;
            }

            char *wholeStr = NULL;
            consumeAndCopyOut(buff, wholeLen, &wholeStr);

            col += wholeLen;

            if (peek(buff) == '.') {
                assert(false);
                // TODO: Handle Floats
            }

            tok.type = Token_ConstNumeric;
            tok.numericWhole = wholeStr;
        }

        else {
            // TODO: Error Handling
            buffer.pos++;
            col++;
        }

        appendTok(outTokens, tok);
    }

    return true;
}

#define printableKeyword(keyword) else if (tok.type == Token_ ## keyword) {\
    printDebug("Keyword: " # keyword "\n");\
}

void printTokens(TokenList tokens) {
    printDebug("Tokens: %lu\n", tokens.numTokens);

    for (uint64_t i = 0; i < tokens.numTokens; i++) {
        Token tok = tokens.tokens[i];

        printDebug("%ld:%ld ", tok.line, tok.col);

        // Assuming this is a character token
        if (tok.type < 127) {
            printDebug("Character: %c\n", tok.type);
        }

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

        else if (tok.type == Token_Ident) {
            printDebug("Ident: %s\n", tok.ident);
        }
        else if (tok.type == Token_Comment) {
            printDebug("%s\n", tok.comment);
        }
        else if (tok.type == Token_ConstString) {
            printDebug("String: %s\n", tok.constString);
        }
        else if (tok.type == Token_ConstNumeric) {
            printDebug("Number: %s\n", tok.numericWhole);
        }
        else if (tok.type == Token_Whitespace) {
            printDebug("Whitespace\n");
        }
        else if (tok.type == Token_NewLine) {
            printDebug("\\n\n");
        }
        else {
            printDebug("type: %d\n", tok.type);
            assert(false);
        }
    }
}
