#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// All functions are prefixed with astr_ for analyzer string
// This string struct uses no allocations. We only pull
// data from existing buffers. Because of this, we can't
// do any real string modifications.

typedef struct String {
    uint8_t *str;
    size_t length;
} String;

String astr(char *str);
bool astr_cmp(String left, String right);

#define astr_format(astr) (int)(astr).length, (astr).str
