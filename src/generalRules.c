#include "generalRules.h"

#include <string.h>
#include <stdio.h>

#include "traversal.h"

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

static bool token_isOnOwnLine(Token *token) {
    Token *prev = token - 1;
    Token *next = token + 1;

    if (prev->line == token->line ||
        next->line == token->line)
    {
        return false;
    }

    return true;
}

static void rule_1_3_b_traverseCompound(TraversalFuncTable *table, CompoundStmt *stmt, void *data) {
    Rule *rule = data;

    // Check if open and close brackets are alone on their line
    if (!token_isOnOwnLine(stmt->openBracket)) {
        reportRuleViolation(rule->name,
            "Open curly bracket must be alone on its line",
            stmt->openBracket->fileName, stmt->openBracket->line);
    }

    if (!token_isOnOwnLine(stmt->closeBracket)) {
        reportRuleViolation(rule->name,
            "Closing curly bracket must be alone on its line",
            stmt->closeBracket->fileName, stmt->closeBracket->line);
    }

    // Check if open and close brackets are on the same column
    if (stmt->openBracket->col != stmt->closeBracket->col) {
        reportRuleViolation(rule->name,
            "Open and close curly bracket must be on same column",
            stmt->closeBracket->fileName, stmt->closeBracket->line);
    }

    defaultTraversal_CompoundStmt(table, stmt, data);
}

// Verifies braces on own line and closing brace in same column
void rule_1_3_b(Rule rule, RuleContext context) {
    TraversalFuncTable funcTable = defaultTraversal();
    funcTable.traverse_CompoundStmt = rule_1_3_b_traverseCompound;

    traverse(funcTable, context.translationUnit, &rule);

    // Functions to override
    //  - Compound Statement
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
