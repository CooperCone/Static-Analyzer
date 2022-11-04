#include "traversal.h"

#include <assert.h>

void defaultTraversal_Designator(TraversalFuncTable *table,
    Designator *desig, void *data)
{
    if (desig->type == Designator_Constant) {
        table->traverse_ConditionalExpr(table, desig->constantExpr, data);
    }
}

void defaultTraversal_Designation(TraversalFuncTable *table,
    Designation *desig, void *data)
{
    sll_foreach(desig->list, node) {
        Designator *designator = slNode_getData(node);
        table->traverse_Designator(table, designator, data);
    }
}

void defaultTraversal_Initializer(TraversalFuncTable *table,
    Initializer *init, void *data)
{
    if (init->type == Initializer_InitializerList) {
        table->traverse_InitializerList(table, init->initializerList, data);
    }
    else if (init->type == Initializer_Assignment) {
        table->traverse_AssignExpr(table, init->assignmentExpr, data);
    }
    else {
        assert(false);
    }
}

void defaultTraversal_DesignationAndInitializer(TraversalFuncTable *table,
    DesignationAndInitializer *desig, void *data)
{
    if (desig->hasDesignation) {
        table->traverse_Designation(table, &(desig->designation), data);
    }

    table->traverse_Initializer(table, &(desig->initializer), data);
}

void defaultTraversal_InitializerList(TraversalFuncTable *table,
    InitializerList *list, void *data)
{
    sll_foreach(list->list, node) {
        DesignationAndInitializer *desig = slNode_getData(node);
        table->traverse_DesignationAndInitializer(table, desig, data);
    }
}

void defaultTraversal_GenericAssociation(TraversalFuncTable *table,
    GenericAssociation *association, void *data)
{
    if (!association->isDefault) {
        table->traverse_TypeName(table, association->typeName, data);
    }
    table->traverse_AssignExpr(table, association->expr, data);
}

void defaultTraversal_GenericSelection(TraversalFuncTable *table,
    GenericSelection *selection, void *data)
{
    table->traverse_AssignExpr(table, selection->expr, data);

    sll_foreach(selection->associations, node) {
        GenericAssociation *association = slNode_getData(node);
        table->traverse_GenericAssociation(table, association, data);
    }
}

void defaultTraversal_ConstantExpr(TraversalFuncTable *table,
    ConstantExpr *expr, void *data)
{
    // Traverse no further
}

void defaultTraversal_PrimaryExpr(TraversalFuncTable *table,
    PrimaryExpr *expr, void *data)
{
    if (expr->type == PrimaryExpr_Constant) {
        table->traverse_ConstantExpr(table, &(expr->constant), data);
    }
    else if (expr->type == PrimaryExpr_Expr) {
        table->traverse_Expr(table, expr->expr, data);
    }
    else if (expr->type == PrimaryExpr_GenericSelection) {
        table->traverse_GenericSelection(table, &(expr->genericSelection), data);
    }
}

void defaultTraversal_ArgExprList(TraversalFuncTable *table,
    ArgExprList *list, void *data)
{
    sll_foreach(list->list, node) {
        AssignExpr *expr = slNode_getData(node);
        table->traverse_AssignExpr(table, expr, data);
    }
}

void defaultTraversal_PostfixOp(TraversalFuncTable *table,
    PostfixOp *op, void *data)
{
    if (op->type == PostfixOp_Index) {
        table->traverse_Expr(table, op->indexExpr, data);
    }
    else if (op->type == PostfixOp_Call) {
        if (!op->callHasEmptyArgs) {
            table->traverse_ArgExprList(table, &(op->callExprs), data);
        }
    }

    // We don't need to traverse any further
}

void defaultTraversal_PostfixExpr(TraversalFuncTable *table,
    PostfixExpr *expr, void *data)
{
    if (expr->type == Postfix_Primary) {
        table->traverse_PrimaryExpr(table, &(expr->primary), data);
    }
    else if (expr->type == Postfix_InitializerList) {
        table->traverse_TypeName(table, expr->initializerListType, data);
        table->traverse_InitializerList(table, &(expr->initializerList), data);
    }
    else {
        assert(false);
    }

    sll_foreach(expr->postfixOps, node) {
        PostfixOp *op = slNode_getData(node);
        table->traverse_PostfixOp(table, op, data);
    }
}

