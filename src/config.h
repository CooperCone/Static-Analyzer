#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    ConfigValue_String,
    ConfigValue_List,
    ConfigValue_Map,
} ConfigValueType;

typedef struct ConfigValue {
    ConfigValueType type;
    union {
        char *string;
        struct {
            uint64_t listSize;
            struct ConfigValue *listValues;
        };
        struct {
            char *mapKey;
            struct ConfigValue *mapValue;
        };
    };
} ConfigValue;

typedef struct {
    uint64_t numConfigValues;
    ConfigValue *configValues;
} Config;

bool readConfigFile(char *configFileName, Config *outConfig);
void printConfig(Config config);
