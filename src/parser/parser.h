/**
* @file parser.h
 * @brief Parser interface and Abstract Syntax Tree definitions for the C compiler.
 *
 * Defines the AST node types, data structures, and function prototypes for
 * parsing tokens into an Abstract Syntax Tree. Implements operator precedence
 * parsing using a Pratt parser approach for expression handling.
 */

#ifndef PARSER_H
#define PARSER_H

#include "errorHandling.h"
#include "../lexer/lexer.h"

ErrorContext *createErrorContextFromParser(TokenList *list, size_t * pos) ;
const char *getTokenTypeName(TokenType type);
const char *getCurrentTokenName(TokenList *list, size_t pos);

#define ADVANCE_TOKEN(list, pos) do { if (*(pos) < (list)->count) (*(pos))++; } while(0)

#define EXPECT_TOKEN(list, pos, expectedType, errCode, errMsg) \
    do { \
        if (*(pos) >= (list)->count || (list)->tokens[*(pos)].type != (expectedType)) { \
            reportError(errCode,createErrorContextFromParser(list, pos), errMsg ? errMsg : "Unexpected token"); \
            return NULL; \
        } \
    } while(0)

#define EXPECT_AND_ADVANCE(list, pos, expectedType, errCode, errMsg) \
    do { \
        EXPECT_TOKEN(list, pos, expectedType,errCode, errMsg); \
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

/**
 * @brief Enumeration of all Abstract Syntax Tree node types.
 *
 * Organized by category for better maintainability:
 * - Program structure nodes
 * - Variable declaration nodes
 * - Literal value nodes
 * - Expression nodes (operators, assignments)
 * - Statement nodes (blocks, control flow)
 *
 * Each node type represents a distinct syntactic construct in the language.
 */
typedef enum {
    // Special nodes
    null_NODE,
    PROGRAM,
    IMPORTDEC,
    EXPORTDEC,
    FROMDEC,

    //array types
    ARRAY_ACCESS,

    // Type references
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

    // Literals
    LITERAL,
    ARRAY_LIT,

    // Variables and assignment
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

    // Bitwise
    BITWISE_AND,
    BITWISE_OR,
    BITWISE_XOR,
    BITWISE_LSHIFT,
    BITWISE_RSHIFT,
    BITWISE_NOT,

    // Binary arithmetic operators
    ADD_OP,
    SUB_OP,
    MUL_OP,
    DIV_OP,
    MOD_OP,

    //casting
    CAST_EXPRESSION,

    // Unary operators
    UNARY_MINUS_OP,
    UNARY_PLUS_OP,

    // Increment/decrement operators
    PRE_INCREMENT,
    PRE_DECREMENT,
    POST_INCREMENT,
    POST_DECREMENT,

    // Logical operators
    LOGIC_AND,
    LOGIC_OR,
    LOGIC_NOT,

    // Comparison operators
    EQUAL_OP,
    NOT_EQUAL_OP,
    LESS_THAN_OP,
    GREATER_THAN_OP,
    LESS_EQUAL_OP,
    GREATER_EQUAL_OP,

    // Control flow statements
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


typedef struct {
    NodeTypes nodeType;
    const char *displayName;
} NodeTypeMap;

static const NodeTypeMap nodeTypeMapping[] = {
    {PROGRAM, "PROGRAM"},
    {LET_DEC, "LET_DECLARATION"},
    {CONST_DEC, "CONST_DECLARATION"},
    {VAR_DEFINITION, "VAR_DEF"},
    {VALUE, "VALUE"},
    {TYPE_REF, "TYPE_REF"},
    {LITERAL, "LITERAL"},
    {VARIABLE, "VARIABLE"},
    {ASSIGNMENT, "ASSIGNMENT"},
    {COMPOUND_ADD_ASSIGN, "COMPOUND_ADD_ASSIGN"},
    {COMPOUND_SUB_ASSIGN, "COMPOUND_SUB_ASSIGN"},
    {COMPOUND_MUL_ASSIGN, "COMPOUND_MULT_ASSIGN"},
    {COMPOUND_DIV_ASSIGN, "COMPOUND_DIV_ASSIGN"},
    {COMPOUND_AND_ASSIGN, "COMPOUND_AND_ASSIGN"},     
    {COMPOUND_OR_ASSIGN, "COMPOUND_OR_ASSIGN"},      
    {COMPOUND_XOR_ASSIGN, "COMPOUND_XOR_ASSIGN"},     
    {COMPOUND_LSHIFT_ASSIGN, "COMPOUND_LSHIFT_ASSIGN"}, 
    {COMPOUND_RSHIFT_ASSIGN, "COMPOUND_RSHIFT_ASSIGN"},
    {BITWISE_AND, "BITWISE_AND"},
    {BITWISE_OR, "BITWISE_OR"},
    {BITWISE_XOR, "BITWISE_XOR"},
    {BITWISE_LSHIFT, "BITWISE_LSHIFT"},
    {BITWISE_RSHIFT, "BITWISE_RSHIFT"},
    {BITWISE_NOT, "BITWISE_NOT"},
    {ADD_OP, "ADD_OP"},
    {SUB_OP, "SUB_OP"},
    {MUL_OP, "MUL_OP"},
    {DIV_OP, "DIV_OP"},
    {MOD_OP, "MOD_OP"},
    {UNARY_MINUS_OP, "UNARY_MINUS_OP"},
    {UNARY_PLUS_OP, "UNARY_PLUS_OP"},
    {PRE_INCREMENT, "PRE_INCREMENT"},
    {PRE_DECREMENT, "PRE_DECREMENT"},
    {POST_INCREMENT, "POST_INCREMENT"},
    {POST_DECREMENT, "POST_DECREMENT"},
    {LOGIC_AND, "LOGIC_AND"},
    {LOGIC_OR, "LOGIC_OR"},
    {LOGIC_NOT, "LOGIC_NOT"},
    {EQUAL_OP, "EQUAL_OP"},
    {NOT_EQUAL_OP, "NOT_EQUAL_OP"},
    {LESS_THAN_OP, "LESS_THAN_OP"},
    {GREATER_THAN_OP, "GREATER_THAN_OP"},
    {LESS_EQUAL_OP, "LESS_EQUAL_OP"},
    {GREATER_EQUAL_OP, "GREATER_EQUAL_OP"},
    {BLOCK_STATEMENT, "BLOCK_STATEMENT"},
    {IF_CONDITIONAL, "IF_CONDITIONAL"},
    {IF_TRUE_BRANCH, "IF_TRUE_BRANCH"},
    {ELSE_BRANCH, "ELSE_BRANCH"},
    {BLOCK_EXPRESSION, "BLOCK_EXPRESSION"},
    {LOOP_STATEMENT, "LOOP_STATEMENT"},
    {FUNCTION_DEFINITION, "FUNCTION_DEFINITION"},
    {FUNCTION_CALL, "FUNCTION_CALL"},
    {PARAMETER_LIST, "PARAMETER_LIST"},
    {PARAMETER, "PARAMETER"},
    {ARGUMENT_LIST, "ARGUMENT_LIST"},
    {RETURN_STATEMENT, "RETURN_STATEMENT"},
    {RETURN_TYPE, "RETURN_TYPE"},
    {STRUCT_DEFINITION, "STRUCT_DEFINITION"},
    {STRUCT_FIELD_LIST, "STRUCT_FIELD_LIST"},
    {STRUCT_FIELD, "STRUCT_FIELD"},
    {STRUCT_VARIABLE_DEFINITION, "STRUCT_VAR_DEF"},
    {MEMBER_ACCESS, "MEMBER_ACCESS"},
    {REF_INT, "TYPE_INT"},
    {REF_STRING, "TYPE_STRING"},
    {REF_FLOAT, "TYPE_FLOAT"},
    {REF_BOOL, "TYPE_BOOL"},
    {REF_VOID, "TYPE_VOID"},
    {REF_DOUBLE, "TYPE_DOUBLE"},
    {REF_CUSTOM, "TYPE_CUSTOM"},
    {CAST_EXPRESSION, "CAST_EXPRESSION"},
    {ARRAY_VARIABLE_DEFINITION, "ARRAY_VAR_DEF"},
    {ARRAY_LIT, "ARRAY_LIT"},
    {ARRAY_ACCESS, "ARRAY_ACCESS"},
    {TERNARY_CONDITIONAL, "TERNARY_CONDITIONAL"},
    {TERNARY_IF_EXPR, "TERNARY_IF_EXPR"},
    {TERNARY_ELSE_EXPR, "TERNARY_ELSE_EXPR"},
    {POINTER, "PTR"},
    {MEMADDRS, "MEMREF"},
    {NULL_LIT, "NULL"},
    {IMPORTDEC, "IMPORT"},
    {EXPORTDEC, "EXPORT"},
    {STRUCT_LIT, "STRUCT_LIT"},
    {STRUCT_FIELD_LIT, "FIELD_LIT"},
    {null_NODE, NULL} // Sentinel - must be last
};

typedef struct ASTNode {
    uint16_t length;
    uint16_t line;
    uint16_t column;
    const char * start;
    NodeTypes nodeType;
    struct ASTNode *children;
    struct ASTNode *brothers;
} * ASTNode;

typedef struct ASTContext {
    const char* buffer;
    const char *filename;
    ASTNode root;
} ASTContext;

typedef ASTNode (*ParseFunc)(TokenList*, size_t*);

typedef struct {
    TokenType token;
    ParseFunc handler;
} StatementHandler;

extern const StatementHandler statementHandlers[];

/**
 * @brief Static mapping table for type definitions.
 *
 * Terminated with TK_NULL entry for easy iteration.
 * Used by getDecType() to convert tokens to AST node types.
 */
typedef struct {
    TokenType TkType;
    NodeTypes type;
} TypeDefMap;

/**
 * todo: update
 * deprecated doc
 * @brief Operator precedence levels for Pratt parser.
 *
 * Higher numeric values indicate higher precedence.
 * Used to resolve operator precedence during expression parsing.
 *
 * Precedence hierarchy (low to high):
 * - PREC_ASSIGN: Assignment operators (=, +=, -=, etc.)
 * - PREC_OR: Logical OR (||)
 * - PREC_AND: Logical AND (&&)
 * - PREC_EQUALITY: Equality operators (==, !=)
 * - PREC_COMPARISON: Comparison operators (<, >, <=, >=)
 * - PREC_TERM: Additive operators (+, -)
 * - PREC_FACTOR: Multiplicative operators (*, /, %)
 * - PREC_UNARY: Unary operators (!, ++, --, unary -)
 */
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

/**
 * @brief Operator information for precedence parsing.
 *
 * Contains all information needed for Pratt parser to handle operators:
 * - Token type recognition
 * - AST node type mapping
 * - Precedence level
 * - Associativity information
 */
typedef struct {
    TokenType token;
    NodeTypes nodeType;
    Precedence precedence;
    int isRightAssociative; // Flag for right-associative operators like '='
} OperatorInfo;

/**
 * @brief Static operator information table.
 *
 * Defines all operators supported by the parser with their precedence
 * and associativity information. Used by getOperatorInfo() during parsing.
 *
 * @note Assignment operators are right-associative (x = y = z evaluates as x = (y = z))
 *       All other operators are left-associative (a + b + c evaluates as (a + b) + c)
 */
static const OperatorInfo operators[] = {
    {TK_ASSIGN, ASSIGNMENT, PREC_ASSIGN, 1},
    {TK_PLUS_ASSIGN, COMPOUND_ADD_ASSIGN, PREC_ASSIGN, 1},
    {TK_MINUS_ASSIGN, COMPOUND_SUB_ASSIGN, PREC_ASSIGN, 1},
    {TK_STAR_ASSIGN, COMPOUND_MUL_ASSIGN, PREC_ASSIGN, 1},
    {TK_SLASH_ASSIGN, COMPOUND_DIV_ASSIGN, PREC_ASSIGN, 1},
    {TK_AND_ASSIGN, COMPOUND_AND_ASSIGN, PREC_ASSIGN, 1},
    {TK_OR_ASSIGN, COMPOUND_OR_ASSIGN, PREC_ASSIGN, 1},
    {TK_XOR_ASSIGN, COMPOUND_XOR_ASSIGN, PREC_ASSIGN, 1},
    {TK_LSHIFT_ASSIGN, COMPOUND_LSHIFT_ASSIGN, PREC_ASSIGN, 1},
    {TK_RSHIFT_ASSIGN, COMPOUND_RSHIFT_ASSIGN, PREC_ASSIGN, 1},

    {TK_OR, LOGIC_OR, PREC_OR, 0},
    {TK_AND, LOGIC_AND, PREC_AND, 0},

    {TK_BIT_OR, BITWISE_OR, PREC_BITWISE_OR, 0},
    {TK_BIT_XOR, BITWISE_XOR, PREC_BITWISE_XOR, 0},
    {TK_AMPERSAND, BITWISE_AND, PREC_BITWISE_AND, 0},
    {TK_LSHIFT, BITWISE_LSHIFT, PREC_SHIFT, 0},
    {TK_RSHIFT, BITWISE_RSHIFT, PREC_SHIFT, 0},

    {TK_EQ, EQUAL_OP, PREC_EQUALITY, 0},
    {TK_NOT_EQ, NOT_EQUAL_OP, PREC_EQUALITY, 0},
    {TK_LESS, LESS_THAN_OP, PREC_COMPARISON, 0},
    {TK_GREATER, GREATER_THAN_OP, PREC_COMPARISON, 0},
    {TK_LESS_EQ, LESS_EQUAL_OP, PREC_COMPARISON, 0},
    {TK_GREATER_EQ, GREATER_EQUAL_OP, PREC_COMPARISON, 0},
    {TK_PLUS, ADD_OP, PREC_TERM, 0},
    {TK_MINUS, SUB_OP, PREC_TERM, 0},
    {TK_STAR, MUL_OP, PREC_FACTOR, 0},
    {TK_SLASH, DIV_OP, PREC_FACTOR, 0},
    {TK_MOD, MOD_OP, PREC_FACTOR, 0},
    {TK_AS, CAST_EXPRESSION, PREC_CAST, 0},
    {TK_NULL, null_NODE, PREC_NONE, 0}
};


// helpers
NodeTypes getDecType(TokenType type);
ASTNode createNode(const Token* token, NodeTypes type, TokenList * list, size_t * pos);
const char *getNodeTypeName(NodeTypes nodeType);
ASTNode createValNode(const Token* current_token, TokenList *list, size_t* pos);
const OperatorInfo *getOperatorInfo(TokenType type);
int isTypeToken(TokenType type);
NodeTypes getUnaryOpType(TokenType t);
NodeTypes detectLitType(const Token* tok, TokenList * list, size_t * pos);
char* extractText(const char* start, size_t length);
int nodeValueEquals(ASTNode node, const char* str);

// Parser function declarations
ASTNode parseStatement(TokenList* list, size_t* pos);
ASTNode parseExpression(TokenList* list, size_t* pos, Precedence minPrec);
ASTNode parseUnary(TokenList* list, size_t* pos);
ASTNode parsePrimaryExp(TokenList* list, size_t* pos);
ASTNode parseFunction(TokenList* list, size_t* pos);
ASTNode parseFunctionCall(TokenList* list, size_t* pos, Token * tok);
ASTNode parseReturnStatement(TokenList* list, size_t* pos);
ASTNode parseLoop(TokenList* list, size_t* pos);
ASTNode parseBlock(TokenList* list, size_t* pos);
ASTNode parseIf(TokenList *list, size_t* pos);
ASTNode parseBlockExpression(TokenList* list, size_t* pos);
ASTNode parseParameter(TokenList* list, size_t* pos);
ASTNode parseArg(TokenList* list, size_t* pos);
ASTNode parseCommaSeparatedLists(TokenList* list, size_t* pos, NodeTypes listType,
                                  ASTNode (*parseElement)(TokenList*, size_t*));
ASTNode parseTernary(TokenList* list, size_t* pos);
ASTNode parseReturnType(TokenList* list, size_t* pos);
ASTNode parseDeclaration(TokenList* list, size_t* pos);
ASTNode parseExpressionStatement(TokenList* list, size_t* pos);
ASTNode parseStruct(TokenList* list, size_t* pos);
ASTNode parseStructField(TokenList* list, size_t* pos);
NodeTypes getTypeNodeFromToken(TokenType type);
ASTNode parseArrayDec(TokenList *list, size_t *pos, Token *varName);
ASTNode parseArrLit(TokenList *list, size_t *pos);
ASTNode parseStructLit(TokenList *list, size_t *pos);
ASTNode parseArrayAccess(TokenList *list, size_t *pos, ASTNode arrNode);
ASTNode parseImport(TokenList *list, size_t *pos);
ASTNode parseExportFunction(TokenList* list, size_t* pos);
ASTNode parseForLoop(TokenList *list, size_t *pos);

// Public function prototypes
ASTContext * ASTGenerator(TokenList* tokenList);
void printAST(ASTNode node, int depth);
void printASTTree(ASTNode node, char *prefix, int isLast);
void freeAST(ASTNode node);
void freeASTContext(ASTContext* ctx);


#endif //PARSER_H
