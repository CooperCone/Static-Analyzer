#include "parser.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "logger.h"
#include "array.h"
#include "debug.h"
#include "linkedList.h"

// Generic Parsers
#define Fail(msg) ((ParseRes){ .success = false, .failMessage = msg })
#define Succeed ((ParseRes){ .success = true })

// CLEANUP: Finish cleanup of parsers and reduction of duplicate code

ParseRes parseList(TokenList *tokens, void *data)
{
    SimpleParser *parser = data;
    assert(parser->type == SimpleParser_List);

    // Parse designator list
    ParseRes res = {0};
    do {
        size_t pos = tokens->pos;

        void *elem = calloc(1, parser->listElemSize);
        res = parser->listElemParser(tokens, elem);
        if (!res.success) {
            tokens->pos = pos;
            break;
        }

        sll_append(parser->listOut, elem, parser->listElemSize);

        free(elem);
    } while (res.success);

    if (parser->listOut->size == 0)
        return Fail(parser->listFailMessage);

    return Succeed;
}

ParseRes parseOptional(TokenList *tokens, void *data) {
    SimpleParser *parser = data;
    assert(parser->type == SimpleParser_Optional);

    size_t pos = tokens->pos;

    ParseRes res = parser->optionalParser(tokens, parser->optionalData);

    if (!res.success) {
        tokens->pos = pos;
    }

    return Succeed;
}

SimpleParser ListParser(Parser listElemParser, SLList *listOut,
    size_t listElemSize, char *listFailMessage)
{
    SimpleParser res = {0};
    res.type = SimpleParser_List;
    res.listElemParser = listElemParser;
    res.listOut = listOut;
    res.listElemSize = listElemSize;
    res.listFailMessage = listFailMessage;
    res.run = parseList;
    return res;
}

SimpleParser OptionalParser(Parser parser, void *data) {
    SimpleParser res = {0};
    res.type = SimpleParser_Optional;
    res.optionalParser = parser;
    res.optionalData = data;
    res.run = parseOptional;
    return res;
}

// End Generic Parsers

static SLList g_typedefTable;

bool typedefTable_find(SLList table, String name) {
    sll_foreach(table, node) {
        String *tableName = slNode_getData(node);
        if (astr_cmp(*tableName, name))
            return true;
    }

    return false;
}

void typedefTable_add(SLList *table, String name) {
    sll_appendLocal(table, name);
}

// CLEANUP: A lot of this can be combined
// - need a pass, fail function
// - optional parser
// - list parser
// - more parsers to make sure I didn't make mistakes

Token peekTok(TokenList *tokens) {
    if (tokens->pos < tokens->numTokens)
        return tokens->tokens[tokens->pos];

    return (Token){ .type = 0 };
}

Token peekAheadTok(TokenList *tokens, size_t lookahead) {
    if (lookahead + tokens->pos >= tokens->numTokens)
        return (Token){ .type = 0 };

    return tokens->tokens[tokens->pos + lookahead];
}

Token consumeTok(TokenList *tokens) {
    Token tok = peekTok(tokens);
    tokens->pos++;
    return tok;
}

void consumeMulti(TokenList *tokens, size_t numToConsume) {
    tokens->pos += numToConsume;
}

bool consumeIfTok(TokenList *tokens, TokenType type) {
    if (peekTok(tokens).type == type) {
        tokens->pos++;
        return true;
    }

    return false;
}

// Forward decls
ParseRes parseAssignExpr(TokenList *tokens, AssignExpr *expr);
ParseRes parseConditionalExpr(TokenList *tokens, ConditionalExpr *expr);
ParseRes parseInitializerList(TokenList *tokens, InitializerList *list);
ParseRes parseTypeName(TokenList *tokens, TypeName *typeName);
ParseRes parseExpr(TokenList *tokens, Expr *expr);
ParseRes parseCastExpr(TokenList *tokens, CastExpr *expr);
ParseRes parseTypeSpecifier(TokenList *tokens, TypeSpecifier *type);
ParseRes parseTypeQualifier(TokenList *tokens, TypeQualifier *typeQualifier);
ParseRes parseAbstractDeclarator(TokenList *tokens, AbstractDeclarator *abstractDeclarator);
ParseRes parseCompoundStmt(TokenList *tokens, CompoundStmt *outStmt);
ParseRes parseStatement(TokenList *tokens, Statement *stmt);
ParseRes parseDeclarationSpecifierList(TokenList *tokens, DeclarationSpecifierList *outList);
ParseRes parseDeclarator(TokenList *tokens, Declarator *declarator);
// End forward decls

ParseRes parseArgExprList(TokenList *tokens, ArgExprList *argExprList) {
    bool hasComma = false;
    do {
        size_t pos = tokens->pos;

        AssignExpr expr = {0};
        ParseRes res = parseAssignExpr(tokens, &expr);
        if (!res.success) {
            tokens->pos = pos;
            break;
        }

        hasComma = consumeIfTok(tokens, ',');

        sll_appendLocal(&argExprList->list, expr);
    } while (hasComma);

    return (ParseRes){ .success = true };
}

ParseRes parseDesignator(TokenList *tokens, Designator *designator) {
    if (consumeIfTok(tokens, '.')) {
        if (peekTok(tokens).type != Token_Ident) {
            return (ParseRes) {
                .success = false,
                .failMessage = "Designator, expected identifier after ."
            };
        }
        designator->type = Designator_Ident;
        designator->ident = consumeTok(tokens).ident;

        return (ParseRes) { .success = true };
    }
    else if (consumeIfTok(tokens, '[')) {

        ConditionalExpr expr = {0};
        if (!parseConditionalExpr(tokens, &expr).success) {
            return (ParseRes) {
                .success = false,
                .failMessage = "Designator, expected conditional expr after ["
            };
        }

        if (!consumeIfTok(tokens, ']')) {
            return (ParseRes) {
                .success = false,
                .failMessage = "Designator, expected ] after expr"
            };
        }

        designator->type = Designator_Constant;
        designator->constantExpr = malloc(sizeof(ConditionalExpr));
        memcpy(designator->constantExpr, &expr, sizeof(expr));

        return (ParseRes) { .success = true };
    }
    else {
        return (ParseRes){
            .success = false,
            .failMessage = "Expected . or [ to start a designator"
        };
    }
}

ParseRes parseDesignation(TokenList *tokens, Designation *designation) {
    // // Parse designator list
    // ParseRes res = {0};
    // do {
    //     size_t pos = tokens->pos;

    //     Designator designator = {0};
    //     res = parseDesignator(tokens, &designator);
    //     if (!res.success) {
    //         tokens->pos = pos;
    //         break;
    //     }

    //     sll_appendLocal(&(designation->list), designator);
    // } while (res.success);

    SimpleParser list = ListParser((Parser)parseDesignator, &(designation->list),
        sizeof(Designator), "Failed to find designator list in designation");
    list.run(tokens, &list);

    if (!consumeIfTok(tokens, '=')) {
        return (ParseRes){
            .success = false,
            .failMessage = "Didn't find = after designation"
        };
    }

    return (ParseRes){ .success = true };
}

ParseRes parseInitializer(TokenList *tokens, Initializer *initializer) {
    if (consumeIfTok(tokens, '{')) {
        InitializerList list = {0};
        ParseRes res = parseInitializerList(tokens, &list);
        if (!res.success) {
            return res;
        }

        // Remove trailing comma
        consumeIfTok(tokens, ',');

        if (!consumeIfTok(tokens, '}')) {
            return (ParseRes) {
                .success = false,
                .failMessage = "Expected } after initializer list"
            };
        }

        initializer->type = Initializer_InitializerList;
        initializer->initializerList = malloc(sizeof(list));
        memcpy(initializer->initializerList, &list, sizeof(list));

        return (ParseRes){ .success = true };
    }

    // No initializer list, so parse an assignment expression
    AssignExpr expr = {0};
    ParseRes assignRes = parseAssignExpr(tokens, &expr);
    if (!assignRes.success)
        return assignRes;

    initializer->type = Initializer_Assignment;
    initializer->assignmentExpr = malloc(sizeof(expr));
    memcpy(initializer->assignmentExpr, &expr, sizeof(expr));

    return (ParseRes){ .success = true };
}

ParseRes parseInitializerList(TokenList *tokens, InitializerList *list) {

    bool hasComma = false;
    bool isAtEnd = false;
    do {
        size_t beforePos = tokens->pos;

        // Try to parse a designation
        Designation designation = {0};
        bool hasDesignation = true;
        if (!parseDesignation(tokens, &designation).success) {
            tokens->pos = beforePos;
            hasDesignation = false;
        }

        // Parse an initializer and add it
        Initializer initializer = {0};
        if (!parseInitializer(tokens, &initializer).success) {
            return (ParseRes) {
                .success = false,
                .failMessage = "Failed to parse an initializer"
            };
        }

        DesignationAndInitializer wholeInitializer = {
            .hasDesignation = hasDesignation,
            .designation = designation,
            .initializer = initializer
        };

        sll_appendLocal(&(list->list), wholeInitializer);

        // Parse a ,
        hasComma = consumeIfTok(tokens, ',');

        isAtEnd = peekTok(tokens).type == '}';

    } while (!isAtEnd && hasComma);

    return (ParseRes){ .success = true };
}

ParseRes parseGenericAssociation(TokenList *tokens, GenericAssociation *association) {
    if (consumeIfTok(tokens, Token_default)) {
        if (!consumeIfTok(tokens, ':')) {
            return (ParseRes) {
                .success = false,
                .failMessage = "Didn't find : after Default in generic association"
            };
        }

        AssignExpr expr = {0};
        ParseRes assignRes = parseAssignExpr(tokens, &expr);
        if (!assignRes.success)
            return assignRes;

        association->isDefault = true;
        association->expr = malloc(sizeof(expr));
        memcpy(association->expr, &expr, sizeof(expr));
        return (ParseRes){ .success = true };
    }

    TypeName typeName = {0};
    ParseRes typeNameRes = parseTypeName(tokens, &typeName);
    if (!typeNameRes.success)
        return typeNameRes;

    if (!consumeIfTok(tokens, ':')) {
        return (ParseRes) {
            .success = false,
            .failMessage = "Didn't find : after type name in generic association"
        };
    }

    AssignExpr expr = {0};
    ParseRes assignRes = parseAssignExpr(tokens, &expr);
    if (!assignRes.success)
        return assignRes;

    association->isDefault = false;
    association->typeName = malloc(sizeof(typeName));
    memcpy(association->typeName, &typeName, sizeof(typeName));
    association->expr = malloc(sizeof(expr));
    memcpy(association->expr, &expr, sizeof(expr));
    return (ParseRes){ .success = true };
}

ParseRes parseGenericSelection(TokenList *tokens, GenericSelection *generic) {
    if (!consumeIfTok(tokens, Token_generic)) {
        return (ParseRes) {
            .success = false,
            .failMessage = "Didn't find _Generic in front of generic selection"
        };
    }

    if (!consumeIfTok(tokens, '(')) {
        return (ParseRes) {
            .success = false,
            .failMessage = "Didn't find ( after _Generic keyword"
        };
    }

    AssignExpr assign = {0};
    ParseRes assignRes = parseAssignExpr(tokens, &assign);
    if (!assignRes.success)
        return assignRes;

    if (!consumeIfTok(tokens, ',')) {
        return (ParseRes) {
            .success = false,
            .failMessage = "Didn't find , after assign expr in generic selection"
        };
    }

    generic->expr = malloc(sizeof(assign));
    memcpy(generic->expr, &assign, sizeof(assign));

    bool hasComma = false;
    do {
        // Parse a generic association
        GenericAssociation association = {0};
        ParseRes res = parseGenericAssociation(tokens, &association);
        if (!res.success) {
            return res;
        }

        sll_appendLocal(&(generic->associations), association);

        // Continue if there's a comma
        hasComma = consumeIfTok(tokens, ',');
    } while(hasComma);

    if (!consumeIfTok(tokens, ')')) {
        return (ParseRes) {
            .success = false,
            .failMessage = "Didn't find ) after generic association list"
        };
    }

    return (ParseRes) { .success = true };
}

ParseRes parsePrimaryExpr(TokenList *tokens, PrimaryExpr *primary) {
    if (peekTok(tokens).type == Token_ConstString) {
        // TODO: Handle this correctly. We currently throw away all
        // but the last strings
        Token tok = {0};
        while (peekTok(tokens).type == Token_ConstString)
            tok = consumeTok(tokens);

        primary->type = PrimaryExpr_String;
        primary->string = tok.constString;
        return (ParseRes){ .success = true };
    }
    else if (peekTok(tokens).type == '(') {
        consumeTok(tokens);

        Expr expr = {0};
        ParseRes res = parseExpr(tokens, &expr);
        if (!res.success)
            return res;

        if (!consumeIfTok(tokens, ')')) {
            return (ParseRes) {
                .success = false,
                .failMessage = "Expected ) after ( expr in primary expr"
            };
        }

        primary->type = PrimaryExpr_Expr;
        primary->expr = malloc(sizeof(expr));
        memcpy(primary->expr, &expr, sizeof(expr));
        return (ParseRes){ .success = true };
    }
    else if (peekTok(tokens).type == Token_ConstNumeric) {
        Token tok = consumeTok(tokens);

        primary->type = PrimaryExpr_Constant;
        primary->constant.type = Constant_Numeric;
        primary->constant.data = tok.numeric;
        return (ParseRes){ .success = true };
    }
    else if (peekTok(tokens).type == Token_funcName) {
        consumeTok(tokens);

        primary->type = PrimaryExpr_FuncName;
        return (ParseRes){ .success = true };
    }
    else if (peekTok(tokens).type == Token_Ident) {
        Token tok = consumeTok(tokens);

        primary->type = PrimaryExpr_Ident;
        primary->ident = tok.ident;
        return (ParseRes){ .success = true };
    }
    else if (peekTok(tokens).type == Token_generic) {
        GenericSelection generic = {0};
        ParseRes genericRes = parseGenericSelection(tokens, &generic);

        if (!genericRes.success) {
            return genericRes;
        }

        primary->type = PrimaryExpr_GenericSelection;
        primary->genericSelection = generic;
        return (ParseRes){ .success = true };
    }

    return (ParseRes) {
        .success = false,
        .failMessage = "Failed to parse any primary exprs"
    };
}

ParseRes parsePostfixOp(TokenList *tokens, PostfixOp *op) {
    if (consumeIfTok(tokens, '[')) {
        Expr expr = {0};
        ParseRes res = parseExpr(tokens, &expr);
        if (!res.success)
            return res;

        if (!consumeIfTok(tokens, ']')) {
            return (ParseRes) {
                .success = false,
                .failMessage = "Expexted ] after indexing postfix"
            };
        }

        op->type = PostfixOp_Index;
        op->indexExpr = malloc(sizeof(expr));
        memcpy(op->indexExpr, &expr, sizeof(expr));
        return (ParseRes){ .success = true };
    }
    if (consumeIfTok(tokens, '(')) {
        op->type = PostfixOp_Call;
        if (consumeIfTok(tokens, ')')) {
            op->callHasEmptyArgs = true;
            return (ParseRes){ .success = true };
        }

        ArgExprList argExprList = {0};
        ParseRes exprRes = parseArgExprList(tokens, &argExprList);
        if (!exprRes.success)
            return exprRes;

        if (!consumeIfTok(tokens, ')')) {
            return (ParseRes) {
                .success = false,
                .failMessage = "Expected ) after call arg expr list"
            };
        }

        op->callExprs = argExprList;
        return (ParseRes){ .success = true };
    }
    if (consumeIfTok(tokens, '.')) {
        if (peekTok(tokens).type != Token_Ident) {
            return (ParseRes) {
                .success = false,
                .failMessage = "Expected ident after . access op"
            };
        }

        Token ident = consumeTok(tokens);

        op->type = PostfixOp_Dot;
        op->dotIdent = ident.ident;
        return (ParseRes){ .success = true };
    }
    if (consumeIfTok(tokens, Token_PtrOp)) {
        if (peekTok(tokens).type != Token_Ident) {
            return (ParseRes) {
                .success = false,
                .failMessage = "Expected ident after -> access op"
            };
        }

        Token ident = consumeTok(tokens);

        op->type = PostfixOp_Arrow;
        op->arrowIdent = ident.ident;
        return (ParseRes){ .success = true };
    }
    if (consumeIfTok(tokens, Token_IncOp)) {
        op->type = PostfixOp_Inc;
        return (ParseRes){ .success = true };
    }
    if (consumeIfTok(tokens, Token_DecOp)) {
        op->type = PostfixOp_Dec;
        return (ParseRes){ .success = true };
    }

    return (ParseRes) {
        .success = false,
        .failMessage = "Didn't find a postfix op"
    };
}

ParseRes parsePostfixExpr(TokenList *tokens, PostfixExpr *postfixExpr) {
    // Check if initializer list
    size_t preInitializeListPos = tokens->pos;

    // FIXME: Is this really a good use of goto chains?

    if (!consumeIfTok(tokens, '('))
        goto PostfixExpr_AfterPostfix;

    TypeName typeName = {0};
    if (!(parseTypeName(tokens, &typeName).success))
        goto PostfixExpr_AfterPostfix;

    if (!consumeIfTok(tokens, ')'))
        goto PostfixExpr_AfterPostfix;

    if (!consumeIfTok(tokens, '{'))
        goto PostfixExpr_AfterPostfix;

    InitializerList list = {0};
    if (!(parseInitializerList(tokens, &list).success))
        goto PostfixExpr_AfterPostfix;

    // Remove trailing comma on initializer list
    consumeIfTok(tokens, ',');

    if (!consumeIfTok(tokens, '}'))
        goto PostfixExpr_AfterPostfix;

    // Succeeded, move on to postfix ops
    postfixExpr->type = Postfix_InitializerList;
    postfixExpr->initializerListType = malloc(sizeof(typeName));
    memcpy(postfixExpr->initializerListType, &typeName, sizeof(typeName));
    postfixExpr->initializerList = list;

    goto PostfixExpr_AfterPrimaryExpr;

PostfixExpr_AfterPostfix:
    tokens->pos = preInitializeListPos;

    // Otherwise is a primary expr
    PrimaryExpr primary = {0};
    ParseRes primaryRes = parsePrimaryExpr(tokens, &primary);
    if (!primaryRes.success) {
        return primaryRes;
    }

    postfixExpr->type = Postfix_Primary;
    postfixExpr->primary = primary;

PostfixExpr_AfterPrimaryExpr:
    ; // Empty statement needed because label cannot precede assignment
    // Check for postfix ops
    ParseRes res = {0};
    do {
        size_t postfixPos = tokens->pos;

        PostfixOp op = {0};
        res = parsePostfixOp(tokens, &op);
        if (!res.success) {
            tokens->pos = postfixPos;
            break;
        }

        sll_appendLocal(&(postfixExpr->postfixOps), op);
    } while (res.success);

    return (ParseRes){ .success = true };
}