void defaultTraversal_UnaryExpr(TraversalFuncTable *table,
    UnaryExpr *expr, void *data)
{
    if (expr->type == UnaryExpr_UnaryOp) {
        table->traverse_CastExpr(table, expr->unaryOpCast, data);
    }
    else if (expr->type == UnaryExpr_Inc) {
        table->traverse_UnaryExpr(table, expr->incOpExpr, data);
    }
    else if (expr->type == UnaryExpr_Dec) {
        table->traverse_UnaryExpr(table, expr->decOpExpr, data);
    }
    else if (expr->type == UnaryExpr_SizeofExpr) {
        table->traverse_UnaryExpr(table, expr->sizeofExpr, data);
    }
    else if (expr->type == UnaryExpr_SizeofType) {
        table->traverse_TypeName(table, expr->sizeofTypeName, data);
    }
    else if (expr->type == UnaryExpr_AlignofType) {
        table->traverse_TypeName(table, expr->alignofTypeName, data);
    }
    else if (UnaryExpr_Base) {
        table->traverse_PostfixExpr(table, &(expr->baseExpr), data);
    }
    else {
        assert(false);
    }
}

void defaultTraversal_CastExpr(TraversalFuncTable *table,
    CastExpr *expr, void *data)
{
    if (expr->type == CastExpr_Unary) {
        table->traverse_UnaryExpr(table, &(expr->unary), data);
    }
    else if (expr->type == CastExpr_Cast) {
        table->traverse_TypeName(table, expr->castType, data);
        table->traverse_CastExpr(table, expr->castExpr, data);
    }
}

void defaultTraversal_MultiplicativePost(TraversalFuncTable *table,
    MultiplicativePost *expr, void *data)
{
    table->traverse_CastExpr(table, &(expr->expr), data);
}

void defaultTraversal_MultiplicativeExpr(TraversalFuncTable *table,
    MultiplicativeExpr *expr, void *data)
{
    table->traverse_CastExpr(table, &(expr->baseExpr), data);

    sll_foreach(expr->postExprs, node) {
        MultiplicativePost *post = slNode_getData(node);
        table->traverse_MultiplicativePost(table, post, data);
    }
}

void defaultTraversal_AdditivePost(TraversalFuncTable *table,
    AdditivePost *expr, void *data)
{
    table->traverse_MultiplicativeExpr(table, &(expr->expr), data);
}

void defaultTraversal_AdditiveExpr(TraversalFuncTable *table,
    AdditiveExpr *expr, void *data)
{
    table->traverse_MultiplicativeExpr(table, &(expr->baseExpr), data);

    sll_foreach(expr->postExprs, node) {
        AdditivePost *post = slNode_getData(node);
        table->traverse_AdditivePost(table, post, data);
    }
}

void defaultTraversal_ShiftPost(TraversalFuncTable *table,
    ShiftPost *expr, void *data)
{
    table->traverse_AdditiveExpr(table, &(expr->expr), data);
}

void defaultTraversal_ShiftExpr(TraversalFuncTable *table,
    ShiftExpr *expr, void *data)
{
    table->traverse_AdditiveExpr(table, &(expr->baseExpr), data);

    sll_foreach(expr->postExprs, node) {
        ShiftPost *post = slNode_getData(node);
        table->traverse_ShiftPost(table, post, data);
    }
}

void defaultTraversal_RelationalPost(TraversalFuncTable *table,
    RelationalPost *expr, void *data)
{
    table->traverse_ShiftExpr(table, &(expr->expr), data);
}

void defaultTraversal_RelationalExpr(TraversalFuncTable *table,
    RelationalExpr *expr, void *data)
{
    table->traverse_ShiftExpr(table, &(expr->baseExpr), data);

    sll_foreach(expr->postExprs, node) {
        RelationalPost *post = slNode_getData(node);
        table->traverse_RelationalPost(table, post, data);
    }
}

void defaultTraversal_EqualityPost(TraversalFuncTable *table,
    EqualityPost *expr, void *data)
{
    table->traverse_RelationalExpr(table, &(expr->expr), data);
}

void defaultTraversal_EqualityExpr(TraversalFuncTable *table,
    EqualityExpr *expr, void *data)
{
    table->traverse_RelationalExpr(table, &(expr->baseExpr), data);

    sll_foreach(expr->postExprs, node) {
        EqualityPost *post = slNode_getData(node);
        table->traverse_EqualityPost(table, post, data);
    }
}

void defaultTraversal_AndExpr(TraversalFuncTable *table,
    AndExpr *expr, void *data)
{
    sll_foreach(expr->list, node) {
        EqualityExpr *equality = slNode_getData(node);
        table->traverse_EqualityExpr(table, equality, data);
    }
}

