#include "parser.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "array.h"
#include "debug.h"

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

typedef struct {
    bool success;
    char *failMessage;
} ParseRes;

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

        ArrayAppend(argExprList->argExprs,
            argExprList->argExprLength,
            expr);

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

    return (ParseRes){
        .success = false,
        .failMessage = "Expected . or [ to start a designator"
    };
}

ParseRes parseDesignation(TokenList *tokens, Designation *designation) {
    // Parse designator list
    ParseRes res = {0};
    do {
        size_t pos = tokens->pos;

        Designator designator = {0};
        res = parseDesignator(tokens, &designator);
        if (!res.success) {
            tokens->pos = pos;
            break;
        }

        ArrayAppend(designation->designators,
            designation->numDesignators,
            designator);

    } while (res.success);

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

        ArrayAppend(list->initializers, list->numInitializers, wholeInitializer);

        // Parse a ,
        hasComma = consumeIfTok(tokens, ',');

    } while (hasComma);

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

        ArrayAppend(generic->associations, generic->numAssociations, association);

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
        Token tok = consumeTok(tokens);

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
        
        primary->type = PrimaryExpr_Expr;
        primary->expr = malloc(sizeof(expr));
        memcpy(primary->expr, &expr, sizeof(expr));
        return (ParseRes){ .success = true };
    }
    else if (peekTok(tokens).type == Token_ConstNumeric) {
        Token tok = consumeTok(tokens);

        primary->type = PrimaryExpr_Constant;
        // FIXME: Add in support for floats too
        primary->constant.type = Constant_Integer;
        primary->constant.data = tok.numericWhole;
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

        ArrayAppend(postfixExpr->postfixOps,
            postfixExpr->numPostfixOps,
            op);
    } while (res.success);

    return (ParseRes){ .success = true };
}

ParseRes parseUnaryExprPrefix(TokenList *tokens, UnaryExprPrefix *prefix) {
    if (consumeIfTok(tokens, Token_IncOp)) {
        prefix->type = UnaryPrefix_Inc;
        return (ParseRes){ .success = true };
    }
    if (consumeIfTok(tokens, Token_DecOp)) {
        prefix->type = UnaryPrefix_Dec;
        return (ParseRes){ .success = true };
    }
    if (consumeIfTok(tokens, Token_sizeof)) {
        prefix->type = UnaryPrefix_Sizeof;
        return (ParseRes){ .success = true };
    }
    // TODO: These are all the same, figure out a way to combine them
    if (consumeIfTok(tokens, '&')) {
        prefix->type = UnaryPrefix_Cast;
        prefix->castType = UnaryPrefixCast_And;

        size_t pos = tokens->pos;

        prefix->castExpr = malloc(sizeof(CastExpr));
        ParseRes res = parseCastExpr(tokens, prefix->castExpr);
        if (!res.success) {
            tokens->pos = pos;
            return res;
        }
        return (ParseRes){ .success = true };
    }
    if (consumeIfTok(tokens, '*')) {
        prefix->type = UnaryPrefix_Cast;
        prefix->castType = UnaryPrefixCast_Star;

        size_t pos = tokens->pos;

        prefix->castExpr = malloc(sizeof(CastExpr));
        ParseRes res = parseCastExpr(tokens, prefix->castExpr);
        if (!res.success) {
            tokens->pos = pos;
            return res;
        }
        return (ParseRes){ .success = true };
    }
    if (consumeIfTok(tokens, '+')) {
        prefix->type = UnaryPrefix_Cast;
        prefix->castType = UnaryPrefixCast_Plus;

        size_t pos = tokens->pos;

        prefix->castExpr = malloc(sizeof(CastExpr));
        ParseRes res = parseCastExpr(tokens, prefix->castExpr);
        if (!res.success) {
            tokens->pos = pos;
            return res;
        }
        return (ParseRes){ .success = true };
    }
    if (consumeIfTok(tokens, '-')) {
        prefix->type = UnaryPrefix_Cast;
        prefix->castType = UnaryPrefixCast_Minus;

        size_t pos = tokens->pos;

        prefix->castExpr = malloc(sizeof(CastExpr));
        ParseRes res = parseCastExpr(tokens, prefix->castExpr);
        if (!res.success) {
            tokens->pos = pos;
            return res;
        }
        return (ParseRes){ .success = true };
    }
    if (consumeIfTok(tokens, '~')) {
        prefix->type = UnaryPrefix_Cast;
        prefix->castType = UnaryPrefixCast_Tilde;

        size_t pos = tokens->pos;

        prefix->castExpr = malloc(sizeof(CastExpr));
        ParseRes res = parseCastExpr(tokens, prefix->castExpr);
        if (!res.success) {
            tokens->pos = pos;
            return res;
        }
        return (ParseRes){ .success = true };
    }
    if (consumeIfTok(tokens, '!')) {
        prefix->type = UnaryPrefix_Cast;
        prefix->castType = UnaryPrefixCast_Not;

        size_t pos = tokens->pos;

        prefix->castExpr = malloc(sizeof(CastExpr));
        ParseRes res = parseCastExpr(tokens, prefix->castExpr);
        if (!res.success) {
            tokens->pos = pos;
            return res;
        }
        return (ParseRes){ .success = true };
    }

    return (ParseRes){
        .success = false,
        .failMessage = "Couldn't find unary expr prefix"
    };
}