ParseRes parseUnaryExprPrefix(TokenList *tokens, UnaryExprPrefixType *type) {
    if (consumeIfTok(tokens, '&')) {
        *type = UnaryPrefix_And;
        return (ParseRes){ .success = true };
    }
    else if (consumeIfTok(tokens, '*')) {
        *type = UnaryPrefix_Star;
        return (ParseRes){ .success = true };
    }
    else if (consumeIfTok(tokens, '+')) {
        *type = UnaryPrefix_Plus;
        return (ParseRes){ .success = true };
    }
    else if (consumeIfTok(tokens, '-')) {
        *type = UnaryPrefix_Minus;
        return (ParseRes){ .success = true };
    }
    else if (consumeIfTok(tokens, '~')) {
        *type = UnaryPrefix_Tilde;
        return (ParseRes){ .success = true };
    }
    else if (consumeIfTok(tokens, '!')) {
        *type = UnaryPrefix_Not;
        return (ParseRes){ .success = true };
    }

    return (ParseRes){
        .success = false,
        .failMessage = "Expected to find a unary prefix op"
    };
}

// TODO: This is NOT correct.
// Try to parse
//    -- unary_operator -> castExpr
//    -- INC or DEC or sizeof -> unary
//    -- sizeof ( typeName )
//    -- alignof ( typeName )
// Else parse
//    -- postfix
ParseRes parseUnaryExpr(TokenList *tokens, UnaryExpr *unaryExpr) {
    size_t pos = tokens->pos;

    // Try to parse a unary operator
    {
        UnaryExprPrefixType prefixType = {0};
        if (!parseUnaryExprPrefix(tokens, &prefixType).success)
            goto ParseUnaryExpr_PostPrefix;

        // Try to parse a cast expr
        CastExpr cast = {0};
        if (!parseCastExpr(tokens, &cast).success)
            goto ParseUnaryExpr_PostPrefix;

        unaryExpr->type = UnaryExpr_UnaryOp;
        unaryExpr->unaryOpType = prefixType;
        unaryExpr->unaryOpCast = malloc(sizeof(cast));
        memcpy(unaryExpr->unaryOpCast, &cast, sizeof(cast));
        return (ParseRes){ .success = true };
    }

ParseUnaryExpr_PostPrefix:

    tokens->pos = pos;

    // Try to parse an increment, decrement, or sizeof
    {
        if (peekTok(tokens).type != Token_IncOp &&
            peekTok(tokens).type != Token_DecOp &&
            peekTok(tokens).type != Token_sizeof)
        {
            goto ParseUnaryExpr_PostIncDecSizeofExpr;
        }

        Token tok = consumeTok(tokens);

        UnaryExpr innerExpr = {0};
        if (!parseUnaryExpr(tokens, &innerExpr).success)
            goto ParseUnaryExpr_PostIncDecSizeofExpr;

        UnaryExpr *innerAllocated = malloc(sizeof(innerExpr));
        memcpy(innerAllocated, &innerExpr, sizeof(innerExpr));
        if (tok.type == Token_IncOp) {
            unaryExpr->type = UnaryExpr_Inc;
            unaryExpr->incOpExpr = innerAllocated;
        }
        else if (tok.type == Token_DecOp) {
            unaryExpr->type = UnaryExpr_Dec;
            unaryExpr->decOpExpr = innerAllocated;
        }
        else {
            unaryExpr->type = UnaryExpr_SizeofExpr;
            unaryExpr->sizeofExpr = innerAllocated;
        }

        return (ParseRes){ .success = true };
    }

ParseUnaryExpr_PostIncDecSizeofExpr:

    tokens->pos = pos;

    // Try to parse a sizeof ( typename )
    {
        if (!consumeIfTok(tokens, Token_sizeof))
            goto ParseUnaryExpr_PostSizeofTypename;

        if (!consumeIfTok(tokens, '('))
            goto ParseUnaryExpr_PostSizeofTypename;

        TypeName typeName = {0};
        if (!parseTypeName(tokens, &typeName).success)
            goto ParseUnaryExpr_PostSizeofTypename;

        if (!consumeIfTok(tokens, ')'))
            goto ParseUnaryExpr_PostSizeofTypename;

        unaryExpr->type = UnaryExpr_SizeofType;
        unaryExpr->sizeofTypeName = malloc(sizeof(typeName));
        memcpy(unaryExpr->sizeofExpr, &typeName, sizeof(typeName));

        return (ParseRes){ .success = true };
    }

ParseUnaryExpr_PostSizeofTypename:

    tokens->pos = pos;

    // Try to parse an alignof typename
    {
        if (!consumeIfTok(tokens, Token_alignof))
            goto ParseUnaryExpr_PostAlignofTypename;

        if (!consumeIfTok(tokens, '('))
            goto ParseUnaryExpr_PostAlignofTypename;

        TypeName typeName = {0};
        if (!parseTypeName(tokens, &typeName).success)
            goto ParseUnaryExpr_PostAlignofTypename;

        if (!consumeIfTok(tokens, ')'))
            goto ParseUnaryExpr_PostAlignofTypename;

        unaryExpr->type = UnaryExpr_AlignofType;
        unaryExpr->alignofTypeName = malloc(sizeof(typeName));
        memcpy(unaryExpr->alignofTypeName, &typeName, sizeof(typeName));

        return (ParseRes){ .success = true };
    }

ParseUnaryExpr_PostAlignofTypename:

    tokens->pos = pos;

    // If we got here then we need to parse a postfix expr
    PostfixExpr postfix = {0};
    ParseRes postfixRes = parsePostfixExpr(tokens, &postfix);
    if (!postfixRes.success)
        return postfixRes;

    unaryExpr->type = UnaryExpr_Base;
    unaryExpr->baseExpr = postfix;

    return (ParseRes){ .success = true };
}

ParseRes parseCastExpr(TokenList *tokens, CastExpr *cast) {
    // Look for an optional cast
    size_t castPos = tokens->pos;

    if (!consumeIfTok(tokens, '('))
        goto Cast_NoCast;

    TypeName typeName = {0};
    ParseRes typeNameRes = parseTypeName(tokens, &typeName);
    if (!typeNameRes.success)
        goto Cast_NoCast;

    if (!consumeIfTok(tokens, ')'))
        goto Cast_NoCast;

    CastExpr newCast = {0};
    ParseRes castRes = parseCastExpr(tokens, &newCast);
    if (!castRes.success)
        goto Cast_NoCast;

    cast->type = CastExpr_Cast;
    cast->castType = malloc(sizeof(typeName));
    memcpy(cast->castType, &typeName, sizeof(typeName));
    cast->castExpr = malloc(sizeof(CastExpr));
    memcpy(cast->castExpr, &newCast, sizeof(newCast));

    return (ParseRes){ .success = true };

Cast_NoCast:
    tokens->pos = castPos;

    // Look for a unary expr
    UnaryExpr unary = {0};
    ParseRes unaryRes = parseUnaryExpr(tokens, &unary);
    if (!unaryRes.success)
        return unaryRes;

    cast->type = CastExpr_Unary;
    cast->unary = unary;

    return (ParseRes){ .success = true };
}

ParseRes parseMultiplicativeExpr(TokenList *tokens, MultiplicativeExpr *multiplicativeExpr) {
    CastExpr castExpr = {0};
    ParseRes res = parseCastExpr(tokens, &castExpr);
    if (!res.success) {
        // Error if this fails
        return res;
    }

    multiplicativeExpr->baseExpr = castExpr;

    while (peekTok(tokens).type == '*' ||
        peekTok(tokens).type == '/' ||
        peekTok(tokens).type == '%')
    {
        TokenType type = consumeTok(tokens).type;
        MultiplicativeOp op = type == '*' ? Multiplicative_Mul :
            type == '/' ? Multiplicative_Div :
            Multiplicative_Mod;

        CastExpr cast = {0};
        ParseRes res = parseCastExpr(tokens, &cast);
        if (!res.success) {
            // Error if this fails
            return res;
        }

        MultiplicativePost post = {
            .op = op,
            .expr = cast
        };

        sll_appendLocal(&(multiplicativeExpr->postExprs), post);
    }

    return (ParseRes){ .success = true };
}

ParseRes parseAdditiveExpr(TokenList *tokens, AdditiveExpr *additiveExpr) {
    MultiplicativeExpr multiplicativeExpr = {0};
    ParseRes res = parseMultiplicativeExpr(tokens, &multiplicativeExpr);
    if (!res.success) {
        // Error if this fails
        return res;
    }

    additiveExpr->baseExpr = multiplicativeExpr;

    while (peekTok(tokens).type == '+' ||
        peekTok(tokens).type == '-')
    {
        TokenType type = consumeTok(tokens).type;
        AdditiveOp op = type == '+' ? Additive_Add : Additive_Sub;

        MultiplicativeExpr multiplicative = {0};
        ParseRes res = parseMultiplicativeExpr(tokens, &multiplicative);
        if (!res.success) {
            // Error if this fails
            return res;
        }

        AdditivePost post = {
            .op = op,
            .expr = multiplicative
        };

        sll_appendLocal(&(additiveExpr->postExprs), post);
    }

    return (ParseRes){ .success = true };
}

ParseRes parseShiftExpr(TokenList *tokens, ShiftExpr *shiftExpr) {
    AdditiveExpr additiveExpr = {0};
    ParseRes res = parseAdditiveExpr(tokens, &additiveExpr);
    if (!res.success) {
        // Error if this fails
        return res;
    }

    shiftExpr->baseExpr = additiveExpr;

    while (peekTok(tokens).type == Token_ShiftLeftOp ||
        peekTok(tokens).type == Token_ShiftRightOp)
    {
        TokenType type = consumeTok(tokens).type;
        ShiftOp op = type == Token_ShiftLeftOp ? Shift_Left : Shift_Right;

        AdditiveExpr additive = {0};
        ParseRes res = parseAdditiveExpr(tokens, &additive);
        if (!res.success) {
            // Error if this fails
            return res;
        }

        ShiftPost post = {
            .op = op,
            .expr = additive
        };

        sll_appendLocal(&(shiftExpr->postExprs), post);
    }

    return (ParseRes){ .success = true };
}

ParseRes parseRelationalExpr(TokenList *tokens, RelationalExpr *relExpr) {
    ShiftExpr shiftExpr = {0};
    ParseRes res = parseShiftExpr(tokens, &shiftExpr);
    if (!res.success) {
        // Error if this fails
        return res;
    }

    relExpr->baseExpr = shiftExpr;

    while (peekTok(tokens).type == '<' ||
        peekTok(tokens).type == '>' ||
        peekTok(tokens).type == Token_LEqOp ||
        peekTok(tokens).type == Token_GEqOp)
    {
        TokenType type = consumeTok(tokens).type;
        RelationalOp op = type == '<' ? Relational_Lt :
            type == '>' ? Relational_Gt :
            type == Token_LEqOp ? Relational_LEq :
            Relational_GEq;

        ShiftExpr shift = {0};
        ParseRes res = parseShiftExpr(tokens, &shift);
        if (!res.success) {
            // Error if this fails
            return res;
        }

        RelationalPost post = {
            .op = op,
            .expr = shift
        };

        sll_appendLocal(&(relExpr->postExprs), post);
    }

    return (ParseRes){ .success = true };
}

ParseRes parseEqualityExpr(TokenList *tokens, EqualityExpr *eqExpr) {
    RelationalExpr relExpr = {0};
    ParseRes res = parseRelationalExpr(tokens, &relExpr);
    if (!res.success) {
        // Error if this fails
        return res;
    }

    eqExpr->baseExpr = relExpr;

    while (peekTok(tokens).type == Token_EqOp ||
        peekTok(tokens).type == Token_NEqOp)
    {
        EqualityOp op = consumeTok(tokens).type == Token_EqOp ?
            Equality_Eq : Equality_NEq;

        RelationalExpr rel = {0};
        ParseRes res = parseRelationalExpr(tokens, &rel);
        if (!res.success) {
            // Error if this fails
            return res;
        }

        EqualityPost post = {
            .op = op,
            .expr = rel,
        };

        sll_appendLocal(&(eqExpr->postExprs), post);
    }

    return (ParseRes){ .success = true };
}

ParseRes parseAndExpr(TokenList *tokens, AndExpr *andExpr) {
    bool foundAndOp = false;
    do {
        EqualityExpr eqExpr = {0};
        ParseRes res = parseEqualityExpr(tokens, &eqExpr);
        if (!res.success) {
            // Error if this fails
            return res;
        }

        foundAndOp = consumeIfTok(tokens, '&');

        sll_appendLocal(&(andExpr->list), eqExpr);
    } while (foundAndOp);

    return (ParseRes){ .success = true };
}

ParseRes parseExclusiveOrExpr(TokenList *tokens, ExclusiveOrExpr *exclusiveOr) {
    bool foundOrOp = false;
    do {
        AndExpr andExpr = {0};
        ParseRes res = parseAndExpr(tokens, &andExpr);
        if (!res.success) {
            // Error if this fails
            return res;
        }

        foundOrOp = consumeIfTok(tokens, '^');

        sll_appendLocal(&(exclusiveOr->list), andExpr);
    } while (foundOrOp);

    return (ParseRes){ .success = true };
}

ParseRes parseInclusiveOrExpr(TokenList *tokens, InclusiveOrExpr *inclusiveOr) {
    bool foundOrOp = false;
    do {
        ExclusiveOrExpr orExpr = {0};
        ParseRes res = parseExclusiveOrExpr(tokens, &orExpr);
        if (!res.success) {
            // Error if this fails
            return res;
        }

        foundOrOp = consumeIfTok(tokens, '|');

        sll_appendLocal(&(inclusiveOr->list), orExpr);
    } while (foundOrOp);

    return (ParseRes){ .success = true };
}

ParseRes parseLogicalAndExpr(TokenList *tokens, LogicalAndExpr *logicalAnd) {
    bool foundAndOp = false;
    do {
        InclusiveOrExpr orExpr = {0};
        ParseRes res = parseInclusiveOrExpr(tokens, &orExpr);
        if (!res.success) {
            // Error if this fails
            return res;
        }

        foundAndOp = consumeIfTok(tokens, Token_LogAndOp);

        sll_appendLocal(&(logicalAnd->list), orExpr);
    } while (foundAndOp);

    return (ParseRes){ .success = true };
}

ParseRes parseLogicalOrExpr(TokenList *tokens, LogicalOrExpr *logicalOr) {
    bool foundOrOp = false;
    do {
        LogicalAndExpr andExpr = {0};
        ParseRes res = parseLogicalAndExpr(tokens, &andExpr);
        if (!res.success) {
            // Error if this fails
            return res;
        }

        foundOrOp = consumeIfTok(tokens, Token_LogOrOp);

        sll_appendLocal(&(logicalOr->list), andExpr);
    } while (foundOrOp);

    return (ParseRes){ .success = true };
}

ParseRes parseConditionalExpr(TokenList *tokens, ConditionalExpr *conditional) {
    LogicalOrExpr logicalOr = {0};
    ParseRes res = parseLogicalOrExpr(tokens, &logicalOr);
    if (!res.success)
        return res;

    conditional->beforeExpr = logicalOr;

    if (!consumeIfTok(tokens, '?')) {
        conditional->hasConditionalOp = false;
        return (ParseRes){ .success = true };
    }

    Expr expr = {0};
    ParseRes exprRes = parseExpr(tokens, &expr);
    if (!exprRes.success)
        return exprRes;

    if (!consumeIfTok(tokens, ':')) {
        return (ParseRes) {
            .success = false,
            .failMessage = "Expected : after ? and expr"
        };
    }

    ConditionalExpr falseExpr = {0};
    ParseRes falseRes = parseConditionalExpr(tokens, &falseExpr);
    if (!falseRes.success)
        return falseRes;

    conditional->hasConditionalOp = true;
    conditional->ifTrueExpr = malloc(sizeof(expr));
    memcpy(conditional->ifTrueExpr, &expr, sizeof(expr));
    conditional->ifFalseExpr = malloc(sizeof(falseExpr));
    memcpy(conditional->ifFalseExpr, &falseExpr, sizeof(falseExpr));

    return (ParseRes){ .success = true };
}

ParseRes parseAssignOp(TokenList *tokens, AssignOp *op) {
    if (consumeIfTok(tokens, '=')) {
        *op = Assign_Eq;
    }
    else if (consumeIfTok(tokens, Token_MulAssign)) {
        *op = Assign_MulEq;
    }
    else if (consumeIfTok(tokens, Token_DivAssign)) {
        *op = Assign_DivEq;
    }
    else if (consumeIfTok(tokens, Token_ModAssign)) {
        *op = Assign_ModEq;
    }
    else if (consumeIfTok(tokens, Token_AddAssign)) {
        *op = Assign_AddEq;
    }
    else if (consumeIfTok(tokens, Token_SubAssign)) {
        *op = Assign_SubEq;
    }
    else if (consumeIfTok(tokens, Token_ShiftLeftAssign)) {
        *op = Assign_ShiftLeftEq;
    }
    else if (consumeIfTok(tokens, Token_ShiftRightAssign)) {
        *op = Assign_ShiftRightEq;
    }
    else if (consumeIfTok(tokens, Token_AndAssign)) {
        *op = Assign_AndEq;
    }
    else if (consumeIfTok(tokens, Token_XorAssign)) {
        *op = Assign_XorEq;
    }
    else if (consumeIfTok(tokens, Token_OrAssign)) {
        *op = Assign_OrEq;
    }
    else {
        return (ParseRes) {
            .success = false,
            .failMessage = "Couldn't find an assignment op"
        };
    }

    return (ParseRes){ .success = true };
}

