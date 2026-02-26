/**
 * @file parserDeclaration.c
 * @brief Variable and array declaration parsing.
 *
 * Responsibilities:
 *   - parseDeclaration(): const/let variable declarations with optional init
 *   - parseArrayDec(): typed array declarations (e.g., int[5])
 *
 * Declarations sit between statements and expressions in complexity.
 * They rely on parserType.c for type parsing and parserExpression.c for
 * initializer expressions.
 */

#include "parserInternal.h"

#include <stdio.h>

#include <stdlib.h>

/**
 * Array declarations
 */
ASTNode parseArrayDec(TokenList* list, size_t* pos, Token* varName){
    if(*pos >= list->count) return NULL;

    /* parse element type */
    ASTNode typeNode = parseType(list, pos);
    if(!typeNode) return NULL;

    /* expect size */
    EXPECT_AND_ADVANCE(list, pos, TK_LBRACKET, ERROR_EXPECTED_OPENING_BRACKET, "Expected '[' after type in array declaration");

    Token* sizeToken = &list->tokens[*pos];
    NodeTypes sizeType = detectLitType(sizeToken, list, pos);
    if(!isIntTypeNode(sizeType) && sizeType != VARIABLE){
        reportError(ERROR_INVALID_EXPRESSION, createErrorContextFromParser(list, pos), "Array size must be an integer literal or variable");
        freeAST(typeNode);
        return NULL;
    }
    
    ASTNode sizeNode = createValNode(sizeToken, list, pos);
    if(!sizeNode){
        freeAST(typeNode);
        return NULL;
    }

    ADVANCE_TOKEN(list, pos);

    EXPECT_AND_ADVANCE(list, pos, TK_RBRACKET, ERROR_EXPECTED_CLOSING_BRACKET, "Expected ']' after array size");

    /* Build array variable def node */
    ASTNode arrayDefNode;
    CREATE_NODE_OR_FAIL(arrayDefNode, varName, ARRAY_VARIABLE_DEFINITION, list, pos);
    ASTNode typeRefNode;
    // NULL token means error wont be able to report location of the error
    CREATE_NODE_OR_FAIL(typeRefNode, NULL, TYPE_REF, list, pos);
    typeRefNode->children = typeNode;
    typeRefNode->brothers = sizeNode;

    arrayDefNode->children = typeRefNode;
    return arrayDefNode;
}

/**
 * Variable declarations
 */

ASTNode parseDeclaration(TokenList* list, size_t* pos){
    if(*pos >= list->count) return NULL;

    int isConst = list->tokens[*pos].type == TK_CONST;
    Token* mutToken = &list->tokens[*pos];
    ADVANCE_TOKEN(list, pos);

    Token* varName = &list->tokens[*pos];
    if(detectLitType(varName, list, pos) != VARIABLE){
        reportError(ERROR_EXPECTED_IDENTIFIER, createErrorContextFromParser(list, pos), "Expected identifier after const/let");
        return NULL;
    }

    ADVANCE_TOKEN(list, pos);
    EXPECT_AND_ADVANCE(list, pos, TK_COLON, ERROR_EXPECTED_COLON, "Expected ':' after identifier");

    /* Lookahead if its an array declaration*/
    int isArray = 0;
    size_t savedPos = *pos;

    while(*pos < list->count && list->tokens[*pos].type == TK_STAR){
        ADVANCE_TOKEN(list, pos);
    }

    Token* lookAheadToken = &list->tokens[*pos];
    if((*pos < list->count && isTypeToken(lookAheadToken->type)) || detectLitType(lookAheadToken, list, pos) == VARIABLE){
        ADVANCE_TOKEN(list, pos);
        if(*pos < list->count && list->tokens[*pos].type == TK_LBRACKET){
            isArray = 1;
        }
    }

    *pos = savedPos; // restore position after lookahead

    /* mut wrapper */
    ASTNode mutWrapNode;
    CREATE_NODE_OR_FAIL(mutWrapNode, mutToken, isConst ? CONST_DEC : LET_DEC, list, pos);
    ASTNode varDefNode;

    if(isArray){
        varDefNode = parseArrayDec(list, pos, varName);
        if(!varDefNode){
            freeAST(mutWrapNode);
            return NULL;
        }
    } else {
        ASTNode typeNode = parseType(list, pos);
        if(!typeNode){
            freeAST(mutWrapNode);
            return NULL;
        }

        ASTNode baseType = typeNode;
        while(baseType && baseType->nodeType == POINTER){
            baseType = baseType->children;
        }

        CREATE_NODE_OR_FAIL(varDefNode, varName, VAR_DEFINITION, list, pos);
        ASTNode typeRefWrapNode;
        // NULL token means error wont be able to report location of the error
        CREATE_NODE_OR_FAIL(typeRefWrapNode, NULL, TYPE_REF, list, pos);
        typeRefWrapNode->children = typeNode;
        varDefNode->children = typeRefWrapNode;
    }
    mutWrapNode->children = varDefNode;

    /* Optional initializer */

    if(*pos < list->count && list->tokens[*pos].type == TK_ASSIGN){
        ADVANCE_TOKEN(list, pos);
        int isRef = *pos < list->count && list->tokens[*pos].type == TK_AMPERSAND;
        Token* refToken = NULL;
        if(isRef){
            refToken = &list->tokens[*pos];
            ADVANCE_TOKEN(list, pos);
        }

        ASTNode initExpression, valueWrap;
        /* NUll token means error won't be able to report location of the error */
        CREATE_NODE_OR_FAIL(valueWrap, NULL, VALUE, list, pos);
        PARSE_OR_FAIL(initExpression, parseExpression(list, pos, PREC_NONE));
        valueWrap->children = initExpression;

        /* attach value to the correct place */
        if (isArray) {
            if (varDefNode->children && varDefNode->children->brothers) {
                ASTNode lastBrother = varDefNode->children->brothers;
                while (lastBrother->brothers)
                    lastBrother = lastBrother->brothers;
                lastBrother->brothers = valueWrap;
            } else if (varDefNode->children) {
                varDefNode->children->brothers = valueWrap;
            }
        } else {
            ASTNode typeRefWrapNode = varDefNode->children;
            if (typeRefWrapNode) {
                typeRefWrapNode->brothers = valueWrap;
            }
        }

        if(isRef){
            ASTNode memNode;
            CREATE_NODE_OR_FAIL(memNode, refToken, MEMADDRS, list, pos);
            memNode->children = valueWrap->children;
            valueWrap->children = memNode;
        }
    } else if(isConst){
        reportError(ERROR_CONST_MUST_BE_INITIALIZED, createErrorContextFromParser(list, pos), "Const declarations must have an initializer");
        freeAST(mutWrapNode);
        return NULL;
    }

    EXPECT_AND_ADVANCE(list, pos, TK_SEMI, ERROR_EXPECTED_SEMICOLON, "Expected ';' after variable declaration");
    return mutWrapNode;
}

