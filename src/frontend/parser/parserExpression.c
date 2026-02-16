#include "parserInternal.h"

#include <stdio.h>

/**
 * forward declarations
 */

static ASTNode parseCastExpression(TokenList* list, size_t* pos, ASTNode node);

/**
 * @brief Parses lowest levels expressions: literals, variables, function calls, ...
 */
ASTNode parsePrimaryExp(TokenList* list, size_t* pos){
    if(*pos >= list->count) return NULL;
    Token* token = &list->tokens[*pos];

    /**
     * Pointer dereference or address-of as prefixes
     */
    if(token->type == TK_STAR || token->type == TK_AMPERSAND){
        Token* opToken = token;
        uint8_t isPointer = token->type == TK_STAR;
        ADVANCE_TOKEN(list, pos);

        ASTNode operand = parsePrimaryExp(list, pos);
        if(!operand) return NULL;

        ASTNode memWrap;
        CREATE_NODE_OR_FAIL(memWrap, opToken, isPointer ? POINTER : MEMADDRS, list, pos);
        memWrap->children = operand;
        return memWrap;
    }

    /**
     * Arrays literals
     */
    if(token->type == TK_LBRACKET){
        return parseArrLit(list, pos);
    }

    /**
     * Struct literal { field: value, ...}
     */
    if(token->type == TK_LBRACE){
        return parseStructLit(list, pos);
    }

    /**
     * Null literal
     */
    if(token->type == TK_NULL){
        ASTNode nullNode;
        CREATE_NODE_OR_FAIL(nullNode, token, NULL_LIT, list, pos);
        ADVANCE_TOKEN(list, pos);
        return nullNode;
    }

    /**
     * Parenthesized expression
     */
    if(token->type == TK_LPAREN){
        ADVANCE_TOKEN(list, pos);
        ASTNode expr = parseExpression(list, pos, PREC_NONE);
        if(!expr) return NULL;
        EXPECT_AND_ADVANCE(list, pos, TK_RPAREN, ERROR_EXPECTED_CLOSING_PAREN, "Expected ')' after expression");
        return expr;
    }

    /**
     * Function call
     */
    if(detectLitType(token, list, pos) == VARIABLE && (*pos + 1 < list->count) && list->tokens[*pos + 1].type == TK_LPAREN){
        Token* fnNameToken = token;
        ADVANCE_TOKEN(list, pos); // consume function name
        ASTNode funcCall = parseFunctionCall(list, pos, fnNameToken);
        return funcCall;
    }

    // Fallback to lit or variable
    ASTNode node = createValNode(token, list, pos);
    if(!node) return NULL;
    ADVANCE_TOKEN(list, pos);

    while(*pos < list->count){
        if(list->tokens[*pos].type == TK_DOT){
            ADVANCE_TOKEN(list, pos);
            if(detectLitType(&list->tokens[*pos], list, pos) != VARIABLE){
                reportError(ERROR_EXPECTED_MEMBER_NAME, createErrorContextFromParser(list, pos), "Expected member name after '.'");
                freeAST(node);
                return NULL;
            }

            Token* memberToken = &list->tokens[*pos];
            ADVANCE_TOKEN(list, pos);

            ASTNode memberAccess, memberNode;
            CREATE_NODE_OR_FAIL(memberAccess, memberToken, MEMBER_ACCESS, list, pos);
            CREATE_NODE_OR_FAIL(memberNode, memberToken, VARIABLE, list, pos);

            memberAccess->children = node;
            node->brothers = memberNode;
            node = memberAccess;
        } else if(list->tokens[*pos].type == TK_LBRACKET){
            node = parseArrayAccess(list, pos, node);
            if(!node) return NULL;
        }else{
            break;
        }
    }
    return node;
}

/**
 * Unary expressions
 */

ASTNode parseUnary(TokenList *list, size_t *pos) {
    if (*pos >= list->count) return NULL;
    Token *token = &list->tokens[*pos];

    /* Prefix operators */
    if (token->type == TK_MINUS || token->type == TK_NOT || token->type == TK_INCR ||
        token->type == TK_DECR || token->type == TK_PLUS || token->type == TK_BIT_NOT) {
        Token *opToken = token;
        ADVANCE_TOKEN(list, pos);

        ASTNode operand, opNode;
        PARSE_OR_FAIL(operand, parseUnary(list, pos));

        NodeTypes opType = getUnaryOpType(opToken->type);
        if (opType == null_NODE) return NULL;

        CREATE_NODE_OR_FAIL(opNode, opToken, opType, list, pos);
        opNode->children = operand;
        return opNode;
    }


    ASTNode node = parsePrimaryExp(list, pos);

    if(*pos < list->count && (list->tokens[*pos].type == TK_INCR || list->tokens[*pos].type == TK_DECR)){
        Token* opToken = &list->tokens[*pos];
        ADVANCE_TOKEN(list, pos);
        ASTNode opNode;
        NodeTypes opType = opToken->type == TK_INCR ? POST_INCREMENT : POST_DECREMENT;
        CREATE_NODE_OR_FAIL(opNode, opToken, opType, list, pos);
        opNode->children = node;
        return opNode;
    }

    return node;
}