ParseRes parseAssignExpr(TokenList *tokens, AssignExpr *assignExpr) {
    // Parse an optional list of assign exprs
    ParseRes assignRes = {0};
    do {
        size_t preAssignPos = tokens->pos;

        UnaryExpr unaryExpr = {0};
        ParseRes res = parseUnaryExpr(tokens, &unaryExpr);
        if (!res.success) {
            tokens->pos = preAssignPos;
            break;
        }

        AssignOp op = {0};
        assignRes = parseAssignOp(tokens, &op);
        if (!assignRes.success) {
            tokens->pos = preAssignPos;
            break;
        }

        AssignPrefix leftExpr = { unaryExpr, op };

        sll_appendLocal(&(assignExpr->leftExprs), leftExpr);
    } while (assignRes.success);

    // Parse a conditional expr
    ConditionalExpr conditional = {0};
    ParseRes res = parseConditionalExpr(tokens, &conditional);
    if (!res.success)
        return res;

    assignExpr->rightExpr = conditional;

    return (ParseRes){ .success = true };
}

ParseRes parseInnerExpr(TokenList *tokens, InnerExpr *inner) {
    size_t pos = tokens->pos;

    // Try to parse parens with a compound statement
    if (!consumeIfTok(tokens, '('))
        goto ParseInnerExpr_AfterCompound;

    CompoundStmt compound = {0};
    if (!parseCompoundStmt(tokens, &compound).success)
        goto ParseInnerExpr_AfterCompound;

    if (!consumeIfTok(tokens, ')'))
        goto ParseInnerExpr_AfterCompound;

    inner->type = InnerExpr_CompoundStatement;
    inner->compoundStmt = malloc(sizeof(compound));
    memcpy(inner->compoundStmt, &compound, sizeof(compound));

    return (ParseRes){ .success = true };

ParseInnerExpr_AfterCompound:
    tokens->pos = pos;

    // Try to parse an assign stmt
    AssignExpr expr = {0};
    if (!parseAssignExpr(tokens, &expr).success)
        goto ParseInnerExpr_AfterAssign;

    inner->type = InnerExpr_Assign;
    inner->assign = expr;

    return (ParseRes){ .success = true };

ParseInnerExpr_AfterAssign:

    return (ParseRes) {
        .success = false,
        .failMessage = "Couldn't parse an inner expr"
    };
}

ParseRes parseExpr(TokenList *tokens, Expr *expr) {
    bool hasComma = false;

    do {
        size_t pos = tokens->pos;

        InnerExpr inner = {0};
        ParseRes res = parseInnerExpr(tokens, &inner);
        if (!res.success) {
            tokens->pos = pos;
            break;
        }

        sll_appendLocal(&(expr->list), inner);

        hasComma = consumeIfTok(tokens, ',');

    } while(hasComma);


    if (expr->list.size == 0) {
        return (ParseRes) {
            .success = false,
            .failMessage = "Expr needs at least one inner expr"
        };
    }

    return (ParseRes){ .success = true };
}

ParseRes parseParameterDeclaration(TokenList *tokens, ParameterDeclaration *decl) {
    DeclarationSpecifierList list = {0};
    ParseRes res = parseDeclarationSpecifierList(tokens, &list);
    if (!res.success)
        return res;

    decl->declarationSpecifiers = malloc(sizeof(list));
    memcpy(decl->declarationSpecifiers, &list, sizeof(list));

    size_t beforeDeclaratorPos = tokens->pos;

    Declarator declarator = {0};
    if (parseDeclarator(tokens, &declarator).success) {
        decl->hasDeclarator = true;
        decl->hasAbstractDeclarator = false;
        decl->declarator = malloc(sizeof(declarator));
        memcpy(decl->declarator, &declarator, sizeof(declarator));
        return (ParseRes){ .success = true };
    }

    tokens->pos = beforeDeclaratorPos;

    // If no declarator, try to parse an abstract declarator

    size_t beforeAbstractDeclaratorPos = tokens->pos;

    AbstractDeclarator abstractDeclarator = {0};
    if (parseAbstractDeclarator(tokens, &abstractDeclarator).success) {
        decl->hasAbstractDeclarator = true;
        decl->hasDeclarator = false;
        decl->abstractDeclarator = malloc(sizeof(abstractDeclarator));
        memcpy(decl->abstractDeclarator, &abstractDeclarator, sizeof(abstractDeclarator));
        return (ParseRes){ .success = true };
    }

    tokens->pos = beforeAbstractDeclaratorPos;

    return (ParseRes){ .success = true };
}

ParseRes parseParameterTypeList(TokenList *tokens, ParameterTypeList *list) {
    bool hasComma = false;
    do {
        // Look for an ellipsis and break
        if (consumeIfTok(tokens, Token_Ellipsis)) {
            list->hasEndingEllipsis = true;
            break;
        }

        ParameterDeclaration decl = {0};
        ParseRes paramRes = parseParameterDeclaration(tokens, &decl);
        if (!paramRes.success) {
            return (ParseRes) {
                .success = false,
                .failMessage = "Expected parameter declaration in type list"
            };
        }

        sll_appendLocal(&(list->paramDecls), decl);

        hasComma = consumeIfTok(tokens, ',');

    } while(hasComma);

    return (ParseRes){ .success = true };
}

ParseRes parsePostDirectAbstractDeclarator(TokenList *tokens,
    PostDirectAbstractDeclarator *postDeclarator)
{
    // Try to parse bracket version
    size_t bracketPos = tokens->pos;

    if (!consumeIfTok(tokens, '['))
        goto PostDirectAbstractDeclarator_Paren;

    // 3 cases:

    // 1: empty
    if (consumeIfTok(tokens, ']')) {
        postDeclarator->type = PostDirectAbstractDeclarator_Bracket;
        postDeclarator->bracketIsEmpty = true;
        return (ParseRes){ .success = true };
    }

    // 2: has a star
    if (peekAheadTok(tokens, 0).type == '*' &&
        peekAheadTok(tokens, 1).type == ']')
    {
        consumeMulti(tokens, 2);
        postDeclarator->type = PostDirectAbstractDeclarator_Bracket;
        postDeclarator->bracketIsStar = true;
        return (ParseRes){ .success = true };
    }

    // 3: has other stuff
    {
        if (consumeIfTok(tokens, Token_static))
            postDeclarator->bracketHasInitialStatic = true;

        // parse opt type qualifier list
        ParseRes res = {0};
        do {
            size_t typeQualifierPos = tokens->pos;

            TypeQualifier typeQualifier = {0};
            res = parseTypeQualifier(tokens, &typeQualifier);
            if (!res.success) {
                tokens->pos = typeQualifierPos;
                break;
            }

            sll_appendLocal(&(postDeclarator->bracketTypeQualifiers), typeQualifier);
        } while (res.success);

        // Look for middle static
        if (consumeIfTok(tokens, Token_static))
            postDeclarator->bracketHasMiddleStatic = true;

        // Look for optional assignment expression
        size_t assignPos = tokens->pos;

        AssignExpr assignExpr = {0};
        ParseRes assignRes = parseAssignExpr(tokens, &assignExpr);
        if (assignRes.success) {
            postDeclarator->bracketHasAssignmentExpr = true;
            postDeclarator->bracketAssignExpr = assignExpr;
        }
        else {
            postDeclarator->bracketHasAssignmentExpr = false;
            tokens->pos = assignPos;
        }

        // Verify it actually worked
        {
            // Cant have 2 statics
            if (postDeclarator->bracketHasInitialStatic &&
                postDeclarator->bracketHasMiddleStatic)
            {
                return (ParseRes) {
                    .success = false,
                    .failMessage = "Direct abstract declarator can't have 2 statics in brackets"
                };
            }
            if (postDeclarator->bracketTypeQualifiers.size == 0 &&
                !postDeclarator->bracketHasAssignmentExpr)
            {
                return (ParseRes) {
                    .success = false,
                    .failMessage = "Direct abstract declarator must have at least a type qualifier list or assignment expr"
                };
            }
        }

        return (ParseRes) { .success = true };
    }


PostDirectAbstractDeclarator_Paren:
    memset(postDeclarator, 0, sizeof(PostDirectAbstractDeclarator));
    tokens->pos = bracketPos;

    // Try to parse paren version
    if (!consumeIfTok(tokens, '(')) {
        return (ParseRes) {
            .success = false,
            .failMessage = "Expected ( for direct abstract declarator"
        };
    }

    if (consumeIfTok(tokens, ')')) {
        // Empty parens
        postDeclarator->type = PostDirectAbstractDeclarator_Paren;
        postDeclarator->parenIsEmpty = true;

        return (ParseRes){ .success = true };
    }

    ParameterTypeList paramList = {0};
    ParseRes res = parseParameterTypeList(tokens, &paramList);
    if (!res.success)
        return res;

    if (!consumeIfTok(tokens, ')')) {
        return (ParseRes) {
            .success = false,
            .failMessage = "Expected ) after param list for direct abstract declarator"
        };
    }

    postDeclarator->type = PostDirectAbstractDeclarator_Paren;
    postDeclarator->parenParamList = paramList;

    return (ParseRes){ .success = true };
}

ParseRes parseDirectAbstractDeclarator(TokenList *tokens,
    DirectAbstractDeclarator *directDeclarator)
{
    // Parse opt abstract declarator in parens
    size_t preAbstractDeclaratorPos = tokens->pos;

    if (!consumeIfTok(tokens, '('))
        goto PostAbstractDeclaratorFail;

    AbstractDeclarator abstractDeclarator = {0};
    ParseRes abstractRes = parseAbstractDeclarator(tokens, &abstractDeclarator);
    if (!abstractRes.success) {
        goto PostAbstractDeclaratorFail;
    }

    if (!consumeIfTok(tokens, ')'))
        goto PostAbstractDeclaratorFail;

    // If we get here, then we succeeded
    directDeclarator->hasAbstractDeclarator = true;
    directDeclarator->abstractDeclarator = malloc(sizeof(abstractDeclarator));
    memcpy(directDeclarator->abstractDeclarator, &abstractDeclarator, sizeof(abstractDeclarator));

    goto PostAbstractDeclaratorDone;

PostAbstractDeclaratorFail:
    directDeclarator->hasAbstractDeclarator = false;
    tokens->pos = preAbstractDeclaratorPos;

PostAbstractDeclaratorDone:
    ; // We need this because the label needs to be before a statement
    // Parse a list of post direct abstract declarators (size can be zero)
    ParseRes res = {0};
    do {
        // parse opt post declarator
        size_t pos = tokens->pos;

        PostDirectAbstractDeclarator postDeclarator = {0};
        res = parsePostDirectAbstractDeclarator(tokens, &postDeclarator);
        if (!res.success) {
            tokens->pos = pos;
            break;
        }

        sll_appendLocal(&(directDeclarator->postDirectAbstractDeclarators),
            postDeclarator);
    } while (res.success);

    if (!directDeclarator->hasAbstractDeclarator &&
        directDeclarator->postDirectAbstractDeclarators.size == 0)
    {
        return (ParseRes) {
            .success = false,
            .failMessage = "Couldn't parse a direct abstract declarator"
        };
    }

    return (ParseRes){ .success = true };
}

ParseRes parsePointer(TokenList *tokens, Pointer *pointer) {
    if (peekTok(tokens).type != '*') {
        return (ParseRes) {
            .success = false,
            .failMessage = "Pointer didn't start with *"
        };
    }

    while (peekTok(tokens).type == '*') {
        consumeTok(tokens);
        pointer->numPtrs++;
    }

    // Parse opt type qualifier list
    ParseRes typeQualifierRes = {0};
    do {
        size_t pos = tokens->pos;

        TypeQualifier typeQualifier = {0};
        typeQualifierRes = parseTypeQualifier(tokens, &typeQualifier);
        if (!typeQualifierRes.success) {
            tokens->pos = pos;
            break;
        }

        sll_appendLocal(&(pointer->typeQualifiers), typeQualifier);
    } while (typeQualifierRes.success);

    // If no type qualifiers, cannot have trailing pointer
    if (pointer->typeQualifiers.size == 0) {
        pointer->hasPtr = false;
        return (ParseRes){ .success = true };
    }

    // Parse opt pointer
    size_t pos = tokens->pos;

    Pointer trailingPointer = {0};
    ParseRes pointerRes = parsePointer(tokens, &trailingPointer);
    if (pointerRes.success) {
        pointer->hasPtr = true;
        pointer->pointer = malloc(sizeof(Pointer));
        memcpy(pointer->pointer, &trailingPointer, sizeof(Pointer));
    }
    else {
        pointer->hasPtr = false;
        tokens->pos = pos;
    }

    return (ParseRes){ .success = true };
}

ParseRes parseAbstractDeclarator(TokenList *tokens,
    AbstractDeclarator *abstractDeclarator)
{
    // Parse opt pointer
    size_t pointerPos = tokens->pos;

    Pointer pointer = {0};
    ParseRes pointerRes = parsePointer(tokens, &pointer);
    if (pointerRes.success) {
        abstractDeclarator->hasPointer = true;
        abstractDeclarator->pointer = pointer;
    }
    else {
        abstractDeclarator->hasPointer = false;
        tokens->pos = pointerPos;
    }

    // Parse opt direct abstract declarator
    size_t directPos = tokens->pos;

    DirectAbstractDeclarator directDeclarator = {0};
    ParseRes directRes = parseDirectAbstractDeclarator(tokens, &directDeclarator);
    if (directRes.success) {
        abstractDeclarator->hasDirectAbstractDeclarator = true;
        abstractDeclarator->directAbstractDeclarator = directDeclarator;
    }
    else {
        abstractDeclarator->hasDirectAbstractDeclarator = false;
        tokens->pos = directPos;
    }

    if (!abstractDeclarator->hasPointer &&
        !abstractDeclarator->hasDirectAbstractDeclarator)
    {
        // If we don't have either, we failed
        return (ParseRes) {
            .success = false,
            .failMessage = "Couldn't parse a pointer or direct "
                "abstract declarator in an abstract declarator"
        };
    }

    return (ParseRes) { .success = true };
}

ParseRes parseIdentifierList(TokenList *tokens, IdentifierList *list) {
    bool hasComma = false;
    do {
        if (peekTok(tokens).type != Token_Ident) {
            break;
        }

        Token tok = consumeTok(tokens);
        sll_appendLocal(&(list->list), tok.ident);

        hasComma = consumeIfTok(tokens, Token_Ident);

    } while (hasComma);

    return (ParseRes){ .success = true };
}

ParseRes parsePostDirectDeclarator(TokenList *tokens, PostDirectDeclarator *postDeclarator) {
    // Parse Bracket
    if (consumeIfTok(tokens, '[')) {
        // 3 Cases:
        postDeclarator->type = PostDirectDeclarator_Bracket;

        // Case 1: is empty
        if (consumeIfTok(tokens, ']')) {
            postDeclarator->bracketIsEmpty = true;
            return (ParseRes){ .success = true };
        }

        // Case 2: has a star
        if (peekAheadTok(tokens, 0).type == '*' &&
            peekAheadTok(tokens, 1).type == ']')
        {
            consumeTok(tokens);
            consumeTok(tokens);
            postDeclarator->bracketIsStar = true;
            return (ParseRes){ .success = true };
        }

        // Case 3: has other stuff in the middle
        {
            // check for opt initial static
            if (consumeIfTok(tokens, Token_static)) {
                postDeclarator->bracketHasInitialStatic = true;
            }

            // check for opt type qualifier list
            ParseRes res = {0};
            do {
                size_t typeQualifierPos = tokens->pos;

                TypeQualifier typeQualifier = {0};
                res = parseTypeQualifier(tokens, &typeQualifier);
                if (!res.success) {
                    tokens->pos = typeQualifierPos;
                    break;
                }

                sll_appendLocal(&(postDeclarator->bracketTypeQualifiers),
                    typeQualifier);
            } while (res.success);

            // Case 3a: early terminate if:
                // no initial static
                // typequalifier len != 0
                // star after
                // ] after star
            if (!postDeclarator->bracketHasInitialStatic &&
                postDeclarator->bracketTypeQualifiers.size > 0 &&
                peekAheadTok(tokens, 0).type == '*' &&
                peekAheadTok(tokens, 1).type == ']')
            {
                consumeTok(tokens);
                consumeTok(tokens);

                postDeclarator->bracketHasStarAfterTypeQualifiers = true;
                return (ParseRes){ .success = true };
            }

            // Check for optional middle static
            if (consumeIfTok(tokens, Token_static)) {
                postDeclarator->bracketHasMiddleStatic = true;
            }

            // Check for optional assign expr
            size_t preAssignPos = tokens->pos;
            AssignExpr expr = {0};
            if (parseAssignExpr(tokens, &expr).success) {
                postDeclarator->bracketHasAssignExpr = true;
                postDeclarator->bracketAssignExpr = expr;
            }
            else {
                tokens->pos = preAssignPos;
            }

            if (!consumeIfTok(tokens, ']')) {
                return (ParseRes) {
                    .success = false,
                    .failMessage = "Expected ] after expr in pos direct declarator"
                };
            }

            // Validate input
            {
                // cannot have 2 statics
                if (postDeclarator->bracketHasInitialStatic &&
                    postDeclarator->bracketHasMiddleStatic)
                {
                    return (ParseRes) {
                        .success = false,
                        .failMessage = "Post direct declarator cannot have 2 statics"
                    };
                }

                // must have at least one of type qualifiers or assignment expr
                if (postDeclarator->bracketTypeQualifiers.size == 0 &&
                    !postDeclarator->bracketHasAssignExpr)
                {
                    return (ParseRes) {
                        .success = false,
                        .failMessage = "Post direct declarator must have at least a type qualifier or assignment expr"
                    };
                }
            }

            return (ParseRes){ .success = true };
        }
    }

    // Parse Paren
    if (consumeIfTok(tokens, '(')) {

        postDeclarator->type = PostDirectDeclarator_Paren;

        // Empty paren early terminate
        if (consumeIfTok(tokens, ')')) {
            postDeclarator->parenType = PostDirectDeclaratorParen_Empty;
            return (ParseRes){ .success = true };
        }

        size_t pos = tokens->pos;

        // Try to parse a parameter type list
        {
            ParameterTypeList paramList = {0};
            ParseRes paramRes = parseParameterTypeList(tokens, &paramList);
            if (!paramRes.success)
                goto ParsePostDirectDeclarator_PostParenParamList;

            postDeclarator->parenType = PostDirectDeclaratorParen_ParamTypelist;
            postDeclarator->parenParamTypeList = paramList;

            if (!consumeIfTok(tokens, ')'))
                goto ParsePostDirectDeclarator_PostParenParamList;

            return (ParseRes){ .success = true };
        }

ParsePostDirectDeclarator_PostParenParamList:

        tokens->pos = pos;

        // Parse an identifier list
        {
            IdentifierList identList = {0};
            ParseRes identRes = parseIdentifierList(tokens, &identList);
            if (!identRes.success) {
                return (ParseRes) {
                    .success = false,
                    .failMessage = "Expected identifier list after ( in direct declarator"
                };
            }

            postDeclarator->parenType = PostDirectDeclaratorParen_IdentList;
            postDeclarator->parenIdentList = identList;

            if (!consumeIfTok(tokens, ')')) {
                return (ParseRes) {
                    .success = false,
                    .failMessage = "Expected ) after identifier list"
                };
            }

            return (ParseRes){ .success = true };

        }

        return (ParseRes) {
            .success = false,
            .failMessage = "Expected either a parameter list or identifier list after ( in post direct declarator"
        };
    }

    return (ParseRes) {
        .success = false,
        .failMessage = "Expected either ( or [ in post direct declarator"
    };
}

