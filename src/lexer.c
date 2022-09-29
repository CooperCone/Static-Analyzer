#include "lexer.h"

#include <stdbool.h>
#include <string.h>
#include <assert.h>

void appendTok(TokenList *tokens, Token tok) {
    tokens->numTokens++;
    tokens->tokens = realloc(tokens->tokens, tokens->numTokens * sizeof(Token));
    tokens->tokens[tokens->numTokens - 1] = tok;
}

char peek(Buffer *buffer) {
    return buffer->bytes[buffer->pos];
}

bool peekMulti(Buffer *buffer, char *str) {
    if (buffer->size <= strlen(str) + buffer->pos)
        return false;

    for (int i = 0; i < strlen(str); i++) {
        if (buffer->bytes[buffer->pos + i] != str[i])
            return false;
    }

    return true;
}

bool consumeMultiIf(Buffer *buffer, char *str) {
    if (peekMulti(buffer, str)) {
        buffer->pos += strlen(str);

        return true;
    }

    return false;
}

bool lexFile(Buffer buffer, TokenList *outTokens) {
    if (NULL == outTokens)
        return -1;

    buffer.pos = 0;

    Buffer *buff = &buffer;

    while (buffer.pos < buffer.size) {
        if (consumeMultiIf(buff, "#include") == true) {
            appendTok(outTokens, (Token){ Token_include });
        }
        else {
            buffer.pos++;
        }
    }

    return 
}

void printTokens(TokenList tokens) {
    printf("Tokens: %lu\n", tokens.numTokens);

    for (int i = 0; i < tokens.numTokens; i++) {
        Token tok = tokens.tokens[i];
        if (tok.type == Token_include) {
            printf("#include\n");
        }
        else {
            assert(false);
        }
    }
}
