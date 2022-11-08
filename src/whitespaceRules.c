#include "whitespaceRules.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "traversal.h"

static bool checkForTrailingSpace(RuleContext context, Token tok, size_t length) {
    uint8_t lastChar = context.fileBuffer.bytes[tok.fileIndex + length];
    return (lastChar == ' ') || (lastChar == '\r') || (lastChar == '\n');
}

static bool checkForLeadingSpace(RuleContext context, Token tok) {
    uint8_t lastChar = context.fileBuffer.bytes[tok.fileIndex - 1];
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

        bool hasSpaceAfterToken = checkForTrailingSpace(context, token, strlen(keywordString));

        if (token.type == Token_return && !hasSpaceAfterToken) {
            // Check if it's a semicolon after return
            if (context.fileBuffer.bytes[token.fileIndex + strlen(keywordString)] == ';') {
                hasSpaceAfterToken = true;
            }
        }

        if (!hasSpaceAfterToken) {
            reportRuleViolation(rule.name, token.fileName, token.line,
                "%s %s", keywordString, "has no trailing space character");
        }
    }
}

// Ensures 1 space before and after =, +=, -=, *=, /=, %=, &=, |=, ^=, and !=
void rule_3_1_b(Rule rule, RuleContext context) {
    for (uint64_t i = 0; i < context.tokens.numTokens; i++) {
        Token token = context.tokens.tokens[i];

        char *opString = NULL;

        if (token.type == '=')
            opString = "=";
        else if (token.type == Token_AddAssign)
            opString = "+=";
        else if (token.type == Token_SubAssign)
            opString = "-=";
        else if (token.type == Token_MulAssign)
            opString = "*=";
        else if (token.type == Token_DivAssign)
            opString = "/=";
        else if (token.type == Token_ModAssign)
            opString = "*=";
        else if (token.type == Token_AndAssign)
            opString = "&=";
        else if (token.type == Token_OrAssign)
            opString = "|=";
        else if (token.type == Token_XorAssign)
            opString = "^=";
        else
            continue;

        bool hasSpaceAfterToken = checkForTrailingSpace(context, token, strlen(opString));
        bool hasSpaceBeforeToken = checkForLeadingSpace(context, token);

        if (!hasSpaceAfterToken) {
            reportRuleViolation(rule.name, token.fileName, token.line,
                "%s %s", opString, "has no trailing space character");
        }

        if (!hasSpaceBeforeToken) {
            reportRuleViolation(rule.name, token.fileName, token.line,
                "%s %s", opString, "has no leading space character");
        }
    }
}

typedef struct {
    Rule rule;
    RuleContext context;
} RuleAndContext;

static void rule_3_1_c_traverseMultiplicative(TraversalFuncTable *table, MultiplicativeExpr *expr, void *data) {
    RuleAndContext *ruleAndContext = data;
    Rule rule = ruleAndContext->rule;
    RuleContext context = ruleAndContext->context;

    sll_foreach(expr->postExprs, node) {
        MultiplicativePost *post = slNode_getData(node);

        char *multiplicativeStr = NULL;

        if (post->op == Multiplicative_Mul)
            multiplicativeStr = "*";
        else if (post->op == Multiplicative_Div)
            multiplicativeStr = "/";
        else if (post->op == Multiplicative_Mod)
            multiplicativeStr = "%";
        else
            assert(false);

        if (!checkForLeadingSpace(context, *(post->tok))) {
            reportRuleViolation(rule.name, post->tok->fileName, post->tok->line,
                "%s %s", multiplicativeStr, "has no leading space character");
        }

        if (!checkForTrailingSpace(context, *(post->tok), strlen(multiplicativeStr))) {
            reportRuleViolation(rule.name, post->tok->fileName, post->tok->line,
                "%s %s", multiplicativeStr, "has no trailing space character");
        }
    }

    defaultTraversal_MultiplicativeExpr(table, expr, data);
}

static void rule_3_1_c_traverseAdditive(TraversalFuncTable *table, AdditiveExpr *expr, void *data) {
    RuleAndContext *ruleAndContext = data;
    Rule rule = ruleAndContext->rule;
    RuleContext context = ruleAndContext->context;

    sll_foreach(expr->postExprs, node) {
        AdditivePost *post = slNode_getData(node);

        char *additiveStr = NULL;

        if (post->op == Additive_Add)
            additiveStr = "+";
        else if (post->op == Additive_Sub)
            additiveStr = "-";
        else
            assert(false);

        if (!checkForLeadingSpace(context, *(post->tok))) {
            reportRuleViolation(rule.name, post->tok->fileName, post->tok->line,
                "%s %s", additiveStr, "has no leading space character");
        }

        if (!checkForTrailingSpace(context, *(post->tok), strlen(additiveStr))) {
            reportRuleViolation(rule.name, post->tok->fileName, post->tok->line,
                "%s %s", additiveStr, "has no trailing space character");
        }
    }

    defaultTraversal_AdditiveExpr(table, expr, data);
}