ParseRes parseDirectDeclarator(TokenList *tokens, DirectDeclarator *directDeclarator) {
    // Parse base
    if (peekTok(tokens).type == Token_Ident) {
        Token ident = consumeTok(tokens);
        directDeclarator->type = DirectDeclarator_Ident;
        directDeclarator->ident = ident.ident;
    }
    else if (consumeIfTok(tokens, '(')) {
        Declarator nestedDeclarator = {0};
        ParseRes nestedDeclRes = parseDeclarator(tokens, &nestedDeclarator);
        if (!nestedDeclRes.success)
            return nestedDeclRes;

        if (!consumeIfTok(tokens, ')')) {
            return (ParseRes) {
                .success = false,
                .failMessage = "Expected ) after direct declarator's nested declarator"
            };
        }

        directDeclarator->type = DirectDeclarator_ParenDeclarator;
        directDeclarator->declarator = malloc(sizeof(nestedDeclarator));
        memcpy(directDeclarator->declarator, &nestedDeclarator, sizeof(nestedDeclarator));
    }
    else {
        return (ParseRes) {
            .success = false,
            .failMessage = "Expected ( or ident for direct declarator"
        };
    }

    // Parse optional list of post direct declarators
    ParseRes postDirectRes = {0};
    do {
        size_t pos = tokens->pos;

        PostDirectDeclarator postDeclarator = {0};
        postDirectRes = parsePostDirectDeclarator(tokens, &postDeclarator);
        if (!postDirectRes.success) {
            tokens->pos = pos;
            break;
        }

        sll_appendLocal(&(directDeclarator->postDirectDeclarators),
            postDeclarator);
    } while(postDirectRes.success);

    return (ParseRes) { .success = true };
}

ParseRes parseDeclarator(TokenList *tokens, Declarator *declarator) {
    // Try parse pointer
    size_t prePointerPos = tokens->pos;

    Pointer pointer = {0};
    if (parsePointer(tokens, &pointer).success) {
        declarator->hasPointer = true;
        declarator->pointer = pointer;
    }
    else {
        tokens->pos = prePointerPos;
        declarator->hasPointer = false;
    }

    // Parse direct declarator
    DirectDeclarator direct = {0};
    ParseRes res = parseDirectDeclarator(tokens, &direct);
    if (!res.success) {
        return res;
    }

    declarator->directDeclarator = direct;
    return (ParseRes){ .success = true };
}

ParseRes parseTypeQualifier(TokenList *tokens, TypeQualifier *outQualifier) {
    ParseRes pass = { .success = true };
    ParseRes fail = {
        .success = false,
        .failMessage = "Couldn't parse type qualifier"
    };

    if (consumeIfTok(tokens, Token_const)) {
        *outQualifier = Qualifier_Const;
        return pass;
    }
    if (consumeIfTok(tokens, Token_restrict)) {
        *outQualifier = Qualifier_Restrict;
        return pass;
    }
    if (consumeIfTok(tokens, Token_volatile)) {
        *outQualifier = Qualifier_Volatile;
        return pass;
    }
    if (consumeIfTok(tokens, Token_atomic)) {
        *outQualifier = Qualifier_Atomic;
        return pass;
    }

    return fail;
}

ParseRes parseSpecifierQualifier(TokenList *tokens,
    SpecifierQualifier *outSpecifierQualifier)
{
    // Parse opt specifier
    size_t pos = tokens->pos;

    TypeSpecifier specifier = {0};
    ParseRes specifierRes = parseTypeSpecifier(tokens, &specifier);
    if (specifierRes.success) {
        outSpecifierQualifier->type = SpecifierQualifier_Specifier;
        outSpecifierQualifier->typeSpecifier = malloc(sizeof(specifier));
        memcpy(outSpecifierQualifier->typeSpecifier, &specifier, sizeof(specifier));
        return (ParseRes){ .success = true };
    }

    tokens->pos = pos;
    // Parse qualifier
    TypeQualifier qualifier = {0};
    ParseRes qualifierRes = parseTypeQualifier(tokens, &qualifier);
    if (qualifierRes.success) {
        outSpecifierQualifier->type = SpecifierQualifier_Qualifier;
        outSpecifierQualifier->typeQualifier = qualifier;
        return (ParseRes){ .success = true };
    }

    // If we're here, we didnt find either
    return (ParseRes) {
        .success = false,
        .failMessage = "Couldn't parse specifier-qualifier"
    };
}

ParseRes parseSpecifierQualifierList(TokenList *tokens,
    SpecifierQualifierList *outList)
{
    // We need at least one, so fail if this doesn't work
    SpecifierQualifier specifierQualifier = {0};
    ParseRes res = parseSpecifierQualifier(tokens, &specifierQualifier);
    if (!res.success) {
        return res;
    }

    do {
        sll_appendLocal(&(outList->list), specifierQualifier);
        res = parseSpecifierQualifier(tokens, &specifierQualifier);
    } while (res.success);

    return (ParseRes) { .success = true };
}

ParseRes parseTypeName(TokenList *tokens, TypeName *outType) {
    // Parse Specifier Qualifier List
    SpecifierQualifierList list = {0};
    ParseRes res = parseSpecifierQualifierList(tokens, &list);
    if (!res.success) {
        return res;
    }

    outType->specifierQualifiers = list;

    // Parse Opt abstract declarator
    size_t pos = tokens->pos;

    AbstractDeclarator abstractDeclarator = {0};
    ParseRes abstractRes = parseAbstractDeclarator(tokens, &abstractDeclarator);
    if (abstractRes.success) {
        outType->hasAbstractDeclarator = true;
        outType->abstractDeclarator = abstractDeclarator;
        return (ParseRes){ .success = true };
    }

    // No abstract declarator, so reset tokens
    tokens->pos = pos;

    outType->hasAbstractDeclarator = false;

    return (ParseRes){ .success = true };
}

ParseRes parseStorageClassSpecifier(TokenList *tokens,
    StorageClassSpecifier *outSpec)
{
    ParseRes pass = { .success = true };
    ParseRes fail = {
        .success = false,
        .failMessage = "Couldn't parse storage class specifier"
    };

    if (consumeIfTok(tokens, Token_typedef)) {
        *outSpec = StorageClass_Typedef;
        return pass;
    }
    else if (consumeIfTok(tokens, Token_extern)) {
        *outSpec = StorageClass_Extern;
        return pass;
    }
    else if (consumeIfTok(tokens, Token_static)) {
        *outSpec = StorageClass_Static;
        return pass;
    }
    else if (consumeIfTok(tokens, Token_threadLocal)) {
        *outSpec = StorageClass_Thread_Local;
        return pass;
    }
    else if (consumeIfTok(tokens, Token_auto)) {
        *outSpec = StorageClass_Auto;
        return pass;
    }
    else if (consumeIfTok(tokens, Token_register)) {
        *outSpec = StorageClass_Register;
        return pass;
    }

    return fail;
}

ParseRes parseStaticAssertDeclaration(TokenList *tokens, StaticAssertDeclaration *decl) {
    if (!consumeIfTok(tokens, Token_staticAssert)) {
        return (ParseRes) {
            .success = false,
            .failMessage = "Expected static assert at front of static assert declaration"
        };
    }

    if (!consumeIfTok(tokens, '(')) {
        return (ParseRes) {
            .success = false,
            .failMessage = "Expected ( after static assert declaration"
        };
    }

    ConditionalExpr constant = {0};
    ParseRes constantRes = parseConditionalExpr(tokens, &constant);
    if (!constantRes.success)
        return constantRes;

    if (!consumeIfTok(tokens, ',')) {
        return (ParseRes) {
            .success = false,
            .failMessage = "Expected , after constant in static assert declaration"
        };
    }

    if (peekTok(tokens).type != Token_ConstString) {
        return (ParseRes) {
            .success = false,
            .failMessage = "Expected string after , in static assert declaration"
        };
    }

    // FIXME: Handle this in the lexer
    while (peekTok(tokens).type == Token_ConstString)
        consumeTok(tokens);

    Token string = consumeTok(tokens);

    if (!consumeIfTok(tokens, ')')) {
        return (ParseRes) {
            .success = false,
            .failMessage = "Expected ) after string in static assert declaration"
        };
    }

    if (!consumeIfTok(tokens, ';')) {
        return (ParseRes) {
            .success = false,
            .failMessage = "Expected ; after ) in static assert declaration"
        };
    }

    decl->constantExpr = constant;
    decl->stringLiteral = string.constString;

    return (ParseRes) { .success = true };
}

ParseRes parseStructDeclarator(TokenList *tokens, StructDeclarator *decl) {
    size_t posBefore = tokens->pos;

    Declarator declarator = {0};
    ParseRes declaratorRes = parseDeclarator(tokens, &declarator);
    if (declaratorRes.success) {
        decl->hasDeclarator = true;
        decl->declarator = declarator;
    }
    else {
        tokens->pos = posBefore;
    }

    if (consumeIfTok(tokens, ':')) {
        // Now there has to be a constant expr or error
        ConditionalExpr expr = {0};
        ParseRes exprRes = parseConditionalExpr(tokens, &expr);
        if (!exprRes.success)
            return exprRes;

        decl->hasConstExpr = true;
        decl->constExpr = expr;
    }

    return (ParseRes){ .success = true };
}

ParseRes parseStructDeclaratorList(TokenList *tokens, StructDeclaratorList *declList) {
    bool hasComma = false;
    do {
        StructDeclarator decl = {0};
        ParseRes res = parseStructDeclarator(tokens, &decl);
        if (!res.success)
            return res;

        sll_appendLocal(&(declList->list), decl);

        hasComma = consumeIfTok(tokens, ',');
    } while(hasComma);

    return (ParseRes){ .success = true };
}

ParseRes parseStructDeclaration(TokenList *tokens, StructDeclaration *declaration) {
    // Static assert declaration
    if (peekTok(tokens).type == Token_staticAssert) {
        StaticAssertDeclaration staticAssert = {0};
        ParseRes staticRes = parseStaticAssertDeclaration(tokens, &staticAssert);
        if (!staticRes.success)
            return staticRes;

        declaration->type = StructDeclaration_StaticAssert;
        declaration->staticAssert = staticAssert;

        return (ParseRes){ .success = true };
    }

    // Normal declaration
    // Parse a specifier qualifier list
    SpecifierQualifierList list = {0};
    ParseRes listRes = parseSpecifierQualifierList(tokens, &list);
    if (!listRes.success)
        return listRes;

    declaration->type = StructDeclaration_Normal;
    declaration->normalSpecifierQualifiers = list;

    // Parse an opt struct declarator list
    size_t preDeclaratorPos = tokens->pos;

    StructDeclaratorList declList = {0};
    ParseRes declRes = parseStructDeclaratorList(tokens, &declList);
    if (declRes.success) {
        declaration->normalHasStructDeclaratorList = true;
        declaration->normalStructDeclaratorList = declList;
    }
    else {
        tokens->pos = preDeclaratorPos;
    }

    // Parse a ;
    if (!consumeIfTok(tokens, ';')) {
        return (ParseRes) {
            .success = false,
            .failMessage = "Expected a ; after a normal struct declaration"
        };
    }

    return (ParseRes) { .success = true };
}

ParseRes parseStructOrUnionSpecifier(TokenList *tokens, StructOrUnionSpecifier *structOrUnion) {
    if (consumeIfTok(tokens, Token_struct)) {
        structOrUnion->structOrUnion = StructOrUnion_Struct;
    }
    else if (consumeIfTok(tokens, Token_union)) {
        structOrUnion->structOrUnion = StructOrUnion_Union;
    }
    else {
        return (ParseRes) {
            .success = false,
            .failMessage = "Expected struct or union keywords for struct or union specifier"
        };
    }

    if (peekTok(tokens).type == Token_Ident) {
        Token tok = consumeTok(tokens);
        structOrUnion->hasIdent = true;
        structOrUnion->ident = tok.ident;
    }

    if (consumeIfTok(tokens, '{')) {
        while (peekTok(tokens).type != '}') {
            StructDeclaration decl = {0};
            ParseRes declRes = parseStructDeclaration(tokens, &decl);
            if (!declRes.success)
                return declRes;

            sll_appendLocal(&(structOrUnion->structDeclarations), decl);
        }

        if (!consumeIfTok(tokens, '}')) {
            return (ParseRes) {
                .success = false,
                .failMessage = "Expected } after struct declaration list"
            };
        }

        return (ParseRes){ .success = true };
    }

    return (ParseRes){ .success = true };
}

ParseRes parseEnumerator(TokenList *tokens, Enumerator *enumerator) {
    if (peekTok(tokens).type != Token_Ident) {
        return (ParseRes) {
            .success = false,
            .failMessage = "Expected identifier when parsing an enumerator"
        };
    }

    // FIXME: Flag enumeration constants as such
    Token tok = consumeTok(tokens);
    enumerator->constantIdent = tok.ident;

    if (consumeIfTok(tokens, '=')) {
        ConditionalExpr constant = {0};
        ParseRes constantRes = parseConditionalExpr(tokens, &constant);
        if (!constantRes.success)
            return constantRes;

        enumerator->hasConstExpr = true;
        enumerator->constantExpr = constant;
    }

    return (ParseRes){ .success = true };
}

ParseRes parseEnumeratorList(TokenList *tokens, EnumeratorList *list) {
    bool foundEndBlock = false;
    bool hasComma = false;
    do {

        Enumerator enumerator = {0};
        ParseRes enumRes = parseEnumerator(tokens, &enumerator);
        if (!enumRes.success)
            return enumRes;

        hasComma = consumeIfTok(tokens, ',');

        sll_appendLocal(&(list->list), enumerator);

        foundEndBlock = peekTok(tokens).type == '}';

    } while(hasComma && !foundEndBlock);

    return (ParseRes){ .success = true };
}

ParseRes parseEnumSpecifier(TokenList *tokens, EnumSpecifier *enumSpecifier) {
    if (!consumeIfTok(tokens, Token_enum)) {
        return (ParseRes) {
            .success = false,
            .failMessage = "Expected enum token to start enum specifier"
        };
    }

    if (peekTok(tokens).type == Token_Ident) {
        Token tok = consumeTok(tokens);
        enumSpecifier->hasIdent = true;
        enumSpecifier->ident = tok.ident;
    }

    if (consumeIfTok(tokens, '{')) {
        EnumeratorList enumList = {0};
        ParseRes listRes = parseEnumeratorList(tokens, &enumList);
        if (!listRes.success)
            return listRes;

        if (!consumeIfTok(tokens, '}')) {
            return (ParseRes) {
                .success = false,
                .failMessage = "Expected } after parsing enumerator list"
            };
        }

        enumSpecifier->hasEnumeratorList = true;
        enumSpecifier->enumeratorList = enumList;
    }

    return (ParseRes){ .success = true };
}

ParseRes parseTypeSpecifier(TokenList *tokens, TypeSpecifier *type) {
    ParseRes pass = { .success = true };
    ParseRes fail = {
        .success = false,
        .failMessage = "Couldn't parse type specifier"
    };

    if (consumeIfTok(tokens, Token_void)) {
        type->type = TypeSpecifier_Void;
        return pass;
    }
    if (consumeIfTok(tokens, Token_char)) {
        type->type = TypeSpecifier_Char;
        return pass;
    }
    if (consumeIfTok(tokens, Token_short)) {
        type->type = TypeSpecifier_Short;
        return pass;
    }
    if (consumeIfTok(tokens, Token_int)) {
        type->type = TypeSpecifier_Int;
        return pass;
    }
    if (consumeIfTok(tokens, Token_long)) {
        type->type = TypeSpecifier_Long;
        return pass;
    }
    if (consumeIfTok(tokens, Token_float)) {
        type->type = TypeSpecifier_Float;
        return pass;
    }
    if (consumeIfTok(tokens, Token_double)) {
        type->type = TypeSpecifier_Double;
        return pass;
    }
    if (consumeIfTok(tokens, Token_signed)) {
        type->type = TypeSpecifier_Signed;
        return pass;
    }
    if (consumeIfTok(tokens, Token_unsigned)) {
        type->type = TypeSpecifier_Unsigned;
        return pass;
    }
    if (consumeIfTok(tokens, Token_bool)) {
        type->type = TypeSpecifier_Bool;
        return pass;
    }
    if (consumeIfTok(tokens, Token_complex)) {
        type->type = TypeSpecifier_Complex;
        return pass;
    }
    if (consumeIfTok(tokens, Token_imaginary)) {
        type->type = TypeSpecifier_Imaginary;
        return pass;
    }

    // Parse atomic type specifier
    size_t pos = tokens->pos;
    {
        if (!consumeIfTok(tokens, Token_atomic))
            goto PostAtomicType;

        if (!consumeIfTok(tokens, '('))
            goto PostAtomicType;

        TypeName atomicTypeName = {0};
        if (!parseTypeName(tokens, &atomicTypeName).success)
            goto PostAtomicType;

        if (!consumeIfTok(tokens, ')'))
            goto PostAtomicType;

        // If we're here, then atomic type specifier succeeded
        type->type = TypeSpecifier_AtomicType;
        type->atomicName = atomicTypeName;
        return pass;
    }

PostAtomicType:
    tokens->pos = pos;

    // Parse struct or union specifier
    StructOrUnionSpecifier structOrUnion = {0};
    ParseRes structOrUnionRes = parseStructOrUnionSpecifier(tokens, &structOrUnion);
    if (structOrUnionRes.success) {
        type->type = TypeSpecifier_StructOrUnion;
        type->structOrUnion = structOrUnion;
        return pass;
    }

    tokens->pos = pos;

    // Parse enum specifier
    EnumSpecifier enumSpecifier = {0};
    ParseRes enumRes = parseEnumSpecifier(tokens, &enumSpecifier);
    if (enumRes.success) {
        type->type = TypeSpecifier_Enum;
        type->enumSpecifier = enumSpecifier;
        return pass;
    }

    tokens->pos = pos;

    // Parse typedef name
    if (peekTok(tokens).type == Token_Ident) {
        Token tok = consumeTok(tokens);
        if (typedefTable_find(g_typedefTable, tok.ident)) {
            type->type = TypeSpecifier_TypedefName;
            type->typedefName = tok.ident;
            return pass;
        }
    }

    tokens->pos = pos;

    return fail;
}

