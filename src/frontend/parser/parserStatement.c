/**
 * @file parserStatement.c
 * @brief Statement-level parsing and control-flow constructs.
 *
 * Responsibilities:
 *   - parseStatement(): top-level dispatch via statementHandlers[]
 *   - parseExpressionStatement(): expression followed by ';'
 *   - parseBlock(): brace-delimited statement list
 *   - parseIf() / parseLoop() / parseForLoop(): control flow
 *   - parseReturnStatement(): return with optional expression
 *   - parseImport() / parseExportFunction(): module system
 *
 * This file consumes tokens at the statement level and delegates to
 * parserExpression.c for expression sub-trees and parserDeclaration.c for
 * declarations.
 */

#include "parserInternal.h"

#include <stdlib.h>

/**
 * Expression statement
 */

ASTNode parseExpressionStatement(TokenList* list, size_t* pos){
    ASTNode expressionNode;
    PARSE_OR_FAIL(expressionNode, parseExpression(list, pos, PREC_NONE));
    EXPECT_AND_ADVANCE(list, pos, TK_SEMI, ERROR_EXPECTED_SEMICOLON, "Expected ';' after expression");
    return expressionNode;
}

/**
 * Statement dispatch
 */

ASTNode parseStatement(TokenList* list, size_t* pos){
    if(*pos >= list->count) return NULL;

    Token* currentToken = &list->tokens[*pos];

    if(currentToken->type == TK_EOF){
        ADVANCE_TOKEN(list, pos);
        return NULL;
    }

    /* skip semicolons */
    if(currentToken->type == TK_SEMI){
        ADVANCE_TOKEN(list, pos);
        return NULL;
    }

    /* Toble driven statements on parseCore.c */
    for(int i = 0; statementHandlers[i].token != TK_NULL; ++i){
        if(currentToken->type == statementHandlers[i].token){
            return statementHandlers[i].handler(list, pos);
        }
    }

    /* cosnt | let declarations */
    if(currentToken->type == TK_CONST || currentToken->type == TK_LET){
        return parseDeclaration(list, pos);
    }

    return parseExpressionStatement(list, pos);
}

/**
 * Block statements
 */

ASTNode parseBlock(TokenList* list, size_t* pos){
    EXPECT_TOKEN(list, pos, TK_LBRACE, ERROR_EXPECTED_OPENING_BRACE, "Expected '{' to start block");
    ADVANCE_TOKEN(list, pos);

    ASTNode blockNode;
    CREATE_NODE_OR_FAIL(blockNode, &list->tokens[*pos - 1], BLOCK_STATEMENT, list, pos);

    ASTNode lastChild = NULL;
    while(*pos < list->count && list->tokens[*pos].type != TK_RBRACE){
        ASTNode statement = parseStatement(list, pos);
        if(statement){
            if(!blockNode->children){
                blockNode->children = statement;
            } else if (lastChild){
                lastChild->brothers = statement;
            }
            lastChild = statement;
        }
    }

    EXPECT_AND_ADVANCE(list, pos, TK_RBRACE, ERROR_EXPECTED_CLOSING_BRACE, "Expected '}' to close block");

    return blockNode;
}

/**
 * if | else statements
 */

ASTNode parseIf(TokenList* list, size_t* pos){
    Token* ifToken = &list->tokens[*pos];
    ADVANCE_TOKEN(list, pos);

    ASTNode conditionalNode, condition, trueBranchWrap, trueBranch, falseBranch = NULL, falseBranchWrap;

    condition = parseExpression(list, pos, PREC_NONE);
    EXPECT_TOKEN(list, pos, TK_LBRACE, ERROR_EXPECTED_OPENING_BRACE, "Expected '{' to start 'if' block");
    trueBranch = parseBlock(list, pos);

    if(list->tokens[*pos].type == TK_ELSE){
        ADVANCE_TOKEN(list, pos);
        if(list->tokens[*pos].type == TK_IF){
            falseBranch = parseIf(list, pos);
        } else {
            PARSE_OR_CLEANUP(falseBranch, parseBlock(list, pos), condition, trueBranch);
        }
    }

    CREATE_NODE_OR_FAIL(conditionalNode, ifToken, IF_CONDITIONAL, list, pos);
    // NULL token means error wont be able to report location of the error
    CREATE_NODE_OR_FAIL(trueBranchWrap, NULL, IF_TRUE_BRANCH, list, pos);

    conditionalNode->children = condition;
    trueBranchWrap->children = trueBranch;
    condition->brothers = trueBranchWrap;

    if(falseBranch){
        // NULL token means error wont be able to report location of the error
        CREATE_NODE_OR_FAIL(falseBranchWrap, NULL, ELSE_BRANCH, list, pos);
        falseBranchWrap->children = falseBranch;
        trueBranchWrap->brothers = falseBranchWrap;
    }

    return conditionalNode;
}

/** 
 * Loops
 */

