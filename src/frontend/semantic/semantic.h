/**
 * @file semantic.h
 * @brief Public API for the semantic analysis.
 *
 * This is the ONLY header that code outside semantic/ should include.
 * Provides type definitions, symbol table interface, type checking context,
 * and the main entry point for semantic analysis of AST trees.
 */

#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "parser.h"
#include "errorHandling.h"

#define SYMBOL_TABLE_BUCKETS 32
#define SYMBOL_TABLE_LOAD_FACTOR 0.75

struct TypeCheckContext;
typedef struct TypeCheckContext* TypeCheckContext;

struct SymbolTable;
typedef struct SymbolTable* SymbolTable;

struct StructType;
typedef struct StructType *StructType;

struct Symbol;
typedef struct Symbol *Symbol;

typedef enum {
    TYPE_I8,
    TYPE_I16,
    TYPE_I32,
    TYPE_I64,
    TYPE_U8,
    TYPE_U16,
    TYPE_U32,
    TYPE_U64,
    TYPE_FLOAT,
    TYPE_DOUBLE,
    TYPE_STRING,
    TYPE_BOOL,
    TYPE_VOID,
    TYPE_STRUCT,
    TYPE_POINTER,
    TYPE_NULL,
    TYPE_UNKNOWN
} DataType;

typedef enum {
    SYMBOL_VARIABLE,
    SYMBOL_FUNCTION,
    SYMBOL_TYPE,
} SymbolType;

typedef enum {
    COMPAT_ERROR = 0,
    COMPAT_OK = 1,
    COMPAT_WARNING = 2
} CompatResult;

typedef enum {
    BUILTIN_PRINT_INT,
    BUILTIN_PRINT_FLOAT,
    BUILTIN_PRINT_DOUBLE,
    BUILTIN_PRINT_STRING,
    BUILTIN_PRINT_BOOL,
    BUILTIN_EXIT,
    BUILTIN_READ_INT,
    BUILTIN_READ_STRING,
    BUILTIN_SYSCALL,
    BUILTIN_UNKNOWN
} BuiltInId;

typedef struct {
    DataType type;
    StructType structType;
} ResolvedType;

typedef struct StructField {
    const char *nameStart;
    size_t nameLength;
    DataType type;
    StructType structType;
    int isPointer;
    int pointerLevel;
    size_t offset;
    struct StructField *next;
} *StructField;

typedef struct StructType {
    const char *nameStart;
    size_t nameLength;
    StructField fields;
    size_t size;
    int fieldCount;
} *StructType;

typedef struct FunctionParameter {
    const char *nameStart;
    size_t nameLength;
    DataType type;
    int isPointer;
    int pointerLevel;
    struct FunctionParameter *next;
} *FunctionParameter;

/**
 * @brief Symbol structure representing a declared identifier.
 */
typedef struct Symbol {
    const char *nameStart;
    uint16_t nameLength;
    SymbolType symbolType;
    DataType type;
    StructType structType;
    union {
        struct {
            FunctionParameter parameters;
            struct Symbol* returnedVar;
            int paramCount;
            int returnsPointer;
            int returnPointerLevel;
            SymbolTable functionScope;
            DataType returnBaseType;
        };
        struct {
            int isInitialized;
            int isConst;
            int isArray;
            int staticSize;
            int constVal;
            int hasConstVal;
            int hasConstMemRef;
            int isPointer;
            int pointerLvl;
            DataType baseType;
        };
    };
    int line;
    int column;
    int scope;
    struct Symbol *next;
} *Symbol;

/**
 * @brief Symbol table structure for managing symbols within a scope.
 */
typedef struct SymbolTable {
    Symbol *symbols;
    int bucketCount;
    struct SymbolTable *parent;
    struct SymbolTable *child;
    struct SymbolTable *brother;
    int scope;
    int symbolCount;
} *SymbolTable;

