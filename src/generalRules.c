#include "generalRules.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>

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

static void rule_1_3_a_traverseSelection(TraversalFuncTable *table, SelectionStatement *stmt, void *data) {
    Rule *rule = data;

    if (stmt->type == SelectionStatement_If) {
        if (stmt->ifTrueStmt->type != Statement_Compound) {
            reportRuleViolation(rule->name,
                "If statement true block isn't a compound statement",
                stmt->ifToken->fileName, stmt->ifToken->line
            );
        }

        if (stmt->ifHasElse && stmt->ifFalseStmt->type != Statement_Compound) {
            reportRuleViolation(rule->name,
                "If statement false block isn't a compound statement",
                stmt->elseToken->fileName, stmt->elseToken->line
            );
        }
    }
    else if (stmt->type == SelectionStatement_Switch) {
        if (stmt->switchStmt->type != Statement_Compound) {
            reportRuleViolation(rule->name,
                "Switch statement block isn't a compound statement",
                stmt->switchToken->fileName, stmt->switchToken->line
            );
        }
    }
    else {
        assert(false);
    }

    // Continue traversal
    defaultTraversal_SelectionStatement(table, stmt, data);
}

static void rule_1_3_a_traverseIteration(TraversalFuncTable *table, IterationStatement *stmt, void *data) {
    Rule *rule = data;

    if (stmt->type == IterationStatement_While) {
        if (stmt->whileStmt->type != Statement_Compound) {
            reportRuleViolation(rule->name,
                "While statement block isn't a compound statement",
                stmt->whileToken->fileName, stmt->whileToken->line
            );
        }
    }
    else if (stmt->type == IterationStatement_DoWhile) {
        if (stmt->doStmt->type != Statement_Compound) {
            reportRuleViolation(rule->name,
                "Do While statement block isn't a compound statement",
                stmt->doToken->fileName, stmt->doToken->line
            );
        }
    }
    else if (stmt->type == IterationStatement_For) {
        if (stmt->forStmt->type != Statement_Compound) {
            reportRuleViolation(rule->name,
                "For statement block isn't a compound statement",
                stmt->forToken->fileName, stmt->forToken->line
            );
        }
    }
    else {
        assert(false);
    }

    defaultTraversal_IterationStatement(table, stmt, data);
}

// Verifies each selection and iteration statement has a compound statement
void rule_1_3_a(Rule rule, RuleContext context) {
    // For

    TraversalFuncTable funcTable = defaultTraversal();
    funcTable.traverse_SelectionStatement = rule_1_3_a_traverseSelection;
    funcTable.traverse_IterationStatement = rule_1_3_a_traverseIteration;

    traverse(funcTable, context.translationUnit, &rule);
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

    // Continue traversal
    defaultTraversal_CompoundStmt(table, stmt, data);
}

// Verifies braces on own line and closing brace in same column
void rule_1_3_b(Rule rule, RuleContext context) {
    TraversalFuncTable funcTable = defaultTraversal();
    funcTable.traverse_CompoundStmt = rule_1_3_b_traverseCompound;

    traverse(funcTable, context.translationUnit, &rule);
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
