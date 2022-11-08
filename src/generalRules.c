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
                    file.fileName, ii + 1,
                    "%s", "Lines no longer than 80 characters"
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
                stmt->ifToken->fileName, stmt->ifToken->line,
                "%s", "If statement true block isn't a compound statement"
            );
        }

        if (stmt->ifHasElse && stmt->ifFalseStmt->type != Statement_Compound) {
            reportRuleViolation(rule->name,
                stmt->elseToken->fileName, stmt->elseToken->line,
                "%s", "If statement false block isn't a compound statement"
            );
        }
    }
    else if (stmt->type == SelectionStatement_Switch) {
        if (stmt->switchStmt->type != Statement_Compound) {
            reportRuleViolation(rule->name,
                stmt->switchToken->fileName, stmt->switchToken->line,
                "%s", "Switch statement block isn't a compound statement"
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
                stmt->whileToken->fileName, stmt->whileToken->line,
                "%s", "While statement block isn't a compound statement"
            );
        }
    }
    else if (stmt->type == IterationStatement_DoWhile) {
        if (stmt->doStmt->type != Statement_Compound) {
            reportRuleViolation(rule->name,
                stmt->doToken->fileName, stmt->doToken->line,
                "%s", "Do While statement block isn't a compound statement"
            );
        }
    }
    else if (stmt->type == IterationStatement_For) {
        if (stmt->forStmt->type != Statement_Compound) {
            reportRuleViolation(rule->name,
                stmt->forToken->fileName, stmt->forToken->line,
                "%s", "For statement block isn't a compound statement"
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
            stmt->openBracket->fileName, stmt->openBracket->line,
            "%s", "Open curly bracket must be alone on its line"
        );
    }

    if (!token_isOnOwnLine(stmt->closeBracket)) {
        reportRuleViolation(rule->name,
            stmt->closeBracket->fileName, stmt->closeBracket->line,
            "%s", "Closing curly bracket must be alone on its line"
        );
    }

    // Check if open and close brackets are on the same column
    if (stmt->openBracket->col != stmt->closeBracket->col) {
        reportRuleViolation(rule->name,
            stmt->closeBracket->fileName, stmt->closeBracket->line,
            "%s", "Open and close curly bracket must be on same column"
        );
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

static void rule_1_4_b_dirtyTraversePostfix(TraversalFuncTable *table, PostfixExpr *expr, void *data) {
    Rule *rule = data;

    if (expr->type != Postfix_Primary) {
        reportRuleViolation(rule->name,
            expr->tok->fileName, expr->tok->line,
            "%s", "Postfix expression was expected to be surrounded by parens because of a root && or ||"
        );
    }
    else {
        table->traverse_PrimaryExpr(table, &(expr->primary), data);
    }
}

static void rule_1_4_b_dirtyTraverseUnary(TraversalFuncTable *table, UnaryExpr *expr, void *data) {
    Rule *rule = data;

    if (expr->type != UnaryExpr_Base) {
        reportRuleViolation(rule->name,
            expr->tok->fileName, expr->tok->line,
            "%s", "Unary expression was expected to be surrounded by parens because of a root && or ||"
        );
    }
    else {
        rule_1_4_b_dirtyTraversePostfix(table, &(expr->baseExpr), data);
    }
}

static void rule_1_4_b_dirtyTraverseCast(TraversalFuncTable *table, CastExpr *expr, void *data) {
    Rule *rule = data;

    if (expr->type != CastExpr_Unary) {
        reportRuleViolation(rule->name,
            expr->tok->fileName, expr->tok->line,
            "%s", "Cast expression was expected to be surrounded by parens because of a root && or ||"
        );
    }
    else {
        rule_1_4_b_dirtyTraverseUnary(table, &(expr->unary), data);
    }
}

static void rule_1_4_b_dirtyTraverseMultiplicative(TraversalFuncTable *table, MultiplicativeExpr *expr, void *data) {
    Rule *rule = data;

    if (expr->postExprs.size > 0) {
        reportRuleViolation(rule->name,
            expr->tok->fileName, expr->tok->line,
            "%s", "Multiplicative expression was expected to be surrounded by parens because of a root && or ||"
        );
    }
    else {
        rule_1_4_b_dirtyTraverseCast(table, &(expr->baseExpr), data);
    }
}

static void rule_1_4_b_dirtyTraverseAdditive(TraversalFuncTable *table, AdditiveExpr *expr, void *data) {
    Rule *rule = data;

    if (expr->postExprs.size > 0) {
        reportRuleViolation(rule->name,
            expr->tok->fileName, expr->tok->line,
            "%s", "Additive expression was expected to be surrounded by parens because of a root && or ||"
        );
    }
    else {
        rule_1_4_b_dirtyTraverseMultiplicative(table, &(expr->baseExpr), data);
    }
}

static void rule_1_4_b_dirtyTraverseShift(TraversalFuncTable *table, ShiftExpr *expr, void *data) {
    Rule *rule = data;

    if (expr->postExprs.size > 0) {
        reportRuleViolation(rule->name,
            expr->tok->fileName, expr->tok->line,
            "%s", "Shift expression was expected to be surrounded by parens because of a root && or ||"
        );
    }
    else {
        rule_1_4_b_dirtyTraverseAdditive(table, &(expr->baseExpr), data);
    }
}

static void rule_1_4_b_dirtyTraverseRelational(TraversalFuncTable *table, RelationalExpr *expr, void *data) {
    Rule *rule = data;

    if (expr->postExprs.size > 0) {
        reportRuleViolation(rule->name,
            expr->tok->fileName, expr->tok->line,
            "%s", "Relational expression was expected to be surrounded by parens because of a root && or ||"
        );
    }
    else {
        rule_1_4_b_dirtyTraverseShift(table, &(expr->baseExpr), data);
    }
}

static void rule_1_4_b_dirtyTraverseEquality(TraversalFuncTable *table, EqualityExpr *expr, void *data) {
    Rule *rule = data;

    if (expr->postExprs.size > 0) {
        reportRuleViolation(rule->name,
            expr->tok->fileName, expr->tok->line,
            "%s", "Equality expression was expected to be surrounded by parens because of a root && or ||"
        );
    }
    else {
        rule_1_4_b_dirtyTraverseRelational(table, &(expr->baseExpr), data);
    }
}

static void rule_1_4_b_dirtyTraverseAnd(TraversalFuncTable *table, AndExpr *expr, void *data) {
    Rule *rule = data;

    if (expr->list.size > 1) {
        reportRuleViolation(rule->name,
            expr->tok->fileName, expr->tok->line,
            "%s", "And expression was expected to be surrounded by parens because of a root && or ||"
        );
    }
    else {
        EqualityExpr *eq = slNode_getData(expr->list.head);
        rule_1_4_b_dirtyTraverseEquality(table, eq, data);
    }
}

static void rule_1_4_b_dirtyTraverseExclusiveOr(TraversalFuncTable *table, ExclusiveOrExpr *expr, void *data) {
    Rule *rule = data;

    if (expr->list.size > 1) {
        reportRuleViolation(rule->name,
            expr->tok->fileName, expr->tok->line,
            "%s", "Exclusive or expression was expected to be surrounded by parens because of a root && or ||"
        );
    }
    else {
        AndExpr *and = slNode_getData(expr->list.head);
        rule_1_4_b_dirtyTraverseAnd(table, and, data);
    }
}

static void rule_1_4_b_dirtyTraverseInclusiveOr(TraversalFuncTable *table, InclusiveOrExpr *expr, void *data) {
    Rule *rule = data;

    if (expr->list.size > 1) {
        reportRuleViolation(rule->name,
            expr->tok->fileName, expr->tok->line,
            "%s", "Inclusive or expression was expected to be surrounded by parens because of a root && or ||"
        );
    }
    else {
        ExclusiveOrExpr *or = slNode_getData(expr->list.head);
        rule_1_4_b_dirtyTraverseExclusiveOr(table, or, data);
    }
}

static void rule_1_4_b_dirtyTraverseLogicalAnd(TraversalFuncTable *table, LogicalAndExpr *expr, void *data) {
    Rule *rule = data;

    if (expr->list.size > 1) {
        reportRuleViolation(rule->name,
            expr->tok->fileName, expr->tok->line,
            "%s", "Logical and expression was expected to be surrounded by parens because of a root && or ||"
        );
    }
    else {
        InclusiveOrExpr *or = slNode_getData(expr->list.head);
        rule_1_4_b_dirtyTraverseInclusiveOr(table, or, data);
    }
}

static void rule_1_4_b_traverseLogicalAnd(TraversalFuncTable *table, LogicalAndExpr *expr, void *data) {
    if (expr->list.size == 0) {
        // I don't think this is possible
        assert(false);
    }
    else if (expr->list.size == 1) {
        defaultTraversal_LogicalAndExpr(table, expr, data);
    }
    else {
        sll_foreach(expr->list, node) {
            InclusiveOrExpr *or = slNode_getData(node);
            rule_1_4_b_dirtyTraverseInclusiveOr(table, or, data);
        }
    }
}

static void rule_1_4_b_traverseLogicalOr(TraversalFuncTable *table, LogicalOrExpr *expr, void *data) {
    if (expr->list.size == 0) {
        // I don't think this is possible
        assert(false);
    }
    else if (expr->list.size == 1) {
        defaultTraversal_LogicalOrExpr(table, expr, data);
    }
    else {
        sll_foreach(expr->list, node) {
            LogicalAndExpr *and = slNode_getData(node);
            rule_1_4_b_dirtyTraverseLogicalAnd(table, and, data);
        }
    }
}

// Verifies && and || use parens on either side for complex exprs
void rule_1_4_b(Rule rule, RuleContext context) {
    TraversalFuncTable funcTable = defaultTraversal();
    funcTable.traverse_LogicalOrExpr = rule_1_4_b_traverseLogicalOr;
    funcTable.traverse_LogicalAndExpr = rule_1_4_b_traverseLogicalAnd;

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
                context.fileName, tok.line,
                "%s", "Use of auto keyword is prohibited"
            );
        }
    }
}

// Verifies there is no use of register keyword
void rule_1_7_b(Rule rule, RuleContext context) {
    for (uint64_t i = 0; i < context.tokens.numTokens; i++) {
        Token tok = context.tokens.tokens[i];
        if (tok.type == Token_register) {
            reportRuleViolation(rule.name,
                context.fileName, tok.line,
                "%s", "Use of register keyword is prohibited"
            );
        }
    }
}
