#include "lexer.h"

#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <ctype.h>

#include "debug.h"
#include "array.h"
#include "logger.h"

bool consumeIntConstSuffix(Buffer *buff) {
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

bool consumeFloatConstSuffix(Buffer *buff) {
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

bool consumeHexFloatExponent(Buffer *buff) {
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

bool consumeDecFloatExponent(Buffer *buff) {
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

void addLineLengthInfo(LineInfo *info, char *fileName, uint64_t line,
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
    col += strlen(name);\
}

bool lexFile(Buffer buffer, TokenList *outTokens, LineInfo *outLines) {
    if (NULL == outTokens)
        return -1;

    buffer.pos = 0;

    Buffer *buff = &buffer;

    uint64_t line = 1;
    uint64_t col = 1;
    char *fileName = NULL;

    while (buffer.pos < buffer.size) {

        Token tok = {0};
        tok.line = line;
        tok.col = col;
        tok.fileName = fileName;
        tok.fileIndex = buffer.pos;

        // Compiler Commands
        if (consumeMultiIf(buff, "#pragma")) {
            while (peek(buff) != '\n') {
                consume(buff);
            }

            line++;
            col = 1;

            continue;
        }

        // Line commands
        else if (consumeIf(buff, '#')) {
            while (isspace(peek(buff)))
                consume(buff);

            // Parse an integer
            uint8_t *integerStart = buffer.bytes + buffer.pos;
            while (isdigit(peek(buff))) {
                consume(buff);
            }

            long newLineNumber = strtol((char*)integerStart, NULL, 10);

            while (isspace(peek(buff)))
                consume(buff);

            // Parse a file
            char *newFileName = NULL;
            if (consume(buff) == '"') {
                char *initial = (char*)buff->bytes + buff->pos;

                size_t length = 0;
                while (peek(buff) != '"') {
                    length++;
                    consume(buff);
                }

                if (consume(buff) != '"') {
                    assert(false);
                }

                newFileName = malloc(length + 1);
                memcpy(newFileName, initial, length);
                newFileName[length] = '\0';
            }

            // consume until newline
            while (peek(buff) != '\n')
                consume(buff);

            consume(buff);

            fileName = newFileName;
            line = newLineNumber;
            col = 1;

            continue;
        }

        // Look for keywords to ignore
        else if (consumeMultiIf(buff, "__extension__")) {
            continue;
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

            continue;
        }

        // Comments
        else if (peekMulti(buff, "//")) {
            uint64_t commentLength = 0;

            while (peekAhead(buff, commentLength) != '\r' &&
                   peekAhead(buff, commentLength) != '\n')
            {
                commentLength++;
            }

            tok.type = Token_Comment;
            tok.comment = (String) {
                .str = buff->bytes + buff->pos,
                .length = commentLength
            };

            col += commentLength;
            buff->pos += commentLength;

            continue;
        }

        else if (peekMulti(buff, "/*")) {
            while (!peekMulti(buff, "*/")) {
                if (peek(buff) == '\n')
                    line++;

                consume(buff);
            }
            consume(buff);
            consume(buff);
            continue;
        }

        // Whitespace
        else if (peek(buff) == '\r' || peek(buff) == '\n') {
            tok.type = Token_NewLine;

            if (peek(buff) == '\r')
                buffer.pos++;
            buffer.pos++;

            // Add line info
            addLineLengthInfo(outLines, fileName, line, col);

            line++;
            col = 1;

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

            tok.type = Token_Whitespace;
            tok.whitespace = (String) {
                .str = buff->bytes + buff->pos,
                .length = whitespaceLen
            };

            col += whitespaceLen;
            buff->pos += whitespaceLen;

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
        Keyword("asm", Token_asm)
        Keyword("__asm__", Token_asm)
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
            size_t identLength = 1;

            while (peekAhead(buff, identLength) == '_' ||
                   isalnum(peekAhead(buff, identLength)))
            {
                identLength++;
            }

            tok.type = Token_Ident;
            tok.ident = (String) {
                .str = buff->bytes + buff->pos,
                .length = identLength
            };

            col += identLength;
            buff->pos += identLength;
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
            size_t stringLength = 0;

            while (peekAhead(buff, stringLength) != '"') {
                if (peekAhead(buff, stringLength) == '\\')
                    stringLength++;

                stringLength++;
            }

            tok.type = Token_ConstString;
            tok.constString = (String) {
                .str = buff->bytes + buff->pos,
                .length = stringLength
            };

            buffer.pos += 1 + stringLength; // Get the last "
            col += stringLength + 2; // 2 so we also get the first one

            // TODO: Should we handle strings next to each other here?
            // We currently do it in the parser
        }
        else if (isdigit(peek(buff))) {
            uint8_t *numeric = buff->bytes + buff->pos;

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
                        printf("%s:%ld\n", fileName, line);
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

            size_t length = (buff->bytes + buff->pos) - numeric;

            tok.type = Token_ConstNumeric;
            tok.numeric = (String) {
                .str = numeric,
                .length = length
            };

            col += length;
        }
        else if (peek(buff) == '\'') {
            size_t length = 1;

            while (peekAhead(buff, length) != '\'') {
                if (peekAhead(buff, length) == '\\')
                    length++;

                length++;
            }

            length++;

            tok.type = Token_ConstNumeric;
            tok.numeric = (String) {
                .str = buff->bytes + buff->pos,
                .length = length
            };

            col += length;
            buff->pos += length;
        }

        else {
            logError("Lexer: Discarding invalid token start character: %c at %s:%lu\n",
                buffer.bytes[buffer.pos], fileName, line);

            buffer.pos++;
            col++;
            continue;
        }

        appendTok(outTokens, tok);
    }

    return true;
}

#define printableKeyword(keyword) else if (tok.type == Token_ ## keyword) {\
    printDebug("Keyword: " # keyword "\n");\
}

#define printableOp(op, text) else if (tok.type == op) {\
    printDebug(text "\n");\
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
