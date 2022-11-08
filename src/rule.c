#include "rule.h"

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdarg.h>

#include "trie.h"
#include "logger.h"
#include "generalRules.h"
#include "whitespaceRules.h"
#include "dataTypeRules.h"
#include "procedureRules.h"

#define TermColorRed "\033[0;31m"
#define TermColorCyan "\033[1;36m"
#define TermColorWhite "\033[0;37m"
#define TermColorReset "\033[0;0m"

static Trie validFileNames;

void findRuleIgnorePaths(Config config) {
    for (int i = 0; i < config.numConfigValues; i++) {
        ConfigValue value = config.configValues[i];
        if (value.type != ConfigValue_Map) {
            continue;
        }

        if (strcasecmp(value.mapKey, "ignorePaths") != 0)
            continue;

        if (value.mapValue->type != ConfigValue_List) {
            logError("Config: Expected ignorePaths value to be a list\n");
            continue;
        }

        ConfigValue *paths = value.mapValue;
        for (int ii = 0; ii < paths->listSize; ii++) {
            if (paths->listValues[ii].type != ConfigValue_String) {
                logError("Config: Expected ignorePaths inner value to be a string\n");
                continue;
            }

            trie_addString(&validFileNames, paths->listValues[ii].string);
        }
    }
}

void reportRuleViolation(char *ruleName, char *fileName, uint64_t line,
    char *descriptionFormat, ...)
{
    if (trie_matchEarlyTerm(&validFileNames, fileName)) {
        return;
    }

    va_list list = {0};
    va_start(list, descriptionFormat);

    printf(TermColorRed);
    printf("Error: ");
    printf(TermColorCyan);
    printf("%s:%ld - ", fileName, line);
    printf(TermColorReset);
    printf("%s - ", ruleName);
    vprintf(descriptionFormat, list);
    printf("\n");

    va_end(list);
}

size_t generateRules(Config config, Rule **outRules) {
    Rule baseRules[] = {
        { "1.2.a", rule_1_2_a },
        { "1.3.a", rule_1_3_a },
        { "1.3.b", rule_1_3_b },
        { "1.4.b", rule_1_4_b },
        { "1.7.a", rule_1_7_a },
        { "1.7.b", rule_1_7_b },
        { "3.1.a", rule_3_1_a },
        { "3.1.b", rule_3_1_b },
        { "3.1.c", rule_3_1_c },
        { "5.2.b", rule_5_2_b },
        { "6.1.a", rule_6_1_a },
        { "6.1.b", rule_6_1_b },
        { "6.1.c", rule_6_1_c },
    };
    size_t baseRuleCount = sizeof(baseRules) / sizeof(Rule);

    // Find ignore rules
    ConfigValue *ignoreValue = NULL;
    for (size_t i = 0; i < config.numConfigValues; i++) {
        ConfigValue value = config.configValues[i];
        if (value.type != ConfigValue_Map)
            continue;

        if (strcasecmp(value.mapKey, "ignorerules") != 0)
            continue;

        if (value.mapValue->type != ConfigValue_List) {
            printf("Warning: Config file has ignoreRules key, but its value isn't a list\n");
            continue;
        }

        ignoreValue = value.mapValue;
    }

    // If we have ignore rules, remove them
    size_t newRuleSize = baseRuleCount;
    if (ignoreValue != NULL) {
        for (size_t ruleIdx = 0; ruleIdx < baseRuleCount; ruleIdx++) {
            Rule *rule = baseRules + ruleIdx;

            for (size_t configIdx = 0; configIdx < ignoreValue->listSize; configIdx++) {
                ConfigValue entryValue = ignoreValue->listValues[configIdx];

                if (entryValue.type != ConfigValue_String)
                    continue;

                // If same names, remove rule
                if (strcasecmp(rule->name, entryValue.string) == 0) {
                    rule->name = NULL;
                    newRuleSize--;
                    break;
                }
            }
        }
    }

    // Copy rules in to a new list
    Rule *newRules = malloc(sizeof(Rule) * newRuleSize);
    size_t ruleIdx = 0;
    for (size_t i = 0; i < newRuleSize; i++) {
        while (baseRules[ruleIdx].name == NULL) {
            ruleIdx++;
        }
        newRules[i] = baseRules[ruleIdx];
        ruleIdx++;
    }

    *outRules = newRules;
    return newRuleSize;
}


