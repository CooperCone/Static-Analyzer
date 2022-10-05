#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "buffer.h"
#include "lexer.h"
#include "parser.h"
#include "rule.h"
#include "debug.h"
#include "config.h"

int main(int argc, char **argv) {

    Config config = {0};
    if (!readConfigFile("config/config.cfg", &config)) {
        printf("Error: couldn't read config file\n");
    }

    Rule *rules = NULL;
    size_t numRules = generateRules(config, &rules);

    // TODO: Modify rules with configs

    for (uint64_t i = 1; i < argc; i++) {
        // TODO: Preprocess file

        // Open file
        Buffer fileBuff = {0};

        if (!openAndReadFileToBuffer(argv[i], &fileBuff)) {
            // There was an error reading the file
            printf("Error: couldn't read file: %s\n", argv[i]);
            continue;
        }

        printDebug("%s\n", fileBuff.bytes);

        // Lex file
        LineInfo lineInfo = {0};
        TokenList tokens = {0};
        if (!lexFile(fileBuff, &tokens, &lineInfo)) {
            continue;
        }

        printTokens(tokens);
        printDebug("\n");

        // Parse tokens
        TranslationUnit unit = {0};
        if (!parseTokens(&tokens, &unit)) {
            continue;
        }

        printTranslationUnit(unit);
        printDebug("\n");

        // Run all rules
        RuleContext context = {
            .fileName = argv[i],
            .tokens = tokens,
            .lineInfo = lineInfo,
        };

        for (uint64_t i = 0; i < numRules; i++) {
            rules[i].validator(rules[i], context);
        }
    }
}
