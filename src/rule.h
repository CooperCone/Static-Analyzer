#pragma once

#include "lexer.h"

typedef struct {
    char *fileName;
    TokenList tokens;
} RuleContext;

typedef struct Rule Rule;

typedef void (*RuleValidator)(Rule rule, RuleContext context);

struct Rule {
    char *name;
    char *description;
    RuleValidator validator;
};

void reportRuleViolation(Rule rule, char *fileName, uint64_t line);