/**
 * Casting
 */

static ASTNode parseCastExpression(TokenList* list, size_t* pos, ASTNode node){
    if(*pos >= list->count || !node) return NULL;
    Token* asToken = &list->tokens[*pos];
    ADVANCE_TOKEN(list, pos);

    Token* currentToken = &list->tokens[*pos];
    if(isTypeToken(currentToken->type)){
        ASTNode refTypeNode, castNode;
        CREATE_NODE_OR_FAIL(refTypeNode, currentToken, getTypeNodeFromToken(currentToken->type), list, pos);
        ADVANCE_TOKEN(list, pos);
        CREATE_NODE_OR_FAIL(castNode, asToken, CAST_EXPRESSION, list, pos);
        castNode->children = node;
        node->brothers = refTypeNode;
        return castNode;
    }

    return NULL;
}

/**
 * PRATT PARSER
 */

ASTNode parseExpression(TokenList* list, size_t* pos, Precedence minPrecedence){
    if(*pos >= list->count) return NULL;
    ASTNode left = parseUnary(list, pos);
    if(!left) return NULL;
    while(*pos < list->count){
        Token* currentToken = &list->tokens[*pos];

        /* Ternaries */
        if(currentToken->type == TK_QUESTION && PREC_TERNARY >= minPrecedence){
            ASTNode conditionNode = parseTernary(list, pos);
            if(!conditionNode) return NULL;
            left->brothers = conditionNode->children;
            conditionNode->children = left;
            left = conditionNode;
            if(!left) return NULL;
            continue;
        }

        /* Casts */
        if(currentToken->type == TK_AS && PREC_CAST >= minPrecedence){
            left = parseCastExpression(list, pos, left);
            if(!left) return NULL;
            continue;
        }

        /* Binary operator */
        const OperatorInfo* opInfo = getOperatorInfo(currentToken->type);
        if(!opInfo || opInfo->precedence < minPrecedence) break;

        Precedence nextMinPrecedence = opInfo->isRightAssociative ? 0 : opInfo->precedence + 1;
        Token* opToken = currentToken;
        ADVANCE_TOKEN(list, pos);
        ASTNode rigth = parseExpression(list, pos, nextMinPrecedence);
        if(!rigth) return NULL;

        ASTNode opNode = createNode(opToken, opInfo->nodeType, list, pos);
        opNode->children = left;
        left->brothers = rigth;
        left = opNode;
    }

    return left;
}

/**
 * Ternary
 */

ASTNode parseTernary(TokenList* list, size_t* pos){
    EXPECT_TOKEN(list, pos, TK_QUESTION, ERROR_EXPECTED_QUESTION_MARK, "Expected '?' for ternary operator");
    Token* questionToken = &list->tokens[*pos];
    ADVANCE_TOKEN(list, pos);

    ASTNode trueBranch = parseExpression(list, pos, PREC_NONE);
    if(!trueBranch) {
        reportError(ERROR_TERNARY_INVALID_CONDITION, createErrorContextFromParser(list, pos), "Invalid condition in ternary operator");
        return NULL;
    }

    if(list->tokens[*pos].type != TK_COLON){
        reportError(ERROR_EXPECTED_COLON, createErrorContextFromParser(list, pos), "Missing false branch in ternary operator");
        freeAST(trueBranch);
        return NULL;
    }
    Token* colonToken = &list->tokens[*pos];
    ADVANCE_TOKEN(list, pos);
    ASTNode falseBranch = parseExpression(list, pos, PREC_NONE);

    ASTNode conditionalNode, trueBranchWrap, falseBranchWrap;
    CREATE_NODE_OR_FAIL(conditionalNode, questionToken, TERNARY_CONDITIONAL, list, pos);
    /** 
     * todo: both questionToken and coloToken doesnt point to the actual branches an they should to get better error repoting
     */
    CREATE_NODE_OR_FAIL(trueBranchWrap, questionToken, TERNARY_IF_EXPR, list, pos); 
    CREATE_NODE_OR_FAIL(falseBranchWrap, colonToken, TERNARY_ELSE_EXPR, list, pos);

    trueBranchWrap->children = trueBranch;
    /** 
     * Temporaeily trueBranch is placed as child so parseExpression can re-link it later
     */
    conditionalNode->children = trueBranchWrap;
    falseBranchWrap->children = falseBranch;
    trueBranchWrap->brothers = falseBranchWrap;

    return conditionalNode;
}