typedef struct BlockScopeNode {
    SymbolTable scope;
    struct BlockScopeNode *next;
} *BlockScopeNode;

/**
 * @brief Type checking context for managing symbol tables and scope.
 */
typedef struct TypeCheckContext {
    SymbolTable current;
    SymbolTable global;
    Symbol currentFunction;
    const char *sourceFile;
    const char *filename;
    BlockScopeNode blockScopesHead;
    BlockScopeNode blockScopesTail;
} *TypeCheckContext;

/* Entry point */

TypeCheckContext typeCheckAST(ASTNode ast, const char *sourceCode,
                              const char *filename, TypeCheckContext ref);
TypeCheckContext createTypeCheckContext(const char *sourceCode, const char *filename);
void freeTypeCheckContext(TypeCheckContext context);

/* Symbol table */

SymbolTable createSymbolTable(SymbolTable parent);
void freeSymbolTable(SymbolTable symbolTable);
void freeSymbol(Symbol symbol);

Symbol addSymbol(SymbolTable table, const char *nameStart, size_t nameLength, DataType type, int line, int column);
Symbol addSymbolFromNode(SymbolTable table, ASTNode node, DataType type);

Symbol addFunctionSymbolFromNode(SymbolTable symbolTable, ASTNode node, DataType returnType, FunctionParameter parameters, int paramCount);
Symbol addFunctionSymbolFromString(SymbolTable symbolTable, const char *name, DataType returnType, FunctionParameter parameters, int paramCount, int line, int column);

Symbol lookupSymbol(SymbolTable symbolTable, const char *name, size_t len);
Symbol lookupSymbolCurrentOnly(SymbolTable table, const char *nameStart, size_t nameLength);

FunctionParameter createParameter(const char *nameStart, size_t nameLen, DataType type);
void freeParamList(FunctionParameter paramList);

/* Symbol resolution */

DataType getDataTypeFromNode(NodeTypes nodeType);

/* Type system */

CompatResult areCompatible(DataType target, DataType source);
DataType getOperationResultType(DataType left, DataType right, NodeTypes op);
DataType getExpressionType(ASTNode node, TypeCheckContext context);
ErrorCode variableErrorCompatibleHandling(DataType varType, DataType initType);
const char *getTypeName(DataType type);
int getStackSize(DataType type);
int isIntegerType(DataType type);
int isSignedInt(DataType type);

/* Scope management */

void enqueueBlockScope(TypeCheckContext context, SymbolTable scope);
SymbolTable dequeueBlockScope(TypeCheckContext context);

/* Built ins */

void initBuiltIns(SymbolTable globalTable);
BuiltInId resolveOverload(const char *nameStart, size_t nameLength, DataType arg[], int argCount);
int isBuiltinFunction(const char *nameStart, size_t nameLength);

/* semantic checks */

int validateVariableDeclaration(ASTNode node, TypeCheckContext context, int isConst);
int validateAssignment(ASTNode node, TypeCheckContext context);
int validateVariableUsage(ASTNode node, TypeCheckContext context);
int typeCheckNode(ASTNode node, TypeCheckContext context);
int typeCheckChildren(ASTNode node, TypeCheckContext context);
int validateFunctionDef(ASTNode node, TypeCheckContext context);
int validateFunctionCall(ASTNode node, TypeCheckContext context);
int validateReturnStatement(ASTNode node, TypeCheckContext context);
FunctionParameter extractParameters(ASTNode paramListNode);
DataType getReturnTypeFromNode(ASTNode returnTypeNode, int *outPointerLevel);
int validateBuiltinFunctionCall(ASTNode node, TypeCheckContext context);
int validateUserDefinedFunctionCall(ASTNode node, TypeCheckContext context);
int validateCastExpression(ASTNode node, TypeCheckContext context);

/* Error context creation */

ErrorContext *createErrorContextFromType(ASTNode node, TypeCheckContext context);
void freeErrorContext(ErrorContext *errCtx);

#endif // SEMANTIC_H