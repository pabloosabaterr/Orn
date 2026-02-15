#include "parserInternal.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Node names table */

typedef struct {
    NodeTypes   nodeType;
    const char *displayName;
} NodeTypeMap;

static const NodeTypeMap nodeTypeMapping[] = {
    {PROGRAM,                "PROGRAM"},
    {LET_DEC,                "LET_DECLARATION"},
    {CONST_DEC,              "CONST_DECLARATION"},
    {VAR_DEFINITION,         "VAR_DEF"},
    {VALUE,                  "VALUE"},
    {TYPE_REF,               "TYPE_REF"},
    {LITERAL,                "LITERAL"},
    {VARIABLE,               "VARIABLE"},
    {ASSIGNMENT,             "ASSIGNMENT"},
    {COMPOUND_ADD_ASSIGN,    "COMPOUND_ADD_ASSIGN"},
    {COMPOUND_SUB_ASSIGN,    "COMPOUND_SUB_ASSIGN"},
    {COMPOUND_MUL_ASSIGN,    "COMPOUND_MULT_ASSIGN"},
    {COMPOUND_DIV_ASSIGN,    "COMPOUND_DIV_ASSIGN"},
    {COMPOUND_AND_ASSIGN,    "COMPOUND_AND_ASSIGN"},
    {COMPOUND_OR_ASSIGN,     "COMPOUND_OR_ASSIGN"},
    {COMPOUND_XOR_ASSIGN,    "COMPOUND_XOR_ASSIGN"},
    {COMPOUND_LSHIFT_ASSIGN, "COMPOUND_LSHIFT_ASSIGN"},
    {COMPOUND_RSHIFT_ASSIGN, "COMPOUND_RSHIFT_ASSIGN"},
    {BITWISE_AND,            "BITWISE_AND"},
    {BITWISE_OR,             "BITWISE_OR"},
    {BITWISE_XOR,            "BITWISE_XOR"},
    {BITWISE_LSHIFT,         "BITWISE_LSHIFT"},
    {BITWISE_RSHIFT,         "BITWISE_RSHIFT"},
    {BITWISE_NOT,            "BITWISE_NOT"},
    {ADD_OP,                 "ADD_OP"},
    {SUB_OP,                 "SUB_OP"},
    {MUL_OP,                 "MUL_OP"},
    {DIV_OP,                 "DIV_OP"},
    {MOD_OP,                 "MOD_OP"},
    {UNARY_MINUS_OP,         "UNARY_MINUS_OP"},
    {UNARY_PLUS_OP,          "UNARY_PLUS_OP"},
    {PRE_INCREMENT,          "PRE_INCREMENT"},
    {PRE_DECREMENT,          "PRE_DECREMENT"},
    {POST_INCREMENT,         "POST_INCREMENT"},
    {POST_DECREMENT,         "POST_DECREMENT"},
    {LOGIC_AND,              "LOGIC_AND"},
    {LOGIC_OR,               "LOGIC_OR"},
    {LOGIC_NOT,              "LOGIC_NOT"},
    {EQUAL_OP,               "EQUAL_OP"},
    {NOT_EQUAL_OP,           "NOT_EQUAL_OP"},
    {LESS_THAN_OP,           "LESS_THAN_OP"},
    {GREATER_THAN_OP,        "GREATER_THAN_OP"},
    {LESS_EQUAL_OP,          "LESS_EQUAL_OP"},
    {GREATER_EQUAL_OP,       "GREATER_EQUAL_OP"},
    {BLOCK_STATEMENT,        "BLOCK_STATEMENT"},
    {IF_CONDITIONAL,         "IF_CONDITIONAL"},
    {IF_TRUE_BRANCH,         "IF_TRUE_BRANCH"},
    {ELSE_BRANCH,            "ELSE_BRANCH"},
    {BLOCK_EXPRESSION,       "BLOCK_EXPRESSION"},
    {LOOP_STATEMENT,         "LOOP_STATEMENT"},
    {FUNCTION_DEFINITION,    "FUNCTION_DEFINITION"},
    {FUNCTION_CALL,          "FUNCTION_CALL"},
    {PARAMETER_LIST,         "PARAMETER_LIST"},
    {PARAMETER,              "PARAMETER"},
    {ARGUMENT_LIST,          "ARGUMENT_LIST"},
    {RETURN_STATEMENT,       "RETURN_STATEMENT"},
    {RETURN_TYPE,            "RETURN_TYPE"},
    {STRUCT_DEFINITION,      "STRUCT_DEFINITION"},
    {STRUCT_FIELD_LIST,      "STRUCT_FIELD_LIST"},
    {STRUCT_FIELD,           "STRUCT_FIELD"},
    {STRUCT_VARIABLE_DEFINITION, "STRUCT_VAR_DEF"},
    {MEMBER_ACCESS,          "MEMBER_ACCESS"},
    {REF_INT,                "TYPE_INT"},
    {REF_STRING,             "TYPE_STRING"},
    {REF_FLOAT,              "TYPE_FLOAT"},
    {REF_BOOL,               "TYPE_BOOL"},
    {REF_VOID,               "TYPE_VOID"},
    {REF_DOUBLE,             "TYPE_DOUBLE"},
    {REF_CUSTOM,             "TYPE_CUSTOM"},
    {CAST_EXPRESSION,        "CAST_EXPRESSION"},
    {ARRAY_VARIABLE_DEFINITION, "ARRAY_VAR_DEF"},
    {ARRAY_LIT,              "ARRAY_LIT"},
    {ARRAY_ACCESS,           "ARRAY_ACCESS"},
    {TERNARY_CONDITIONAL,    "TERNARY_CONDITIONAL"},
    {TERNARY_IF_EXPR,        "TERNARY_IF_EXPR"},
    {TERNARY_ELSE_EXPR,      "TERNARY_ELSE_EXPR"},
    {POINTER,                "PTR"},
    {MEMADDRS,               "MEMREF"},
    {NULL_LIT,               "NULL"},
    {IMPORTDEC,              "IMPORT"},
    {EXPORTDEC,              "EXPORT"},
    {STRUCT_LIT,             "STRUCT_LIT"},
    {STRUCT_FIELD_LIT,       "FIELD_LIT"},
    {null_NODE,              NULL}   /* sentinel — must be last */
};

