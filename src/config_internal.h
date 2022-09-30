#pragma once

#include <stdlib.h>
#include <stdint.h>

#include "buffer.h"

typedef enum {
    ConfigToken_String = 255,
} ConfigTokenType;

typedef struct {
    ConfigTokenType type;
    union {
        char *string;
    };
} ConfigToken;

typedef struct {
    size_t numTokens;
    size_t curToken;
    ConfigToken *tokens;
} ConfigTokenList;

bool tokenizeConfig(Buffer *buff, ConfigTokenList *outList);
