/**
 * @file parserFunction.c
 * @brief Function definition, function call, and struct definition parsing.
 *
 * Responsibilities:
 *   - parseFunction(): fn name(params) -> retType { body }
 *   - parseFunctionCall(): name(args)
 *   - parseCommaSeparatedLists(): generic ( elem, elem, ... ) parser
 *   - parseParameter() / parseArg(): individual parameter and argument
 *   - parseReturnType(): -> type
 *   - parseStruct(): struct definition with field list
 *   - parseStructField(): single field in a struct definition
 *   - parseStructFieldLit(): single field in a struct literal
 *
 * Functions and structs are grouped together because they share the
 * comma-separated-list pattern and both represent top-level definitions.
 * they can be splitted in the future if logic grows too much.
 */

#include "parserInternal.h"

#include <stdlib.h>

ASTNode parseCommaSeparatedLists(TokenList* list, size_t* pos, NodeTypes listType, ASTNode (*parseElement)(TokenList*, size_t*)){
    EXPECT_AND_ADVANCE(list, pos, TK_LPAREN, ERROR_EXPECTED_OPENING_PAREN,  "Expected '('");

    ASTNode listNode;
    CREATE_NODE_OR_FAIL(listNode, NULL, listType, list, pos);

    ASTNode last = NULL;
    while(*pos < list->count && list->tokens[*pos].type != TK_RPAREN){
        ASTNode elem;
        PARSE_OR_CLEANUP(elem, parseElement(list, pos), listNode);

        if(!listNode->children) listNode->children = elem;
        else last->brothers = elem;

        last = elem;

        if(*pos < list->count && list->tokens[*pos].type == TK_COMMA){
            ADVANCE_TOKEN(list, pos);
        } else if(list->tokens[*pos].type != TK_RPAREN){
            reportError(ERROR_EXPECTED_COMMA_OR_PAREN, createErrorContextFromParser(list, pos), "Expected ',' or ')'");
            freeAST(listNode);
            return NULL;
        }
    }

    EXPECT_AND_ADVANCE(list, pos, TK_RPAREN, ERROR_EXPECTED_CLOSING_PAREN, "Expected ')'");
    return listNode;
}

ASTNode parseParameter(TokenList* list, size_t* pos){
    if(*pos >= list->count) return NULL;
    Token* token = &list->tokens[*pos];

    if(detectLitType(token, list, pos) != VARIABLE){
        reportError(ERROR_EXPECTED_PARAMETER_NAME, createErrorContextFromParser(list, pos), "Expected parameter name");
        return NULL;
    }

    ASTNode paramNode;
    CREATE_NODE_OR_FAIL(paramNode, token, PARAMETER, list, pos);
    ADVANCE_TOKEN(list, pos);

    EXPECT_AND_ADVANCE(list, pos, TK_COLON, ERROR_EXPECTED_COLON, "Expected ':' after parameter name");

    ASTNode typeNode = parseType(list, pos);
    if(!typeNode) return NULL;

    ASTNode typeRefWrap;
    /* NUll token means that error wont be able to locate on report */
    CREATE_NODE_OR_FAIL(typeRefWrap, NULL, TYPE_REF, list, pos);
    typeRefWrap->children = typeNode;
    paramNode->children = typeRefWrap;

    return paramNode;
}

ASTNode parseArg(TokenList* list, size_t* pos){
    return parseExpression(list, pos, PREC_NONE);
}

/**
 * Return type
 */

ASTNode parseReturnType(TokenList* list, size_t* pos){
    EXPECT_AND_ADVANCE(list, pos, TK_ARROW, ERROR_EXPECTED_ARROW, "Expected '->'");

    ASTNode typeNode = parseType(list, pos);
    if(!typeNode) return NULL;

    ASTNode returnTypeNode;
    /* NUll token means that error wont be able to locate on report */
    CREATE_NODE_OR_FAIL(returnTypeNode, NULL, RETURN_TYPE, list, pos);
    returnTypeNode->children = typeNode;
    return returnTypeNode;
}

/**
 * Function call
 */

ASTNode parseFunctionCall(TokenList* list, size_t* pos, Token* token){
    ASTNode callNode, argList;
    CREATE_NODE_OR_FAIL(callNode, token, FUNCTION_CALL, list, pos);

    PARSE_OR_CLEANUP(argList, parseCommaSeparatedLists(list, pos, ARGUMENT_LIST, parseArg), callNode);

    callNode->children = argList;
    return callNode;
}

/**
 * Function definition
 */