const char* getNodeTypeName(NodeTypes nodeType){
    for(int i = 0; nodeTypeMapping[i].nodeType != null_NODE; ++i){
        if(nodeTypeMapping[i].nodeType == nodeType){
            return nodeTypeMapping[i].displayName;
        }
    }
    return "UNKNOWN_NODE_TYPE";
}

/**
 * Text utilities
 */

char *extractText(const char *start, size_t length){
    if (!start || length == 0) return NULL;
    char *str = malloc(length + 1);
    if (!str) return NULL;
    memcpy(str, start, length);
    str[length] = '\0';
    return str;
}

int nodeValueEquals(ASTNode node, const char *str) {
    if (!node || !node->start || !str) return 0;
    size_t strLen = strlen(str);
    return (node->length == strLen && memcmp(node->start, str, strLen) == 0);
}

/**
 * Literal type detection
 */

static int containsFChar(const char *val, size_t i, int hasDot, size_t len) {
    return (val[i] == 'f' || val[i] == 'F') && i == len - 1 && hasDot;
}

/**
 * @brief Classifies a token as a specific literal type (int, float, string, bool, variable, etc.)
 */
NodeTypes detectLitType(const Token *tok, TokenList *list, size_t *pos) {
    if (!tok || !tok->start || tok->length == 0) return null_NODE;

    size_t len       = tok->length;
    const char *val  = tok->start;

    /* String literal */
    if (len >= 2 && val[0] == '"' && val[len - 1] == '"')
        return REF_STRING;

    /* Boolean literal */
    if ((len == 4 && memcmp(val, "true",  4) == 0) ||
        (len == 5 && memcmp(val, "false", 5) == 0)) {
        return REF_BOOL;
    }

    /* Struct literal (opens with '{') */
    if (val[0] == '{') {
        return STRUCT_LIT;
    }

    /* Numeric literal */
    size_t start = (val[0] == '-') ? 1 : 0;
    int hasDot = 0, allDigits = 1;
    for (size_t i = start; i < len; i++) {
        if (val[i] == '.') {
            if (hasDot) { allDigits = 0; break; }
            hasDot = 1;
        } else if (!isdigit((unsigned char)val[i])) {
            if (containsFChar(val, i, hasDot, len)) continue;
            allDigits = 0;
            break;
        }
    }

    if (allDigits) {
        if (hasDot) {
            if (containsFChar(val, len - 1, hasDot, len)) return REF_FLOAT;
            return REF_DOUBLE;
        }
        return REF_INT;
    }

    /* Identifier / variable */
    if (isalpha((unsigned char)val[0]) || val[0] == '_') {
        for (size_t i = 1; i < len; i++) {
            if (!isalnum((unsigned char)val[i]) && val[i] != '_') {
                reportError(ERROR_INVALID_EXPRESSION,
                            createErrorContextFromParser(list, pos),
                            extractText(tok->start, tok->length));
                return null_NODE;
            }
        }
        return VARIABLE;
    }

    reportError(ERROR_INVALID_EXPRESSION,
                createErrorContextFromParser(list, pos),
                extractText(tok->start, tok->length));
    return null_NODE;
}

/**
 * Node construction
 */

ASTNode createNode(const Token *token, NodeTypes type, TokenList *list, size_t *pos) {
    ASTNode node = malloc(sizeof(struct ASTNode));
    if (!node) {
        reportError(ERROR_MEMORY_ALLOCATION_FAILED, createErrorContextFromParser(list, pos),
                    token ? extractText(token->start, token->length) : "");
        return NULL;
    }

    if (token) {
        node->start = token->start;
        node->length = token->length;
        node->line = token->line;
        node->column = token->column;
    } else {
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

ASTNode createValNode(const Token *currentToken, TokenList *list, size_t *pos) {
    if (currentToken == NULL) return NULL;

    NodeTypes type = detectLitType(currentToken, list, pos);
    if (type == null_NODE) return NULL;

    if (type != VARIABLE) {
        ASTNode litNode, typeRefNode;
        CREATE_NODE_OR_FAIL(litNode, currentToken, LITERAL, list, pos);
        CREATE_NODE_OR_FAIL(typeRefNode, NULL, type, list, pos);
        litNode->children = typeRefNode;
        return litNode;
    }

    return createNode(currentToken, type, list, pos);
}

