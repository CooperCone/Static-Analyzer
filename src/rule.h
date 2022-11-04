#pragma once

#include "lexer.h"
#include "parser.h"
#include "config.h"

typedef struct {
    char *fileName;
    TokenList tokens;
    LineInfo lineInfo;
    TranslationUnit translationUnit;
} RuleContext;

typedef struct Rule Rule;

typedef void (*RuleValidator)(Rule rule, RuleContext context);

struct Rule {
    char *name;
    RuleValidator validator;
};

size_t generateRules(Config config, Rule **outRules);

void reportRuleViolation(char *name, char *description,
    char *fileName, uint64_t line);

void findRuleIgnorePaths(Config config);
