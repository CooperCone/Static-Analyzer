#pragma once

#include "parser.h"

struct TraversalFuncTable;

#define TraversalFuncDef(type) void (* traverse_ ## type )(struct TraversalFuncTable*,\
 type*, void*)

void defaultTraversal_Designator(struct TraversalFuncTable *table, Designator *desig, void *data);
void defaultTraversal_Designation(struct TraversalFuncTable *table, Designation *desig, void *data);
void defaultTraversal_Initializer(struct TraversalFuncTable *table, Initializer *init, void *data);
void defaultTraversal_DesignationAndInitializer(struct TraversalFuncTable *table, DesignationAndInitializer *desig, void *data);
void defaultTraversal_InitializerList(struct TraversalFuncTable *table, InitializerList *list, void *data);
void defaultTraversal_GenericAssociation(struct TraversalFuncTable *table, GenericAssociation *association, void *data);
void defaultTraversal_GenericSelection(struct TraversalFuncTable *table, GenericSelection *selection, void *data);
void defaultTraversal_ConstantExpr(struct TraversalFuncTable *table, ConstantExpr *expr, void *data);
void defaultTraversal_PrimaryExpr(struct TraversalFuncTable *table, PrimaryExpr *expr, void *data);
void defaultTraversal_ArgExprList(struct TraversalFuncTable *table, ArgExprList *list, void *data);
void defaultTraversal_PostfixOp(struct TraversalFuncTable *table, PostfixOp *op, void *data);
void defaultTraversal_PostfixExpr(struct TraversalFuncTable *table, PostfixExpr *expr, void *data);
void defaultTraversal_UnaryExpr(struct TraversalFuncTable *table, UnaryExpr *expr, void *data);
void defaultTraversal_CastExpr(struct TraversalFuncTable *table, CastExpr *expr, void *data);
void defaultTraversal_MultiplicativePost(struct TraversalFuncTable *table, MultiplicativePost *expr, void *data);
void defaultTraversal_MultiplicativeExpr(struct TraversalFuncTable *table, MultiplicativeExpr *expr, void *data);
void defaultTraversal_AdditivePost(struct TraversalFuncTable *table, AdditivePost *expr, void *data);
void defaultTraversal_AdditiveExpr(struct TraversalFuncTable *table, AdditiveExpr *expr, void *data);
void defaultTraversal_ShiftPost(struct TraversalFuncTable *table, ShiftPost *expr, void *data);
void defaultTraversal_ShiftExpr(struct TraversalFuncTable *table, ShiftExpr *expr, void *data);
void defaultTraversal_RelationalPost(struct TraversalFuncTable *table, RelationalPost *expr, void *data);
void defaultTraversal_RelationalExpr(struct TraversalFuncTable *table, RelationalExpr *expr, void *data);
void defaultTraversal_EqualityPost(struct TraversalFuncTable *table, EqualityPost *expr, void *data);
void defaultTraversal_EqualityExpr(struct TraversalFuncTable *table, EqualityExpr *expr, void *data);
void defaultTraversal_AndExpr(struct TraversalFuncTable *table, AndExpr *expr, void *data);
void defaultTraversal_ExclusiveOrExpr(struct TraversalFuncTable *table, ExclusiveOrExpr *expr, void *data);
void defaultTraversal_InclusiveOrExpr(struct TraversalFuncTable *table, InclusiveOrExpr *expr, void *data);
void defaultTraversal_LogicalAndExpr(struct TraversalFuncTable *table, LogicalAndExpr *expr, void *data);
void defaultTraversal_LogicalOrExpr(struct TraversalFuncTable *table, LogicalOrExpr *expr, void *data);
void defaultTraversal_ConditionalExpr(struct TraversalFuncTable *table, ConditionalExpr *expr, void *data);
void defaultTraversal_AssignPrefix(struct TraversalFuncTable *table, AssignPrefix *prefix, void *data);
void defaultTraversal_AssignExpr(struct TraversalFuncTable *table, AssignExpr *expr, void *data);
void defaultTraversal_InnerExpr(struct TraversalFuncTable *table, InnerExpr *expr, void *data);
void defaultTraversal_Expr(struct TraversalFuncTable *table, Expr *expr, void *data);
void defaultTraversal_ParameterDeclaration(struct TraversalFuncTable *table, ParameterDeclaration *decl, void *data);
void defaultTraversal_ParameterTypeList(struct TraversalFuncTable *table, ParameterTypeList *list, void *data);
void defaultTraversal_PostDirectAbstractDeclarator(struct TraversalFuncTable *table, PostDirectAbstractDeclarator *decl, void *data);
void defaultTraversal_DirectAbstractDeclarator(struct TraversalFuncTable *table, DirectAbstractDeclarator *decl, void *data);
void defaultTraversal_Pointer(struct TraversalFuncTable *table, Pointer *pointer, void *data);
void defaultTraversal_AbstractDeclarator(struct TraversalFuncTable *table, AbstractDeclarator *decl, void *data);
void defaultTraversal_IdentifierList(struct TraversalFuncTable *table, IdentifierList *list, void *data);
void defaultTraversal_PostDirectDeclarator(struct TraversalFuncTable *table, PostDirectDeclarator *decl, void *data);
void defaultTraversal_DirectDeclarator(struct TraversalFuncTable *table, DirectDeclarator *decl, void *data);
void defaultTraversal_Declarator(struct TraversalFuncTable *table, Declarator *decl, void *data);
void defaultTraversal_TypeQualifier(struct TraversalFuncTable *table, TypeQualifier *qual, void *data);
void defaultTraversal_SpecifierQualifier(struct TraversalFuncTable *table, SpecifierQualifier *spec, void *data);
void defaultTraversal_SpecifierQualifierList(struct TraversalFuncTable *table, SpecifierQualifierList *list, void *data);
void defaultTraversal_TypeName(struct TraversalFuncTable *table, TypeName *type, void *data);
void defaultTraversal_StructDeclarator(struct TraversalFuncTable *table, StructDeclarator *decl, void *data);
void defaultTraversal_StructDeclaratorList(struct TraversalFuncTable *table, StructDeclaratorList *list, void *data);
void defaultTraversal_StaticAssertDeclaration(struct TraversalFuncTable *table, StaticAssertDeclaration *decl, void *data);
void defaultTraversal_StructDeclaration(struct TraversalFuncTable *table, StructDeclaration *decl, void *data);
void defaultTraversal_StructOrUnionSpecifier(struct TraversalFuncTable *table, StructOrUnionSpecifier *enumerator, void *data);
void defaultTraversal_Enumerator(struct TraversalFuncTable *table, Enumerator *enumerator, void *data);
void defaultTraversal_EnumeratorList(struct TraversalFuncTable *table, EnumeratorList *list, void *data);
void defaultTraversal_EnumSpecifier(struct TraversalFuncTable *table, EnumSpecifier *spec, void *data);
void defaultTraversal_TypeSpecifier(struct TraversalFuncTable *table, TypeSpecifier *spec, void *data);
void defaultTraversal_StorageClassSpecifier(struct TraversalFuncTable *table, StorageClassSpecifier *spec, void *data);
void defaultTraversal_FunctionSpecifier(struct TraversalFuncTable *table, FunctionSpecifier *spec, void *data);
void defaultTraversal_AlignmentSpecifier(struct TraversalFuncTable *table, AlignmentSpecifier *spec, void *data);
void defaultTraversal_DeclarationSpecifier(struct TraversalFuncTable *table, DeclarationSpecifier *list, void *data);
void defaultTraversal_DeclarationSpecifierList(struct TraversalFuncTable *table, DeclarationSpecifierList *list, void *data);
void defaultTraversal_InitDeclarator(struct TraversalFuncTable *table, InitDeclarator *decl, void *data);
void defaultTraversal_InitDeclaratorList(struct TraversalFuncTable *table, InitDeclaratorList *list, void *data);
void defaultTraversal_Declaration(struct TraversalFuncTable *table, Declaration *decl, void *data);
void defaultTraversal_LabeledStatement(struct TraversalFuncTable *table, LabeledStatement *stmt, void *data);
void defaultTraversal_SelectionStatement(struct TraversalFuncTable *table, SelectionStatement *stmt, void *data);
void defaultTraversal_ExpressionStatement(struct TraversalFuncTable *table, ExpressionStatement *stmt, void *data);
void defaultTraversal_IterationStatement(struct TraversalFuncTable *table, IterationStatement *stmt, void *data);
void defaultTraversal_JumpStatement(struct TraversalFuncTable *table, JumpStatement *stmt, void *data);
void defaultTraversal_AsmStatement(struct TraversalFuncTable *table, AsmStatement *stmt, void *data);
void defaultTraversal_Statement(struct TraversalFuncTable *table, Statement *stmt, void *data);
void defaultTraversal_BlockItem(struct TraversalFuncTable *table, BlockItem *item, void *data);
void defaultTraversal_BlockItemList(struct TraversalFuncTable *table, BlockItemList *list, void *data);
void defaultTraversal_CompoundStmt(struct TraversalFuncTable *table, CompoundStmt *stmt, void *data);
void defaultTraversal_FuncDef(struct TraversalFuncTable *table, FuncDef *def, void *data);
void defaultTraversal_ExternalDecl(struct TraversalFuncTable *table, ExternalDecl *decl, void *data);
void defaultTraversal_TranslationUnit(struct TraversalFuncTable *table, TranslationUnit *unit, void *data);

