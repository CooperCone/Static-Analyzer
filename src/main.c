#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "buffer.h"
#include "lexer.h"
#include "rule.h"
#include "debug.h"
#include "config.h"

extern Rule g_rules[];
extern size_t g_rulesCount;

int main(int argc, char **argv) {

// I didn't really want to finish working on config stuff
// TODO: Finish configurations
#if 0
    // Read in config file
    Config configFile = {0};
    if (!readConfigFile("config/config.cfg", &configFile)) {
        fflush(stdout);
        assert(false);
    }

    // Delete rules from configs
    for (uint64_t i = 0; i < configFile.numConfigValues; i++) {
        ConfigValue value = configFile.configValues[i];
        if (value.type != ConfigValue_Map)
            continue;
        
        if (strcmp(value.mapKey, "ignore") != 0)
            continue;

        if (value.mapValue.type != ConfigValue_List) {
            printf("Config Error: ignore map rule must have value of list\n");
            continue;
        }

        for (uint64_t lstIdx = 0; lstIdx < value.mapValue->listSize; lstIdx++) {
            ConfigValue elem = value.mapValue->listValues[lstIdx];
            if (elem.type != ConfigValue_String) {
                printf("Config Error: ignore list elem must be a string\n");
                continue;
            }

            // FIXME: This isn't a good idea. We should get a list of masked
            // rules, pass it into a function that gives us rules other than
            // the masked ones to ignore
            for (uint64_t ruleIdx = 0; ruleIdx < g_rulesCount; ruleIdx++) {
                if (strcmp())
            }
        }
    }
#endif

    // TODO: Modify rules with configs

    for (uint64_t i = 1; i < argc; i++) {
        // Open file
        Buffer fileBuff = {0};

        if (!openAndReadFileToBuffer(argv[i], &fileBuff)) {
            // There was an error reading the file
            printf("Error: couldn't read file: %s\n", argv[i]);
            continue;
        }

        printDebug("%s\n", fileBuff.bytes);

        // Lex file
        TokenList tokens = {0};

        if (!lexFile(fileBuff, &tokens)) {
            continue;
        }

        printTokens(tokens);
        printDebug("\n");

        // Parse tokens


        // Run all rules
        RuleContext context = {
            .fileName = argv[i],
            .tokens = tokens
        };

        for (uint64_t i = 0; i < g_rulesCount; i++) {
            g_rules[i].validator(g_rules[i], context);
        }
    }
}
