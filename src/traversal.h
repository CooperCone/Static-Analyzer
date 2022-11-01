#pragma once

#include "parser.h"

struct TraversalFuncTable;

#define TraversalFuncDef(type) void (* traverse_ ## type )(struct TraversalFuncTable*,\
 type*, void*)

typedef struct TraversalFuncTable {
    TraversalFuncDef(TranslationUnit);
    TraversalFuncDef(ExternalDecl);
    TraversalFuncDef(FuncDef);
    TraversalFuncDef(CompoundStmt);
    TraversalFuncDef(BlockItemList);
    TraversalFuncDef(BlockItem);
    TraversalFuncDef(Statement);
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
