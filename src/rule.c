#include "rule.h"

#include <stdio.h>
#include <stddef.h>

#include "generalRules.h"

#define TermColorRed "\033[0;31m"
#define TermColorCyan "\033[1;36m"
#define TermColorWhite "\033[0;37m"
#define TermColorReset "\033[0;0m"

void reportRuleViolation(Rule rule, char *fileName, uint64_t line)
{
    printf(TermColorRed);
    printf("Error: ");
    printf(TermColorCyan);
    printf("%s:%ld - ", fileName, line);
    printf(TermColorReset);
    printf("%s - %s\n", rule.name, rule.description);
}

Rule g_rules[] = {
    { "1.2.a", "Lines no longer than 80 characters", rule_1_2_a }
};

size_t g_rulesCount = sizeof(g_rules) / sizeof(Rule);