void defaultTraversal_ExclusiveOrExpr(TraversalFuncTable *table,
    ExclusiveOrExpr *expr, void *data)
{
    sll_foreach(expr->list, node) {
        AndExpr *and = slNode_getData(node);
        table->traverse_AndExpr(table, and, data);
    }
}

void defaultTraversal_InclusiveOrExpr(TraversalFuncTable *table,
    InclusiveOrExpr *expr, void *data)
{
    sll_foreach(expr->list, node) {
        ExclusiveOrExpr *or = slNode_getData(node);
        table->traverse_ExclusiveOrExpr(table, or, data);
    }
}

void defaultTraversal_LogicalAndExpr(TraversalFuncTable *table,
    LogicalAndExpr *expr, void *data)
{
    sll_foreach(expr->list, node) {
        InclusiveOrExpr *or = slNode_getData(node);
        table->traverse_InclusiveOrExpr(table, or, data);
    }
}

void defaultTraversal_LogicalOrExpr(TraversalFuncTable *table,
    LogicalOrExpr *expr, void *data)
{
    sll_foreach(expr->list, node) {
        LogicalAndExpr *and = slNode_getData(node);
        table->traverse_LogicalAndExpr(table, and, data);
    }
}

void defaultTraversal_ConditionalExpr(TraversalFuncTable *table,
    ConditionalExpr *expr, void *data)
{
    table->traverse_LogicalOrExpr(table, &(expr->beforeExpr), data);

    if (expr->hasConditionalOp) {
        table->traverse_Expr(table, expr->ifTrueExpr, data);
        table->traverse_ConditionalExpr(table, expr->ifFalseExpr, data);
    }
}

void defaultTraversal_AssignPrefix(TraversalFuncTable *table,
    AssignPrefix *prefix, void *data)
{
    table->traverse_UnaryExpr(table, &(prefix->leftExpr), data);
}

void defaultTraversal_AssignExpr(TraversalFuncTable *table,
    AssignExpr *expr, void *data)
{
    sll_foreach(expr->leftExprs, node) {
        AssignPrefix *prefix = slNode_getData(node);
        table->traverse_AssignPrefix(table, prefix, data);
    }

    table->traverse_ConditionalExpr(table, &(expr->rightExpr), data);
}

void defaultTraversal_InnerExpr(TraversalFuncTable *table,
    InnerExpr *expr, void *data)
{
    if (expr->type == InnerExpr_Assign)
        table->traverse_AssignExpr(table, &(expr->assign), data);
    else if (expr->type == InnerExpr_CompoundStatement)
        table->traverse_CompoundStmt(table, expr->compoundStmt, data);
}

void defaultTraversal_Expr(TraversalFuncTable *table,
    Expr *expr, void *data)
{
    sll_foreach(expr->list, node) {
        InnerExpr *inner = slNode_getData(node);
        table->traverse_InnerExpr(table, inner, data);
    }
}

void defaultTraversal_ParameterDeclaration(TraversalFuncTable *table,
    ParameterDeclaration *decl, void *data)
{
    table->traverse_DeclarationSpecifierList(table, decl->declarationSpecifiers, data);

    if (decl->hasDeclarator)
        table->traverse_Declarator(table, decl->declarator, data);

    if (decl->hasAbstractDeclarator)
        table->traverse_AbstractDeclarator(table, decl->abstractDeclarator, data);
}

void defaultTraversal_ParameterTypeList(TraversalFuncTable *table,
    ParameterTypeList *list, void *data)
{
    sll_foreach(list->paramDecls, node) {
        ParameterDeclaration *decl = slNode_getData(node);
        table->traverse_ParameterDeclaration(table, decl, data);
    }
}

void defaultTraversal_PostDirectAbstractDeclarator(TraversalFuncTable *table,
    PostDirectAbstractDeclarator *decl, void *data)
{
    if (decl->type == PostDirectAbstractDeclarator_Bracket) {
        sll_foreach(decl->bracketTypeQualifiers, node) {
            TypeQualifier *qual = slNode_getData(node);
            table->traverse_TypeQualifier(table, qual, data);
        }

        table->traverse_AssignExpr(table, &(decl->bracketAssignExpr), data);
    }
    else if (decl->type == PostDirectAbstractDeclarator_Paren) {
        table->traverse_ParameterTypeList(table, &(decl->parenParamList), data);
    }
    else {
        assert(false);
    }
}

