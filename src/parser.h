#pragma once

#include <stdbool.h>

#include "lexer.h"
#include "astring.h"
#include "linkedList.h"

typedef struct {
    bool success;
    char *failMessage;
} ParseRes;

typedef ParseRes (*Parser)(TokenList *tokens, void *data);

typedef enum {
    SimpleParser_List,
    SimpleParser_Optional,
} SimpleParserType;

typedef struct SimpleParser {
    SimpleParserType type;
    union {
        struct {
            Parser listElemParser;
            SLList *listOut;
            size_t listElemSize;
            char *listFailMessage;
        };
        struct {
            void *optionalData;
            Parser optionalParser;
        };
    };
    Parser run;
} SimpleParser;

SimpleParser ListParser(Parser listElemParser, SLList *listOut,
    size_t listElemSize, char *listFailMessage);
SimpleParser OptionalParser(Parser parser, void *data);

struct ConditionalExpr;
struct InitializerList;
struct AssignExpr;
struct TypeName;
struct Expr;
struct CastExpr;
enum TypeQualifier;
struct TypeSpecifier;
struct AbstractDeclarator;
struct Declarator;
struct Statement;
struct CompoundStmt;

typedef enum {
    Designator_Constant,
    Designator_Ident,
} DesignatorType;

typedef struct {
    DesignatorType type;
    union {
        struct ConditionalExpr *constantExpr; // With constraints?
        String ident;
    };
} Designator;

typedef struct {
    SLList list; // Designator
} Designation;

typedef enum {
    Initializer_InitializerList,
    Initializer_Assignment,
} InitializerType;

typedef struct {
    InitializerType type;
    union {
        struct InitializerList *initializerList;
        struct AssignExpr *assignmentExpr;
    };
} Initializer;

typedef struct {
    bool hasDesignation;
    Designation designation;
    Initializer initializer;
} DesignationAndInitializer;

typedef struct InitializerList {
    SLList list; // DesignationAndInitializer
} InitializerList;

typedef struct {
    bool isDefault;
    struct TypeName *typeName;
    struct AssignExpr *expr;
} GenericAssociation;

typedef struct {
    struct AssignExpr *expr;
    SLList associations; // GenericAssociation
} GenericSelection;

typedef enum {
    Constant_Numeric,
    Constant_Character,
} ConstantType;

// FIXME: Should we have real data for the different constants?
typedef struct {
    ConstantType type;
    String data;
} ConstantExpr;

typedef enum {
    PrimaryExpr_Ident,
    PrimaryExpr_Constant,
    PrimaryExpr_String,
    PrimaryExpr_FuncName,
    PrimaryExpr_Expr,
    PrimaryExpr_GenericSelection,
} PrimaryExprType;

// FIXME: We currently don't distinguish between idents and enum constants
typedef struct {
    PrimaryExprType type;
    union {
        String ident;
        ConstantExpr constant;
        String string;
        struct Expr *expr;
        GenericSelection genericSelection;
    };
} PrimaryExpr;

typedef struct {
    SLList list; // AssignExpr
} ArgExprList;

typedef enum {
    PostfixOp_Index,
    PostfixOp_Call,
    PostfixOp_Dot,
    PostfixOp_Arrow,
    PostfixOp_Inc,
    PostfixOp_Dec,
} PostfixOpType;

typedef struct {
    PostfixOpType type;
    union {
        struct Expr *indexExpr;
        struct {
            bool callHasEmptyArgs;
            ArgExprList callExprs;
        };
        String dotIdent;
        String arrowIdent;
    };
} PostfixOp;

typedef enum {
    Postfix_Primary,
    Postfix_InitializerList,
} PostfixType;

typedef struct {
    PostfixType type;
    union {
        PrimaryExpr primary;
        struct {
            struct TypeName *initializerListType;
            InitializerList initializerList;
        };
    };

    SLList postfixOps; // PostfixOp
} PostfixExpr;

typedef enum {
    UnaryPrefix_And,
    UnaryPrefix_Star,
    UnaryPrefix_Plus,
    UnaryPrefix_Minus,
    UnaryPrefix_Tilde,
    UnaryPrefix_Not,
} UnaryExprPrefixType;

