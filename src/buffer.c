#include "buffer.h"

#include <stdio.h>
#include <string.h>

bool openAndReadFileToBuffer(char *fileName, Buffer *outBuff) {
    if (outBuff == NULL)
        return false;

    FILE *file = fopen(fileName, "r");

    if (file == NULL)
        return false;

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    uint8_t *bytes = malloc(size + 1);
    fread(bytes, size, 1, file);
    fclose(file);

    outBuff->size = size;
    outBuff->bytes = bytes;

    return true;
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

    if (c == '\n') {
        buffer->line++;
        buffer->col = 0;
    }
    else if (c != '\0') {
        buffer->col++;
    }

    return c;
}

void consumeMulti(Buffer *buffer, size_t numBytes) {
    for (size_t i = 0; i < numBytes; i++) {
        consume(buffer);
    }
}

bool consumeIf(Buffer *buffer, char c) {
    if (peek(buffer) == c) {
        consume(buffer);
        return true;
    }

    return false;
}

bool consumeMultiIf(Buffer *buffer, char *str) {
    if (peekMulti(buffer, str)) {
        consumeMulti(buffer, strlen(str));

        return true;
    }

    return false;
}

void consumeAndCopyOut(Buffer *buffer, size_t numBytes, char **outStr) {
    char *str = malloc(numBytes + 1);
    memcpy(str, buffer->bytes + buffer->pos, numBytes);
    str[numBytes] = '\0';

    consumeMulti(buffer, numBytes);

    *outStr = str;
}

uint8_t *buffCurr(Buffer *buffer) {
    return buffer->bytes + buffer->pos;
}