void defaultTraversal_DirectAbstractDeclarator(TraversalFuncTable *table,
    DirectAbstractDeclarator *decl, void *data)
{
    if (decl->hasAbstractDeclarator)
        table->traverse_AbstractDeclarator(table, decl->abstractDeclarator, data);

    sll_foreach(decl->postDirectAbstractDeclarators, node) {
        PostDirectAbstractDeclarator *post = slNode_getData(node);
        table->traverse_PostDirectAbstractDeclarator(table, post, data);
    }
}

void defaultTraversal_Pointer(TraversalFuncTable *table,
    Pointer *pointer, void *data)
{
    sll_foreach(pointer->typeQualifiers, node) {
        TypeQualifier *qual = slNode_getData(node);
        table->traverse_TypeQualifier(table, qual, data);
    }

    table->traverse_Pointer(table, pointer->pointer, data);
}

void defaultTraversal_AbstractDeclarator(TraversalFuncTable *table,
    AbstractDeclarator *decl, void *data)
{
    if (decl->hasPointer)
        table->traverse_Pointer(table, &(decl->pointer), data);

    if (decl->hasDirectAbstractDeclarator)
        table->traverse_DirectAbstractDeclarator(table, &(decl->directAbstractDeclarator), data);
}

void defaultTraversal_IdentifierList(TraversalFuncTable *table,
    IdentifierList *list, void *data)
{
    // Traverse no further
}

void defaultTraversal_PostDirectDeclarator(TraversalFuncTable *table,
    PostDirectDeclarator *decl, void *data)
{
    if (decl->type == PostDirectDeclarator_Bracket) {
        sll_foreach(decl->bracketTypeQualifiers, node) {
            TypeQualifier *qual = slNode_getData(node);
            table->traverse_TypeQualifier(table, qual, data);
        }

        if (decl->bracketHasAssignExpr)
            table->traverse_AssignExpr(table, &(decl->bracketAssignExpr), data);
    }
    else if (decl->type == PostDirectDeclarator_Paren) {
        if (decl->parenType == PostDirectDeclaratorParen_IdentList)
            table->traverse_IdentifierList(table, &(decl->parenIdentList), data);
        else if (decl->parenType == PostDirectDeclaratorParen_ParamTypelist)
            table->traverse_ParameterTypeList(table, &(decl->parenParamTypeList), data);
    }
}

void defaultTraversal_DirectDeclarator(TraversalFuncTable *table,
    DirectDeclarator *decl, void *data)
{
    if (decl->type == DirectDeclarator_Ident) {
        // Do Nothing
    }
    else if (decl->type == DirectDeclarator_ParenDeclarator) {
        table->traverse_Declarator(table, decl->declarator, data);
    }

    sll_foreach(decl->postDirectDeclarators, node) {
        PostDirectDeclarator *post = slNode_getData(node);
        table->traverse_PostDirectDeclarator(table, post, data);
    }
}

void defaultTraversal_Declarator(TraversalFuncTable *table,
    Declarator *decl, void *data)
{
    if (decl->hasPointer)
        table->traverse_Pointer(table, &(decl->pointer), data);

    table->traverse_DirectDeclarator(table, &(decl->directDeclarator), data);
}

void defaultTraversal_TypeQualifier(TraversalFuncTable *table,
    TypeQualifier *qual, void *data)
{
    // No need to go further
}

void defaultTraversal_SpecifierQualifier(TraversalFuncTable *table,
    SpecifierQualifier *spec, void *data)
{
    if (spec->type == SpecifierQualifier_Specifier)
        table->traverse_TypeSpecifier(table, spec->typeSpecifier, data);
    else if (spec->type == SpecifierQualifier_Qualifier)
        table->traverse_TypeQualifier(table, &(spec->typeQualifier), data);
    else
        assert(false);
}

void defaultTraversal_SpecifierQualifierList(TraversalFuncTable *table,
    SpecifierQualifierList *list, void *data)
{
    sll_foreach(list->list, node) {
        SpecifierQualifier *spec = slNode_getData(node);
        table->traverse_SpecifierQualifier(table, spec, data);
    }
}

void defaultTraversal_TypeName(TraversalFuncTable *table,
    TypeName *type, void *data)
{
    table->traverse_SpecifierQualifierList(table, &(type->specifierQualifiers), data);

    if (type->hasAbstractDeclarator)
        table->traverse_AbstractDeclarator(table, &(type->abstractDeclarator), data);
}