ASTNode parseFunction(TokenList* list, size_t* pos){
    EXPECT_TOKEN(list, pos, TK_FN, ERROR_EXPECTED_FN, "Expected 'fn' keyword");
    ADVANCE_TOKEN(list, pos);

    if(*pos >= list->count || detectLitType(&list->tokens[*pos], list, pos) != VARIABLE){
        reportError(ERROR_EXPECTED_FUNCTION_NAME, createErrorContextFromParser(list, pos), "Expected function name");
        return NULL;
    }
    ASTNode functionNode;
    CREATE_NODE_OR_FAIL(functionNode, &list->tokens[*pos], FUNCTION_DEFINITION, list, pos);

    ADVANCE_TOKEN(list, pos);
    ASTNode paramList, returnType, body;
    PARSE_OR_CLEANUP(paramList, parseCommaSeparatedLists(list, pos, PARAMETER_LIST, parseParameter), functionNode);
    PARSE_OR_CLEANUP(returnType, parseReturnType(list, pos), functionNode, paramList);
    EXPECT_TOKEN(list, pos, TK_LBRACE, ERROR_EXPECTED_OPENING_BRACE, "Expected '{' to start function body");
    PARSE_OR_CLEANUP(body, parseBlock(list, pos), functionNode, paramList, returnType);

    functionNode->children = paramList;
    paramList->brothers = returnType;
    returnType->brothers = body;

    return functionNode;
}

/**
 * Struct definition
 */

ASTNode parseStructField(TokenList* list, size_t* pos){
    Token* name = &list->tokens[*pos];
    if(detectLitType(name, list, pos) != VARIABLE){
        reportError(ERROR_EXPECTED_FIELD_NAME, createErrorContextFromParser(list, pos), "Expected field name");
        return NULL;
    }

    ASTNode fieldNode;
    CREATE_NODE_OR_FAIL(fieldNode, name, STRUCT_FIELD, list, pos);
    ADVANCE_TOKEN(list, pos);
    EXPECT_AND_ADVANCE(list, pos, TK_COLON, ERROR_EXPECTED_COLON, "Expected ':' after field name");

    ASTNode typeNode = parseType(list, pos);
    if(!typeNode) return NULL;

    ASTNode typeRefWrap;
    /* NUll token means that error wont be able to locate on report */
    CREATE_NODE_OR_FAIL(typeRefWrap, NULL, TYPE_REF, list, pos);
    typeRefWrap->children = typeNode;
    fieldNode->children = typeRefWrap;

    return fieldNode;
}

ASTNode parseStructFieldLit(TokenList *list, size_t *pos) {
    Token *name = &list->tokens[*pos];
    if (detectLitType(name, list, pos) != VARIABLE) {
        reportError(ERROR_EXPECTED_FIELD_NAME, createErrorContextFromParser(list, pos), "Expected field name in struct literal");
        return NULL;
    }

    ASTNode fieldNode;
    CREATE_NODE_OR_FAIL(fieldNode, name, STRUCT_FIELD_LIT, list, pos);
    ADVANCE_TOKEN(list, pos);

    EXPECT_AND_ADVANCE(list, pos, TK_COLON, ERROR_EXPECTED_COLON, "Expected ':' after field name in struct literal");

    ASTNode valueNode = parseExpression(list, pos, PREC_NONE);
    if (!valueNode) return NULL;

    fieldNode->children = valueNode;
    return fieldNode;
}

ASTNode parseStruct(TokenList *list, size_t *pos) {
    EXPECT_TOKEN(list, pos, TK_STRUCT, ERROR_EXPECTED_STRUCT, "expected struct");
    ADVANCE_TOKEN(list, pos);

    Token *name = &list->tokens[*pos];
    if (detectLitType(name, list, pos) != VARIABLE) {
        reportError(ERROR_EXPECTED_STRUCT_NAME,
                    createErrorContextFromParser(list, pos),
                    "Expected name for struct");
        return NULL;
    }

    ASTNode structNode;
    CREATE_NODE_OR_FAIL(structNode, name, STRUCT_DEFINITION, list, pos);
    ADVANCE_TOKEN(list, pos);

    EXPECT_AND_ADVANCE(list, pos, TK_LBRACE, ERROR_EXPECTED_OPENING_BRACE, "Expected '{'");

    ASTNode fieldList;
    CREATE_NODE_OR_FAIL(fieldList, NULL, STRUCT_FIELD_LIST, list, pos);

    ASTNode last = NULL;
    while (list->tokens[*pos].type != TK_RBRACE) {
        ASTNode field;
        PARSE_OR_CLEANUP(field, parseStructField(list, pos), structNode, fieldList);

        if (!fieldList->children)
            fieldList->children = field;
        else if (last)
            last->brothers = field;
        last = field;

        /* Semicolons between fields are optional */
        if (*pos < list->count && list->tokens[*pos].type == TK_SEMI) {
            ADVANCE_TOKEN(list, pos);
        }
    }

    EXPECT_AND_ADVANCE(list, pos, TK_RBRACE, ERROR_EXPECTED_CLOSING_BRACE, "Expected '}' to close struct");
    if (list->tokens[*pos].type == TK_SEMI) {
        ADVANCE_TOKEN(list, pos);
    }

    structNode->children = fieldList;
    return structNode;
}