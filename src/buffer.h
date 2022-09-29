#pragma once

typedef struct {
    size_t size;
    size_t pos;
    unsigned char *bytes;
} Buffer;

int openAndReadFileToBuffer(char *fileName, Buffer *outBuff);
