#pragma once

#include "lexer.h"

typedef struct {
    char *fileName;
    TokenList tokens;
    LineInfo lineInfo;
} RuleContext;

typedef struct Rule Rule;

typedef void (*RuleValidator)(Rule rule, RuleContext context);

struct Rule {
    char *name;
    RuleValidator validator;
};

void reportRuleViolation(char *name, char *description,
    char *fileName, uint64_t line);
