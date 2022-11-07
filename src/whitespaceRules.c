#include "whitespaceRules.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>

static bool checkForTrailingSpace(RuleContext context, uint64_t pos, char *keyword) {
    Token tok = context.tokens.tokens[pos];
    uint8_t lastChar = context.fileBuffer.bytes[tok.fileIndex + strlen(keyword)];
    return (lastChar == ' ') || (lastChar == '\r') || (lastChar == '\n');
}

// Ensures 1 space after if, while, for, switch, and return
void rule_3_1_a(Rule rule, RuleContext context) {
    for (uint64_t i = 0; i < context.tokens.numTokens; i++) {
        Token token = context.tokens.tokens[i];

        char *keywordString = NULL;

        switch (token.type)
        {
            case Token_if: {
                keywordString = "if";
                break;
            }
            case Token_while: {
                keywordString = "while";
                break;
            }
            case Token_for: {
                keywordString = "for";
                break;
            }
            case Token_switch: {
                keywordString = "switch";
                break;
            }
            case Token_return: {
                keywordString = "return";
                break;
            }
            default: {
                continue;
            }
        }

        bool hasSpaceAfterToken = checkForTrailingSpace(context, i, keywordString);

        if (token.type == Token_return && !hasSpaceAfterToken) {
            // Check if it's a semicolon after return
            if (context.fileBuffer.bytes[token.fileIndex + strlen(keywordString)] == ';') {
                hasSpaceAfterToken = true;
            }
        }

        if (!hasSpaceAfterToken) {
            const char *msgBase = "has no trailing space character";

            int length = snprintf(NULL, 0, "%s %s", keywordString, msgBase);
            char *msg = calloc(length + 1, sizeof(char));
            snprintf(msg, length + 1, "%s %s", keywordString, msgBase);

            reportRuleViolation(rule.name,
                msg, token.fileName, token.line);
        }
    }
}
