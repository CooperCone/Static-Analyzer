#include "generalRules.h"

#include <string.h>

// Verifies that lines not longer than 80 characters
void rule_1_2_a(Rule rule, RuleContext context) {
    for (uint64_t i = 0; i < context.tokens.numTokens; i++) {
        Token tok = context.tokens.tokens[i];

        // Greater than 81 because it's allowed for the previous
        // token to be at character 80
        if (tok.type == Token_NewLine &&
            tok.col > 81)
        {
            reportRuleViolation(rule.name,
                "Lines no longer than 80 characters",
                context.fileName, tok.line);
        }
    }
}

// Verifies braces on own line and closing brace in same column
void rule_1_3_b(Rule rule, RuleContext context) {
    uint64_t *braceStack = NULL;
    uint64_t stackSize = 0;

    Token *tokens = context.tokens.tokens;

    for (uint64_t i = 0; i < context.tokens.numTokens; i++) {
        Token tok = tokens[i];

        if (tok.type != '{' && tok.type != '}')
            continue;

        // FIXME: These two are very similar, could probably be simplified
        // Check if anything else before brace
        if (i != 0) {
            int64_t prevIdx = i - 1;
            while (prevIdx >= 0 && tokens[prevIdx].type != Token_NewLine) {
                // Only allowed to be whitespace or comments
                if (tokens[prevIdx].type != Token_Whitespace &&
                    tokens[prevIdx].type != Token_Comment)
                {
                    reportRuleViolation(rule.name,
                        "Braces must not have anything before them",
                        context.fileName, tok.line);
                    break;
                }
                prevIdx--;
            }
        }

        // Check if anything after brace
        if (i != context.tokens.numTokens) {
            int64_t postIdx = i + 1;
            while (postIdx < context.tokens.numTokens &&
                   tokens[postIdx].type != Token_NewLine)
            {
                // Only allowed to be whitespace or comments
                if (tokens[postIdx].type != Token_Whitespace &&
                    tokens[postIdx].type != Token_Comment)
                {
                    reportRuleViolation(rule.name,
                        "Braces must not have anything after them",
                        context.fileName, tok.line);
                    break;
                }
                postIdx++;
            }
        }

        // Add to stack
        if (tok.type == '{') {
            stackSize++;
            braceStack = realloc(braceStack, sizeof(uint64_t) * stackSize);
            braceStack[stackSize - 1] = tok.col;
        }

        // Pop from stack and verify same column
        else if (tok.type == '}') {
            stackSize--;
            uint64_t openColumn = braceStack[stackSize];

            if (openColumn != tok.col) {
                reportRuleViolation(rule.name,
                    "Closing braces must be on the same column as their opening",
                    context.fileName, tok.line);
            }
        }
    }
}

// Verifies no single letter variable names
// NOTE: Doesn't catch all abbreviations!!!
// TODO: Should we look for common abbreviations like idx?
void rule_1_5_a(Rule rule, RuleContext context) {
    for (uint64_t i = 0; i < context.tokens.numTokens; i++) {
        Token tok = context.tokens.tokens[i];

        if (tok.type != Token_Ident)
            continue;

        if (strlen(tok.ident) > 1)
            continue;

        // TODO: Pull these from a config file
        if (strcmp(tok.ident, "i") == 0)
            continue;

        if (strcmp(tok.ident, "c") == 0)
            continue;

        if (strcmp(tok.ident, "x") == 0)
            continue;

        if (strcmp(tok.ident, "y") == 0)
            continue;

        reportRuleViolation(rule.name,
            "Variable names should be descriptive, thus not a single letter",
            context.fileName, tok.line);
    }
}

// FIXME: Rules 1.7.a and 1.7.b are pretty much identical,
//        so they should probably be refactored
// Verifies there is no use of auto keyword
void rule_1_7_a(Rule rule, RuleContext context) {
    for (uint64_t i = 0; i < context.tokens.numTokens; i++) {
        Token tok = context.tokens.tokens[i];
        if (tok.type == Token_auto) {
            reportRuleViolation(rule.name,
                "Use of auto keyword is prohibited",
                context.fileName, tok.line);
        }
    }
}

// Verifies there is no use of register keyword
void rule_1_7_b(Rule rule, RuleContext context) {
    for (uint64_t i = 0; i < context.tokens.numTokens; i++) {
        Token tok = context.tokens.tokens[i];
        if (tok.type == Token_register) {
            reportRuleViolation(rule.name,
                "Use of register keyword is prohibited",
                context.fileName, tok.line);
        }
    }
}