typedef struct TraversalFuncTable {
    TraversalFuncDef(TranslationUnit);
    TraversalFuncDef(ExternalDecl);
    TraversalFuncDef(FuncDef);
    TraversalFuncDef(CompoundStmt);
    TraversalFuncDef(BlockItemList);
    TraversalFuncDef(BlockItem);
    TraversalFuncDef(Statement);
    TraversalFuncDef(AsmStatement);
    TraversalFuncDef(JumpStatement);
    TraversalFuncDef(IterationStatement);
    TraversalFuncDef(ExpressionStatement);
    TraversalFuncDef(SelectionStatement);
    TraversalFuncDef(LabeledStatement);
    TraversalFuncDef(Declaration);
    TraversalFuncDef(InitDeclaratorList);
    TraversalFuncDef(InitDeclarator);
    TraversalFuncDef(DeclarationSpecifierList);
    TraversalFuncDef(DeclarationSpecifier);
    TraversalFuncDef(AlignmentSpecifier);
    TraversalFuncDef(FunctionSpecifier);
    TraversalFuncDef(StorageClassSpecifier);
    TraversalFuncDef(TypeSpecifier);
    TraversalFuncDef(EnumSpecifier);
    TraversalFuncDef(EnumeratorList);
    TraversalFuncDef(Enumerator);
    TraversalFuncDef(StructOrUnionSpecifier);
    TraversalFuncDef(StructDeclaration);
    TraversalFuncDef(StaticAssertDeclaration);
    TraversalFuncDef(StructDeclaratorList);
    TraversalFuncDef(StructDeclarator);
    TraversalFuncDef(TypeName);
    TraversalFuncDef(SpecifierQualifierList);
    TraversalFuncDef(SpecifierQualifier);
    TraversalFuncDef(TypeQualifier);
    TraversalFuncDef(Declarator);
    TraversalFuncDef(DirectDeclarator);
    TraversalFuncDef(PostDirectDeclarator);
    TraversalFuncDef(IdentifierList);
    TraversalFuncDef(AbstractDeclarator);
    TraversalFuncDef(Pointer);
    TraversalFuncDef(DirectAbstractDeclarator);
    TraversalFuncDef(PostDirectAbstractDeclarator);
    TraversalFuncDef(ParameterTypeList);
    TraversalFuncDef(ParameterDeclaration);
    TraversalFuncDef(Expr);
    TraversalFuncDef(InnerExpr);
    TraversalFuncDef(AssignExpr);
    TraversalFuncDef(AssignPrefix);
    TraversalFuncDef(ConditionalExpr);
    TraversalFuncDef(LogicalOrExpr);
    TraversalFuncDef(LogicalAndExpr);
    TraversalFuncDef(InclusiveOrExpr);
    TraversalFuncDef(ExclusiveOrExpr);
    TraversalFuncDef(AndExpr);
    TraversalFuncDef(EqualityExpr);
    TraversalFuncDef(EqualityPost);
    TraversalFuncDef(RelationalExpr);
    TraversalFuncDef(RelationalPost);
    TraversalFuncDef(ShiftExpr);
    TraversalFuncDef(ShiftPost);
    TraversalFuncDef(AdditiveExpr);
    TraversalFuncDef(AdditivePost);
    TraversalFuncDef(MultiplicativeExpr);
    TraversalFuncDef(MultiplicativePost);
    TraversalFuncDef(CastExpr);
    TraversalFuncDef(UnaryExpr);
    TraversalFuncDef(PostfixExpr);
    TraversalFuncDef(PostfixOp);
    TraversalFuncDef(ArgExprList);
    TraversalFuncDef(PrimaryExpr);
    TraversalFuncDef(ConstantExpr);
    TraversalFuncDef(GenericSelection);
    TraversalFuncDef(GenericAssociation);
    TraversalFuncDef(InitializerList);
    TraversalFuncDef(DesignationAndInitializer);
    TraversalFuncDef(Initializer);
    TraversalFuncDef(Designation);
    TraversalFuncDef(Designator);
} TraversalFuncTable;

TraversalFuncTable defaultTraversal();
void traverse(TraversalFuncTable table, TranslationUnit unit, void *data);
