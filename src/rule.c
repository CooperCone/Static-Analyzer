#include "rule.h"

#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "trie.h"

#include "generalRules.h"

#define TermColorRed "\033[0;31m"
#define TermColorCyan "\033[1;36m"
#define TermColorWhite "\033[0;37m"
#define TermColorReset "\033[0;0m"

static Trie validFileNames;

void addInvalidRulePath(char *path) {
    trie_addString(&validFileNames, path);
}

void reportRuleViolation(char *ruleName, char *description,
    char *fileName, uint64_t line)
{
    if (trie_matchEarlyTerm(&validFileNames, fileName)) {
        // TODO: Should we log that we ignored a filename?
        return;
    }

    printf(TermColorRed);
    printf("Error: ");
    printf(TermColorCyan);
    printf("%s:%ld - ", fileName, line);
    printf(TermColorReset);
    printf("%s - %s\n", ruleName, description);
}

size_t generateRules(Config config, Rule **outRules) {
    Rule baseRules[] = {
        { "1.2.a", rule_1_2_a },
        { "1.3.b", rule_1_3_b },
        { "1.7.a", rule_1_7_a },
        { "1.7.b", rule_1_7_b },
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


