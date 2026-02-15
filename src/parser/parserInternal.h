#ifndef PARSER_INTERNAL_H
#define PARSER_INTERNAL_H

#include "parser.h"
#include "errorHandling.h"
#include "../lexer/lexer.h"

// macros

#define ADVANCE_TOKEN(list, pos) \
    do { if (*(pos) < (list)->count) (*(pos))++; } while(0)

#define EXPECT_TOKEN(list, pos, expectedType, errCode, errMsg) \
    do { \
        if (*(pos) >= (list)->count || \
            (list)->tokens[*(pos)].type != (expectedType)) { \
            reportError(errCode, \
                        createErrorContextFromParser(list, pos), \
                        errMsg ? errMsg : "Unexpected token"); \
            return NULL; \
        } \
    } while(0)

#define EXPECT_AND_ADVANCE(list, pos, expectedType, errCode, errMsg) \
    do { \
        EXPECT_TOKEN(list, pos, expectedType, errCode, errMsg); \
        ADVANCE_TOKEN(list, pos); \
    } while(0)

#define CREATE_NODE_OR_FAIL(var, token, type, list, pos) \
    do { \
        var = createNode(token, type, list, pos); \
        if (!var) return NULL; \
    } while(0)

#define PARSE_OR_CLEANUP(var, parseExpr, ...) \
    do { \
        var = (parseExpr); \
        if (!var) { \
            ASTNode _cleanup_nodes[] = {__VA_ARGS__}; \
            size_t _count = sizeof(_cleanup_nodes)/sizeof(_cleanup_nodes[0]); \
            for (size_t _i = 0; _i < _count; _i++) { \
                if (_cleanup_nodes[_i]) freeAST(_cleanup_nodes[_i]); \
            } \
            return NULL; \
        } \
    } while(0)

#define PARSE_OR_FAIL(var, parseExpr) \
    do { \
        var = (parseExpr); \
        if (!var) { \
            return NULL; \
        } \
    } while(0)

typedef enum {
    PREC_NONE,
    PREC_ASSIGN,
    PREC_TERNARY,
    PREC_OR,
    PREC_AND,
    PREC_BITWISE_OR,
    PREC_BITWISE_XOR,
    PREC_BITWISE_AND,
    PREC_EQUALITY,
    PREC_COMPARISON,
    PREC_SHIFT,
    PREC_TERM,
    PREC_FACTOR,
    PREC_CAST,
    PREC_UNARY
} Precedence;

typedef struct {
    TokenType token;
    NodeTypes nodeType;
    Precedence precedence;
    int isRightAssociative;
} OperatorInfo;

typedef ASTNode (*ParseFunc)(TokenList *list, size_t *pos);

typedef struct {
    TokenType token;
    ParseFunc handler;
} StatementHandler;

// on parserCore.c
extern const StatementHandler statementHandlers[];

ErrorContext *createErrorContextFromParser(TokenList* list, size_t* pos);

/**
 * Node construction
 */

ASTNode createNode(const Token* token, NodeTypes type, TokenList* list, size_t* pos);
ASTNode createValNode(const Token* currentToken, TokenList* list, size_t* pos);
NodeTypes detectLitType(const Token* tok, TokenList* list, size_t* pos);

/**
 * type helpers
 */

ASTNode parseType(TokenList* list, size_t* pos);
NodeTypes getDecType(const TokenType type);
NodeTypes getTypeNodeFromToken(TokenType type);
int isTypeToken(TokenType type);

/**
 * Operators
 */

const OperatorInfo* getOperatorInfo(TokenType type);
const char *getTokenTypeName(TokenType type);
const char *getCurrentTokenName(TokenList* list, size_t* pos);

/**
 * Expressions
 */

ASTNode parseExpression(TokenList* list, size_t* pos, Precedence minPrec);
ASTNode parseUnary(TokenList* list, size_t* pos);
NodeTypes getUnaryOpType(TokenType t);
ASTNode parsePrimaryExp(TokenList* list, size_t* pos);
ASTNode parseTernary(TokenList* list, size_t* pos);
ASTNode parseBlockExpression(TokenList* list, size_t* pos);
ASTNode parseArrayAccess(TokenList* list, size_t* pos, ASTNode array);
ASTNode parseArrLit(TokenList* list, size_t* pos);
ASTNode parseStructLit(TokenList* list, size_t* pos);

/**
 * Statements
 */

ASTNode parseStatement(TokenList *list, size_t *pos);
ASTNode parseExpressionStatement(TokenList *list, size_t *pos);
ASTNode parseBlock(TokenList *list, size_t *pos);
ASTNode parseIf(TokenList *list, size_t *pos);
ASTNode parseLoop(TokenList *list, size_t *pos);
ASTNode parseForLoop(TokenList *list, size_t *pos);
ASTNode parseReturnStatement(TokenList *list, size_t *pos);
ASTNode parseImport(TokenList *list, size_t *pos);
ASTNode parseExportFunction(TokenList *list, size_t *pos);

/**
 * Declarations
 */

ASTNode parseDeclaration(TokenList *list, size_t *pos);
ASTNode parseArrayDec(TokenList *list, size_t *pos, Token *varName);

/**
 * Functions & Structs
 */

ASTNode parseFunction(TokenList *list, size_t *pos);
ASTNode parseFunctionCall(TokenList *list, size_t *pos, Token *tok);
ASTNode parseParameter(TokenList *list, size_t *pos);
ASTNode parseArg(TokenList *list, size_t *pos);
ASTNode parseCommaSeparatedLists(TokenList *list, size_t *pos, NodeTypes listType, ASTNode (*parseElement)(TokenList *, size_t *));
ASTNode parseReturnType(TokenList *list, size_t *pos);
ASTNode parseStruct(TokenList *list, size_t *pos);
ASTNode parseStructField(TokenList *list, size_t *pos);
ASTNode parseStructFieldLit(TokenList *list, size_t *pos);

#endif // PARSER_INTERNAL_H