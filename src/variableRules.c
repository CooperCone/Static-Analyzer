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

// No variable names as keywords in c++
void rule_7_1_a(Rule rule, RuleContext context) {
    TraversalFuncTable table = defaultTraversal();
    table.traverse_Declaration = rule_7_1_a_traverseDeclaration;

    traverse(table, context.translationUnit, &rule);
}
