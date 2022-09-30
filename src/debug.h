#pragma once

#include <stdio.h>

#if DEBUG
    #define printDebug(...) printf(__VA_ARGS__)
#else
    #define printDebug(...)
#endif
