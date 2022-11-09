#include "variableRules.h"

#include "traversal.h"

static void rule_7_1_a_traverseDeclaration(TraversalFuncTable *table, Declaration *decl, void *data) {
    Rule *rule = data;

    static char *names[] = {
        "interrupt",
        "class",
        "true",
        "false",
        "public",
        "private",
        "friend",
        "protected",
    };

    sll_foreach(decl->initDeclaratorList.list, node) {
        InitDeclarator *initDecl = slNode_getData(node);
        Declarator declarator = initDecl->decl;

        String name = directDeclarator_getName(declarator.directDeclarator);

        for (int i = 0; i < sizeof(names) / sizeof(char*); i++) {
            if (astr_ccmp(name, names[i])) {
                reportRuleViolation(rule->name, declarator.tok->fileName, declarator.tok->line,
                    "%s", "Variable cannot have a name identical to a c++ keyword");
                break;
            }
        }
    }

    defaultTraversal_Declaration(table, decl, data);
}

// No variable names can be the same as keywords in c++
void rule_7_1_a(Rule rule, RuleContext context) {
    TraversalFuncTable table = defaultTraversal();
    table.traverse_Declaration = rule_7_1_a_traverseDeclaration;

    traverse(table, context.translationUnit, &rule);
}

static void rule_7_1_b_traverseDeclaration(TraversalFuncTable *table, Declaration *decl, void *data) {
    Rule *rule = data;

    static char *names[] = {
        "errno"
    };

    sll_foreach(decl->initDeclaratorList.list, node) {
        InitDeclarator *initDecl = slNode_getData(node);
        Declarator declarator = initDecl->decl;

        String name = directDeclarator_getName(declarator.directDeclarator);

        for (int i = 0; i < sizeof(names) / sizeof(char*); i++) {
            if (astr_ccmp(name, names[i])) {
                reportRuleViolation(rule->name, declarator.tok->fileName, declarator.tok->line,
                    "%s", "Variable cannot have a name identical to a c standard library name");
                break;
            }
        }
    }

    defaultTraversal_Declaration(table, decl, data);
}

static void rule_7_1_c_traverseDeclaration(TraversalFuncTable *table, Declaration *decl, void *data) {
    Rule *rule = data;

    sll_foreach(decl->initDeclaratorList.list, node) {
        InitDeclarator *initDecl = slNode_getData(node);
        Declarator declarator = initDecl->decl;

        String name = directDeclarator_getName(declarator.directDeclarator);

        if (name.length > 0 && name.str[0] == '_') {
            reportRuleViolation(rule->name, declarator.tok->fileName, declarator.tok->line,
                "%s", "Variable cannot start with a _");
        }
    }

    defaultTraversal_Declaration(table, decl, data);
}

// No variable names can be the same as c standard library names
void rule_7_1_b(Rule rule, RuleContext context) {
    TraversalFuncTable table = defaultTraversal();
    table.traverse_Declaration = rule_7_1_b_traverseDeclaration;

    traverse(table, context.translationUnit, &rule);
}

// No variable names can start with a _
void rule_7_1_c(Rule rule, RuleContext context) {
    TraversalFuncTable table = defaultTraversal();
    table.traverse_Declaration = rule_7_1_c_traverseDeclaration;

    traverse(table, context.translationUnit, &rule);
}