void defaultTraversal_StructDeclarator(TraversalFuncTable *table,
    StructDeclarator *decl, void *data)
{
    if (decl->hasDeclarator)
        table->traverse_Declarator(table, &(decl->declarator), data);

    if (decl->hasConstExpr)
        table->traverse_ConditionalExpr(table, &(decl->constExpr), data);
}

void defaultTraversal_StructDeclaratorList(TraversalFuncTable *table,
    StructDeclaratorList *list, void *data)
{
    sll_foreach(list->list, node) {
        StructDeclarator *decl = slNode_getData(node);
        table->traverse_StructDeclarator(table, decl, data);
    }
}

void defaultTraversal_StaticAssertDeclaration(TraversalFuncTable *table,
    StaticAssertDeclaration *decl, void *data)
{
    table->traverse_ConditionalExpr(table, &(decl->constantExpr), data);
}

void defaultTraversal_StructDeclaration(TraversalFuncTable *table,
    StructDeclaration *decl, void *data)
{
    if (decl->type == StructDeclaration_StaticAssert) {
        table->traverse_StaticAssertDeclaration(table, &(decl->staticAssert), data);
    }
    else if (decl->type == StructDeclaration_Normal) {
        table->traverse_SpecifierQualifierList(table, &(decl->normalSpecifierQualifiers), data);
        if (decl->normalHasStructDeclaratorList) {
            table->traverse_StructDeclaratorList(table, &(decl->normalStructDeclaratorList), data);
        }
    }
    else {
        assert(false);
    }
}

void defaultTraversal_StructOrUnionSpecifier(TraversalFuncTable *table,
    StructOrUnionSpecifier *enumerator, void *data)
{
    if (enumerator->hasStructDeclarationList) {
        sll_foreach(enumerator->structDeclarations, node) {
            ConditionalExpr *expr = slNode_getData(node);
            table->traverse_ConditionalExpr(table, expr, data);
        }
    }
}

void defaultTraversal_Enumerator(TraversalFuncTable *table,
    Enumerator *enumerator, void *data)
{
    if (enumerator->hasConstExpr) {
        table->traverse_ConditionalExpr(table, &(enumerator->constantExpr), data);
    }
}

void defaultTraversal_EnumeratorList(TraversalFuncTable *table,
    EnumeratorList *list, void *data)
{
    sll_foreach(list->list, node) {
        Enumerator *enumerator = slNode_getData(node);
        table->traverse_Enumerator(table, enumerator, data);
    }
}

void defaultTraversal_EnumSpecifier(TraversalFuncTable *table,
    EnumSpecifier *spec, void *data)
{
    if (spec->hasEnumeratorList) {
        table->traverse_EnumeratorList(table, &(spec->enumeratorList), data);
    }
}

void defaultTraversal_TypeSpecifier(TraversalFuncTable *table,
    TypeSpecifier *spec, void *data)
{
    if (spec->type == TypeSpecifier_AtomicType) {
        table->traverse_TypeName(table, &(spec->atomicName), data);
    }
    else if (spec->type == TypeSpecifier_StructOrUnion) {
        table->traverse_StructOrUnionSpecifier(table, &(spec->structOrUnion), data);
    }
    else if (spec->type == TypeSpecifier_Enum) {
        table->traverse_EnumSpecifier(table, &(spec->enumSpecifier), data);
    }
}

void defaultTraversal_StorageClassSpecifier(TraversalFuncTable *table,
    StorageClassSpecifier *spec, void *data)
{
    // Can't go deeper
}

void defaultTraversal_FunctionSpecifier(TraversalFuncTable *table,
    FunctionSpecifier *spec, void *data)
{
    // Can't go deeper
}

void defaultTraversal_AlignmentSpecifier(TraversalFuncTable *table,
    AlignmentSpecifier *spec, void *data)
{
    if (spec->type == AlignmentSpecifier_TypeName) {
        table->traverse_TypeName(table, &(spec->typeName), data);
    }
    else if (spec->type == AlignmentSpecifier_Constant) {
        table->traverse_ConditionalExpr(table, &(spec->constant), data);
    }
    else {
        assert(false);
    }
}

