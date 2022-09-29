#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    size_t size;
    size_t pos;
    uint8_t *bytes;
} Buffer;

bool openAndReadFileToBuffer(char *fileName, Buffer *outBuff);
