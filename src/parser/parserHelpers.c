/**
* @file parserHelpers.c
 * @brief Helper functions and utilities for the parser module.
 *
 * Contains validation functions, AST node creation utilities, and lookup
 * functions that support the main parser implementation. These functions
 * provide:
 * - Token type to AST node type conversion
 * - Literal value validation and classification
 * - AST node construction and initialization
 * - Operator precedence lookup for Pratt parser
 * - Node type name mapping for debugging output
 *
 * This module isolates commonly used utility functions to improve
 * code organization and maintainability.
 */

#include "parser.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "../errorHandling/errorHandling.h"
#include "../lexer/lexer.h"

// return the the type its refering to
NodeTypes getDecType(TokenType type) {
    switch (type)
	{
		case TK_INT: return REF_INT;
		case TK_FLOAT: return REF_FLOAT;
		case TK_DOUBLE: return REF_DOUBLE;
		case TK_BOOL: return REF_BOOL;
		case TK_STRING: return REF_STRING;
		default: return null_NODE;
	}
}

static int containsFChar(const char * val, size_t i, int hasDot, size_t len){
	return (val[i] == 'f' || val[i] == 'F') && i == len - 1 && hasDot;
}

/**
 * @brief Unified literal type detection with optimized checks.
 * Single function replaces all validation functions.
 */
NodeTypes detectLitType(const Token * tok, TokenList * list, size_t * pos) {
	if (!tok || !tok->start || tok->length == 0) return null_NODE;

	size_t len = tok->length;
	const char *val = tok->start;

	if (len >= 2 && val[0] == '"' && val[len-1] == '"')
		return REF_STRING;
	if ((len == 4 && memcmp(val, "true", 4)==0 )||
		(len == 5 && memcmp(val, "false", 5)==0)) {
		return REF_BOOL;
	}

	// everything in a declaration that start with { is a struct lit ?
	if(val[0] == '{'){
		return STRUCT_LIT;
	}

	size_t start = (val[0] == '-') ? 1 : 0;

	int hasDot = 0, allDigits = 1;
	for (size_t i = start; i<len; i++) {
		if (val[i] == '.') {
			if (hasDot) { allDigits = 0; break; }
			hasDot = 1;
		} else if (!isdigit(val[i])) {
			if(containsFChar(val, i, hasDot, len)) continue;
			allDigits = 0;
			break;
		}
	}

	if (allDigits) {
		if (hasDot) {
            if(containsFChar(val, len-1, hasDot, len)) return REF_FLOAT;
            return REF_DOUBLE;
        }
		return REF_INT;
	}
	
	if (isalpha(val[0]) || val[0] == '_') {
		for (size_t i = 1; i<len; i++) {
			if (!isalnum(val[i]) && val[i] != '_') {
				reportError(ERROR_INVALID_EXPRESSION,createErrorContextFromParser(list, pos), extractText(tok->start, tok->length));
				return null_NODE;
			}
		}
		return VARIABLE;
	}

	reportError(ERROR_INVALID_EXPRESSION,createErrorContextFromParser(list, pos), extractText(tok->start, tok->length));
	return null_NODE;
}

/**
 * @brief Converts AST node type to human-readable string for debugging.
 *
 * Looks up the node type in the nodeTypeMapping table to find the
 * corresponding display name. Used primarily for AST visualization
 * and debugging output to provide meaningful node type names.
 *
 * @param nodeType AST node type to convert
 * @return Human-readable string representation of the node type
 *
 * @note Returns "UNKNOWN" for unrecognized node types. The nodeTypeMapping
 *       table should be kept in sync with the NodeTypes enumeration.
 */
const char *getNodeTypeName(NodeTypes nodeType) {
    for (int i = 0; nodeTypeMapping[i].displayName != NULL; i++) {
        if (nodeTypeMapping[i].nodeType == nodeType) {
            return nodeTypeMapping[i].displayName;
        }
    }
    return "UNKNOWN";
}