void defaultTraversal_DeclarationSpecifier(TraversalFuncTable *table,
    DeclarationSpecifier *list, void *data)
{
    switch(list->type) {
        case DeclarationSpecifier_StorageClass: {
            table->traverse_StorageClassSpecifier(table, &(list->storageClass), data);
            break;
        }
        case DeclarationSpecifier_Type: {
            table->traverse_TypeSpecifier(table, &(list->typeSpecifier), data);
            break;
        }
        case DeclarationSpecifier_TypeQualifier: {
            table->traverse_TypeQualifier(table, &(list->typeQualifier), data);
            break;
        }
        case DeclarationSpecifier_Func: {
            table->traverse_FunctionSpecifier(table, &(list->function), data);
            break;
        }
        case DeclarationSpecifier_Alignment: {
            table->traverse_AlignmentSpecifier(table, &(list->alignment), data);
            break;
        }
        default:
            assert(false);
            break;
    }
}

void defaultTraversal_DeclarationSpecifierList(TraversalFuncTable *table,
    DeclarationSpecifierList *list, void *data)
{
    sll_foreach(list->list, node) {
        DeclarationSpecifier *decl = slNode_getData(node);
        table->traverse_DeclarationSpecifier(table, decl, data);
    }
}

void defaultTraversal_InitDeclarator(TraversalFuncTable *table,
    InitDeclarator *decl, void *data)
{
    table->traverse_Declarator(table, &(decl->decl), data);
    if (decl->hasInitializer)
        table->traverse_Initializer(table, &(decl->initializer), data);
}

void defaultTraversal_InitDeclaratorList(TraversalFuncTable *table,
    InitDeclaratorList *list, void *data)
{
    sll_foreach(list->list, node) {
        InitDeclarator *decl = slNode_getData(node);
        table->traverse_InitDeclarator(table, decl, data);
    }
}

void defaultTraversal_Declaration(TraversalFuncTable *table,
    Declaration *decl, void *data)
{
    if (decl->type == Declaration_StaticAssert) {
        table->traverse_StaticAssertDeclaration(table, &(decl->staticAssert), data);
    }
    else if (decl->type == Declaration_Normal) {
        table->traverse_DeclarationSpecifierList(table, &(decl->declSpecifiers),
            data);
        if (decl->hasInitDeclaratorList) {
            table->traverse_InitDeclaratorList(table, &(decl->initDeclaratorList),
                data);
        }
    }
}

void defaultTraversal_LabeledStatement(TraversalFuncTable *table,
    LabeledStatement *stmt, void *data)
{
    if (stmt->type == LabeledStatement_Case) {
        table->traverse_ConditionalExpr(table, &(stmt->caseConstExpr), data);
    }

    table->traverse_Statement(table, stmt->stmt, data);
}

void defaultTraversal_SelectionStatement(TraversalFuncTable *table,
    SelectionStatement *stmt, void *data)
{
    if (stmt->type == SelectionStatement_If) {
        table->traverse_Expr(table, &(stmt->ifExpr), data);
        table->traverse_Statement(table, stmt->ifTrueStmt, data);
        if (stmt->ifHasElse) {
            table->traverse_Statement(table, stmt->ifFalseStmt, data);
        }
    }
    else if (stmt->type == SelectionStatement_Switch) {
        table->traverse_Expr(table, &(stmt->switchExpr), data);
        table->traverse_Statement(table, stmt->switchStmt, data);
    }
    else {
        assert(false);
    }
}

void defaultTraversal_ExpressionStatement(TraversalFuncTable *table,
    ExpressionStatement *stmt, void *data)
{
    if (!stmt->isEmpty) {
        table->traverse_Expr(table, &(stmt->expr), data);
    }
}

void defaultTraversal_IterationStatement(TraversalFuncTable *table,
    IterationStatement *stmt, void *data)
{
    if (stmt->type == IterationStatement_While) {
        table->traverse_Expr(table, &(stmt->whileExpr), data);
        table->traverse_Statement(table, stmt->whileStmt, data);
    }
    else if (stmt->type == IterationStatement_DoWhile) {
        table->traverse_Statement(table, stmt->doStmt, data);
        table->traverse_Expr(table, &(stmt->doExpr), data);
    }
    else if (stmt->type == IterationStatement_For) {
        if (stmt->forHasInitialDeclaration) {
            table->traverse_Declaration(table, &(stmt->forInitialDeclaration), data);
        }

        table->traverse_ExpressionStatement(table, &(stmt->forInitialExprStmt), data);
        table->traverse_ExpressionStatement(table, &(stmt->forInnerExprStmt), data);

        if (stmt->forHasFinalExpr) {
            table->traverse_Expr(table, &(stmt->forFinalExpr), data);
        }
        table->traverse_Statement(table, stmt->forStmt, data);
    }
    else {
        assert(false);
    }
}

