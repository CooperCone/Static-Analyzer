#include "rule.h"

#include <stdio.h>
#include <stddef.h>

#include "generalRules.h"

#define TermColorRed "\033[0;31m"
#define TermColorCyan "\033[1;36m"
#define TermColorWhite "\033[0;37m"
#define TermColorReset "\033[0;0m"

void reportRuleViolation(char *ruleName, char *description,
    char *fileName, uint64_t line)
{
    printf(TermColorRed);
    printf("Error: ");
    printf(TermColorCyan);
    printf("%s:%ld - ", fileName, line);
    printf(TermColorReset);
    printf("%s - %s\n", ruleName, description);
}

Rule g_rules[] = {
    { "1.2.a", rule_1_2_a },
    { "1.3.b", rule_1_3_b },
    { "1.5.a", rule_1_5_a },
};

size_t g_rulesCount = sizeof(g_rules) / sizeof(Rule);