/**
 * @brief Creates a new AST node with given token and type information.
 *
 * Allocates and initializes a new AST node with:
 * - Copied value string from token (if present)
 * - Specified node type
 * - Position information for error reporting
 * - Initialized child and sibling pointers
 *
 * @param token Source token (can be NULL for synthetic nodes)
 * @param type AST node type to assign
 * @return Newly allocated AST node or NULL on allocation failure
 *
 * @note The value string is duplicated, so the original token can be freed.
 *       The caller is responsible for freeing the returned node.
 */
ASTNode createNode(const Token * token, NodeTypes type, TokenList * list, size_t * pos) {
    ASTNode node = malloc(sizeof(struct ASTNode));
	if (!node) {
		reportError(ERROR_MEMORY_ALLOCATION_FAILED,createErrorContextFromParser(list, pos),  token ? extractText(token->start, token->length) : "");
		return NULL;
	}

	if (token) {
		node->start = token->start;
		node->length = token->length;
		node->line = token->line;
		node->column = token->column;
	}else {
		node->start = NULL;
		node->length = 0;
		node->line = 0;
		node->column = 0;
	}

	node->nodeType = type;
	node->children = NULL;
	node->brothers = NULL;
    return node;
}

/**
 * @brief Creates appropriate AST node based on token value and context.
 *
 * Analyzes the token value to determine the correct literal type and
 * creates the corresponding AST node. Handles type inference for:
 * - String literals (quoted strings)
 * - Float literals (contains decimal point)
 * - Integer literals (all digits)
 * - Boolean literals (true/false)
 * - Variable references (valid identifiers)
 *
 * @param currentToken Token to convert to AST node
 * @return Newly created AST node or NULL on error
 *
 * @note Reports ERROR_INVALID_EXPRESSION for unrecognized token formats
 */
ASTNode createValNode(const Token * currentToken, TokenList * list, size_t*pos) {
    if (currentToken == NULL) return NULL;

    NodeTypes type = detectLitType(currentToken, list, pos);
    if (type == null_NODE) return NULL;
	if (type != VARIABLE){
		ASTNode litNode, typeRefNode;
		CREATE_NODE_OR_FAIL(litNode, currentToken, LITERAL, list, pos);
		CREATE_NODE_OR_FAIL(typeRefNode, NULL, type, list, pos);
		litNode->children = typeRefNode;
		return litNode;
	}

    return createNode(currentToken, type,  list, pos);
}

/**
 * @brief Looks up operator information for precedence parsing.
 *
 * Searches the static operators array to find the OperatorInfo
 * structure corresponding to the given token type.
 *
 * @param type Token type to look up
 * @return Pointer to OperatorInfo structure or NULL if not found
 *
 * @note Used by the Pratt parser to determine operator precedence
 *       and associativity during expression parsing.
 */
const OperatorInfo *getOperatorInfo(TokenType type) {
    for (int i = 0; operators[i].token != TK_NULL; i++) {
        if (operators[i].token == type) return &operators[i];
    }
    return NULL;
}

/**
 * @brief Checks if a token represents a valid type.
 *
 * @param type Token type to check
 * @return 1 if it's a type token, 0 otherwise
 */
int isTypeToken(TokenType type) {
    return (type == TK_INT ||
            type == TK_STRING ||
            type == TK_FLOAT ||
            type == TK_BOOL ||
			type == TK_DOUBLE ||
			type == TK_LIT ||
            type == TK_VOID);
}

NodeTypes getUnaryOpType(TokenType t) {
	switch (t) {
		case TK_MINUS: return UNARY_MINUS_OP;
		case TK_PLUS: return UNARY_PLUS_OP;
		case TK_NOT: return LOGIC_NOT;
		case TK_BIT_NOT: return BITWISE_NOT;
		case TK_INCR: return PRE_INCREMENT;
		case TK_DECR: return PRE_DECREMENT;
		default: return null_NODE;
	}
}

char* extractText(const char* start, size_t length) {
	if (!start || length == 0) return NULL;

	char* str = malloc(length + 1);
	if (!str) return NULL;

	memcpy(str, start, length);
	str[length] = '\0';
	return str;
}

int nodeValueEquals(const ASTNode node, const char* str) {
	if (!node || !node->start || !str) return 0;
	size_t strLen = strlen(str);
	return (node->length == strLen &&
			memcmp(node->start, str, strLen) == 0);
}