ASTNode parseLoop(TokenList* list, size_t* pos){
    if(*pos >= list->count) return NULL;
    Token* loopToken = &list->tokens[*pos];
    ADVANCE_TOKEN(list, pos);

    ASTNode condition, loopBody, loopNode;
    PARSE_OR_FAIL(condition, parseExpression(list, pos, PREC_NONE));
    EXPECT_TOKEN(list, pos, TK_LBRACE, ERROR_EXPECTED_OPENING_BRACE, "Expected '{' to start loop body");
    PARSE_OR_FAIL(loopBody, parseBlock(list, pos));
    CREATE_NODE_OR_FAIL(loopNode, loopToken, LOOP_STATEMENT, list, pos);

    loopNode->children = condition;
    condition->brothers = loopBody;
    return loopNode;
}

ASTNode parseForLoop(TokenList* list, size_t* pos){
    if (*pos >= list->count) return NULL;
    Token *forToken = &list->tokens[*pos];
    ADVANCE_TOKEN(list, pos);

    ASTNode init = NULL, condition = NULL, increment = NULL;
    ASTNode loopBody = NULL, loopNode = NULL, blockNode = NULL;

    EXPECT_AND_ADVANCE(list, pos, TK_LPAREN, ERROR_EXPECTED_OPENING_PAREN, "Expected '(' after 'for'");
    PARSE_OR_FAIL(init, parseStatement(list, pos));
    PARSE_OR_FAIL(condition, parseExpression(list, pos, PREC_NONE));
    EXPECT_AND_ADVANCE(list, pos, TK_SEMI, ERROR_EXPECTED_SEMICOLON, "Expected ';' after loop condition");
    PARSE_OR_FAIL(increment, parseExpression(list, pos, PREC_NONE));
    EXPECT_AND_ADVANCE(list, pos, TK_RPAREN, ERROR_EXPECTED_CLOSING_PAREN, "Expected ')' after for clauses");
    EXPECT_TOKEN(list, pos, TK_LBRACE, ERROR_EXPECTED_OPENING_BRACE, "Expected '{' after for clauses");
    PARSE_OR_FAIL(loopBody, parseBlock(list, pos));

    /* Append increment to end of loop body */
    if (loopBody->children == NULL) {
        loopBody->children = increment;
    } else {
        ASTNode lastChild = loopBody->children;
        while (lastChild->brothers)
            lastChild = lastChild->brothers;
        lastChild->brothers = increment;
    }

    CREATE_NODE_OR_FAIL(loopNode,  forToken, LOOP_STATEMENT,  list, pos);
    loopNode->children  = condition;
    condition->brothers = loopBody;

    CREATE_NODE_OR_FAIL(blockNode, NULL, BLOCK_STATEMENT, list, pos);
    blockNode->children = init;
    init->brothers      = loopNode;

    return blockNode;
}

/**
 * Return statement
 */

ASTNode parseReturnStatement(TokenList* list, size_t* pos){
    EXPECT_TOKEN(list, pos, TK_RETURN, ERROR_EXPECTED_RETURN, "Expected 'return' keyword");
    Token *returnToken = &list->tokens[*pos];
    ADVANCE_TOKEN(list, pos);

    ASTNode returnNode;
    CREATE_NODE_OR_FAIL(returnNode, returnToken, RETURN_STATEMENT, list, pos);

    if (*pos < list->count && list->tokens[*pos].type != TK_SEMI) {
        returnNode->children = parseExpression(list, pos, PREC_NONE);
    }

    EXPECT_AND_ADVANCE(list, pos, TK_SEMI, ERROR_EXPECTED_SEMICOLON, "Expected ';' after return statement");
    return returnNode;
}

/**
 * import | export
 */

ASTNode parseImport(TokenList *list, size_t *pos) {
    EXPECT_TOKEN(list, pos, TK_IMPORT, ERROR_EXPECTED_IMPORT, "Expected 'import'");
    ADVANCE_TOKEN(list, pos);

    EXPECT_TOKEN(list, pos, TK_STR,ERROR_EXPECTED_MODULE_PATH, "Expected module path string after 'import'");
    Token *pathTok = &list->tokens[*pos];

    ASTNode importNode;
    CREATE_NODE_OR_FAIL(importNode, pathTok, IMPORTDEC, list, pos);
    ADVANCE_TOKEN(list, pos);

    EXPECT_AND_ADVANCE(list, pos, TK_SEMI, ERROR_EXPECTED_SEMICOLON, "Expected ';' after import");
    return importNode;
}

ASTNode parseExportFunction(TokenList *list, size_t *pos) {
    EXPECT_TOKEN(list, pos, TK_EXPORT, ERROR_EXPECTED_EXPORT, "Expected 'export'");
    Token *exportTok = &list->tokens[*pos];
    ADVANCE_TOKEN(list, pos);

    ASTNode childNode;

    if (list->tokens[*pos].type == TK_FN) {
        PARSE_OR_FAIL(childNode, parseFunction(list, pos));
    }
    else if (list->tokens[*pos].type == TK_STRUCT) {
        PARSE_OR_FAIL(childNode, parseStruct(list, pos));
    }
    else {
        reportError(ERROR_EXPECTED_FN_AFTER_EXPORT, createErrorContextFromParser(list, pos), "Expected 'fn' or 'struct' after 'export'");
        return NULL;
    }

    ASTNode exportNode;
    CREATE_NODE_OR_FAIL(exportNode, exportTok, EXPORTDEC, list, pos);
    exportNode->children = childNode;
    return exportNode;
}