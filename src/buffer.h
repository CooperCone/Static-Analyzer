#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    size_t size;
    size_t pos;
    size_t line;
    size_t col;
    uint8_t *bytes;
} Buffer;

bool openAndReadFileToBuffer(char *fileName, Buffer *outBuff);

char peek(Buffer *buffer);
char peekAhead(Buffer *buffer, size_t lookahead);
bool peekMulti(Buffer *buffer, char *str);
char consume(Buffer *buffer);
void consumeMulti(Buffer *buffer, size_t numBytes);
bool consumeIf(Buffer *buffer, char c);
bool consumeMultiIf(Buffer *buffer, char *str);
void consumeAndCopyOut(Buffer *buffer, size_t numBytes, char **outStr);
uint8_t *buffCurr(Buffer *buffer);
