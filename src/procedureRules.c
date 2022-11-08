#include "procedureRules.h"

#include "traversal.h"

static void rule_6_1_a_traverseFuncDef(TraversalFuncTable *table, FuncDef *def, void *data) {
    Rule *rule = data;

    static char *names[] = {
        "interrupt",
        "inline",
        "class",
        "true",
        "false",
        "public",
        "private",
        "friend",
        "protected",
    };

    for (size_t i = 0; i < sizeof(names) / sizeof(char*); i++) {
        String name = directDeclarator_getName(def->declarator.directDeclarator);
        Token *tok = def->declarator.tok;

        if (astr_ccmp(name, names[i])) {
            reportRuleViolation(rule->name, tok->fileName, tok->line,
                "Procedure cannot have name of: %s", names[i]);
        }
    }
}

// Procedures cannot have names that is a keyword in c or c++
// TODO: Find more names that should be restricted
void rule_6_1_a(Rule rule, RuleContext context) {
    TraversalFuncTable table = defaultTraversal();
    table.traverse_FuncDef = rule_6_1_a_traverseFuncDef;

    traverse(table, context.translationUnit, &rule);
}

static void rule_6_1_b_traverseFuncDef(TraversalFuncTable *table, FuncDef *def, void *data) {
    Rule *rule = data;

    static char *names[] = {
        "strlen",
        "atoi",
        "memset",
    };

    for (size_t i = 0; i < sizeof(names) / sizeof(char*); i++) {
        String name = directDeclarator_getName(def->declarator.directDeclarator);
        Token *tok = def->declarator.tok;

        if (astr_ccmp(name, names[i])) {
            reportRuleViolation(rule->name, tok->fileName, tok->line,
                "Procedure cannot have name of: %s", names[i]);
        }
    }
}

// Procedures cannot have a name that is a function in c stdlib
// TODO: Find more names that should be restricted
void rule_6_1_b(Rule rule, RuleContext context) {
    TraversalFuncTable table = defaultTraversal();
    table.traverse_FuncDef = rule_6_1_b_traverseFuncDef;

    traverse(table, context.translationUnit, &rule);
}

static void rule_6_1_c_traverseFuncDef(TraversalFuncTable *table, FuncDef *def, void *data) {
    Rule *rule = data;

    String name = directDeclarator_getName(def->declarator.directDeclarator);
    Token *tok = def->declarator.tok;

    if (name.length > 0 && name.str[0] == '_') {
        reportRuleViolation(rule->name, tok->fileName, tok->line,
            "%s", "Procedure cannot have name that starts with _");

    }
}

// Procedures cannot have a name that starts with an _
void rule_6_1_c(Rule rule, RuleContext context) {
    TraversalFuncTable table = defaultTraversal();
    table.traverse_FuncDef = rule_6_1_c_traverseFuncDef;

    traverse(table, context.translationUnit, &rule);
}

static void rule_6_1_d_traverseFuncDef(TraversalFuncTable *table, FuncDef *def, void *data) {
    Rule *rule = data;

    String name = directDeclarator_getName(def->declarator.directDeclarator);
    Token *tok = def->declarator.tok;

    if (name.length > 31) {
        reportRuleViolation(rule->name, tok->fileName, tok->line,
            "%s", "Procedure cannot have name thats longer than 31 characters");
    }
}

// Procedures cannot have a name thats longer than 31 characters
void rule_6_1_d(Rule rule, RuleContext context) {
    TraversalFuncTable table = defaultTraversal();
    table.traverse_FuncDef = rule_6_1_d_traverseFuncDef;

    traverse(table, context.translationUnit, &rule);
}