void defaultTraversal_JumpStatement(TraversalFuncTable *table,
    JumpStatement *stmt, void *data)
{
    // Return jump is the only one that needs to traverse deeper
    if (stmt->type == JumpStatement_Return) {
        table->traverse_Expr(table, &(stmt->returnExpr), data);
    }
}

void defaultTraversal_AsmStatement(TraversalFuncTable *table,
    AsmStatement *stmt, void *data)
{
    // TODO: Implement this if we fill in asm struct
}

void defaultTraversal_Statement(TraversalFuncTable *table,
    Statement *stmt, void *data)
{
    switch (stmt->type) {
        case Statement_Labeled: {
            table->traverse_LabeledStatement(table, &(stmt->labeled), data);
            break;
        }
        case Statement_Compound: {
            table->traverse_CompoundStmt(table, stmt->compound, data);
            break;
        }
        case Statement_Expression: {
            table->traverse_ExpressionStatement(table, &(stmt->expression), data);
            break;
        }
        case Statement_Selection: {
            table->traverse_SelectionStatement(table, &(stmt->selection), data);
            break;
        }
        case Statement_Iteration: {
            table->traverse_IterationStatement(table, &(stmt->iteration), data);
            break;
        }
        case Statement_Jump: {
            table->traverse_JumpStatement(table, &(stmt->jump), data);
            break;
        }
        case Statement_Asm: {
            table->traverse_AsmStatement(table, &(stmt->assembly), data);
            break;
        }
        default: {
            assert(false);
            break;
        }
    }
}

void defaultTraversal_BlockItem(TraversalFuncTable *table,
    BlockItem *item, void *data)
{
    if (item->type == BlockItem_Declaration) {
        table->traverse_Declaration(table, &(item->decl), data);
    }
    else if (item->type == BlockItem_Statement) {
        table->traverse_Statement(table, &(item->stmt), data);
    }
    else {
        assert(false);
    }
}

void defaultTraversal_BlockItemList(TraversalFuncTable *table,
    BlockItemList *list, void *data)
{
    sll_foreach(list->list, node) {
        BlockItem *item = slNode_getData(node);
        table->traverse_BlockItem(table, item, data);
    }
}

void defaultTraversal_CompoundStmt(TraversalFuncTable *table,
    CompoundStmt *stmt, void *data)
{
    if (!stmt->isEmpty) {
        table->traverse_BlockItemList(table, &(stmt->blockItemList), data);
    }
}

void defaultTraversal_FuncDef(TraversalFuncTable *table,
    FuncDef *def, void *data)
{
    table->traverse_DeclarationSpecifierList(table, &(def->specifiers), data);
    table->traverse_Declarator(table, &(def->declarator), data);

    sll_foreach(def->declarations, node) {
        Declaration *decl = slNode_getData(node);
        table->traverse_Declaration(table, decl, data);
    }

    table->traverse_CompoundStmt(table, &(def->stmt), data);
}

void defaultTraversal_ExternalDecl(TraversalFuncTable *table,
    ExternalDecl *decl, void *data)
{
    if (decl->type == ExternalDecl_FuncDef) {
        table->traverse_FuncDef(table, &(decl->func), data);
    }
    else if (decl->type == ExternalDecl_Decl) {
        table->traverse_Declaration(table, &(decl->decl), data);
    }
    else {
        assert(false);
    }
}

void defaultTraversal_TranslationUnit(TraversalFuncTable *table,
    TranslationUnit *unit, void *data)
{
    sll_foreach(unit->externalDecls, node) {
        ExternalDecl *externalDecl = slNode_getData(node);
        table->traverse_ExternalDecl(table, externalDecl, data);
    }
}

#define SetDefaultTraversalFunc(type) table.traverse_ ## type = defaultTraversal_ ## type