ParseRes parseUnaryExpr(TokenList *tokens, UnaryExpr *unaryExpr) {
    // Look for optional prefixes
    ParseRes res = {0};
    do {
        size_t prefixPos = tokens->pos;

        UnaryExprPrefix prefixExpr = {0};
        res = parseUnaryExprPrefix(tokens, &prefixExpr);
        if (!res.success) {
            tokens->pos = prefixPos;
            break;
        }

        ArrayAppend(unaryExpr->prefixes,
            unaryExpr->numPrefixes,
            prefixExpr);
    } while (res.success);

    // TODO: Examine if this sizeof parsing generates the correct tree

    // Check to see if this is a sizeof expr
    size_t preSizeofPos = tokens->pos;
    if (!consumeIfTok(tokens, Token_sizeof))
        goto UnaryExpr_AfterSizeof;

    if (!consumeIfTok(tokens, '('))
        goto UnaryExpr_AfterSizeof;

    TypeName sizeofTypeName = {0};
    if (!parseTypeName(tokens, &sizeofTypeName).success)
        goto UnaryExpr_AfterSizeof;

    if (!consumeIfTok(tokens, ')'))
        goto UnaryExpr_AfterSizeof;

    unaryExpr->type = UnaryExpr_Sizeof;
    unaryExpr->sizeofType = malloc(sizeof(sizeofTypeName));
    memcpy(unaryExpr->sizeofType, &sizeofTypeName, sizeof(sizeofTypeName));

    return (ParseRes){ .success = true };

UnaryExpr_AfterSizeof:
    tokens->pos = preSizeofPos;

    // Check to see if this is a alignof expr
    preSizeofPos = tokens->pos;
    if (!consumeIfTok(tokens, Token_alignof))
        goto UnaryExpr_AfterAlignof;

    if (!consumeIfTok(tokens, '('))
        goto UnaryExpr_AfterAlignof;

    TypeName alignofTypeName = {0};
    if (!parseTypeName(tokens, &alignofTypeName).success)
        goto UnaryExpr_AfterAlignof;

    if (!consumeIfTok(tokens, ')'))
        goto UnaryExpr_AfterAlignof;

    unaryExpr->type = UnaryExpr_Alignof;
    unaryExpr->sizeofType = malloc(sizeof(alignofTypeName));
    memcpy(unaryExpr->sizeofType, &alignofTypeName, sizeof(alignofTypeName));

    return (ParseRes){ .success = true };

UnaryExpr_AfterAlignof:
    tokens->pos = preSizeofPos;

    PostfixExpr postfixExpr = {0};
    ParseRes postfixRes = parsePostfixExpr(tokens, &postfixExpr);
    if (postfixRes.success) {
        unaryExpr->type = UnaryExpr_Postfix;
        unaryExpr->expr = postfixExpr;
        return (ParseRes){ .success = true };
    }

    return postfixRes;
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
        return castRes;

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

        ArrayAppend(multiplicativeExpr->postExprs,
            multiplicativeExpr->numPostExprs,
            post);
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

        ArrayAppend(additiveExpr->postExprs,
            additiveExpr->numPostExprs,
            post);
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
        peekTok(tokens).type == Token_GEqOp)
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

        ArrayAppend(shiftExpr->postExprs,
            shiftExpr->numPostExprs,
            post);
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

        ArrayAppend(relExpr->postExprs,
            relExpr->numPostExprs,
            post);
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

        ArrayAppend(eqExpr->postExprs,
            eqExpr->numPostExprs,
            post);
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

        ArrayAppend(andExpr->exprs,
            andExpr->numExprs,
            eqExpr);

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

        ArrayAppend(exclusiveOr->exprs,
            exclusiveOr->numExprs,
            andExpr);

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

        ArrayAppend(inclusiveOr->exprs,
            inclusiveOr->numExprs,
            orExpr);

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

        ArrayAppend(logicalAnd->exprs,
            logicalAnd->numExprs,
            orExpr);

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

        ArrayAppend(logicalOr->exprs,
            logicalOr->numExprs,
            andExpr);

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
        if (!res.success) {
            tokens->pos = preAssignPos;
        }

        AssignPrefix leftExpr = { unaryExpr, op };

        ArrayAppend(assignExpr->leftExprs,
            assignExpr->numAssignOps,
            leftExpr);
    } while (assignRes.success);

    // Parse a conditional expr
    ConditionalExpr conditional = {0};
    ParseRes res = parseConditionalExpr(tokens, &conditional);
    if (!res.success)
        return res;

    assignExpr->rightExpr = conditional;

    return (ParseRes){ .success = true };
}

