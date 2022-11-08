#include "dataTypeRules.h"

// Cannot use keywords short and long
void rule_5_2_b(Rule rule, RuleContext context) {
    for (uint64_t i = 0; i < context.tokens.numTokens; i++) {
        Token tok = context.tokens.tokens[i];
        if (tok.type == Token_short) {
            reportRuleViolation(rule.name, tok.fileName, tok.line,
                "%s", "Type cannot use the keyword short");
        }
        else if (tok.type == Token_long) {
            reportRuleViolation(rule.name, tok.fileName, tok.line,
                "%s", "Type cannot use the keyword long");
        }
    }
}