ParseRes parseFunctionSpecifier(TokenList *tokens, FunctionSpecifier *funcSpecifier) {
    if (consumeIfTok(tokens, Token_inline)) {
        *funcSpecifier = FunctionSpecifier_Inline;
        return (ParseRes){ .success = true };
    }
    if (consumeIfTok(tokens, Token_noreturn)) {
        *funcSpecifier = FunctionSpecifier_Noreturn;
        return (ParseRes){ .success = true };
    }

    return (ParseRes) {
        .success = false,
        .failMessage = "Expected inline or noreturn for function specifier"
    };
}

ParseRes parseAlignmentSpecifier(TokenList *tokens, AlignmentSpecifier *alignment) {
    if (!consumeIfTok(tokens, Token_alignas)) {
        return (ParseRes) {
            .success = false,
            .failMessage = "Alignment specifier expected an alignas keyword"
        };
    }

    if (!consumeIfTok(tokens, '(')) {
        return (ParseRes) {
            .success = false,
            .failMessage = "Alignment specifier requires a ( after alignas"
        };
    }

    size_t pos = tokens->pos;

    TypeName typeName = {0};
    if (parseTypeName(tokens, &typeName).success) {
        if (!consumeIfTok(tokens, ')')) {
            return (ParseRes) {
                .success = false,
                .failMessage = "Alignment specifier requires a ) after type name"
            };
        }

        alignment->type = AlignmentSpecifier_TypeName;
        alignment->typeName = typeName;
        return (ParseRes){ .success = true };
    }

    tokens->pos = pos;

    ConditionalExpr expr = {0};
    if (parseConditionalExpr(tokens, &expr).success) {
        if (!consumeIfTok(tokens, ')')) {
            return (ParseRes) {
                .success = false,
                .failMessage = "Alignment specifier requires a ) after constant expr"
            };
        }

        alignment->type = AlignmentSpecifier_Constant;
        alignment->constant = expr;
    }

    return (ParseRes){
        .success = false,
        .failMessage = "Alignment specifier need a constant expr or type name"
    };
}

ParseRes parseDeclarationSpecifier(TokenList *tokens,
    DeclarationSpecifier *outSpec)
{
    size_t pos = tokens->pos;

    // Try parse storage class specifier
    StorageClassSpecifier storageClass = {0};
    ParseRes storageClassRes = parseStorageClassSpecifier(tokens, &storageClass);
    if (storageClassRes.success) {
        outSpec->type = DeclarationSpecifier_StorageClass;
        outSpec->storageClass = storageClass;
        return (ParseRes){ .success = true };
    }

    tokens->pos = pos;

    // Try parse type specifier
    TypeSpecifier type = {0};
    ParseRes typeRes = parseTypeSpecifier(tokens, &type);
    if (typeRes.success) {
        outSpec->type = DeclarationSpecifier_Type;
        outSpec->typeSpecifier = type;
        return (ParseRes){ .success = true };
    }

    tokens->pos = pos;

    // Try parse type qualifier
    TypeQualifier qualifier = {0};
    ParseRes qualifierRes = parseTypeQualifier(tokens, &qualifier);
    if (qualifierRes.success) {
        outSpec->type = DeclarationSpecifier_TypeQualifier;
        outSpec->typeQualifier = qualifier;
        return (ParseRes){ .success = true };
    }

    tokens->pos = pos;

    // Try parse function specifier
    FunctionSpecifier funcSpecifier = {0};
    ParseRes funcSpecRes = parseFunctionSpecifier(tokens, &funcSpecifier);
    if (funcSpecRes.success) {
        outSpec->type = DeclarationSpecifier_Func;
        outSpec->function = funcSpecifier;
        return (ParseRes){ .success = true };
    }

    tokens->pos = pos;

    // Try parse alignment specifier
    AlignmentSpecifier alignSpec = {0};
    ParseRes alignRes = parseAlignmentSpecifier(tokens, &alignSpec);
    if (alignRes.success) {
        outSpec->type = DeclarationSpecifier_Alignment;
        outSpec->alignment = alignSpec;
        return (ParseRes){ .success = true };
    }

    return (ParseRes) {
        .success = false,
        .failMessage = "Couldn't parse declaration specifier"
    };
}

ParseRes parseDeclarationSpecifierList(TokenList *tokens,
    DeclarationSpecifierList *outList)
{
    // We need at least one, so fail if this doesn't work
    DeclarationSpecifier specifier = {0};
    ParseRes res = parseDeclarationSpecifier(tokens, &specifier);
    if (!res.success) {
        return res;
    }

    do {
        sll_appendLocal(&(outList->list), specifier);
        res = parseDeclarationSpecifier(tokens, &specifier);
    } while (res.success);

    return (ParseRes) { .success = true };
}

ParseRes parseInitDeclarator(TokenList *tokens, InitDeclarator *initDeclarator) {
    Declarator decl = {0};
    ParseRes declaratorRes = parseDeclarator(tokens, &decl);
    if (!declaratorRes.success)
        return declaratorRes;

    initDeclarator->decl = decl;

    if (consumeIfTok(tokens, '=')) {
        Initializer init = {0};
        ParseRes initRes = parseInitializer(tokens, &init);
        if (!initRes.success)
            return initRes;

        initDeclarator->hasInitializer = true;
        initDeclarator->initializer = init;
    }

    return (ParseRes) { .success = true };
}

ParseRes parseInitDeclaratorList(TokenList *tokens, InitDeclaratorList *initList) {
    bool hasComma = false;
    do {

        size_t pos = tokens->pos;

        // Parse an optional init declarator
        InitDeclarator decl = {0};
        ParseRes declRes = parseInitDeclarator(tokens, &decl);
        if (!declRes.success) {
            tokens->pos = pos;
            break;
        }

        // Parse an optional assembly rename
        // We don't need to save the data in here because this is
        // just for an extension in gcc to rename a symbol in the linker
        if (consumeIfTok(tokens, Token_asm)) {
            // Look for a (
            if (!consumeIfTok(tokens, '(')) {
                return (ParseRes) {
                    .success = false,
                    .failMessage = "Expected ( after asm in declaration rename"
                };
            }

            // Look for String
            if (!consumeIfTok(tokens, Token_ConstString)) {
                return (ParseRes) {
                    .success = false,
                    .failMessage = "Expected String after 'asm (' in declaration rename"
                };
            }

            // FIXME: This probably should be handled in lexer
            // Consume adjacent strings
            while (peekTok(tokens).type == Token_ConstString)
                consumeTok(tokens);

            // Look for )
            if (!consumeIfTok(tokens, ')')) {
                return (ParseRes) {
                    .success = false,
                    .failMessage = "Expected ) after 'asm ( String' in declaration rename"
                };
            }
        }

        sll_appendLocal(&(initList->list), decl);

        // Parse a ,
        hasComma = consumeIfTok(tokens, ',');

    } while (hasComma);

    // Must have at least one
    if (initList->list.size == 0) {
        return (ParseRes) {
            .success = false,
            .failMessage = "Expected init declarators in init declarator list"
        };
    }

    return (ParseRes){ .success = true };
}

String directDeclarator_getName(DirectDeclarator decl) {
    if (decl.type == DirectDeclarator_Ident)
        return decl.ident;
    else
        return directDeclarator_getName(decl.declarator->directDeclarator);
}

ParseRes parseDeclaration(TokenList *tokens, Declaration *outDef) {
    // Try parse a static assert declaration
    if (peekTok(tokens).type == Token_staticAssert) {
        StaticAssertDeclaration staticDecl = {0};
        ParseRes staticRes = parseStaticAssertDeclaration(tokens, &staticDecl);
        if (!staticRes.success)
            return staticRes;

        outDef->type = Declaration_StaticAssert;
        outDef->staticAssert = staticDecl;
        return (ParseRes){ .success = true };
    }

    // Parse declaration specifiers
    DeclarationSpecifierList specifiers = {0};
    ParseRes listRes = parseDeclarationSpecifierList(tokens, &specifiers);
    if (!listRes.success)
        return listRes;

    outDef->type = Declaration_Normal;
    outDef->declSpecifiers = specifiers;

    // Parse optional init declarator list
    size_t pos = tokens->pos;

    InitDeclaratorList initList = {0};
    ParseRes initRes = parseInitDeclaratorList(tokens, &initList);
    if (initRes.success) {
        outDef->hasInitDeclaratorList = true;
        outDef->initDeclaratorList = initList;
    }
    else {
        outDef->hasInitDeclaratorList = false;
        tokens->pos = pos;
    }

    // Parse semicolon
    if (!consumeIfTok(tokens, ';')) {
        return (ParseRes) {
            .success = false,
            .failMessage = "Expected semicolon after declaration"
        };
    }

    // If starts with typedef, look through init declarator list and
    // pull out typedef names
    if (specifiers.list.size == 0)
        goto Post_Typedef_Ident;

    DeclarationSpecifier *declSpec = slNode_getData(specifiers.list.head);
    if (declSpec->type != DeclarationSpecifier_StorageClass)
        goto Post_Typedef_Ident;

    if (declSpec->storageClass != StorageClass_Typedef)
        goto Post_Typedef_Ident;

    // Now that we know we have a typedef, add all the names
    sll_foreach(initList.list, node) {
        InitDeclarator *decl = slNode_getData(node);
        String name = directDeclarator_getName(decl->decl.directDeclarator);
        if (name.length > 0) {
            typedefTable_add(&g_typedefTable, name);
        }
    }

Post_Typedef_Ident:

    return (ParseRes){ .success = true };
}

ParseRes parseLabeledStatement(TokenList *tokens, LabeledStatement *stmt) {
    if (peekTok(tokens).type == Token_Ident) {
        Token tok = consumeTok(tokens);

        if (!consumeIfTok(tokens, ':')) {
            return (ParseRes) {
                .success = false,
                .failMessage = "Expected : after label identifier"
            };
        }

        stmt->type = LabeledStatement_Ident;
        stmt->ident = tok.ident;
    }
    else if (consumeIfTok(tokens, Token_case)) {
        ConditionalExpr constant = {0};
        ParseRes constRes = parseConditionalExpr(tokens, &constant);
        if (!constRes.success) {
            return constRes;
        }

        if (!consumeIfTok(tokens, ':')) {
            return (ParseRes) {
                .success = false,
                .failMessage = "Expected : after case label"
            };
        }

        stmt->type = LabeledStatement_Case;
        stmt->caseConstExpr = constant;
    }
    else if (consumeIfTok(tokens, Token_default)) {
        if (!consumeIfTok(tokens, ':')) {
            return (ParseRes) {
                .success = false,
                .failMessage = "Expected : after case label"
            };
        }

        stmt->type = LabeledStatement_Default;
    }
    else {
        return (ParseRes) {
            .success = false,
            .failMessage = "Expected ident, case, or default for a labeled statment"
        };
    }

    Statement inner = {0};
    ParseRes res = parseStatement(tokens, &inner);
    if (!res.success)
        return res;

    stmt->stmt = malloc(sizeof(inner));
    memcpy(stmt->stmt, &inner, sizeof(inner));
    return (ParseRes){ .success = true };
}

ParseRes parseSelectionStatement(TokenList *tokens, SelectionStatement *selection) {
    if (consumeIfTok(tokens, Token_if)) {
        selection->ifToken = tokens->tokens + tokens->pos - 1;

        if (!consumeIfTok(tokens, '(')) {
            return (ParseRes) {
                .success = false,
                .failMessage = "Expected ( after if"
            };
        }

        Expr expr = {0};
        ParseRes exprRes = parseExpr(tokens, &expr);
        if (!exprRes.success)
            return exprRes;

        selection->type = SelectionStatement_If;
        selection->ifExpr = expr;

        if (!consumeIfTok(tokens, ')')) {
            return (ParseRes) {
                .success = false,
                .failMessage = "Expected ) after expr in if statement"
            };
        }

        Statement stmt = {0};
        ParseRes stmtRes = parseStatement(tokens, &stmt);
        if (!stmtRes.success)
            return stmtRes;

        selection->ifTrueStmt = malloc(sizeof(stmt));
        memcpy(selection->ifTrueStmt, &stmt, sizeof(stmt));

        if (consumeIfTok(tokens, Token_else)) {
            selection->elseToken = tokens->tokens + tokens->pos - 1;

            Statement elseStmt = {0};

            ParseRes elseStmtRes = parseStatement(tokens, &elseStmt);
            if (!elseStmtRes.success)
                return elseStmtRes;

            selection->ifHasElse = true;
            selection->ifFalseStmt = malloc(sizeof(elseStmt));
            memcpy(selection->ifFalseStmt, &elseStmt, sizeof(elseStmt));
        }

        return (ParseRes){ .success = true };
    }
    else if (consumeIfTok(tokens, Token_switch)) {
        selection->switchToken = tokens->tokens + tokens->pos - 1;

        if (!consumeIfTok(tokens, '(')) {
            return (ParseRes) {
                .success = false,
                .failMessage = "Expected ( after switch"
            };
        }

        Expr expr = {0};
        ParseRes exprRes = parseExpr(tokens, &expr);
        if (!exprRes.success)
            return exprRes;

        selection->type = SelectionStatement_Switch;
        selection->switchExpr = expr;

        if (!consumeIfTok(tokens, ')')) {
            return (ParseRes) {
                .success = false,
                .failMessage = "Expected ) after expr in switch statement"
            };
        }

        Statement stmt = {0};
        ParseRes stmtRes = parseStatement(tokens, &stmt);
        if (!stmtRes.success)
            return stmtRes;

        selection->switchStmt = malloc(sizeof(stmt));
        memcpy(selection->switchStmt, &stmt, sizeof(stmt));

        return (ParseRes){ .success = true };
    }
    else {
        return (ParseRes) {
            .success = false,
            .failMessage = "Expected if or switch for selection statement"
        };
    }
}

ParseRes parseExpressionStatement(TokenList *tokens, ExpressionStatement *expression) {
    if (consumeIfTok(tokens, ';')) {
        expression->isEmpty = true;
        return (ParseRes){ .success = true };
    }

    Expr expr = {0};
    ParseRes exprRes = parseExpr(tokens, &expr);
    if (!exprRes.success)
        return exprRes;

    if (!consumeIfTok(tokens, ';')) {
        return (ParseRes) {
            .success = false,
            .failMessage = "Expected ; after expr in expression stmt"
        };
    }

    expression->isEmpty = false;
    expression->expr = expr;

    return (ParseRes){ .success = true };
}

ParseRes parseIterationStatement(TokenList *tokens, IterationStatement *iteration) {
    // Try parse while
    if (consumeIfTok(tokens, Token_while)) {
        iteration->whileToken = tokens->tokens + tokens->pos - 1;

        if (!consumeIfTok(tokens, '(')) {
            return (ParseRes) {
                .success = false,
                .failMessage = "Expected ( after while"
            };
        }

        Expr expr = {0};
        ParseRes exprRes = parseExpr(tokens, &expr);
        if (!exprRes.success)
            return exprRes;

        iteration->type = IterationStatement_While;
        iteration->whileExpr = expr;

        if (!consumeIfTok(tokens, ')')) {
            return (ParseRes) {
                .success = false,
                .failMessage = "Expected ) after expr in while stmt"
            };
        }

        Statement stmt = {0};
        ParseRes stmtRes = parseStatement(tokens, &stmt);
        if (!stmtRes.success)
            return stmtRes;

        iteration->whileStmt = malloc(sizeof(stmt));
        memcpy(iteration->whileStmt, &stmt, sizeof(stmt));

        return (ParseRes){ .success = true };
    }

    // Try parse do while
    else if (consumeIfTok(tokens, Token_do)) {
        iteration->doToken = tokens->tokens + tokens->pos - 1;

        Statement stmt = {0};
        ParseRes stmtRes = parseStatement(tokens, &stmt);
        if (!stmtRes.success)
            return stmtRes;

        iteration->type = IterationStatement_DoWhile;
        iteration->doStmt = malloc(sizeof(stmt));
        memcpy(iteration->doStmt, &stmt, sizeof(stmt));

        if (!consumeIfTok(tokens, Token_while)) {
            return (ParseRes) {
                .success = false,
                .failMessage = "Expected while after do statement"
            };
        }

        if (!consumeIfTok(tokens, '(')) {
            return (ParseRes) {
                .success = false,
                .failMessage = "Expected ( after while in do while stmt"
            };
        }

        Expr expr = {0};
        ParseRes exprRes = parseExpr(tokens, &expr);
        if (!exprRes.success)
            return exprRes;

        iteration->doExpr = expr;

        if (!consumeIfTok(tokens, ')')) {
            return (ParseRes) {
                .success = false,
                .failMessage = "Expected ) after expr in do while stmt"
            };
        }

        if (!consumeIfTok(tokens, ';')) {
            return (ParseRes) {
                .success = false,
                .failMessage = "Expected ; after ) in do while stmt"
            };
        }

        return (ParseRes){ .success = true };
    }

    // Try parse for
    else if (consumeIfTok(tokens, Token_for)) {
        iteration->forToken = tokens->tokens + tokens->pos - 1;

        if (!consumeIfTok(tokens, '(')) {
            return (ParseRes) {
                .success = false,
                .failMessage = "Expected ( after for in for stmt"
            };
        }

        iteration->type = IterationStatement_For;

        size_t pos = tokens->pos;

        // Parse optional declaration
        Declaration decl = {0};
        ParseRes declRes = parseDeclaration(tokens, &decl);
        if (declRes.success) {
            iteration->forHasInitialDeclaration = true;
            iteration->forInitialDeclaration = decl;
        }
        else {
            tokens->pos = pos;

            iteration->forHasInitialDeclaration = false;

            ExpressionStatement exprStmt = {0};
            ParseRes exprStmtRes = parseExpressionStatement(tokens, &exprStmt);
            if (!exprStmtRes.success)
                return exprStmtRes;

            iteration->forInitialExprStmt = exprStmt;
        }

        ExpressionStatement innerExprStmt = {0};
        ParseRes innerExprRes = parseExpressionStatement(tokens, &innerExprStmt);
        if (!innerExprRes.success)
            return innerExprRes;

        iteration->forInnerExprStmt = innerExprStmt;

        size_t preExprPos = tokens->pos;

        Expr expr = {0};
        ParseRes exprRes = parseExpr(tokens, &expr);
        if (exprRes.success) {
            iteration->forHasFinalExpr = true;
            iteration->forFinalExpr = expr;
        }
        else {
            tokens->pos = preExprPos;
        }

        if (!consumeIfTok(tokens, ')')) {
            return (ParseRes) {
                .success = false,
                .failMessage = "Expected ) after expressions in for stmt"
            };
        }

        Statement stmt = {0};
        ParseRes stmtRes = parseStatement(tokens, &stmt);
        if (!stmtRes.success)
            return stmtRes;

        iteration->forStmt = malloc(sizeof(stmt));
        memcpy(iteration->forStmt, &stmt, sizeof(stmt));

        return (ParseRes){ .success = true };
    }

    else {
        return (ParseRes) {
            .success = false,
            .failMessage = "Expected while, do, or for in iteration statement"
        };
    }
}

