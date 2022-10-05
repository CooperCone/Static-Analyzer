#pragma once

#include "lexer.h"
#include "config.h"

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

// TODO: Incorporate command line args
size_t generateRules(Config config, Rule **outRules);

void reportRuleViolation(char *name, char *description,
    char *fileName, uint64_t line);
