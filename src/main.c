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

    // Read in config file
    Config configFile = {0};
    if (!readConfigFile("config/config.cfg", &configFile)) {
        fflush(stdout);
        assert(false);
    }

    // Create rules

    // Delete rules from configs

    // Modify rules with configs

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
