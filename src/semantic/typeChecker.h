//
// Created by pablo on 08/09/2025.
//
#ifndef CINTERPRETER_TYPECHECKER_H
#define CINTERPRETER_TYPECHECKER_H
#include "symbolTable.h"
#include "../errorHandling/errorHandling.h"

typedef struct BlockScopeNode {
    SymbolTable scope;
    struct BlockScopeNode *next;
} *BlockScopeNode;

/**
 * @brief Type checking context for managing symbol tables and scope.
 *
 * Maintains the current symbol table context for type checking operations.
 * Tracks both the global scope and current active scope to enable proper
 * variable resolution and scope management during AST traversal.
 */
typedef struct TypeCheckContext {
    SymbolTable current;
    SymbolTable global;
    Symbol currentFunction;
    const char *sourceFile;
    const char *filename;

    // Block scope tracking for IR generation
    BlockScopeNode blockScopesHead;  // Queue head (for dequeue during IR)
    BlockScopeNode blockScopesTail;  // Queue tail (for enqueue during type checking)
} *TypeCheckContext;

typedef enum {
    COMPAT_ERROR = 0,
    COMPAT_OK = 1,
    COMPAT_WARNING = 2
} CompatResult;

TypeCheckContext createTypeCheckContext(const char *sourceCode, const char *filename);
void freeTypeCheckContext(TypeCheckContext context);
CompatResult areCompatible(DataType target, DataType source);
DataType getOperationResultType(DataType left, DataType right, NodeTypes op);
DataType getExpressionType(ASTNode node, TypeCheckContext context);
ErrorCode variableErrorCompatibleHandling(DataType varType, DataType initType);
int validateVariableDeclaration(ASTNode node, TypeCheckContext context, int isConst);
int validateAssignment(ASTNode node, TypeCheckContext context);
int validateVariableUsage(ASTNode node, TypeCheckContext context);
int typeCheckNode(ASTNode node, TypeCheckContext context);
int typeCheckChildren(ASTNode node, TypeCheckContext context);
TypeCheckContext typeCheckAST(ASTNode ast, const char *sourceCode, const char *filename, TypeCheckContext ref);
int validateFunctionDef(ASTNode node, TypeCheckContext context);
int validateFunctionCall(ASTNode node, TypeCheckContext context);
int validateReturnStatement(ASTNode node, TypeCheckContext context);
FunctionParameter extractParameters(ASTNode paramListNode);
DataType getReturnTypeFromNode(ASTNode returnTypeNode, int *outPointerLevel);
int validateBuiltinFunctionCall(ASTNode node, TypeCheckContext context);
int validateUserDefinedFunctionCall(ASTNode node, TypeCheckContext context);
int validateCastExpression(ASTNode node, TypeCheckContext context);

void enqueueBlockScope(TypeCheckContext context, SymbolTable scope);
SymbolTable dequeueBlockScope(TypeCheckContext context);

#endif //CINTERPRETER_TYPECHECKER_H
