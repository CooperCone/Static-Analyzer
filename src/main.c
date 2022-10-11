#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

#include "buffer.h"
#include "lexer.h"
#include "parser.h"
#include "rule.h"
#include "debug.h"
#include "config.h"
#include "logger.h"

int main(int argc, char **argv) {

    Config config = {0};
    if (!readConfigFile("config/config.cfg", &config)) {
        // TODO: Print out help info for making the config correct
        logWarn("Config: Invalid configuration file\n");
    }

    Rule *rules = NULL;
    size_t numRules = generateRules(config, &rules);

    findRuleIgnorePaths(config);

    for (uint64_t i = 1; i < argc; i++) {
        // TODO: Preprocess file

        // Open file
        Buffer fileBuff = {0};

        if (!openAndReadFileToBuffer(argv[i], &fileBuff)) {
            // There was an error reading the file
            char *fileError = strerror(errno);
            logError("Main: Couldn't read source file: %s with error: %s\n    Continuing\n",
                argv[i], fileError);
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
