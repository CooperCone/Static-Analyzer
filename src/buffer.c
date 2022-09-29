#include "buffer.h"

#include <stdio.h>

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