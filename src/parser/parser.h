#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>
#include <stddef.h>
#include "../lexer/lexer.h"

typedef enum {
    // structure
    null_NODE,
    PROGRAM,
    IMPORTDEC,
    EXPORTDEC,
    FROMDEC, // not used yet

    // ARRAY TYPES
    ARRAY_ACCESS,

    // type references
    REF_INT,
    REF_STRING,
    REF_FLOAT,
    REF_BOOL,
    REF_VOID,
    REF_DOUBLE,
    REF_CUSTOM,
    POINTER,
    MEMADDRS,
    NULL_LIT,

    // Variable definitions
    STRUCT_VARIABLE_DEFINITION,
    ARRAY_VARIABLE_DEFINITION,
    VAR_DEFINITION,
    TYPE_REF,
    VALUE,

    CONST_DEC,
    LET_DEC,

    // literals
    LITERAL,
    ARRAY_LIT,

    // Variable and assignment
    VARIABLE,
    ASSIGNMENT,
    COMPOUND_ADD_ASSIGN,
    COMPOUND_SUB_ASSIGN,
    COMPOUND_MUL_ASSIGN,
    COMPOUND_DIV_ASSIGN,
    COMPOUND_AND_ASSIGN,
    COMPOUND_OR_ASSIGN,
    COMPOUND_XOR_ASSIGN,
    COMPOUND_LSHIFT_ASSIGN,
    COMPOUND_RSHIFT_ASSIGN,

    // bitwise
    BITWISE_AND,
    BITWISE_OR,
    BITWISE_XOR,
    BITWISE_LSHIFT,
    BITWISE_RSHIFT,
    BITWISE_NOT,

    // Binary arithmetic
    ADD_OP,
    SUB_OP,
    MUL_OP,
    DIV_OP,
    MOD_OP,

    // Casting

    CAST_EXPRESSION,

    // Unary operators
    UNARY_MINUS_OP,
    UNARY_PLUS_OP,

    // Incremenents and decrements
    PRE_INCREMENT,
    POST_INCREMENT,
    PRE_DECREMENT,
    POST_DECREMENT,

    // Logical
    LOGIC_AND,
    LOGIC_OR,
    LOGIC_NOT,

    // Comparison
    EQUAL_OP,
    NOT_EQUAL_OP,
    LESS_THAN_OP,
    GREATER_THAN_OP,
    LESS_EQUAL_OP,
    GREATER_EQUAL_OP,

    // Control flow
    TERNARY_CONDITIONAL,
    TERNARY_IF_EXPR,
    TERNARY_ELSE_EXPR,
    BLOCK_STATEMENT,
    IF_CONDITIONAL,
    IF_TRUE_BRANCH,
    ELSE_BRANCH,
    BLOCK_EXPRESSION,
    LOOP_STATEMENT,

    // Functions
    FUNCTION_DEFINITION,
    FUNCTION_CALL,
    PARAMETER_LIST,
    PARAMETER,
    ARGUMENT_LIST,
    RETURN_STATEMENT,
    RETURN_TYPE,

    // Structs
    STRUCT_LIT,
    STRUCT_DEFINITION,
    STRUCT_FIELD_LIST,
    STRUCT_FIELD,
    STRUCT_FIELD_LIT,
    MEMBER_ACCESS,
} NodeTypes;

typedef struct ASTNode {
    uint16_t length;
    uint16_t line;
    uint16_t column;
    const char* start;
    NodeTypes nodeType;
    struct ASTNode* children;
    struct ASTNode* brothers;
} *ASTNode;

typedef struct ASTContext {
    const char* buffer;
    const char* filename;
    ASTNode root;
} ASTContext;

// public api

ASTContext* ASTGenerator(TokenList* tokenList);

void printAST(ASTNode node, int depth);
void printASTTree(ASTNode node, char* prefix, int isLast);

void freeAST(ASTNode node);

void freeASTContext(ASTContext* context);

const char* getNodeTypeName(NodeTypes nodeType);

char *extractText(const char *start, size_t length);
int   nodeValueEquals(ASTNode node, const char *str);

#endif // PARSER_H