typedef enum {
    UnaryExpr_UnaryOp,
    UnaryExpr_Inc,
    UnaryExpr_Dec,
    UnaryExpr_SizeofExpr,
    UnaryExpr_SizeofType,
    UnaryExpr_AlignofType,
    UnaryExpr_Base,
} UnaryExprType;

typedef struct UnaryExpr {
    UnaryExprType type;
    union {
        struct {
            UnaryExprPrefixType unaryOpType;
            struct CastExpr *unaryOpCast;
        };
        struct UnaryExpr *incOpExpr;
        struct UnaryExpr *decOpExpr;
        struct UnaryExpr *sizeofExpr;
        struct TypeName *sizeofTypeName;
        struct TypeName *alignofTypeName;
        PostfixExpr baseExpr;
    };
} UnaryExpr;

typedef enum {
    CastExpr_Unary,
    CastExpr_Cast,
} CastExprType;

typedef struct CastExpr {
    CastExprType type;
    union {
        UnaryExpr unary;
        struct {
            struct TypeName *castType;
            struct CastExpr *castExpr;
        };
    };
} CastExpr;

typedef enum {
    Multiplicative_Mul,
    Multiplicative_Div,
    Multiplicative_Mod,
} MultiplicativeOp;

typedef struct {
    MultiplicativeOp op;
    CastExpr expr;
} MultiplicativePost;

typedef struct {
    CastExpr baseExpr;
    SLList postExprs; // MultiplicativePost
} MultiplicativeExpr;

typedef enum {
    Additive_Add,
    Additive_Sub,
} AdditiveOp;

typedef struct {
    AdditiveOp op;
    MultiplicativeExpr expr;
} AdditivePost;

typedef struct {
    MultiplicativeExpr baseExpr;
    SLList postExprs; // AdditivePost
} AdditiveExpr;

typedef enum {
    Shift_Left,
    Shift_Right,
} ShiftOp;

typedef struct {
    ShiftOp op;
    AdditiveExpr expr;
} ShiftPost;

typedef struct {
    AdditiveExpr baseExpr;
    SLList postExprs;
} ShiftExpr;

typedef enum {
    Relational_Lt,
    Relational_Gt,
    Relational_LEq,
    Relational_GEq,
} RelationalOp;

typedef struct {
    RelationalOp op;
    ShiftExpr expr;
} RelationalPost;

typedef struct {
    ShiftExpr baseExpr;
    SLList postExprs;
} RelationalExpr;

typedef enum {
    Equality_Eq,
    Equality_NEq,
} EqualityOp;

typedef struct {
    EqualityOp op;
    RelationalExpr expr;
} EqualityPost;

typedef struct {
    RelationalExpr baseExpr;
    SLList postExprs;
} EqualityExpr;

typedef struct {
    SLList list;
} AndExpr;

typedef struct {
    SLList list;
} ExclusiveOrExpr;

typedef struct {
    SLList list;
} InclusiveOrExpr;

typedef struct {
    SLList list;
} LogicalAndExpr;

typedef struct {
    SLList list;
} LogicalOrExpr;

typedef struct ConditionalExpr {
    LogicalOrExpr beforeExpr;

    bool hasConditionalOp;
    struct Expr *ifTrueExpr;
    struct ConditionalExpr *ifFalseExpr;
} ConditionalExpr;

typedef enum {
    Assign_Eq,
    Assign_MulEq,
    Assign_DivEq,
    Assign_ModEq,
    Assign_AddEq,
    Assign_SubEq,
    Assign_ShiftLeftEq,
    Assign_ShiftRightEq,
    Assign_AndEq,
    Assign_XorEq,
    Assign_OrEq,
} AssignOp;

typedef struct {
    UnaryExpr leftExpr;
    AssignOp op;
} AssignPrefix;

typedef struct AssignExpr {
    SLList leftExprs;

    ConditionalExpr rightExpr;
} AssignExpr;

// This is done just to appease the gcc
// extension gods
typedef enum {
    InnerExpr_Assign,
    InnerExpr_CompoundStatement,
} InnerExprType;