static void rule_3_1_c_traverseShift(TraversalFuncTable *table, ShiftExpr *expr, void *data) {
    RuleAndContext *ruleAndContext = data;
    Rule rule = ruleAndContext->rule;
    RuleContext context = ruleAndContext->context;

    sll_foreach(expr->postExprs, node) {
        ShiftPost *post = slNode_getData(node);

        char *shiftStr = NULL;

        if (post->op == Shift_Left)
            shiftStr = "<<";
        else if (post->op == Shift_Right)
            shiftStr = ">>";
        else
            assert(false);

        if (!checkForLeadingSpace(context, *(post->tok))) {
            reportRuleViolation(rule.name, post->tok->fileName, post->tok->line,
                "%s %s", shiftStr, "has no leading space character");
        }

        if (!checkForTrailingSpace(context, *(post->tok), strlen(shiftStr))) {
            reportRuleViolation(rule.name, post->tok->fileName, post->tok->line,
                "%s %s", shiftStr, "has no trailing space character");
        }
    }

    defaultTraversal_ShiftExpr(table, expr, data);
}

static void rule_3_1_c_traverseRelational(TraversalFuncTable *table, RelationalExpr *expr, void *data) {
    RuleAndContext *ruleAndContext = data;
    Rule rule = ruleAndContext->rule;
    RuleContext context = ruleAndContext->context;

    sll_foreach(expr->postExprs, node) {
        RelationalPost *post = slNode_getData(node);

        char *relationalStr = NULL;

        if (post->op == Relational_Lt)
            relationalStr = "<";
        else if (post->op == Relational_Gt)
            relationalStr = ">";
        else if (post->op == Relational_LEq)
            relationalStr = "<=";
        else if (post->op == Relational_GEq)
            relationalStr = ">=";
        else
            assert(false);

        if (!checkForLeadingSpace(context, *(post->tok))) {
            reportRuleViolation(rule.name, post->tok->fileName, post->tok->line,
                "%s %s", relationalStr, "has no leading space character");
        }

        if (!checkForTrailingSpace(context, *(post->tok), strlen(relationalStr))) {
            reportRuleViolation(rule.name, post->tok->fileName, post->tok->line,
                "%s %s", relationalStr, "has no trailing space character");
        }
    }

    defaultTraversal_RelationalExpr(table, expr, data);
}

static void rule_3_1_c_traverseEquality(TraversalFuncTable *table, EqualityExpr *expr, void *data) {
    RuleAndContext *ruleAndContext = data;
    Rule rule = ruleAndContext->rule;
    RuleContext context = ruleAndContext->context;

    sll_foreach(expr->postExprs, node) {
        EqualityPost *post = slNode_getData(node);

        char *equalityStr = NULL;

        if (post->op == Equality_Eq)
            equalityStr = "==";
        else if (post->op == Equality_NEq)
            equalityStr = "!=";
        else
            assert(false);

        if (!checkForLeadingSpace(context, *(post->tok))) {
            reportRuleViolation(rule.name, post->tok->fileName, post->tok->line,
                "%s %s", equalityStr, "has no leading space character");
        }

        if (!checkForTrailingSpace(context, *(post->tok), strlen(equalityStr))) {
            reportRuleViolation(rule.name, post->tok->fileName, post->tok->line,
                "%s %s", equalityStr, "has no trailing space character");
        }
    }

    defaultTraversal_EqualityExpr(table, expr, data);
}

static void rule_3_1_c_traverseAnd(TraversalFuncTable *table, AndExpr *expr, void *data) {
    RuleAndContext *ruleAndContext = data;
    Rule rule = ruleAndContext->rule;
    RuleContext context = ruleAndContext->context;

    if (expr->list.size == 0) {
        // I don't think this is possible
        assert(false);
    }
    else if (expr->list.size > 1) {
        SLList oneAfter = expr->list;
        oneAfter.head = oneAfter.head->next;

        sll_foreach(oneAfter, node) {
            EqualityExpr *eq = slNode_getData(node);
            Token *tok = eq->tok - 1;

            if (!checkForLeadingSpace(context, *tok)) {
                reportRuleViolation(rule.name, tok->fileName, tok->line,
                    "& has no leading space character");
            }

            if (!checkForTrailingSpace(context, *tok, strlen("&"))) {
                reportRuleViolation(rule.name, tok->fileName, tok->line,
                    "& has no trailing space character");
            }
        }
    }

    defaultTraversal_AndExpr(table, expr, data);
}

static void rule_3_1_c_traverseExclusiveOr(TraversalFuncTable *table, ExclusiveOrExpr *expr, void *data) {
    RuleAndContext *ruleAndContext = data;
    Rule rule = ruleAndContext->rule;
    RuleContext context = ruleAndContext->context;

    if (expr->list.size == 0) {
        // I don't think this is possible
        assert(false);
    }
    else if (expr->list.size > 1) {
        SLList oneAfter = expr->list;
        oneAfter.head = oneAfter.head->next;

        sll_foreach(oneAfter, node) {
            AndExpr *and = slNode_getData(node);
            Token *tok = and->tok - 1;

            if (!checkForLeadingSpace(context, *tok)) {
                reportRuleViolation(rule.name, tok->fileName, tok->line,
                    "^ has no leading space character");
            }

            if (!checkForTrailingSpace(context, *tok, strlen("^"))) {
                reportRuleViolation(rule.name, tok->fileName, tok->line,
                    "^ has no trailing space character");
            }
        }
    }

    defaultTraversal_ExclusiveOrExpr(table, expr, data);
}

