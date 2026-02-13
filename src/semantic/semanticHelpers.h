//
// Created by pablo on 15/09/2025.
//

#ifndef CINTERPRETER_SEMANTICHELPERS_H
#define CINTERPRETER_SEMANTICHELPERS_H

#include "parser.h"
#include "errorHandling.h"
#include "typeChecker.h"


ErrorContext *createErrorContextFromType(ASTNode node, TypeCheckContext context);
void reportErrorWithText(ErrorCode error, ASTNode node, TypeCheckContext context, 
                                const char *fallbackMsg);
int checkConstViolation(Symbol sym, ASTNode node, TypeCheckContext context, 
                                int isPointerDeref);
int validateArrayAccessNode(ASTNode arrayAccess, TypeCheckContext context);
int validateAddressOf(ASTNode addrNode, TypeCheckContext context);
void updateConstMemRef(Symbol targetSymbol, ASTNode sourceNode, TypeCheckContext context);
int validatePointerLevels(Symbol targetSym, Symbol sourceSym, ASTNode node, 
                                TypeCheckContext context, int isMemRef);
int validateArrayLiteralInit(ASTNode arrLitNode, DataType expectedType, 
                                    int expectedSize, int isConst, 
                                    TypeCheckContext context);
int validateArrayCopyInit(ASTNode sourceVarNode, Symbol targetSym, 
                                 TypeCheckContext context);
int validateArrayInitialization(Symbol newSymbol, ASTNode initNode, 
                                       DataType varType, int isConst,
                                       TypeCheckContext context);
int validateScalarInitialization(Symbol newSymbol, ASTNode node, 
                                        DataType varType, int isConst, int isMemRef,
                                        TypeCheckContext context);
void freeErrorContext(ErrorContext * ctx);
int validateStructInlineInitialization(Symbol sym, ASTNode init, DataType type, int isConst, TypeCheckContext ctx);
Symbol lookupSymbolOrError(TypeCheckContext context, ASTNode node);

#endif //CINTERPRETER_SEMANTICHELPERS_H