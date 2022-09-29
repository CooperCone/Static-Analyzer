#include "rule.h"

#include <stdio.h>
#include <stddef.h>

#include "generalRules.h"

void reportRuleViolation(char *ruleName, char *fileName, uint64_t line) {
    printf("Error: %s:%ld - %s\n", fileName, line, ruleName);
}

Rule g_rules[] = {
    { "1.2.a", rule_1_2_a }
};

size_t g_rulesCount = sizeof(g_rules) / sizeof(Rule);