ParseRes parseJumpStatement(TokenList *tokens, JumpStatement *jump) {
    if (consumeIfTok(tokens, Token_goto)) {
        Token tok = consumeTok(tokens);
        if (tok.type != Token_Ident) {
            return (ParseRes) {
                .success = false,
                .failMessage = "Expected identifier after goto"
            };
        }

        if (!consumeIfTok(tokens, ';')) {
            return (ParseRes) {
                .success = false,
                .failMessage = "Expected ; after goto identifier"
            };
        }

        jump->type = JumpStatement_Goto;
        jump->gotoIdent = tok.ident;
        return (ParseRes){ .success = true };
    }
    else if (consumeIfTok(tokens, Token_continue)) {
        if (!consumeIfTok(tokens, ';')) {
            return (ParseRes) {
                .success = false,
                .failMessage = "Expected ; after continue"
            };
        }

        jump->type = JumpStatement_Continue;
        return (ParseRes){ .success = true };
    }
    else if (consumeIfTok(tokens, Token_break)) {
        if (!consumeIfTok(tokens, ';')) {
            return (ParseRes) {
                .success = false,
                .failMessage = "Expected ; after break"
            };
        }

        jump->type = JumpStatement_Break;
        return (ParseRes){ .success = true };
    }
    else if (consumeIfTok(tokens, Token_return)) {
        jump->type = JumpStatement_Return;

        if (consumeIfTok(tokens, ';')) {
            jump->returnHasExpr = false;
            return (ParseRes){ .success = true };
        }

        Expr expr = {0};
        ParseRes exprRes = parseExpr(tokens, &expr);
        if (!exprRes.success)
            return exprRes;

        if (!consumeIfTok(tokens, ';')) {
            return (ParseRes) {
                .success = false,
                .failMessage = "Expected ; after return expr"
            };
        }

        jump->returnHasExpr = true;
        jump->returnExpr = expr;

        return (ParseRes){ .success = true };
    }
    else {
        return (ParseRes) {
            .success = false,
            .failMessage = "Expected goto, continue, break, or return in jump statement"
        };
    }
}

ParseRes parseAsmStatement(TokenList *tokens, AsmStatement *stmt) {
    if (consumeTok(tokens).type != Token_asm)
        return Fail("Expected asm at beginning of asm statement");

    while (peekTok(tokens).type == Token_volatile ||
           peekTok(tokens).type == Token_inline ||
           peekTok(tokens).type == Token_goto)
    {
        consumeTok(tokens);
    }

    if (consumeTok(tokens).type != '(') {
        return Fail("Expected ( after asm token in asm statement");
    }

    uint64_t numParens = 0;

    while (peekTok(tokens).type != ')' || (numParens != 0)) {
        if (peekTok(tokens).type == '(') {
            numParens++;
        }
        else if (peekTok(tokens).type == ')') {
            numParens--;
        }
        consumeTok(tokens);
    }

    assert(consumeTok(tokens).type == ')');

    if (consumeTok(tokens).type != ';') {
        return Fail("Expected ; after asm statement");
    }

    return Succeed;
}

ParseRes parseStatement(TokenList *tokens, Statement *stmt) {
    size_t pos = tokens->pos;

    LabeledStatement labeled = {0};
    if (parseLabeledStatement(tokens, &labeled).success) {
        stmt->type = Statement_Labeled;
        stmt->labeled = labeled;
        return (ParseRes){ .success = true };
    }

    tokens->pos = pos;

    // Parse compound statement
    CompoundStmt compound = {0};
    if (parseCompoundStmt(tokens, &compound).success) {
        stmt->type = Statement_Compound;
        stmt->compound = malloc(sizeof(compound));
        memcpy(stmt->compound, &compound, sizeof(compound));
        return (ParseRes){ .success = true };
    }

    tokens->pos = pos;

    // Parse selection statement
    SelectionStatement selection = {0};
    if (parseSelectionStatement(tokens, &selection).success) {
        stmt->type = Statement_Selection;
        stmt->selection = selection;
        return (ParseRes){ .success = true };
    }

    tokens->pos = pos;

    // Parse iteration statement
    IterationStatement iteration = {0};
    if (parseIterationStatement(tokens, &iteration).success) {
        stmt->type = Statement_Iteration;
        stmt->iteration = iteration;
        return (ParseRes){ .success = true };
    }

    tokens->pos = pos;

    // Parse jump statement
    JumpStatement jump = {0};
    if (parseJumpStatement(tokens, &jump).success) {
        stmt->type = Statement_Jump;
        stmt->jump = jump;
        return (ParseRes){ .success = true };
    }

    tokens->pos = pos;

    // Parse expression statement
    ExpressionStatement expression = {0};
    if (parseExpressionStatement(tokens, &expression).success) {
        stmt->type = Statement_Expression;
        stmt->expression = expression;
        return (ParseRes){ .success = true };
    }

    // Parse asm statement
    // TODO: Do we need any real data from the asm stmt?
    AsmStatement assembly = {0};
    if (parseAsmStatement(tokens, &assembly).success) {
        stmt->type = Statement_Asm;
        stmt->assembly = assembly;
        return (ParseRes){ .success = true };
    }

    return (ParseRes) {
        .success = false,
        .failMessage = "Didn't find any statements"
    };
}

ParseRes parseBlockItem(TokenList *tokens, BlockItem *item) {
    // either a declaration or statement
    size_t pos = tokens->pos;

    Declaration decl = {0};
    if (parseDeclaration(tokens, &decl).success) {
        item->type = BlockItem_Declaration;
        item->decl = decl;
        return (ParseRes){ .success = true };
    }

    tokens->pos = pos;

    Statement stmt = {0};
    if (parseStatement(tokens, &stmt).success) {
        item->type = BlockItem_Statement;
        item->stmt = stmt;
        return (ParseRes){ .success = true };
    }

    return (ParseRes){
        .success = false,
        .failMessage = "Expected either declaration or stmt in block item"
    };
}

ParseRes parseBlockItemList(TokenList *tokens, BlockItemList *list) {
    ParseRes blockItemRes = {0};
    do {

        // Parse opt block item
        size_t pos = tokens->pos;

        BlockItem item = {0};
        blockItemRes = parseBlockItem(tokens, &item);
        if (!blockItemRes.success) {
            tokens->pos = pos;
            break;
        }

        sll_appendLocal(&(list->list), item);
    } while(blockItemRes.success);

    // Make sure we have at least one block item
    if (list->list.size == 0) {
        return (ParseRes) {
            .success = false,
            .failMessage = "Expected at least one block item in a block item list"
        };
    }

    return (ParseRes){ .success = true };
}

ParseRes parseCompoundStmt(TokenList *tokens, CompoundStmt *outStmt) {
    if (!consumeIfTok(tokens, '{')) {
        return (ParseRes) {
            .success = false,
            .failMessage = "Expected { to start compound statement"
        };
    }

    // Set hooks back to original token
    // TODO: Make this into a macro?
    outStmt->openBracket = tokens->tokens + tokens->pos - 1;

    if (consumeIfTok(tokens, '}')) {
        // TODO: Make this into a macro?
        outStmt->closeBracket = tokens->tokens + tokens->pos - 1;

        outStmt->isEmpty = true;
        return (ParseRes){ .success = true };
    }

    BlockItemList list = {0};
    ParseRes listRes = parseBlockItemList(tokens, &list);
    if (!listRes.success)
        return listRes;

    if (!consumeIfTok(tokens, '}')) {
        return (ParseRes) {
            .success = false,
            .failMessage = "Expected } after block item list in compound stmt"
        };
    }

    // TODO: Make this into a macro?
    outStmt->closeBracket = tokens->tokens + tokens->pos - 1;

    outStmt->isEmpty = false;
    outStmt->blockItemList = list;
    return (ParseRes){ .success = true };
}

ParseRes parseFuncDef(TokenList *tokens, FuncDef *outDef) {
    // Parse Declaration Specifiers
    DeclarationSpecifierList specifierList = {0};
    ParseRes specifierRes = parseDeclarationSpecifierList(
        tokens, &specifierList);
    if (!specifierRes.success) {
        return specifierRes;
    }
    outDef->specifiers = specifierList;

    // Parse Declarator
    Declarator declarator = {0};
    ParseRes declRes = parseDeclarator(tokens, &declarator);
    if (!declRes.success)
        return declRes;

    outDef->declarator = declarator;

    // Parse Opt Declaration List
    ParseRes listRes = {0};
    do {

        size_t pos = tokens->pos;

        Declaration declaration = {0};
        listRes = parseDeclaration(tokens, &declaration);
        if (!listRes.success) {
            tokens->pos = pos;
            break;
        }

        sll_appendLocal(&(outDef->declarations), declaration);
    } while(listRes.success);

    // Parse Compound Statement
    CompoundStmt stmt = {0};
    ParseRes stmtRes = parseCompoundStmt(tokens, &stmt);
    if (!stmtRes.success)
        return stmtRes;

    outDef->stmt = stmt;

    return (ParseRes){ .success = true };
}

ParseRes parseExternalDecl(TokenList *tokens, ExternalDecl *outDecl) {
    // Parse opt function definition
    size_t pos = tokens->pos;
    FuncDef def = {0};
    ParseRes res = parseFuncDef(tokens, &def);
    if (res.success) {
        outDecl->type = ExternalDecl_FuncDef;
        outDecl->func = def;
        return res;
    }
    tokens->pos = pos;

    // Parse declaration
    Declaration decl = {0};
    ParseRes declRes = parseDeclaration(tokens, &decl);
    if (declRes.success) {
        outDecl->type = ExternalDecl_Decl;
        outDecl->decl = decl;
        return (ParseRes){ .success = true };
    }

    return (ParseRes) {
        .success = false,
        .failMessage = "Couldn't find a function definition or declaration"
    };
}

bool parseTokens(TokenList *tokens, TranslationUnit *outUnit) {
    // TODO: specify these in the config file
    typedefTable_add(&g_typedefTable, astr("__builtin_va_list"));
    typedefTable_add(&g_typedefTable, astr("_Float128"));

    tokens->pos = 0;

    while (tokens->pos < tokens->numTokens) {
        size_t pos = tokens->pos;

        ExternalDecl decl = {0};
        ParseRes res = parseExternalDecl(tokens, &decl);
        if (!res.success) {
            tokens->pos = pos;
            Token tok = tokens->tokens[tokens->pos];
            logError("Parser: %s:%ld: %s\n  Current token position: %ld\n", tok.fileName, tok.line, res.failMessage, tokens->pos);
            return false;
        }

        sll_appendLocal(&(outUnit->externalDecls), decl);
    }

    return true;
}

#define BaseIndent 2

void printConditionalExpr(ConditionalExpr expr, uint64_t indent);
void printInitializer(Initializer init, uint64_t indent);
void printAssignExpr(AssignExpr expr, uint64_t indent);
void printInitializerList(InitializerList list, uint64_t indent);
void printTypeName(TypeName type, uint64_t indent);
void printExpr(Expr expr, uint64_t indent);
void printCastExpr(CastExpr expr, uint64_t indent);
void printDeclarationSpecifierList(DeclarationSpecifierList list, uint64_t indent);
void printDeclarator(Declarator decl, uint64_t indent);
void printAbstractDeclarator(AbstractDeclarator decl, uint64_t indent);
void printTypeQualifier(TypeQualifier type, uint64_t indent);
void printTypeSpecifier(TypeSpecifier type, uint64_t indent);
void printStatement(Statement stmt, uint64_t indent);
void printCompoundStmt(CompoundStmt stmt, uint64_t indent);

void printIndent(uint64_t indent) {
    for (uint64_t i = 0; i < indent; i++)
        printDebug(" ");
}

void printDesignator(Designator designator, uint64_t indent) {
    if (designator.type == Designator_Constant) {
        printConditionalExpr(*(designator.constantExpr), indent);
    }
    else if (designator.type == Designator_Ident) {
        printIndent(indent);
        printDebug("%.*s\n", astr_format(designator.ident));
    }
}

void printDesignation(Designation desig, uint64_t indent) {
    printIndent(indent);
    printDebug("Designation: %ld\n", desig.list.size);
    sll_foreach(desig.list, node) {
        Designator *designator = slNode_getData(node);
        printDesignator(*designator, indent + BaseIndent);
    }
}

void printInitializer(Initializer init, uint64_t indent) {
    if (init.type == Initializer_InitializerList) {
        printInitializerList(*(init.initializerList), indent);
    }
    else if (init.type == Initializer_Assignment) {
        printAssignExpr(*(init.assignmentExpr), indent);
    }
}

void printDesignationAndInitializer(DesignationAndInitializer desig, uint64_t indent) {
    if (desig.hasDesignation) {
        printDesignation(desig.designation, indent);
    }

    printInitializer(desig.initializer, indent + BaseIndent);
}

void printInitializerList(InitializerList list, uint64_t indent) {
    printIndent(indent);
    printDebug("Initializer List: %ld\n", list.list.size);
    sll_foreach(list.list, node) {
        DesignationAndInitializer *desig = slNode_getData(node);
        printDesignationAndInitializer(*desig, indent + BaseIndent);
    }
}

void printGenericAssociation(GenericAssociation assoc, uint64_t indent) {
    if (assoc.isDefault) {
        printIndent(indent);
        printDebug("Default\n");
    }
    else {
        printTypeName(*(assoc.typeName), indent);
    }

    printAssignExpr(*(assoc.expr), indent + BaseIndent);
}

void printGenericSelection(GenericSelection generic, uint64_t indent) {
    printIndent(indent);
    printDebug("Generic (\n");

    printAssignExpr(*(generic.expr), indent + BaseIndent);

    printIndent(indent);
    printDebug("Association List: %ld\n", generic.associations.size);
    sll_foreach(generic.associations, node) {
        GenericAssociation *assoc = slNode_getData(node);
        printGenericAssociation(*assoc, indent + BaseIndent);
    }
}

void printConstantExpr(ConstantExpr expr, uint64_t indent) {
    if (expr.type == Constant_Numeric) {
        printIndent(indent);
        printDebug("Numeic: %.*s\n", astr_format(expr.data));
    }
    else if (expr.type == Constant_Character) {
        printIndent(indent);
        printDebug("Char: %.*s\n", astr_format(expr.data));
    }
    else {
        assert(false);
    }
}

void printPrimaryExpr(PrimaryExpr expr, uint64_t indent) {
    if (expr.type == PrimaryExpr_Ident) {
        printIndent(indent);
        printDebug("Ident: %.*s\n", astr_format(expr.ident));
    }
    else if (expr.type == PrimaryExpr_Constant) {
        printConstantExpr(expr.constant, indent);
    }
    else if (expr.type == PrimaryExpr_String) {
        printIndent(indent);
        printDebug("\"%.*s\"\n", astr_format(expr.string));
    }
    else if (expr.type == PrimaryExpr_FuncName) {
        printIndent(indent);
        printDebug("__func__\n");
    }
    else if (expr.type == PrimaryExpr_Expr) {
        printIndent(indent);
        printDebug("(\n");

        printExpr(*(expr.expr), indent + BaseIndent);

        printIndent(indent);
        printDebug(")\n");
    }
    else if (expr.type == PrimaryExpr_GenericSelection) {
        printGenericSelection(expr.genericSelection, indent);
    }
    else {
        assert(false);
    }
}

void printArgExprList(ArgExprList args, uint64_t indent) {
    printIndent(indent);
    printDebug("Arg Expr List: %ld\n", args.list.size);
    sll_foreach(args.list, node) {
        AssignExpr *expr = slNode_getData(node);
        printAssignExpr(*expr, indent + BaseIndent);
    }
}

void printPostfixOp(PostfixOp op, uint64_t indent) {
    if (op.type == PostfixOp_Index) {
        printIndent(indent);
        printDebug("[\n");

        printExpr(*(op.indexExpr), indent + BaseIndent);

        printIndent(indent);
        printDebug("]\n");
    }
    else if (op.type == PostfixOp_Call) {
        if (op.callHasEmptyArgs) {
            printIndent(indent);
            printDebug("( )\n");
        }
        else {
            printIndent(indent);
            printDebug("(\n");

            printArgExprList(op.callExprs, indent + BaseIndent);

            printIndent(indent);
            printDebug(")\n");
        }
    }
    else if (op.type == PostfixOp_Dot) {
        printIndent(indent);
        printDebug(".%.*s\n", astr_format(op.dotIdent));
    }
    else if (op.type == PostfixOp_Arrow) {
        printIndent(indent);
        printDebug("->%.*s\n", astr_format(op.arrowIdent));
    }
    else if (op.type == PostfixOp_Inc) {
        printIndent(indent);
        printDebug("++\n");
    }
    else if (op.type == PostfixOp_Dec) {
        printIndent(indent);
        printDebug("--\n");
    }
    else {
        assert(false);
    }
}