typedef struct {
    InnerExprType type;
    union {
        AssignExpr assign;
        struct CompoundStmt *compoundStmt;
    };
} InnerExpr;

typedef struct Expr {
    SLList list;
} Expr;

typedef struct {
    struct DeclarationSpecifierList *declarationSpecifiers;
    bool hasDeclarator;
    struct Declarator *declarator;
    bool hasAbstractDeclarator;
    struct AbstractDeclarator *abstractDeclarator;
} ParameterDeclaration;

typedef struct {
    SLList paramDecls;
    bool hasEndingEllipsis;
} ParameterTypeList;

typedef enum {
    PostDirectAbstractDeclarator_Bracket,
    PostDirectAbstractDeclarator_Paren,
} PostDirectAbstractDeclaratorType;

typedef struct {
    PostDirectAbstractDeclaratorType type;
    union {
        struct {
            bool bracketIsEmpty;
            bool bracketIsStar;
            bool bracketHasInitialStatic;

            SLList bracketTypeQualifiers;

            bool bracketHasMiddleStatic;

            bool bracketHasAssignmentExpr;
            AssignExpr bracketAssignExpr;
        };
        struct {
            bool parenIsEmpty;
            ParameterTypeList parenParamList;
        };
    };
} PostDirectAbstractDeclarator;

typedef struct {
    bool hasAbstractDeclarator;
    struct AbstractDeclarator *abstractDeclarator;
    SLList postDirectAbstractDeclarators;
} DirectAbstractDeclarator;

typedef struct Pointer {
    size_t numPtrs;
    SLList typeQualifiers;
    bool hasPtr;
    struct Pointer *pointer;
} Pointer;

typedef struct AbstractDeclarator {
    bool hasPointer;
    Pointer pointer;
    bool hasDirectAbstractDeclarator;
    DirectAbstractDeclarator directAbstractDeclarator;
} AbstractDeclarator;

typedef struct {
    SLList list; // List of strings
} IdentifierList;

typedef enum {
    PostDirectDeclaratorParen_Empty,
    PostDirectDeclaratorParen_IdentList,
    PostDirectDeclaratorParen_ParamTypelist,
} PostDirectDeclaratorParenType;

typedef enum {
    PostDirectDeclarator_Paren,
    PostDirectDeclarator_Bracket,
} PostDirectDeclaratorType;

typedef struct {
    PostDirectDeclaratorType type;
    union {
        struct {
            bool bracketIsEmpty;
            bool bracketIsStar;

            bool bracketHasInitialStatic;
            SLList bracketTypeQualifiers;

            bool bracketHasStarAfterTypeQualifiers;

            bool bracketHasMiddleStatic;

            bool bracketHasAssignExpr;
            AssignExpr bracketAssignExpr;
        };
        struct {
            PostDirectDeclaratorParenType parenType;
            union {
                IdentifierList parenIdentList;
                ParameterTypeList parenParamTypeList;
            };
        };
    };
} PostDirectDeclarator;

typedef enum {
    DirectDeclarator_Ident,
    DirectDeclarator_ParenDeclarator,
} DirectDeclaratorType;

typedef struct {
    DirectDeclaratorType type;
    union {
        String ident;
        struct Declarator *declarator;
    };

    SLList postDirectDeclarators;
} DirectDeclarator;

typedef struct Declarator {
    bool hasPointer;
    Pointer pointer;
    DirectDeclarator directDeclarator;
} Declarator;

typedef enum TypeQualifier {
    Qualifier_Const,
    Qualifier_Restrict,
    Qualifier_Volatile,
    Qualifier_Atomic,
} TypeQualifier;

typedef enum {
    SpecifierQualifier_Specifier,
    SpecifierQualifier_Qualifier,
} SpecifierQualifierType;

typedef struct {
    SpecifierQualifierType type;
    union {
        struct TypeSpecifier *typeSpecifier;
        TypeQualifier typeQualifier;
    };
} SpecifierQualifier;

typedef struct {
    SLList list;
} SpecifierQualifierList;

typedef struct TypeName {
    SpecifierQualifierList specifierQualifiers;
    bool hasAbstractDeclarator;
    AbstractDeclarator abstractDeclarator;
} TypeName;

