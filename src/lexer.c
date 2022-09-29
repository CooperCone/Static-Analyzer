#include "lexer.h"

#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <ctype.h>

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
    appendTok(outTokens, (Token){ op });\
}

#define Keyword(name, type) else if (consumeMultiIf(buff, name)) {\
    appendTok(outTokens, (Token){ type });\
}

bool lexFile(Buffer buffer, TokenList *outTokens) {
    if (NULL == outTokens)
        return -1;

    buffer.pos = 0;

    Buffer *buff = &buffer;

    while (buffer.pos < buffer.size) {

        // Whitespace
        if (isspace(peek(buff))) {
            size_t whitespaceLen = 1;

            while (isspace(peekAhead(buff, whitespaceLen))) {
                whitespaceLen++;
            }

            char *whitespace = malloc(whitespaceLen + 1);
            memcpy(whitespace, buff->bytes + buff->pos, whitespaceLen);
            whitespace[whitespaceLen] = '\0';

            Token whitespaceTok = {
                .type = Token_Whitespace,
                .whitespace = whitespace
            };

            buff->pos += whitespaceLen;

            appendTok(outTokens, whitespaceTok);
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

            Token identTok = {
                .type = Token_Ident,
                .ident = ident
            };

            appendTok(outTokens, identTok);

            buffer.pos += identLength;
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

            Token strTok = {
                .type = Token_ConstString,
                .constString = str
            };

            appendTok(outTokens, strTok);
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

            if (peek(buff) == '.') {
                assert(false);
                // TODO: Handle Floats
            }

            Token tok = {
                .type = Token_ConstNumeric,
                .numericWhole = wholeStr
            };

            appendTok(outTokens, tok);
        }

        else {
            // TODO: Error Handling
            buffer.pos++;
        }
    }

    return true;
}

#define printableKeyword(keyword) else if (tok.type == Token_ ## keyword) {\
    printf("Keyword: " # keyword "\n");\
}

void printTokens(TokenList tokens) {
    printf("Tokens: %lu\n", tokens.numTokens);

    for (uint64_t i = 0; i < tokens.numTokens; i++) {
        Token tok = tokens.tokens[i];
        // Assuming this is a character token
        if (tok.type < 127) {
            printf("Character: %c\n", tok.type);
        }

        printableKeyword(include)
        printableKeyword(int)
        printableKeyword(char)

        else if (tok.type == Token_Ident) {
            printf("Ident: %s\n", tok.ident);
        }
        else if (tok.type == Token_ConstString) {
            printf("String: %s\n", tok.constString);
        }
        else if (tok.type == Token_ConstNumeric) {
            printf("Number: %s\n", tok.numericWhole);
        }
        else if (tok.type == Token_Whitespace) {
            printf("Whitespace\n");
        }
        else {
            printf("type: %d\n", tok.type);
            assert(false);
        }
    }
}
