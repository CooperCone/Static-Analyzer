#pragma once

#if DEBUG
    #define printDebug(...) printf(__VA_ARGS__)
#else
    #define printDebug(...)
#endif