typedef struct {
    bool hasDeclarator;
    Declarator declarator;
    bool hasConstExpr;
    ConditionalExpr constExpr;
} StructDeclarator;

typedef struct {
    SLList list;
} StructDeclaratorList;

typedef struct {
    ConditionalExpr constantExpr;
    String stringLiteral;
} StaticAssertDeclaration;

typedef enum {
    StructDeclaration_StaticAssert,
    StructDeclaration_Normal, // FIXME: Better name?
} StructDeclarationType;

typedef struct {
    StructDeclarationType type;
    union {
        StaticAssertDeclaration staticAssert;
        struct {
            SpecifierQualifierList normalSpecifierQualifiers;
            bool normalHasStructDeclaratorList;
            StructDeclaratorList normalStructDeclaratorList;
        };
    };
} StructDeclaration;

typedef enum {
    StructOrUnion_Struct,
    StructOrUnion_Union,
} StructOrUnion;

typedef struct {
    StructOrUnion structOrUnion;
    bool hasIdent;
    String ident;
    bool hasStructDeclarationList;
    SLList structDeclarations;
} StructOrUnionSpecifier;

typedef struct {
    String constantIdent;
    bool hasConstExpr;
    ConditionalExpr constantExpr;
} Enumerator;

typedef struct {
    SLList list;
} EnumeratorList;

typedef struct {
    bool hasIdent;
    String ident;
    bool hasEnumeratorList;
    EnumeratorList enumeratorList;
} EnumSpecifier;

typedef enum {
    TypeSpecifier_Void,
    TypeSpecifier_Char,
    TypeSpecifier_Short,
    TypeSpecifier_Int,
    TypeSpecifier_Long,
    TypeSpecifier_Float,
    TypeSpecifier_Double,
    TypeSpecifier_Signed,
    TypeSpecifier_Unsigned,
    TypeSpecifier_Bool,
    TypeSpecifier_Complex,
    TypeSpecifier_Imaginary,
    TypeSpecifier_AtomicType,
    TypeSpecifier_StructOrUnion,
    TypeSpecifier_Enum,
    TypeSpecifier_TypedefName,
} TypeSpecifierType;

typedef struct TypeSpecifier {
    TypeSpecifierType type;
    union {
        TypeName atomicName;
        StructOrUnionSpecifier structOrUnion;
        EnumSpecifier enumSpecifier;
        String typedefName;
    };
} TypeSpecifier;

typedef enum {
    StorageClass_Typedef,
    StorageClass_Extern,
    StorageClass_Static,
    StorageClass_Thread_Local,
    StorageClass_Auto,
    StorageClass_Register,
} StorageClassSpecifier;

typedef enum {
    FunctionSpecifier_Inline,
    FunctionSpecifier_Noreturn,
} FunctionSpecifier;

typedef enum {
    AlignmentSpecifier_TypeName,
    AlignmentSpecifier_Constant,
} AlignmentSpecifierType;

typedef struct {
    AlignmentSpecifierType type;
    union {
        TypeName typeName;
        ConditionalExpr constant;
    };
} AlignmentSpecifier;

typedef enum {
    DeclarationSpecifier_StorageClass,
    DeclarationSpecifier_Type,
    DeclarationSpecifier_TypeQualifier,
    DeclarationSpecifier_Func,
    DeclarationSpecifier_Alignment,
} DeclarationSpecifierType;

typedef struct {
    DeclarationSpecifierType type;
    union {
        StorageClassSpecifier storageClass;
        TypeSpecifier typeSpecifier;
        TypeQualifier typeQualifier;
        FunctionSpecifier function;
        AlignmentSpecifier alignment;
    };
} DeclarationSpecifier;

typedef struct DeclarationSpecifierList {
    SLList list;
} DeclarationSpecifierList;

typedef struct {
    Declarator decl;
    bool hasInitializer;
    Initializer initializer;
} InitDeclarator;

typedef struct {
    SLList list;
 } InitDeclaratorList;

typedef enum {
    Declaration_StaticAssert,
    Declaration_Normal, // FIXME: Better name?
} DeclarationType;