void printPostfixExpr(PostfixExpr expr, uint64_t indent) {
    if (expr.type == Postfix_Primary) {
        printPrimaryExpr(expr.primary, indent);
    }
    else if (expr.type == Postfix_InitializerList) {
        printInitializerList(expr.initializerList, indent);
    }
    else {
        assert(false);
    }

    sll_foreach(expr.postfixOps, node) {
        PostfixOp *op = slNode_getData(node);
        printPostfixOp(*op, indent);
    }
}

void printUnaryExprPrefix(UnaryExprPrefixType type, uint64_t indent) {
    printIndent(indent);
    switch (type) {
        case UnaryPrefix_And: {
            printDebug("&\n");
            break;
        }
        case UnaryPrefix_Star: {
            printDebug("*\n");
            break;
        }
        case UnaryPrefix_Plus: {
            printDebug("+\n");
            break;
        }
        case UnaryPrefix_Minus: {
            printDebug("-\n");
            break;
        }
        case UnaryPrefix_Tilde: {
            printDebug("~\n");
            break;
        }
        case UnaryPrefix_Not: {
            printDebug("!\n");
            break;
        }
        default: {
            assert(false);
            return;
        }
    }
}

void printUnaryExpr(UnaryExpr expr, uint64_t indent) {
    switch (expr.type) {
        case UnaryExpr_UnaryOp: {
            printUnaryExprPrefix(expr.unaryOpType, indent);
            printCastExpr(*(expr.unaryOpCast), indent + BaseIndent);
            break;
        }
        case UnaryExpr_Inc: {
            printIndent(indent);
            printDebug("++\n");
            printUnaryExpr(*(expr.incOpExpr), indent + BaseIndent);
            break;
        }
        case UnaryExpr_Dec: {
            printIndent(indent);
            printDebug("--\n");
            printUnaryExpr(*(expr.decOpExpr), indent + BaseIndent);
            break;
        }
        case UnaryExpr_SizeofExpr: {
            printIndent(indent);
            printDebug("sizeof expr\n");
            printUnaryExpr(*(expr.decOpExpr), indent + BaseIndent);
            break;
        }
        case UnaryExpr_SizeofType: {
            printIndent(indent);
            printDebug("sizeof typename\n");
            printTypeName(*(expr.sizeofTypeName), indent + BaseIndent);
            break;
        }
        case UnaryExpr_AlignofType: {
            printIndent(indent);
            printDebug("alignof\n");
            printTypeName(*(expr.alignofTypeName), indent + BaseIndent);
            break;
        }
        case UnaryExpr_Base: {
            printPostfixExpr(expr.baseExpr, indent);
            break;
        }
        default: {
            assert(false);
            return;
        }
    }
}

void printCastExpr(CastExpr cast, uint64_t indent) {
    if (cast.type == CastExpr_Unary) {
        printUnaryExpr(cast.unary, indent);
    }
    else if (cast.type == CastExpr_Cast) {
        printIndent(indent);
        printDebug("Cast: (\n");
        printTypeName(*(cast.castType), indent + BaseIndent);

        printIndent(indent);
        printDebug(")\n");
        printCastExpr(*(cast.castExpr), indent + BaseIndent);
    }
    else {
        assert(false);
    }
}

void printMultiplicativeExpr(MultiplicativeExpr expr, uint64_t indent) {
    printCastExpr(expr.baseExpr, indent);
    sll_foreach(expr.postExprs, node) {
        MultiplicativePost *post = slNode_getData(node);
        printIndent(indent + BaseIndent);

        if (post->op == Multiplicative_Mul) {
            printDebug("*\n");
        }
        else if (post->op == Multiplicative_Div) {
            printDebug("/\n");
        }
        else if (post->op == Multiplicative_Mod) {
            printDebug("%%\n");
        }
        else {
            assert(false);
        }
        printCastExpr(post->expr, indent + BaseIndent);
    }
}

// FIXME: Fix the way we print out binary exprs.
// Currently, we print the left side without knowing if there
// is an operation
void printAdditiveExpr(AdditiveExpr expr, uint64_t indent) {
    printMultiplicativeExpr(expr.baseExpr, indent);
    sll_foreach(expr.postExprs, node) {
        AdditivePost *post = slNode_getData(node);
        printIndent(indent + BaseIndent);

        if (post->op == Additive_Add) {
            printDebug("+\n");
        }
        else if (post->op == Additive_Sub) {
            printDebug("-\n");
        }
        else {
            assert(false);
        }
        printMultiplicativeExpr(post->expr, indent + BaseIndent);
    }
}

void printShiftExpr(ShiftExpr expr, uint64_t indent) {
    printAdditiveExpr(expr.baseExpr, indent);
    sll_foreach(expr.postExprs, node) {
        ShiftPost *post = slNode_getData(node);
        if (node->next != NULL) {
        // if (i != expr.numPostExprs - 1) {
            printIndent(indent + BaseIndent);

            if (post->op == Shift_Left) {
                printDebug("<<\n");
            }
            else if (post->op == Shift_Right) {
                printDebug(">>\n");
            }
            else {
                assert(false);
            }
        }
        printAdditiveExpr(post->expr, indent + BaseIndent);
    }
}

void printRelationalExpr(RelationalExpr expr, uint64_t indent) {
    printShiftExpr(expr.baseExpr, indent);
    sll_foreach(expr.postExprs, node) {
        RelationalPost *post = slNode_getData(node);
        if (node->next != NULL) {
        // if (i != expr.numPostExprs - 1) {
            printIndent(indent + BaseIndent);

            if (post->op == Relational_Lt) {
                printDebug("<\n");
            }
            else if (post->op == Relational_Gt) {
                printDebug(">\n");
            }
            else if (post->op == Relational_LEq) {
                printDebug("<=\n");
            }
            else if (post->op == Relational_GEq) {
                printDebug(">=\n");
            }
            else {
                assert(false);
            }
        }
        printShiftExpr(post->expr, indent + BaseIndent);
    }
}

void printEqualityExpr(EqualityExpr expr, uint64_t indent) {
    printRelationalExpr(expr.baseExpr, indent);
    sll_foreach(expr.postExprs, node) {
        EqualityPost *post = slNode_getData(node);
        if (node->next != NULL) {
        // if (i != expr.numPostExprs - 1) {
            printIndent(indent + BaseIndent);

            if (post->op == Equality_Eq) {
                printDebug("==\n");
            }
            else if (post->op == Equality_NEq) {
                printDebug("!=\n");
            }
            else {
                assert(false);
            }
        }
        printRelationalExpr(post->expr, indent + BaseIndent);
    }
}

void printAndExpr(AndExpr expr, uint64_t indent) {
    sll_foreach(expr.list, node) {
        EqualityExpr *eqExpr = slNode_getData(node);
        printEqualityExpr(*eqExpr, indent);
        if (node->next != NULL) {
        // if (i != expr.numExprs - 1) {
            printIndent(indent + BaseIndent);
            printDebug("&\n");
        }
    }
}

void printExclusiveOrExpr(ExclusiveOrExpr expr, uint64_t indent) {
    sll_foreach(expr.list, node) {
        AndExpr *andExpr = slNode_getData(node);
        printAndExpr(*andExpr, indent);
        if (node->next != NULL) {
        // if (i != expr.numExprs - 1) {
            printIndent(indent + BaseIndent);
            printDebug("^\n");
        }
    }
}

void printInclusiveOrExpr(InclusiveOrExpr expr, uint64_t indent) {
    sll_foreach(expr.list, node) {
        ExclusiveOrExpr *orExpr = slNode_getData(node);
        printExclusiveOrExpr(*orExpr, indent);
        if (node->next != NULL) {
        // if (i != expr.numExprs - 1) {
            printIndent(indent + BaseIndent);
            printDebug("|\n");
        }
    }
}

void printLogicalAndExpr(LogicalAndExpr expr, uint64_t indent) {
    sll_foreach(expr.list, node) {
        InclusiveOrExpr *orExpr = slNode_getData(node);
        printInclusiveOrExpr(*orExpr, indent);
        if (node->next != NULL) {
        // if (i != expr.numExprs - 1) {
            printIndent(indent + BaseIndent);
            printDebug("&&\n");
        }
    }
}

void printLogicalOrExpr(LogicalOrExpr orExpr, uint64_t indent) {
    sll_foreach(orExpr.list, node) {
        LogicalAndExpr *andExpr = slNode_getData(node);
        printLogicalAndExpr(*andExpr, indent);
        if (node->next != NULL) {
        // if (i != orExpr.numExprs - 1) {
            printIndent(indent + BaseIndent);
            printDebug("||\n");
        }
    }
}

void printConditionalExpr(ConditionalExpr expr, uint64_t indent) {
    printLogicalOrExpr(expr.beforeExpr, indent);

    if (expr.hasConditionalOp) {
        printIndent(indent);
        printDebug("?\n");
        printExpr(*(expr.ifTrueExpr), indent + BaseIndent);
        printIndent(indent);
        printDebug(":\n");
        printConditionalExpr(*(expr.ifFalseExpr), indent + BaseIndent);
    }
}

void printAssignOp(AssignOp op, uint64_t indent) {
    printIndent(indent);
    switch (op) {
        case Assign_Eq: {
            printDebug("=\n");
            break;
        }
        case Assign_MulEq: {
            printDebug("*=\n");
            break;
        }
        case Assign_DivEq: {
            printDebug("/=\n");
            break;
        }
        case Assign_ModEq: {
            printDebug("%%=\n");
            break;
        }
        case Assign_AddEq: {
            printDebug("+=\n");
            break;
        }
        case Assign_SubEq: {
            printDebug("-=\n");
            break;
        }
        case Assign_ShiftLeftEq: {
            printDebug("<<=\n");
            break;
        }
        case Assign_ShiftRightEq: {
            printDebug(">>=\n");
            break;
        }
        case Assign_AndEq: {
            printDebug("&=\n");
            break;
        }
        case Assign_XorEq: {
            printDebug("^=\n");
            break;
        }
        case Assign_OrEq: {
            printDebug("|=\n");
            break;
        }
        default: {
            assert(false);
            break;
        }
    }
}

void printAssignExpr(AssignExpr expr, uint64_t indent) {
    sll_foreach(expr.leftExprs, node) {
        AssignPrefix *prefix = slNode_getData(node);
        printUnaryExpr(prefix->leftExpr, indent);
        printAssignOp(prefix->op, indent + BaseIndent);
    }
    printConditionalExpr(expr.rightExpr, indent);
}

void printInnerExpr(InnerExpr expr, uint64_t indent) {
    if (expr.type == InnerExpr_Assign) {
        printAssignExpr(expr.assign, indent);
    }
    else if (expr.type == InnerExpr_CompoundStatement) {
        printCompoundStmt(*(expr.compoundStmt), indent);
    }
    else {
        assert(false);
    }
}

void printExpr(Expr expr, uint64_t indent) {
    printIndent(indent);
    printDebug("Expr: %ld\n", expr.list.size);
    sll_foreach(expr.list, node) {
        InnerExpr *inner = slNode_getData(node);
        printInnerExpr(*inner, indent + BaseIndent);
    }
}

void printParameterDeclaration(ParameterDeclaration decl, uint64_t indent) {
    printIndent(indent);
    printDebug("Parameter Decl:\n");

    printDeclarationSpecifierList(*(decl.declarationSpecifiers), indent + BaseIndent);

    if (decl.hasDeclarator) {
        printDeclarator(*(decl.declarator), indent + BaseIndent);
    }

    if (decl.hasAbstractDeclarator) {
        printAbstractDeclarator(*(decl.abstractDeclarator), indent + BaseIndent);
    }
}

void printParameterTypeList(ParameterTypeList list, uint64_t indent) {
    printIndent(indent);
    printDebug("Parameter Type List: %ld\n", list.paramDecls.size);
    sll_foreach(list.paramDecls, node) {
        ParameterDeclaration *decl = slNode_getData(node);
        printParameterDeclaration(*decl, indent + BaseIndent);
    }
    if (list.hasEndingEllipsis) {
        printIndent(indent + BaseIndent);
        printDebug("...\n");
    }
}

void printPostDirectAbstractDeclarator(PostDirectAbstractDeclarator post, uint64_t indent) {
    printIndent(indent);
    if (post.type == PostDirectAbstractDeclarator_Paren) {
        if (post.parenIsEmpty) {
            printDebug("( )\n");
        }
        else {
            printDebug("(\n");

            printParameterTypeList(post.parenParamList, indent + BaseIndent);

            printIndent(indent);
            printDebug(")\n");
        }
    }
    else if (post.type == PostDirectAbstractDeclarator_Bracket) {
        if (post.bracketIsEmpty) {
            printDebug("[ ]\n");
        }
        else if (post.bracketIsStar) {
            printDebug("[ * ]\n");
        }
        else {
            if (post.bracketHasInitialStatic) {
                printDebug("static\n");
            }

            printIndent(indent + BaseIndent);
            printDebug("Type qualifier list: %ld\n", post.bracketTypeQualifiers.size);
            sll_foreach(post.bracketTypeQualifiers, node) {
                TypeQualifier *qualifier = slNode_getData(node);
                printTypeQualifier(*qualifier, indent + BaseIndent + BaseIndent);
            }

            if (post.bracketHasMiddleStatic) {
                printIndent(indent + BaseIndent);
                printDebug("static\n");
            }

            if (post.bracketHasAssignmentExpr) {
                printIndent(indent + BaseIndent);
                printAssignExpr(post.bracketAssignExpr, indent + BaseIndent);
            }
        }
    }
    else {
        assert(false);
    }
}

void printDirectAbstractDeclarator(DirectAbstractDeclarator direct, uint64_t indent) {
    printIndent(indent);
    printDebug("Direct Abstract Declarator\n");

    if (direct.hasAbstractDeclarator) {
        printIndent(indent);
        printDebug("(\n");

        printAbstractDeclarator(*(direct.abstractDeclarator), indent + BaseIndent);

        printIndent(indent);
        printDebug(")\n");
    }

    sll_foreach(direct.postDirectAbstractDeclarators, node) {
        PostDirectAbstractDeclarator *post = slNode_getData(node);
        printPostDirectAbstractDeclarator(*post, indent + BaseIndent);
    }
}

void printPointer(Pointer pointer, uint64_t indent) {
    printIndent(indent);
    for (size_t i = 0; i < pointer.numPtrs; i++) {
        printDebug("*");
    }

    printDebug("\n");

    sll_foreach(pointer.typeQualifiers, node) {
        TypeQualifier *qualifier = slNode_getData(node);
        printTypeQualifier(*qualifier, indent + BaseIndent);
    }

    if (pointer.hasPtr) {
        printPointer(*(pointer.pointer), indent + BaseIndent);
    }
}

void printAbstractDeclarator(AbstractDeclarator decl, uint64_t indent) {
    printIndent(indent);
    printDebug("Abstract Declarator:\n");
    if (decl.hasPointer)
        printPointer(decl.pointer, indent + BaseIndent);

    if (decl.hasDirectAbstractDeclarator)
        printDirectAbstractDeclarator(decl.directAbstractDeclarator, indent + BaseIndent);
}

void printIdentifierList(IdentifierList list, uint64_t indent) {
    printIndent(indent);
    printDebug("Identifier list: %ld\n", list.list.size);
    sll_foreach(list.list, node) {
        printIndent(indent + BaseIndent);
        String *str = slNode_getData(node);
        printDebug("%.*s\n", astr_format(*str));
        str = str; // This is just there to get rid of the warning
    }
}

void printPostDirectDeclarator(PostDirectDeclarator post, uint64_t indent) {
    if (post.type == PostDirectDeclarator_Paren) {
        if (post.parenType == PostDirectDeclaratorParen_Empty) {
            printIndent(indent);
            printDebug("( )\n");
        }
        else if (post.parenType == PostDirectDeclaratorParen_IdentList) {
            printIndent(indent);
            printDebug("(\n");

            printIdentifierList(post.parenIdentList, indent + BaseIndent);

            printIndent(indent);
            printDebug(")\n");
        }
        else if (post.parenType == PostDirectDeclaratorParen_ParamTypelist) {
            printIndent(indent);
            printDebug("(\n");

            printParameterTypeList(post.parenParamTypeList, indent + BaseIndent);

            printIndent(indent);
            printDebug(")\n");
        }
        else {
            assert(false);
        }
    }
    else if (post.type == PostDirectDeclarator_Bracket) {
        printIndent(indent);
        if (post.bracketIsEmpty) {
            printDebug("[ ]\n");
        }
        else if (post.bracketIsStar) {
            printDebug("[ * ]\n");
        }
        else {
            printDebug("[\n");

            if (post.bracketHasInitialStatic) {
                printIndent(indent + BaseIndent);
                printDebug("static\n");
            }

            printIndent(indent + BaseIndent);
            printDebug("Type qualifier list: %ld\n", post.bracketTypeQualifiers.size);
            sll_foreach(post.bracketTypeQualifiers, node) {
                TypeQualifier *qualifier = slNode_getData(node);
                printTypeQualifier(*qualifier, indent + BaseIndent + BaseIndent);
            }

            if (post.bracketHasMiddleStatic) {
                printIndent(indent + BaseIndent);
                printDebug("static\n");
            }

            if (post.bracketHasAssignExpr) {
                printIndent(indent + BaseIndent);
                printAssignExpr(post.bracketAssignExpr, indent + BaseIndent);
            }
        }
    }
    else {
        assert(false);
    }
}

void printDirectDeclarator(DirectDeclarator direct, uint64_t indent) {
    printIndent(indent);
    if (direct.type == DirectDeclarator_Ident) {
        printDebug("%.*s\n", astr_format(direct.ident));
    }
    else if (direct.type == DirectDeclarator_ParenDeclarator) {
        printDebug("(\n");

        printDeclarator(*(direct.declarator), indent + BaseIndent);

        printIndent(indent);
        printDebug(")\n");
    }
    else {
        assert(false);
    }

    printIndent(indent);
    printDebug("Post Direct Declarator: %ld\n", direct.postDirectDeclarators.size);
    sll_foreach(direct.postDirectDeclarators, node) {
        PostDirectDeclarator *post = slNode_getData(node);
        printPostDirectDeclarator(*post, indent + BaseIndent);
    }
}

