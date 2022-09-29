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

char peek(Buffer *buffer) {
    return buffer->bytes[buffer->pos];
}

char peekAhead(Buffer *buffer, size_t lookahead) {
    if (buffer->pos + lookahead > buffer->size)
        return '\0';

    return buffer->bytes[buffer->pos + lookahead];
}

bool peekMulti(Buffer *buffer, char *str) {
    if (buffer->size <= strlen(str) + buffer->pos)
        return false;

    for (uint64_t i = 0; i < strlen(str); i++) {
        if (buffer->bytes[buffer->pos + i] != str[i])
            return false;
    }

    return true;
}

char consume(Buffer *buffer) {
    char c = peek(buffer);
    buffer->pos++;
    return c;
}

bool consumeIf(Buffer *buffer, char c) {
    if (peek(buffer) == c) {
        buffer->pos++;
        return true;
    }

    return false;
}

bool consumeMultiIf(Buffer *buffer, char *str) {
    if (peekMulti(buffer, str)) {
        buffer->pos += strlen(str);

        return true;
    }

    return false;
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

            char *commentStr = malloc(commentLength + 1);
            memcpy(commentStr, buff->bytes, commentLength);
            commentStr[commentLength] = '\0';

            tok.type = Token_Comment;
            tok.comment = commentStr;

            buff->pos += commentLength;
            col += commentLength;
        }

        // Whitespace
        else if (peek(buff) == '\r' || peek(buff) == '\n') {
            tok.type = Token_NewLine;

            if (peek(buff) == '\r')
                buffer.pos++;
            buffer.pos++;

            line++;
            col = 1;
        }
        else if (isspace(peek(buff))) {
            size_t whitespaceLen = 1;

            while (isspace(peekAhead(buff, whitespaceLen)) &&
                (peekAhead(buff, whitespaceLen) != '\n') &&
                (peekAhead(buff, whitespaceLen) != '\r'))
            {
                whitespaceLen++;
            }

            char *whitespace = malloc(whitespaceLen + 1);
            memcpy(whitespace, buff->bytes + buff->pos, whitespaceLen);
            whitespace[whitespaceLen] = '\0';

            buff->pos += whitespaceLen;
            col += whitespaceLen;

            tok.type = Token_Whitespace;
            tok.whitespace = whitespace;
        }

        // Keywords
        Keyword("include", Token_include)

        // Types
        Keyword("int", Token_int)
        Keyword("char", Token_char)

        // Identifier
        else if (peek(buff) == '_' || isalpha(peek(buff))) {
            size_t identLength = 1;

            while (peekAhead(buff, identLength) == '_' ||
                   isalnum(peekAhead(buff, identLength)))
            {
                identLength++;
            }

            char *ident = malloc(identLength + 1);
            memcpy(ident, buff->bytes + buff->pos, identLength);
            ident[identLength] = '\0';

            buffer.pos += identLength;
            col += identLength;

            tok.type = Token_Ident;
            tok.ident = ident;
        }

        // Operators
        SingleCharacterOp('#')
        SingleCharacterOp('.')
        SingleCharacterOp(',')
        SingleCharacterOp(';')
        SingleCharacterOp('<')
        SingleCharacterOp('>')
        SingleCharacterOp('{')
        SingleCharacterOp('}')
        SingleCharacterOp('(')
        SingleCharacterOp(')')
        SingleCharacterOp('*')

        // Constants
        else if (consumeIf(buff, '"')) {
            size_t stringLength = 0;

            while (peekAhead(buff, stringLength) != '"') {
                stringLength++;
            }

            char *str = malloc(stringLength + 1);
            memcpy(str, buffer.bytes + buffer.pos, stringLength);
            str[stringLength] = '\0';

            // + 1 so we can get the last "
            buffer.pos += stringLength + 1;
            col += stringLength + 2; // 2 so we also get the first one

            tok.type = Token_ConstString;
            tok.constString = str;
        }
        // TODO: Prefix
        // TODO: Suffix
        else if (isdigit(peek(buff))) {
            size_t wholeLen = 1;

            while (isdigit(peekAhead(buff, wholeLen))) {
                wholeLen++;
            }

            char *wholeStr = malloc(wholeLen + 1);
            memcpy(wholeStr, buff->bytes + buff->pos, wholeLen);
            wholeStr[wholeLen] = '\0';

            buffer.pos += wholeLen;
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

        printableKeyword(include)
        printableKeyword(int)
        printableKeyword(char)

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
