/**
 * @file semanticInternal.h
 * @brief Internal shared header for semantic analysis modules.
 *
 * This header is included ONLY by semantic/*.c files.
 * It must NEVER be included outside the semantic/ directory.
 *
 * Contains internal function prototypes and helper declarations.
 */

#ifndef SEMANTIC_INTERNAL_H
#define SEMANTIC_INTERNAL_H

#include "semantic.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* External Functions */

extern int parseInt(const char *start, size_t length);
extern char *extractText(const char *start, size_t length);

/* types helpers */

ASTNode getBaseTypeFromPointerChain(ASTNode typeRefNode, int *outPointerLevel);
ResolvedType resolveMemberAccessType(ASTNode node, TypeCheckContext context);
DataType validateMemberAccess(ASTNode node, TypeCheckContext context);
int isPrecisionLossCast(DataType source, DataType target);
int isNumType(DataType type);
CompatResult isCastAllowed(DataType target, DataType source);

/* symbol helpers */

Symbol lookupSymbolOrError(TypeCheckContext context, ASTNode node);

/* error helpers */

void reportErrorWithText(ErrorCode error, ASTNode node, TypeCheckContext context, const char *fallbackMsg);
char *extractSourceLine(const char *source, int lineNum);

/* semantic checks helpers */

int checkConstViolation(Symbol sym, ASTNode node, TypeCheckContext context, int isPointerDeref);
int validateArrayAccessNode(ASTNode arrayAccess, TypeCheckContext context);
int validateAddressOf(ASTNode addrNode, TypeCheckContext context);
void updateConstMemRef(Symbol targetSymbol, ASTNode sourceNode, TypeCheckContext context);
int validatePointerLevels(Symbol targetSym, Symbol sourceSym, ASTNode node, TypeCheckContext context, int isMemRef);
int validateArrayLiteralInit(ASTNode arrLitNode, DataType expectedType, int expectedSize, int isConst, TypeCheckContext context);
int validateArrayCopyInit(ASTNode sourceVarNode, Symbol targetSym, TypeCheckContext context);
int validateArrayInitialization(Symbol newSymbol, ASTNode initNode, DataType varType, int isConst, TypeCheckContext context);
int validateScalarInitialization(Symbol newSymbol, ASTNode node, DataType varType, int isConst, int isMemRef, TypeCheckContext context);
int validateStructInlineInitialization(Symbol sym, ASTNode init, DataType type, int isConst, TypeCheckContext ctx);
int containsReturnStatement(ASTNode node);
int validateStructDef(ASTNode node, TypeCheckContext context);
int validateStructVarDec(ASTNode node, TypeCheckContext context);
StructType createStructType(ASTNode node, TypeCheckContext context);

/* builtin function type */

typedef struct BuiltInFunction {
    char *name;
    DataType returnType;
    DataType *paramTypes;
    char **paramNames;
    int paramCount;
    BuiltInId id;
} BuiltInFunction;

#endif /* SEMANTIC_INTERNAL_H */