/**
 * Block expression
 */

ASTNode parseBlockExpression(TokenList* list, size_t* pos){
    if(*pos >= list->count) return NULL;

    ASTNode block = parseBlock(list, pos);
    if(block) block->nodeType = BLOCK_EXPRESSION;
    return block;
}

/**
 * Array access
 */

ASTNode parseArrLit(TokenList* list, size_t* pos){
    if(*pos >= list->count) return NULL;

    Token* startToken = &list->tokens[*pos];
    EXPECT_AND_ADVANCE(list, pos, TK_LBRACKET, ERROR_EXPECTED_OPENING_BRACKET, "Expected '[' to start array literal");

    ASTNode arrayLitNode;
    CREATE_NODE_OR_FAIL(arrayLitNode, startToken, ARRAY_LIT, list, pos);

    /* Empty array */
    if(*pos < list->count && list->tokens[*pos].type == TK_RBRACKET){
        ADVANCE_TOKEN(list, pos);
        return arrayLitNode;
    }

    ASTNode lastElement = NULL;
    while(*pos < list->count && list->tokens[*pos].type != TK_RBRACKET){
        ASTNode element = parseExpression(list, pos, PREC_NONE);
        if(!element){
            freeAST(arrayLitNode);
            return NULL;
        }

        if(!arrayLitNode->children){
            arrayLitNode->children = element;
            lastElement = element;
        } else {
            lastElement->brothers = element;
            lastElement = element;
        }

        if(*pos < list->count && list->tokens[*pos].type == TK_COMMA){
            ADVANCE_TOKEN(list, pos);
        } else if(*pos < list->count && list->tokens[*pos].type != TK_RBRACKET) {
            reportError(ERROR_EXPECTED_COMMA, createErrorContextFromParser(list, pos), "Expected ',' between array literal elements");
            freeAST(arrayLitNode);
            return NULL;
        }
    }

    EXPECT_AND_ADVANCE(list, pos, TK_RBRACKET, ERROR_EXPECTED_CLOSING_BRACKET, "Expected ']' to close array literal");
    return arrayLitNode;
}

/**
 * Struct literal
 */


ASTNode parseStructLit(TokenList* list, size_t* pos){
    if(*pos >= list->count) return NULL;

    Token* startToken = &list->tokens[*pos];
    EXPECT_AND_ADVANCE(list, pos, TK_LBRACE, ERROR_EXPECTED_OPENING_BRACE, "Expected '{' to start struct literal");

    ASTNode structLitNode;
    CREATE_NODE_OR_FAIL(structLitNode, startToken, STRUCT_LIT, list, pos);

    /* Empty struct literal */
    if(*pos < list->count && list->tokens[*pos].type == TK_RBRACE){
        ADVANCE_TOKEN(list, pos);
        return structLitNode;
    }

    ASTNode lastField = NULL;
    while(*pos < list->count && list->tokens[*pos].type != TK_RBRACE){
        ASTNode fieldNode;
        PARSE_OR_FAIL(fieldNode, parseStructFieldLit(list, pos));

        if(!structLitNode->children){
            structLitNode->children = fieldNode;
        } else {
            lastField->brothers = fieldNode;
        }
        lastField = fieldNode;

        /* Optional comma between fields */
        if(*pos < list->count && list->tokens[*pos].type == TK_COMMA){
            ADVANCE_TOKEN(list, pos);
        }
    }

    EXPECT_AND_ADVANCE(list, pos, TK_RBRACE, ERROR_EXPECTED_CLOSING_BRACE, "Expected '}' to close struct literal");
    return structLitNode;
}

/**
 * Array access
 */

ASTNode parseArrayAccess(TokenList* list, size_t* pos, ASTNode array){
    if (*pos >= list->count || !array) return NULL;

    EXPECT_AND_ADVANCE(list, pos, TK_LBRACKET, ERROR_EXPECTED_OPENING_BRACKET, "Expected '['.");

    ASTNode indexExpr = parseExpression(list, pos, PREC_NONE);
    if (!indexExpr) {
        freeAST(array);
        return NULL;
    }

    EXPECT_AND_ADVANCE(list, pos, TK_RBRACKET, ERROR_EXPECTED_CLOSING_BRACKET,"Expected ']' after array index");

    ASTNode accessNode;
    CREATE_NODE_OR_FAIL(accessNode, NULL, ARRAY_ACCESS, list, pos);
    accessNode->children = array;
    array->brothers = indexExpr;

    return accessNode;
}