ParseRes parseExpr(TokenList *tokens, Expr *expr) {
    bool hasComma = false;

    do {
        size_t pos = tokens->pos;

        AssignExpr assign = {0};
        ParseRes res = parseAssignExpr(tokens, &assign);
        if (!res.success) {
            tokens->pos = pos;
            break;
        }

        ArrayAppend(expr->exprs, expr->numExprs, assign);

        hasComma = consumeIfTok(tokens, ',');

    } while(hasComma);

    if (expr->numExprs == 0) {
        return (ParseRes) {
            .success = false,
            .failMessage = "Expr needs at least one assign expr"
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

        ArrayAppend(list->paramDecls, list->numParamDecls, decl);

        hasComma = consumeIfTok(tokens, ',');

    } while(hasComma);
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

            ArrayAppend(postDeclarator->bracketTypeQualifiers,
                postDeclarator->bracketTypeQualifierListLength,
                typeQualifier);
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
            if (postDeclarator->bracketTypeQualifierListLength == 0 &&
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

        ArrayAppend(directDeclarator->postDirectAbstractDeclarators,
            directDeclarator->numPostDirectAbstractDeclarators,
            postDeclarator);
    } while (res.success);

    if (!directDeclarator->hasAbstractDeclarator &&
        directDeclarator->numPostDirectAbstractDeclarators == 0)
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

        ArrayAppend(pointer->typeQualifiers, pointer->numTypeQualifiers, typeQualifier);
    } while (typeQualifierRes.success);

    // If no type qualifiers, cannot have trailing pointer
    if (pointer->numTypeQualifiers == 0) {
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

                ArrayAppend(postDeclarator->bracketTypeQualifiers,
                    postDeclarator->bracketNumTypeQualifiers,
                    typeQualifier);
            } while (res.success);

            // Case 3a: early terminate if:
                // no initial static
                // typequalifier len != 0
                // star after
                // ] after star
            if (!postDeclarator->bracketHasInitialStatic &&
                postDeclarator->bracketNumTypeQualifiers > 0 &&
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
                if (postDeclarator->bracketNumTypeQualifiers == 0 &&
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

        // Empty paren early terminate
        if (consumeIfTok(tokens, ')')) {
            postDeclarator->parenNumIdents = 0;
        }

        bool hasComma = false;
        do {
            Token tok = peekTok(tokens);
            if (!consumeIfTok(tokens, Token_Ident)) {
                return (ParseRes) {
                    .success = false,
                    .failMessage = "Expected identifier in identifier list in post direct declarator"
                };
            }

            ArrayAppend(postDeclarator->parenIdents,
                postDeclarator->parenNumIdents,
                tok.ident);

            hasComma = consumeIfTok(tokens, ',');
        } while (hasComma);
    
        return (ParseRes) { .success = true };
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
    else if (peekTok(tokens).type == '(') {
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
        memcpy(directDeclarator->declarator, &directDeclarator, sizeof(directDeclarator));
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

        ArrayAppend(directDeclarator->postDirectDeclarators,
            directDeclarator->numPostDirectDeclarators,
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
        ArrayAppend(outList->specifierQualifiers,
            outList->numSpecifierQualifiers, specifierQualifier);
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

        ArrayAppend(declList->structDeclarators,
            declList->numStructDeclarators,
            decl);

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
        declaration->staticAsset = staticAssert;

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
            
            ArrayAppend(structOrUnion->structDeclarations,
                structOrUnion->numStructDecls,
                decl);
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
    bool hasComma = false;
    do {

        Enumerator enumerator = {0};
        ParseRes enumRes = parseEnumerator(tokens, &enumerator);
        if (!enumRes.success)
            return enumRes;
        
        hasComma = consumeIfTok(tokens, ',');
    
        ArrayAppend(list->enumerators, list->numEnumerators, enumerator);

    } while(hasComma);

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

    // Parse typedef nam
    // FIXME: Keep Track of which names are typedefed
    assert(false);

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
        ArrayAppend(outList->specifiers, outList->numSpecifiers, specifier);
        res = parseDeclarationSpecifier(tokens, &specifier);
    } while (res.success);

    return (ParseRes) { .success = true };
}

ParseRes parseInitDeclarator(TokenList *tokens, InitDeclarator *initDeclarator) {
    Declarator decl = {0};
    ParseRes declaratorRes = parseDeclarator(tokens, &decl);
    
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

        ArrayAppend(initList->initDeclarators,
            initList->numInitDeclarators,
            decl);

        // Parse a ,
        hasComma = consumeIfTok(tokens, ',');

    } while (hasComma);

    // Must have at least one
    if (initList->numInitDeclarators == 0) {
        return (ParseRes) {
            .success = false,
            .failMessage = "Expected init declarators in init declarator list"
        };
    }

    return (ParseRes){ .success = true };
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
        if (constRes.success) {
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

    expression->isEmpty = false;
    expression->expr = expr;

    return (ParseRes){ .success = true };
}

ParseRes parseIterationStatement(TokenList *tokens, IterationStatement *iteration) {
    // Try parse while
    if (consumeIfTok(tokens, Token_while)) {
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
        Statement stmt = {0};
        ParseRes stmtRes = parseStatement(tokens, &stmt);
        if (!stmtRes.success)
            return stmtRes;

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

        ArrayAppend(list->blockItems, list->numBlockItems, item);

    } while(blockItemRes.success);

    // Make sure we have at least one block item
    if (list->numBlockItems == 0) {
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

    if (consumeIfTok(tokens, '}')) {
        outStmt->isEmpty = true;
        return (ParseRes){ .success = true };
    }

    BlockItemList list = {0};
    ParseRes listRes = parseBlockItemList(tokens, &list);
    if (!listRes.success)
        return listRes;

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

        ArrayAppend(outDef->declarations,
            outDef->numDeclarations,
            declaration);

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
    tokens->pos = 0;

    while (tokens->pos < tokens->numTokens) {
        ExternalDecl decl = {0};
        ParseRes res = parseExternalDecl(tokens, &decl);
        if (!res.success) {
            printf("Error: %s\n", res.failMessage);
        }
        ArrayAppend(outUnit->decls, outUnit->numDecls, decl);
    }
}

#define BaseIndent 2

void printIndent(uint64_t indent) {
    for (uint64_t i = 0; i < indent; i++)
        printDebug(" ");
}

void printAndExpr(AndExpr expr, uint64_t indent) {
    for (size_t i = 0; i < expr.numExprs; i++) {
        printEqualityExpr(expr.exprs[i], indent);
        if (i != expr.numExprs - 1) {
            printIndent(indent + BaseIndent);
            printDebug("&\n");
        }
    }
}

void printExclusiveOrExpr(ExclusiveOrExpr expr, uint64_t indent) {
    for (size_t i = 0; i < expr.numExprs; i++) {
        printAndExpr(expr.exprs[i], indent);
        if (i != expr.numExprs - 1) {
            printIndent(indent + BaseIndent);
            printDebug("^\n");
        }
    }
}

void printInclusiveOrExpr(InclusiveOrExpr expr, uint64_t indent) {
    for (size_t i = 0; i < expr.numExprs; i++) {
        printExclusiveOrExpr(expr.exprs[i], indent);
        if (i != expr.numExprs - 1) {
            printIndent(indent + BaseIndent);
            printDebug("|\n");
        }
    }
}

void printLogicalAndExpr(LogicalAndExpr expr, uint64_t indent) {
    for (size_t i = 0; i < expr.numExprs; i++) {
        printInclusiveOrExpr(expr.exprs[i], indent);
        if (i != expr.numExprs - 1) {
            printIndent(indent + BaseIndent);
            printDebug("&&\n");
        }
    }
}

void printLogicalOrExpr(LogicalOrExpr orExpr, uint64_t indent) {
    for (size_t i = 0; i < orExpr.numExprs; i++) {
        printLogicalAndExpr(orExpr.exprs[i], indent);
        if (i != orExpr.numExprs - 1) {
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
    for (size_t i = 0; i < expr.numAssignOps; i++) {
        printUnaryExpr(expr.leftExprs[i].leftExpr, indent);
        printAssignOp(expr.leftExprs[i].op, indent + BaseIndent);
    }
    printConditionalExpr(expr.rightExpr, indent);
}

void printExpr(Expr expr, uint64_t indent) {
    printIndent(indent);
    printDebug("Expr: %ld\n", expr.numExprs);
    for (size_t i = 0; i < expr.numExprs; i++) {
        printAssignExpr(expr.exprs[i]);
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
    printDebug("Parameter Type List: %ld\n", list.numParamDecls);
    for (size_t i = 0; i < list.numParamDecls; i++) {
        printParameterDeclaration(list.paramDecls[i], indent + BaseIndent);
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
            printDebug("Type qualifier list: %ld\n", post.bracketTypeQualifierListLength);
            for (size_t i = 0; i < post.bracketTypeQualifierListLength; i++) {
                printTypeQualifier(post.bracketTypeQualifiers[i], indent + BaseIndent + BaseIndent);
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

    for (size_t i = 0; i < direct.numPostDirectAbstractDeclarators; i++) {
        printPostDirectAbstractDeclarator(direct.postDirectAbstractDeclarators[i], indent + BaseIndent);
    }
}

void printPointer(Pointer pointer, uint64_t indent) {
    printIndent(indent);
    for (size_t i = 0; i < pointer.numPtrs; i++) {
        printDebug("*");
    }

    printDebug("\n");

    for (size_t i = 0; i < pointer.numTypeQualifiers; i++) {
        printTypeQualifier(pointer.typeQualifiers[i], indent + BaseIndent);
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

void printPostDirectDeclarator(PostDirectDeclarator post, uint64_t indent) {
    if (post.type == PostDirectDeclarator_Paren) {
        printIndent(indent);
        if (post.parenNumIdents == 0) {
            printDebug("( )\n");
        }
        else {
            printDebug("(\n");

            for (size_t i = 0; i < post.parenNumIdents; i++) {
                printIndent(indent + BaseIndent);
                printDebug("%s\n", post.parenIdents[i]);
            }

            printIndent(indent);
            printDebug(")\n");
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
            printDebug("Type qualifier list: %ld\n", post.bracketNumTypeQualifiers);
            for (size_t i = 0; i < post.bracketNumTypeQualifiers; i++) {
                printTypeQualifier(post.bracketTypeQualifiers[i], indent + BaseIndent + BaseIndent);
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
        printDebug("%s\n", direct.ident);
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

    printIndent(indent + BaseIndent);
    printDebug("Post Direct Declarator: %ld\n", direct.numPostDirectDeclarators);
    for (size_t i = 0; i < direct.numPostDirectDeclarators; i++) {
        printPostDirectDeclarator(direct.postDirectDeclarators[i], indent + BaseIndent + BaseIndent);
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
    printDebug("Specifier Qualifier List: %ld\n", list.numSpecifierQualifiers);
    for (size_t i = 0; i < list.numSpecifierQualifiers; i++) {
        printSpecifierQualifier(list.specifierQualifiers[i], indent + BaseIndent);
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
    printDebug("Struct Declarator List: %lu\n", list.numStructDeclarators);
    for (size_t i = 0; i < list.numStructDeclarators; i++) {
        printStructDeclarator(list.structDeclarators[i], indent + BaseIndent);
    }
}

void printStaticAssertDeclaration(StaticAssertDeclaration staticAssert, uint64_t indent) {
    printIndent(indent);
    printDebug("Static Assert\n");
    printConditionalExpr(staticAssert.constantExpr, indent + BaseIndent);
    printIndent(indent + BaseIndent);
    printDebug(",\n");
    printIndent(indent + BaseIndent);
    printDebug("%s\n", staticAssert.stringLiteral);
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
        printDebug(" %s\n", structOrUnion.ident);
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

        for (size_t i = 0; i < structOrUnion.numStructDecls; i++) {
            printStructDeclaration(structOrUnion.structDeclarations[i], indent + BaseIndent);
        }

        printIndent(indent);
        printDebug("}\n");
    }
}

void printEnumerator(Enumerator enumer, uint64_t indent) {
    printIndent(indent);
    printDebug("%s", enumer.constantIdent);
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
    printDebug("EnumeratorList: %lu\n", enumList.numEnumerators);
    for (size_t i = 0; i < enumList; i++) {
        printEnumerator(enumList.enumerators[i], indent + BaseIndent);
    }
}

void printEnumSpecifier(EnumSpecifier enumSpec, uint64_t indent) {
    printIndent(indent);
    if (enumSpec.hasIdent)
        printDebug("enum %s\n", enumSpec.ident);
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
            printDebug("void");
            break;
        }
        case TypeSpecifier_Char: {
            printIndent(indent);
            printDebug("char");
            break;
        }
        case TypeSpecifier_Short: {
            printIndent(indent);
            printDebug("short");
            break;
        }
        case TypeSpecifier_Int: {
            printIndent(indent);
            printDebug("int");
            break;
        }
        case TypeSpecifier_Long: {
            printIndent(indent);
            printDebug("long");
            break;
        }
        case TypeSpecifier_Float: {
            printIndent(indent);
            printDebug("float");
            break;
        }
        case TypeSpecifier_Double: {
            printIndent(indent);
            printDebug("double");
            break;
        }
        case TypeSpecifier_Signed: {
            printIndent(indent);
            printDebug("signed");
            break;
        }
        case TypeSpecifier_Unsigned: {
            printIndent(indent);
            printDebug("unsigned");
            break;
        }
        case TypeSpecifier_Bool: {
            printIndent(indent);
            printDebug("bool");
            break;
        }
        case TypeSpecifier_Complex: {
            printIndent(indent);
            printDebug("complex");
            break;
        }
        case TypeSpecifier_Imaginary: {
            printIndent(indent);
            printDebug("imaginary");
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
            printDebug("Typedefed name: %s\n", type.typedefName);
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
    printDebug("Declaration Specifier List: %ld\n", list.numSpecifiers);
    for (size_t i = 0; i < list.numSpecifiers; i++) {
        printDeclarationSpecifier(list.specifiers[i], indent + BaseIndent);
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
    printDebug("Init declarator list: %lu\n", list.numInitDeclarators);
    for (size_t i = 0; i < list.numInitDeclarators; i++) {
        printInitDeclarator(list.initDeclarators[i], indent + BaseIndent);
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
            printDebug("Label: %s\n", label.ident);
            break;
        }
        case LabeledStatement_Case: {
            printDebug("case\n");
            printConditionalExpr(label.caseConstExpr);
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
            printExpr(*(sel.ifExpr), indent + BaseIndent);
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
            printStatement(iter.whileStmt, indent + BaseIndent);
            break;
        }
        case IterationStatement_DoWhile: {
            printDebug("do while\n");
            printStatement(iter.doStmt, indent + BaseIndent);
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
            printDebug("goto %s\n", jump.gotoIdent);
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

void printStatement(Statement stmt, uint64_t indent) {
    switch (stmt.type) {
        case Statement_Labeled: {
            printLabeledStatement(stmt.labeled, indent);
            break;
        }
        case Statement_Compound: {
            printCompoundStatement(stmt.compound, indent);
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
        printStatement(item.stmt);
    }
    else {
        assert(false);
    }
}

void printBlockItemList(BlockItemList list, uint64_t indent) {
    printIndent(indent);
    printDebug("Block Item List: %lu\n", list.numBlockItems);

    for (size_t i = 0; i < list.numBlockItems; i++) {
        printBlockItemIndent(list.blockItems[i], indent + BaseIndent);
    }
}

void printCompoundStmt(CompoundStmt stmt, uint64_t indent) {
    if (stmt.isEmpty) {
        printIndent(indent);
        printDebug("{ }\n");
    }
    else {
        printIndent(indent);
        printDebug("{\n")

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
    printDebug("Declarations: %ld\n", def.numDeclarations);

    for (uint64_t i = 0; i < def.numDeclarations; i++) {
        printDeclaration(def.declarations[i], newIndent + BaseIndent);
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

    for (uint64_t i = 0; i < translationUnit.numDecls; i++) {
        printExternalDecl(translationUnit.decls[i], BaseIndent);
    }
}
