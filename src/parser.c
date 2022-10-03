#include "parser.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "array.h"

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
    else if (consumeIfTok(tokens, Token_MulEqOp)) {
        *op = Assign_MulEq;
    }
    else if (consumeIfTok(tokens, Token_DivEqOp)) {
        *op = Assign_DivEq;
    }
    else if (consumeIfTok(tokens, Token_ModEqOp)) {
        *op = Assign_ModEq;
    }
    else if (consumeIfTok(tokens, Token_AddEqOp)) {
        *op = Assign_AddEq;
    }
    else if (consumeIfTok(tokens, Token_SubEqOp)) {
        *op = Assign_SubEq;
    }
    else if (consumeIfTok(tokens, Token_ShiftLeftEqOp)) {
        *op = Assign_ShiftLeftEq;
    }
    else if (consumeIfTok(tokens, Token_ShiftRightEqOp)) {
        *op = Assign_ShiftRightEq;
    }
    else if (consumeIfTok(tokens, Token_AndEqOp)) {
        *op = Assign_AndEq;
    }
    else if (consumeIfTok(tokens, Token_XorEqOp)) {
        *op = Assign_XorEq;
    }
    else if (consumeIfTok(tokens, Token_OrEqOp)) {
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
    if (parseAbstractDeclarator(tokens, &abstractDeclarator)) {
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
        if (consumeIfTok(tokens, Token_EllipsisOp)) {
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
                peekTokAhead(tokens, 0).type == '*' &&
                peekTokAhead(tokens, 1).type == ']')
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
    return (ParseRes){ .success = true; }
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
    if (peek(tokens, Token_staticAssert)) {
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
        declaration->normalHasStructDeclaratorList = declList;
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
        if (!listRes)
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
    assert(false);

    return fail;
}

ParseRes parseDeclarationSpecifier(TokenList *tokens,
    DeclarationSpecifier *outSpec)
{
    // Try parse storage class specifier
    StorageClassSpecifier storageClass = {0};
    ParseRes storageClassRes = parseStorageClassSpecifier(tokens, &storageClass);
    if (storageClassRes.success) {
        outSpec->type = DeclarationSpecifier_StorageClass;
        outSpec->storageClass = storageClass;
        return (ParseRes){ .success = true };
    }

    // Try parse type specifier
    TypeSpecifier type = {0};
    ParseRes typeRes = parseTypeSpecifier(tokens, &type);
    if (typeRes.success) {
        outSpec->type = DeclarationSpecifier_Type;
        outSpec->typeSpecifier = type;
        return (ParseRes){ .success = true };
    }

    // Try parse type qualifier
    assert(false);

    // Try parse function specifier
    assert(false);

    // Try parse alignment specifier
    assert(false);

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
    assert(false);

    // Parse Opt Declaration List
    assert(false);

    // Parse Compound Statement
    assert(false);

    return (ParseRes){0};
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
    assert(false);
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