static void rule_3_1_c_traverseInclusiveOr(TraversalFuncTable *table, InclusiveOrExpr *expr, void *data) {
    RuleAndContext *ruleAndContext = data;
    Rule rule = ruleAndContext->rule;
    RuleContext context = ruleAndContext->context;

    if (expr->list.size == 0) {
        // I don't think this is possible
        assert(false);
    }
    else if (expr->list.size > 1) {
        SLList oneAfter = expr->list;
        oneAfter.head = oneAfter.head->next;

        sll_foreach(oneAfter, node) {
            ExclusiveOrExpr *or = slNode_getData(node);
            Token *tok = or->tok - 1;

            if (!checkForLeadingSpace(context, *tok)) {
                reportRuleViolation(rule.name, tok->fileName, tok->line,
                    "| has no leading space character");
            }

            if (!checkForTrailingSpace(context, *tok, strlen("|"))) {
                reportRuleViolation(rule.name, tok->fileName, tok->line,
                    "| has no trailing space character");
            }
        }
    }

    defaultTraversal_InclusiveOrExpr(table, expr, data);
}

static void rule_3_1_c_traverseLogicalAnd(TraversalFuncTable *table, LogicalAndExpr *expr, void *data) {
    RuleAndContext *ruleAndContext = data;
    Rule rule = ruleAndContext->rule;
    RuleContext context = ruleAndContext->context;

    if (expr->list.size == 0) {
        // I don't think this is possible
        assert(false);
    }
    else if (expr->list.size > 1) {
        SLList oneAfter = expr->list;
        oneAfter.head = oneAfter.head->next;

        sll_foreach(oneAfter, node) {
            InclusiveOrExpr *or = slNode_getData(node);
            Token *tok = or->tok - 1;

            if (!checkForLeadingSpace(context, *tok)) {
                reportRuleViolation(rule.name, tok->fileName, tok->line,
                    "&& has no leading space character");
            }

            if (!checkForTrailingSpace(context, *tok, strlen("&&"))) {
                reportRuleViolation(rule.name, tok->fileName, tok->line,
                    "&& has no trailing space character");
            }
        }
    }

    defaultTraversal_LogicalAndExpr(table, expr, data);
}

static void rule_3_1_c_traverseLogicalOr(TraversalFuncTable *table, LogicalOrExpr *expr, void *data) {
    RuleAndContext *ruleAndContext = data;
    Rule rule = ruleAndContext->rule;
    RuleContext context = ruleAndContext->context;

    if (expr->list.size == 0) {
        // I don't think this is possible
        assert(false);
    }
    else if (expr->list.size > 1) {
        SLList oneAfter = expr->list;
        oneAfter.head = oneAfter.head->next;

        sll_foreach(oneAfter, node) {
            LogicalAndExpr *and = slNode_getData(node);
            Token *tok = and->tok - 1;

            if (!checkForLeadingSpace(context, *tok)) {
                reportRuleViolation(rule.name, tok->fileName, tok->line,
                    "|| has no leading space character");
            }

            if (!checkForTrailingSpace(context, *tok, strlen("||"))) {
                reportRuleViolation(rule.name, tok->fileName, tok->line,
                    "|| has no trailing space character");
            }
        }
    }

    defaultTraversal_LogicalOrExpr(table, expr, data);
}

// Ensures 1 space before and after +, -, *, /, %, <, <=, >, >=, ==,
// !=, <<, >>, &, |, ^, &&, ||
void rule_3_1_c(Rule rule, RuleContext context) {
    TraversalFuncTable funcTable = defaultTraversal();
    funcTable.traverse_LogicalOrExpr = rule_3_1_c_traverseLogicalOr;
    funcTable.traverse_LogicalAndExpr = rule_3_1_c_traverseLogicalAnd;
    funcTable.traverse_InclusiveOrExpr = rule_3_1_c_traverseInclusiveOr;
    funcTable.traverse_ExclusiveOrExpr = rule_3_1_c_traverseExclusiveOr;
    funcTable.traverse_AndExpr = rule_3_1_c_traverseAnd;
    funcTable.traverse_EqualityExpr = rule_3_1_c_traverseEquality;
    funcTable.traverse_RelationalExpr = rule_3_1_c_traverseRelational;
    funcTable.traverse_ShiftExpr = rule_3_1_c_traverseShift;
    funcTable.traverse_AdditiveExpr = rule_3_1_c_traverseAdditive;
    funcTable.traverse_MultiplicativeExpr = rule_3_1_c_traverseMultiplicative;

    RuleAndContext ruleAndContext = { .rule = rule, .context = context };
    traverse(funcTable, context.translationUnit, &ruleAndContext);
}