TraversalFuncTable defaultTraversal() {
    TraversalFuncTable table = {0};
    SetDefaultTraversalFunc(TranslationUnit);
    SetDefaultTraversalFunc(ExternalDecl);
    SetDefaultTraversalFunc(FuncDef);
    SetDefaultTraversalFunc(CompoundStmt);
    SetDefaultTraversalFunc(BlockItemList);
    SetDefaultTraversalFunc(BlockItem);
    SetDefaultTraversalFunc(Statement);
    SetDefaultTraversalFunc(AsmStatement);
    SetDefaultTraversalFunc(JumpStatement);
    SetDefaultTraversalFunc(IterationStatement);
    SetDefaultTraversalFunc(ExpressionStatement);
    SetDefaultTraversalFunc(SelectionStatement);
    SetDefaultTraversalFunc(LabeledStatement);
    SetDefaultTraversalFunc(Declaration);
    SetDefaultTraversalFunc(InitDeclaratorList);
    SetDefaultTraversalFunc(InitDeclarator);
    SetDefaultTraversalFunc(DeclarationSpecifierList);
    SetDefaultTraversalFunc(DeclarationSpecifier);
    SetDefaultTraversalFunc(AlignmentSpecifier);
    SetDefaultTraversalFunc(FunctionSpecifier);
    SetDefaultTraversalFunc(StorageClassSpecifier);
    SetDefaultTraversalFunc(TypeSpecifier);
    SetDefaultTraversalFunc(EnumSpecifier);
    SetDefaultTraversalFunc(EnumeratorList);
    SetDefaultTraversalFunc(Enumerator);
    SetDefaultTraversalFunc(StructOrUnionSpecifier);
    SetDefaultTraversalFunc(StructDeclaration);
    SetDefaultTraversalFunc(StaticAssertDeclaration);
    SetDefaultTraversalFunc(StructDeclaratorList);
    SetDefaultTraversalFunc(StructDeclarator);
    SetDefaultTraversalFunc(TypeName);
    SetDefaultTraversalFunc(SpecifierQualifierList);
    SetDefaultTraversalFunc(SpecifierQualifier);
    SetDefaultTraversalFunc(TypeQualifier);
    SetDefaultTraversalFunc(Declarator);
    SetDefaultTraversalFunc(DirectDeclarator);
    SetDefaultTraversalFunc(PostDirectDeclarator);
    SetDefaultTraversalFunc(IdentifierList);
    SetDefaultTraversalFunc(AbstractDeclarator);
    SetDefaultTraversalFunc(Pointer);
    SetDefaultTraversalFunc(DirectAbstractDeclarator);
    SetDefaultTraversalFunc(PostDirectAbstractDeclarator);
    SetDefaultTraversalFunc(ParameterTypeList);
    SetDefaultTraversalFunc(ParameterDeclaration);
    SetDefaultTraversalFunc(Expr);
    SetDefaultTraversalFunc(InnerExpr);
    SetDefaultTraversalFunc(AssignExpr);
    SetDefaultTraversalFunc(AssignPrefix);
    SetDefaultTraversalFunc(ConditionalExpr);
    SetDefaultTraversalFunc(LogicalOrExpr);
    SetDefaultTraversalFunc(LogicalAndExpr);
    SetDefaultTraversalFunc(InclusiveOrExpr);
    SetDefaultTraversalFunc(ExclusiveOrExpr);
    SetDefaultTraversalFunc(AndExpr);
    SetDefaultTraversalFunc(EqualityExpr);
    SetDefaultTraversalFunc(EqualityPost);
    SetDefaultTraversalFunc(RelationalExpr);
    SetDefaultTraversalFunc(RelationalPost);
    SetDefaultTraversalFunc(ShiftExpr);
    SetDefaultTraversalFunc(ShiftPost);
    SetDefaultTraversalFunc(AdditiveExpr);
    SetDefaultTraversalFunc(AdditivePost);
    SetDefaultTraversalFunc(MultiplicativeExpr);
    SetDefaultTraversalFunc(MultiplicativePost);
    SetDefaultTraversalFunc(CastExpr);
    SetDefaultTraversalFunc(UnaryExpr);
    SetDefaultTraversalFunc(PostfixExpr);
    SetDefaultTraversalFunc(PostfixOp);
    SetDefaultTraversalFunc(ArgExprList);
    SetDefaultTraversalFunc(PrimaryExpr);
    SetDefaultTraversalFunc(ConstantExpr);
    SetDefaultTraversalFunc(GenericSelection);
    SetDefaultTraversalFunc(GenericAssociation);
    SetDefaultTraversalFunc(InitializerList);
    SetDefaultTraversalFunc(DesignationAndInitializer);
    SetDefaultTraversalFunc(Initializer);
    SetDefaultTraversalFunc(Designation);
    SetDefaultTraversalFunc(Designator);

    return table;
}

void traverse(TraversalFuncTable table, TranslationUnit unit, void *data) {
    table.traverse_TranslationUnit(&table, &unit, data);
}
