#include "generalRules.h"

#include <string.h>
#include <stdio.h>

// Verifies that lines not longer than 80 characters
void rule_1_2_a(Rule rule, RuleContext context) {
    LineInfo info = context.lineInfo;
    for (size_t i = 0; i < info.numFiles; i++) {

        FileInfo file = info.fileInfo[i];
        for (size_t ii = 0; ii < file.numLines; ii++) {
            if (file.lineLengths[ii] > 81) {
                reportRuleViolation(rule.name,
                    "Lines no longer than 80 characters",
                    file.fileName, ii + 1   
                );
            }
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
            while (prevIdx >= 0 && tokens[prevIdx].line == tok.line) {
                // Only allowed to be whitespace or comments
                if (tokens[prevIdx].type != Token_Comment) {
                    reportRuleViolation(rule.name,
                        "Braces must not have anything before them",
                        tok.fileName, tok.line);
                    break;
                }
                prevIdx--;
            }
        }

        // Check if anything after brace
        if (i != context.tokens.numTokens) {
            int64_t postIdx = i + 1;
            while (postIdx < context.tokens.numTokens &&
                tokens[postIdx].line == tok.line)
            {
                // Only allowed to be whitespace or comments
                if (tokens[postIdx].type != Token_Comment) {
                    reportRuleViolation(rule.name,
                        "Braces must not have anything after them",
                        tok.fileName, tok.line);
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
                    tok.fileName, tok.line);
            }
        }
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
