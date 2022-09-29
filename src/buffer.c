#include "buffer.h"

int openAndReadFileToBuffer(char *fileName, Buffer *outBuff) {
    if (outBuff == NULL)
        return -1;
    
    FILE *file = fopen(fileName, "r");

    if (file == NULL)
        return -1;

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *bytes = malloc(size + 1);
    fread(bytes, size, 1, file);
    fclose(file);

    outBuff->size = size;
    outBuff->bytes = bytes;

    return 0;
}