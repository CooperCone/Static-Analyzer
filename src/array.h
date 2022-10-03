#pragma once

#define ArrayAppend(array, length, elem) {\
    length++;\
    array = realloc(array, length * sizeof(elem));\
    array[length - 1] = elem;\
}