void printDeclarator(Declarator decl, uint64_t indent) {
    printIndent(indent);
    printDebug("Declarator\n");
    if (decl.hasPointer) {
        printPointer(decl.pointer, indent + BaseIndent);
    }
    printDirectDeclarator(decl.directDeclarator, indent + BaseIndent);
}

void printTypeQualifier(TypeQualifier qual, uint64_t indent) {
    printIndent(indent);
    switch (qual) {
        case Qualifier_Const: {
            printDebug("const\n");
            break;
        }
        case Qualifier_Restrict: {
            printDebug("restrict\n");
            break;
        }
        case Qualifier_Volatile: {
            printDebug("volatile\n");
            break;
        }
        case Qualifier_Atomic: {
            printDebug("atomic\n");
            break;
        }
        default: {
            assert(false);
            break;
        }
    }
}

void printSpecifierQualifier(SpecifierQualifier specQual, uint64_t indent) {
    if (specQual.type == SpecifierQualifier_Specifier) {
        printTypeSpecifier(*(specQual.typeSpecifier), indent);
    }
    else if (specQual.type == SpecifierQualifier_Qualifier) {
        printTypeQualifier(specQual.typeQualifier, indent);
    }
    else {
        assert(false);
    }
}

void printSpecifierQualifierList(SpecifierQualifierList list, uint64_t indent) {
    printIndent(indent);
    printDebug("Specifier Qualifier List: %ld\n", list.list.size);
    sll_foreach(list.list, node) {
        SpecifierQualifier *spec = slNode_getData(node);
        printSpecifierQualifier(*spec, indent + BaseIndent);
    }
}

void printTypeName(TypeName name, uint64_t indent) {
    printIndent(indent);
    printDebug("Type Name\n");
    printSpecifierQualifierList(name.specifierQualifiers, indent + BaseIndent);
    if (name.hasAbstractDeclarator)
        printAbstractDeclarator(name.abstractDeclarator, indent + BaseIndent);
}

void printStructDeclarator(StructDeclarator decl, uint64_t indent) {
    printIndent(indent);
    printDebug("Struct Declarator\n");
    if (decl.hasDeclarator)
        printDeclarator(decl.declarator, indent + BaseIndent);
    if (decl.hasConstExpr)
        printConditionalExpr(decl.constExpr, indent + BaseIndent);
}

void printStructDeclaratorList(StructDeclaratorList list, uint64_t indent) {
    printIndent(indent);
    printDebug("Struct Declarator List: %lu\n", list.list.size);
    sll_foreach(list.list, node) {
        StructDeclarator *decl = slNode_getData(node);
        printStructDeclarator(*decl, indent + BaseIndent);
    }
}

void printStaticAssertDeclaration(StaticAssertDeclaration staticAssert, uint64_t indent) {
    printIndent(indent);
    printDebug("Static Assert\n");
    printConditionalExpr(staticAssert.constantExpr, indent + BaseIndent);
    printIndent(indent + BaseIndent);
    printDebug(",\n");
    printIndent(indent + BaseIndent);
    printDebug("%.*s\n", astr_format(staticAssert.stringLiteral));
}

void printStructDeclaration(StructDeclaration decl, uint64_t indent) {
    if (decl.type == StructDeclaration_StaticAssert) {
        printStaticAssertDeclaration(decl.staticAssert, indent);
    }
    else if (decl.type == StructDeclaration_Normal) {
        printIndent(indent);
        printDebug("Struct Declaration:\n");
        printSpecifierQualifierList(decl.normalSpecifierQualifiers, indent + BaseIndent);
        if (decl.normalHasStructDeclaratorList) {
            printStructDeclaratorList(decl.normalStructDeclaratorList, indent + BaseIndent);
        }
    }
    else {
        assert(false);
    }
}

void printStructOrUnionSpecifier(StructOrUnionSpecifier structOrUnion, uint64_t indent) {
    printIndent(indent);
    if (structOrUnion.structOrUnion == StructOrUnion_Struct) {
        printDebug("struct");
    }
    else if (structOrUnion.structOrUnion == StructOrUnion_Union) {
        printDebug("enum");
    }
    else {
        assert(false);
    }

    if (structOrUnion.hasIdent) {
        printDebug(" %.*s\n", astr_format(structOrUnion.ident));
    }
    else {
        printDebug("\n");
    }

    printIndent(indent);
    if (!structOrUnion.hasStructDeclarationList) {
        printDebug("{ }\n");
    }
    else {
        printDebug("{\n");

        sll_foreach(structOrUnion.structDeclarations, node) {
            StructDeclaration *decl = slNode_getData(node);
            printStructDeclaration(*decl, indent + BaseIndent);
        }

        printIndent(indent);
        printDebug("}\n");
    }
}

void printEnumerator(Enumerator enumer, uint64_t indent) {
    printIndent(indent);
    printDebug("%.*s", astr_format(enumer.constantIdent));
    if (enumer.hasConstExpr) {
        printDebug(" = \n");
        printConditionalExpr(enumer.constantExpr, indent + BaseIndent);
    }
    else {
        printDebug("\n");
    }
}

void printEnumeratorList(EnumeratorList enumList, uint64_t indent) {
    printIndent(indent);
    printDebug("EnumeratorList: %lu\n", enumList.list.size);
    sll_foreach(enumList.list, node) {
        Enumerator *enumerator = slNode_getData(node);
        printEnumerator(*enumerator, indent + BaseIndent);
    }
}

void printEnumSpecifier(EnumSpecifier enumSpec, uint64_t indent) {
    printIndent(indent);
    if (enumSpec.hasIdent)
        printDebug("enum %.*s\n", astr_format(enumSpec.ident));
    else
        printDebug("enum\n");

    if (enumSpec.hasEnumeratorList) {
        printEnumeratorList(enumSpec.enumeratorList, indent + BaseIndent);
    }
}

void printTypeSpecifier(TypeSpecifier type, uint64_t indent) {
    switch (type.type) {
        case TypeSpecifier_Void: {
            printIndent(indent);
            printDebug("void\n");
            break;
        }
        case TypeSpecifier_Char: {
            printIndent(indent);
            printDebug("char\n");
            break;
        }
        case TypeSpecifier_Short: {
            printIndent(indent);
            printDebug("short\n");
            break;
        }
        case TypeSpecifier_Int: {
            printIndent(indent);
            printDebug("int\n");
            break;
        }
        case TypeSpecifier_Long: {
            printIndent(indent);
            printDebug("long\n");
            break;
        }
        case TypeSpecifier_Float: {
            printIndent(indent);
            printDebug("float\n");
            break;
        }
        case TypeSpecifier_Double: {
            printIndent(indent);
            printDebug("double\n");
            break;
        }
        case TypeSpecifier_Signed: {
            printIndent(indent);
            printDebug("signed\n");
            break;
        }
        case TypeSpecifier_Unsigned: {
            printIndent(indent);
            printDebug("unsigned\n");
            break;
        }
        case TypeSpecifier_Bool: {
            printIndent(indent);
            printDebug("bool\n");
            break;
        }
        case TypeSpecifier_Complex: {
            printIndent(indent);
            printDebug("complex\n");
            break;
        }
        case TypeSpecifier_Imaginary: {
            printIndent(indent);
            printDebug("imaginary\n");
            break;
        }
        case TypeSpecifier_AtomicType: {
            printIndent(indent);
            printDebug("Atomic type specifier:\n");
            printTypeName(type.atomicName, indent + BaseIndent);
            break;
        }
        case TypeSpecifier_StructOrUnion: {
            printStructOrUnionSpecifier(type.structOrUnion, indent + BaseIndent);
            break;
        }
        case TypeSpecifier_Enum: {
            printEnumSpecifier(type.enumSpecifier, indent + BaseIndent);
            break;
        }
        case TypeSpecifier_TypedefName: {
            printIndent(indent);
            printDebug("Typedefed name: %.*s\n", astr_format(type.typedefName));
            break;
        }
        default: {
            assert(false);
            break;
        }
    }
}

void printStorageClassSpecifier(StorageClassSpecifier storage, uint64_t indent) {
    printIndent(indent);
    switch (storage) {
        case StorageClass_Typedef: {
            printDebug("typedef\n");
            break;
        }
        case StorageClass_Extern: {
            printDebug("extern\n");
            break;
        }
        case StorageClass_Static: {
            printDebug("static\n");
            break;
        }
        case StorageClass_Thread_Local: {
            printDebug("thread_local\n");
            break;
        }
        case StorageClass_Auto: {
            printDebug("auto\n");
            break;
        }
        case StorageClass_Register: {
            printDebug("register\n");
            break;
        }
        default: {
            assert(false);
            break;
        }
    }
}

void printFunctionSpecifier(FunctionSpecifier func, uint64_t indent) {
    printIndent(indent);
    if (func == FunctionSpecifier_Inline) {
        printDebug("inline\n");
    }
    else if (func == FunctionSpecifier_Noreturn) {
        printDebug("noreturn\n");
    }
    else {
        assert(false);
    }
}

void printAlignmentSpecifier(AlignmentSpecifier align, uint64_t indent) {
    printIndent(indent);
    if (align.type == AlignmentSpecifier_TypeName) {
        printDebug("Align specifier type name\n");
        printTypeName(align.typeName, indent + BaseIndent);
    }
    else if (align.type == AlignmentSpecifier_Constant) {
        printDebug("Align specifier constant\n");
        printConditionalExpr(align.constant, indent + BaseIndent);
    }
    else {
        assert(false);
    }
}

void printDeclarationSpecifier(DeclarationSpecifier decl, uint64_t indent) {
    switch (decl.type) {
        case DeclarationSpecifier_StorageClass: {
            printStorageClassSpecifier(decl.storageClass, indent);
            break;
        }
        case DeclarationSpecifier_Type: {
            printTypeSpecifier(decl.typeSpecifier, indent);
            break;
        }
        case DeclarationSpecifier_TypeQualifier: {
            printTypeQualifier(decl.typeQualifier, indent);
            break;
        }
        case DeclarationSpecifier_Func: {
            printFunctionSpecifier(decl.function, indent);
            break;
        }
        case DeclarationSpecifier_Alignment: {
            printAlignmentSpecifier(decl.alignment, indent);
            break;
        }
        default: {
            assert(false);
            break;
        }
    }
}

void printDeclarationSpecifierList(DeclarationSpecifierList list, uint64_t indent) {
    printIndent(indent);
    printDebug("Declaration Specifier List: %ld\n", list.list.size);
    sll_foreach(list.list, node) {
        DeclarationSpecifier *decl = slNode_getData(node);
        printDeclarationSpecifier(*decl, indent + BaseIndent);
    }
}

void printInitDeclarator(InitDeclarator init, uint64_t indent) {
    printIndent(indent);
    printDebug("Init Declarator\n");
    printDeclarator(init.decl, indent + BaseIndent);
    if (init.hasInitializer)
        printInitializer(init.initializer, indent + BaseIndent);
}

void printInitDeclaratorList(InitDeclaratorList list, uint64_t indent) {
    printIndent(indent);
    printDebug("Init declarator list: %lu\n", list.list.size);
    sll_foreach(list.list, node) {
        InitDeclarator *init = slNode_getData(node);
        printInitDeclarator(*init, indent + BaseIndent);
    }
}

void printDeclaration(Declaration decl, uint64_t indent) {
    if (decl.type == Declaration_StaticAssert) {
        printStaticAssertDeclaration(decl.staticAssert, indent);
    }
    else if (decl.type == Declaration_Normal) {
        printIndent(indent);
        printDebug("Decl:\n");
        printDeclarationSpecifierList(decl.declSpecifiers, indent + BaseIndent);
        if (decl.hasInitDeclaratorList) {
            printInitDeclaratorList(decl.initDeclaratorList, indent + BaseIndent);
        }
    }
    else {
        assert(false);
    }
}

void printLabeledStatement(LabeledStatement label, uint64_t indent) {
    printIndent(indent);
    switch (label.type) {
        case LabeledStatement_Ident: {
            printDebug("Label: %.*s\n", astr_format(label.ident));
            break;
        }
        case LabeledStatement_Case: {
            printDebug("case:\n");
            printConditionalExpr(label.caseConstExpr, indent + BaseIndent);
            break;
        }
        case LabeledStatement_Default: {
            printDebug("default\n");
            break;
        }
        default: {
            assert(false);
            break;
        }
    }
}

void printSelectionStatement(SelectionStatement sel, uint64_t indent) {
    printIndent(indent);
    switch (sel.type) {
        case SelectionStatement_If: {
            printDebug("if\n");
            printExpr(sel.ifExpr, indent + BaseIndent);
            printStatement(*(sel.ifTrueStmt), indent + BaseIndent);

            if (sel.ifHasElse) {
                printIndent(indent);
                printDebug("else\n");
                printStatement(*(sel.ifFalseStmt), indent + BaseIndent);
            }

            break;
        }
        case SelectionStatement_Switch: {
            printDebug("switch\n");
            printExpr(sel.switchExpr, indent + BaseIndent);
            printStatement(*(sel.switchStmt), indent + BaseIndent);
            break;
        }
        default: {
            assert(false);
            break;
        }
    }
}

void printExpressionStatement(ExpressionStatement expr, uint64_t indent) {
    if (expr.isEmpty) {
        printIndent(indent);
        printDebug(";\n");
    }
    else {
        printExpr(expr.expr, indent);
    }
}

void printIterationStatement(IterationStatement iter, uint64_t indent) {
    printIndent(indent);
    switch (iter.type) {
        case IterationStatement_While: {
            printDebug("while\n");
            printExpr(iter.whileExpr, indent + BaseIndent);
            printStatement(*(iter.whileStmt), indent + BaseIndent);
            break;
        }
        case IterationStatement_DoWhile: {
            printDebug("do while\n");
            printStatement(*(iter.doStmt), indent + BaseIndent);
            printExpr(iter.doExpr, indent + BaseIndent);
            break;
        }
        case IterationStatement_For: {
            printDebug("for\n");
            if (iter.forHasInitialDeclaration)
                printDeclaration(iter.forInitialDeclaration, indent + BaseIndent);
            else
                printExpressionStatement(iter.forInitialExprStmt, indent + BaseIndent);

            printExpressionStatement(iter.forInnerExprStmt, indent + BaseIndent);

            if (iter.forHasFinalExpr)
                printExpr(iter.forFinalExpr, indent + BaseIndent);

            printStatement(*(iter.forStmt), indent + BaseIndent);

            break;
        }
        default: {
            assert(false);
            break;
        }
    }
}

void printJumpStatement(JumpStatement jump, uint64_t indent) {
    printIndent(indent);
    switch (jump.type) {
        case JumpStatement_Goto: {
            printDebug("goto %.*s\n", astr_format(jump.gotoIdent));
            break;
        }
        case JumpStatement_Continue: {
            printDebug("continue\n");
            break;
        }
        case JumpStatement_Break: {
            printDebug("break\n");
            break;
        }
        case JumpStatement_Return: {
            printDebug("return\n");

            if (jump.returnHasExpr) {
                printExpr(jump.returnExpr, indent + BaseIndent);
            }

            break;
        }
        default: {
            assert(false);
            break;
        }
    }
}

void printAsmStatement(AsmStatement stmt, uint64_t indent) {
    printIndent(indent);
    printDebug("asm statement\n");
}

void printStatement(Statement stmt, uint64_t indent) {
    switch (stmt.type) {
        case Statement_Labeled: {
            printLabeledStatement(stmt.labeled, indent);
            break;
        }
        case Statement_Compound: {
            printCompoundStmt(*(stmt.compound), indent);
            break;
        }
        case Statement_Expression: {
            printExpressionStatement(stmt.expression, indent);
            break;
        }
        case Statement_Selection: {
            printSelectionStatement(stmt.selection, indent);
            break;
        }
        case Statement_Iteration: {
            printIterationStatement(stmt.iteration, indent);
            break;
        }
        case Statement_Jump: {
            printJumpStatement(stmt.jump, indent);
            break;
        }
        case Statement_Asm: {
            printAsmStatement(stmt.assembly, indent);
            break;
        }
        default: {
            assert(false);
            break;
        }
    }
}

void printBlockItem(BlockItem item, uint64_t indent) {
    if (item.type == BlockItem_Declaration) {
        printDeclaration(item.decl, indent);
    }
    else if (item.type == BlockItem_Statement) {
        printStatement(item.stmt, indent);
    }
    else {
        assert(false);
    }
}

void printBlockItemList(BlockItemList list, uint64_t indent) {
    printIndent(indent);
    printDebug("Block Item List: %lu\n", list.list.size);

    sll_foreach(list.list, node) {
        BlockItem *item = slNode_getData(node);
        printBlockItem(*item, indent + BaseIndent);
    }
}

void printCompoundStmt(CompoundStmt stmt, uint64_t indent) {
    if (stmt.isEmpty) {
        printIndent(indent);
        printDebug("{ }\n");
    }
    else {
        printIndent(indent);
        printDebug("{\n");

        printBlockItemList(stmt.blockItemList, indent + BaseIndent);

        printIndent(indent);
        printDebug("}\n");
    }
}

void printFuncDef(FuncDef def, uint64_t indent) {
    printIndent(indent);
    printDebug("FuncDef:\n");

    uint64_t newIndent = indent + BaseIndent;
    printDeclarationSpecifierList(def.specifiers, newIndent);
    printDeclarator(def.declarator, newIndent);

    printIndent(newIndent);
    printDebug("Declarations: %ld\n", def.declarations.size);

    sll_foreach(def.declarations, node) {
        Declaration *decl = slNode_getData(node);
        printDeclaration(*decl, newIndent + BaseIndent);
    }

    printCompoundStmt(def.stmt, newIndent);
}

void printExternalDecl(ExternalDecl decl, uint64_t indent) {
    if (decl.type == ExternalDecl_FuncDef)
        printFuncDef(decl.func, indent);
    else if (decl.type == ExternalDecl_Decl)
        printDeclaration(decl.decl, indent);
    else {
        assert(false);
    }
}

void printTranslationUnit(TranslationUnit translationUnit) {
    printDebug("Translation Unit:\n");

    sll_foreach(translationUnit.externalDecls, node) {
        ExternalDecl *decl = slNode_getData(node);
        printExternalDecl(*decl, BaseIndent);
    }
}
