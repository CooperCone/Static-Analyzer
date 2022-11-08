#include "astring.h"

#include <string.h>

String astr(char *str) {
    return (String) {
        .str = (uint8_t*)str,
        .length = strlen(str)
    };
}

bool astr_cmp(String left, String right) {
    if (left.length != right.length)
        return false;

    for (size_t i = 0; i < left.length; i++) {
        if (left.str[i] != right.str[i])
            return false;
    }

    return true;
}

bool astr_ccmp(String left, char *right) {
    String rightStr = astr(right);
    return astr_cmp(left, rightStr);
}