typedef struct {
    DeclarationType type;
    union {
        StaticAssertDeclaration staticAssert;
        struct {
            DeclarationSpecifierList declSpecifiers;
            bool hasInitDeclaratorList;
            InitDeclaratorList initDeclaratorList;
        };
    };
} Declaration;

typedef enum {
    LabeledStatement_Ident,
    LabeledStatement_Case,
    LabeledStatement_Default,
} LabeledStatementType;

typedef struct {
    LabeledStatementType type;
    union {
        String ident;
        ConditionalExpr caseConstExpr;
    };
    struct Statement *stmt;
} LabeledStatement;

typedef enum {
    SelectionStatement_If,
    SelectionStatement_Switch,
} SelectionStatementType;

typedef struct {
    SelectionStatementType type;
    union {
        struct {
            Token *ifToken;
            Token *elseToken;

            Expr ifExpr;
            struct Statement *ifTrueStmt;
            bool ifHasElse;
            struct Statement *ifFalseStmt;
        };
        struct {
            Token *switchToken;

            Expr switchExpr;
            struct Statement *switchStmt;
        };
    };
} SelectionStatement;

typedef struct {
    bool isEmpty;
    Expr expr;
} ExpressionStatement;

typedef enum {
    IterationStatement_While,
    IterationStatement_DoWhile,
    IterationStatement_For,
} IterationStatementType;

typedef struct {
    IterationStatementType type;
    union {
        struct {
            Token *whileToken;

            Expr whileExpr;
            struct Statement *whileStmt;
        };
        struct {
            Token *doToken;

            struct Statement *doStmt;
            Expr doExpr;
        };
        struct {
            Token *forToken;

            bool forHasInitialDeclaration;
            Declaration forInitialDeclaration;
            ExpressionStatement forInitialExprStmt;
            ExpressionStatement forInnerExprStmt;
            bool forHasFinalExpr;
            Expr forFinalExpr;
            struct Statement *forStmt;
        };
    };
} IterationStatement;

typedef enum {
    JumpStatement_Goto,
    JumpStatement_Continue,
    JumpStatement_Break,
    JumpStatement_Return,
} JumpStatementType;

typedef struct {
    JumpStatementType type;
    union {
        String gotoIdent;
        struct {
            bool returnHasExpr;
            Expr returnExpr;
        };
    };
} JumpStatement;

typedef struct {
    uint8_t tmp;
} AsmStatement;

typedef enum {
    Statement_Labeled,
    Statement_Compound,
    Statement_Expression,
    Statement_Selection,
    Statement_Iteration,
    Statement_Jump,
    Statement_Asm,
} StatementType;

typedef struct Statement {
    StatementType type;
    union {
        LabeledStatement labeled;
        struct CompoundStmt *compound;
        SelectionStatement selection;
        IterationStatement iteration;
        JumpStatement jump;
        ExpressionStatement expression;
        AsmStatement assembly;
    };
} Statement;

typedef enum {
    BlockItem_Declaration,
    BlockItem_Statement,
} BlockItemType;

typedef struct {
    BlockItemType type;
    union {
        Declaration decl;
        Statement stmt;
    };
} BlockItem;

typedef struct {
    SLList list;
} BlockItemList;

typedef struct CompoundStmt {
    Token *openBracket;
    Token *closeBracket;

    bool isEmpty;
    BlockItemList blockItemList;
} CompoundStmt;

typedef struct {
    DeclarationSpecifierList specifiers;
    Declarator declarator;
    SLList declarations;
    CompoundStmt stmt;
} FuncDef;

typedef enum {
    ExternalDecl_FuncDef,
    ExternalDecl_Decl
} ExternalDeclType;

typedef struct {
    ExternalDeclType type;
    union {
        FuncDef func;
        Declaration decl;
    };
} ExternalDecl;

typedef struct {
    SLList externalDecls;
} TranslationUnit;

void translationUnit_cleanup(TranslationUnit unit);

bool parseTokens(TokenList *tokens,
    TranslationUnit *outTranslationUnit);

void printTranslationUnit(TranslationUnit translationUnit);
