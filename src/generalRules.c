#include "generalRules.h"

void rule_1_2_a(Rule rule, RuleContext context) {
    for (uint64_t i = 0; i < context.tokens.numTokens; i++) {
        Token tok = context.tokens.tokens[i];

        // Greater than 81 because it's allowed for the previous
        // token to be at character 80
        if (tok.type == Token_NewLine &&
            tok.col > 81)
        {
            reportRuleViolation(rule.name, context.fileName, tok.line);
        }
    }
}
