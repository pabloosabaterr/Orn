//
// Created by pablo on 15/09/2025.
//

#include "semanticHelpers.h"


#include <stdlib.h>
#include <string.h>
#include <stdio.h>

extern int parseInt(const char* start, size_t length);
extern const char* getTypeName(DataType type);
extern char* extractText(const char* start, size_t length);
extern DataType getExpressionType(ASTNode node, TypeCheckContext context);
extern CompatResult areCompatible(DataType type1, DataType type2);
extern ErrorCode variableErrorCompatibleHandling(DataType expected, DataType got);

Symbol lookupSymbolOrError(TypeCheckContext context, ASTNode node) {
    Symbol sym = lookupSymbol(context->current, node->start, node->length);
    if (!sym) {
        reportErrorWithText(ERROR_UNDEFINED_VARIABLE, node, context, "Undefined variable");
    }
    return sym;
}

char *extractSourceLine(const char *source, int lineNum) {
    if (!source || lineNum <= 0) return NULL;

    const char *lineStart = source;
    int currentLine = 1;

    // Find the start of the requested line
    while (*lineStart && currentLine < lineNum) {
        if (*lineStart == '\n') {
            currentLine++;
        }
        lineStart++;
    }

    if (*lineStart == '\0') return NULL;

    // Find the end of the line
    const char *lineEnd = lineStart;
    while (*lineEnd && *lineEnd != '\n') {
        lineEnd++;
    }

    // Create the line string
    size_t lineLength = lineEnd - lineStart;
    char *line = malloc(lineLength + 1);
    if (line) {
        strncpy(line, lineStart, lineLength);
        line[lineLength] = '\0';
    }

    return line;
}

ErrorContext *createErrorContextFromType(ASTNode node, TypeCheckContext context) {
    if(!node || !context) return NULL;
    ErrorContext * errCtx = malloc(sizeof(ErrorContext));
    if(!errCtx) return NULL;
    char * sourceLine = NULL;
    if(context->sourceFile){
        sourceLine = extractSourceLine(context->sourceFile, node->line);
    }
    errCtx->file = context->filename ? context->filename : "source";
    errCtx->line = node->line;
    errCtx->column = node->column;
    errCtx->source = sourceLine;
    errCtx->length = node->length;
    errCtx->startColumn = node->column;
    return errCtx;
}

void reportErrorWithText(ErrorCode error, ASTNode node, TypeCheckContext context, 
                                const char *fallbackMsg) {
    char *tempText = extractText(node->start, node->length);
    REPORT_ERROR(error, node, context, tempText ? tempText : fallbackMsg);
    free(tempText);
}

int checkConstViolation(Symbol sym, ASTNode node, TypeCheckContext context, 
                                int isPointerDeref) {
    if (!sym) return 0;
    
    if (!isPointerDeref && sym->isConst) {
        reportErrorWithText(ERROR_CONSTANT_REASSIGNMENT, node, context, "Cannot modify const");
        return 1;
    }
    
    if (isPointerDeref && sym->hasConstMemRef) {
        REPORT_ERROR(ERROR_CONSTANT_REASSIGNMENT, node, context, 
                    "Cannot modify through pointer to const");
        return 1;
    }
    
    return 0;
}

int validateArrayAccessNode(ASTNode arrayAccess, TypeCheckContext context) {
    if (arrayAccess->nodeType != ARRAY_ACCESS) return 1;
    
    ASTNode baseNode = arrayAccess->children;
    if (!baseNode || baseNode->nodeType != VARIABLE) return 1;
    
    Symbol arraySym = lookupSymbolOrError(context, baseNode);
    if (!arraySym) return 0;
    
    if (!arraySym->isArray && !arraySym->isPointer) {
        reportErrorWithText(ERROR_INVALID_OPERATION_FOR_TYPE, baseNode, context,
                          "Array subscript requires array or pointer type");
        return 0;
    }
    
    ASTNode indexNode = baseNode->brothers;
    if (!indexNode) return 1;
    
    DataType indexType = getExpressionType(indexNode, context);
    if (indexType == TYPE_UNKNOWN) {
        REPORT_ERROR(ERROR_ARRAY_INDEX_INVALID_EXPR, indexNode, context,
                    "Invalid array index expression");
        return 0;
    }
    
    if (indexType != TYPE_INT) {
        REPORT_ERROR(ERROR_ARRAY_INDEX_NOT_INTEGER, indexNode, context,
                    "Array index must be integer type");
        return 0;
    }
    
    if (arraySym->isArray && indexNode->nodeType == LITERAL) {
        int indexValue = parseInt(indexNode->start, indexNode->length);
        if (indexValue < 0 || indexValue >= arraySym->staticSize) {
            char msg[100];
            snprintf(msg, sizeof(msg), "Array index %d out of bounds [0, %d)",
                    indexValue, arraySym->staticSize);
            REPORT_ERROR(ERROR_INVALID_EXPRESSION, indexNode, context, msg);
            return 0;
        }
    }
    
    return 1;
}

int validateAddressOf(ASTNode addrNode, TypeCheckContext context) {
    if (addrNode->nodeType == LITERAL) {
        REPORT_ERROR(ERROR_CANNOT_TAKE_ADDRESS_OF_LITERAL, addrNode, context,
                    "Cannot take address of literal");
        return 0;
    }
    
    if (addrNode->nodeType != VARIABLE && addrNode->nodeType != ARRAY_ACCESS) {
        REPORT_ERROR(ERROR_CANNOT_TAKE_ADDRESS_OF_TEMPORARY, addrNode, context,
                    "Cannot take address of temporary expression");
        return 0;
    }
    
    return 1;
}

void updateConstMemRef(Symbol targetSymbol, ASTNode sourceNode, TypeCheckContext context) {
    if (!targetSymbol) return;
    
    if (sourceNode->nodeType == VARIABLE) {
        Symbol sourceSym = lookupSymbol(context->current, sourceNode->start, sourceNode->length);
        if (sourceSym) {
            targetSymbol->hasConstMemRef = sourceSym->isConst || sourceSym->hasConstMemRef;
        }
    } else if (sourceNode->nodeType == ARRAY_ACCESS) {
        ASTNode baseNode = sourceNode->children;
        if (baseNode && baseNode->nodeType == VARIABLE) {
            Symbol baseSym = lookupSymbol(context->current, baseNode->start, baseNode->length);
            if (baseSym && baseSym->isConst) {
                targetSymbol->hasConstMemRef = 1;
            }
        }
    }
}

int validatePointerLevels(Symbol targetSym, Symbol sourceSym, ASTNode node, 
                                TypeCheckContext context, int isMemRef) {
    if (!targetSym->isPointer || !sourceSym) return 1;
    
    int expectedLevel;
    if (isMemRef) {
        expectedLevel = sourceSym->isPointer ? sourceSym->pointerLvl + 1 : 1;
    } else {
        expectedLevel = sourceSym->isPointer ? sourceSym->pointerLvl : 0;
    }
    
    if (targetSym->pointerLvl != expectedLevel) {
        char msg[100];
        snprintf(msg, sizeof(msg),
                "Cannot initialize pointer of level %d with pointer of level %d",
                targetSym->pointerLvl, expectedLevel);
        REPORT_ERROR(ERROR_INVALID_EXPRESSION, node, context, msg);
        return 0;
    }
    
    return 1;
}

int validateArrayLiteralInit(ASTNode arrLitNode, DataType expectedType, int expectedSize, int isConst, TypeCheckContext context) {
    if (arrLitNode->nodeType != ARRAY_LIT) return 0;
    
    int initCount = 0;
    ASTNode elem = arrLitNode->children;
    
    while (elem) {
        DataType elemType = getExpressionType(elem, context);
        CompatResult compat = areCompatible(expectedType, elemType);
        
        if (elemType == TYPE_UNKNOWN || compat == COMPAT_ERROR) {
            char msg[100];
            snprintf(msg, sizeof(msg), 
                    "Array element %d has incompatible type", initCount + 1);
            REPORT_ERROR(ERROR_ARRAY_INIT_ELEMENT_TYPE, elem, context, msg);
            return 0;
        }
        
        initCount++;
        elem = elem->brothers;
    }
    
    // let dec can have initilization with less elements than declared, but not more
    if (initCount != expectedSize && isConst) {
        char msg[100];
        snprintf(msg, sizeof(msg), 
                "declared %d, initialized with %d elements",
                expectedSize, initCount);
        REPORT_ERROR(ERROR_ARRAY_INIT_SIZE_MISMATCH, arrLitNode, context, msg);
        return 0;
    }
    
    return 1;
}

int validateArrayCopyInit(ASTNode sourceVarNode, Symbol targetSym, 
                                 TypeCheckContext context) {
    Symbol sourceSym = lookupSymbol(context->current, sourceVarNode->start, 
                                   sourceVarNode->length);
    
    if (!sourceSym) {
        reportErrorWithText(ERROR_UNDEFINED_VARIABLE, sourceVarNode, context, 
                          "Undefined variable");
        return 0;
    }
    
    if (!sourceSym->isArray) {
        REPORT_ERROR(ERROR_CANNOT_ASSIGN_SCALAR_TO_ARRAY, sourceVarNode, context,
                    "Cannot initialize array with scalar value");
        return 0;
    }
    
    if (sourceSym->type != targetSym->type) {
        char msg[100];
        snprintf(msg, sizeof(msg), "cannot assign %s[] to %s[]",
                getTypeName(sourceSym->type), getTypeName(targetSym->type));
        REPORT_ERROR(ERROR_ARRAY_LITERAL_TYPE_MISMATCH, sourceVarNode, context, msg);
        return 0;
    }
    
    if (sourceSym->staticSize != targetSym->staticSize) {
        char msg[100];
        snprintf(msg, sizeof(msg),
                "cannot assign array of size %d to array of size %d",
                sourceSym->staticSize, targetSym->staticSize);
        REPORT_ERROR(ERROR_ARRAY_SIZE_MISMATCH, sourceVarNode, context, msg);
        return 0;
    }
    
    return 1;
}

int validateStructInlineInitialization(Symbol sym, ASTNode init, DataType type, int isConst, TypeCheckContext ctx){
    ASTNode valNode = init;
    // initilization neededness happens on caller
    if(!valNode || valNode->nodeType != VALUE || !valNode->children){
        return 1;
    }
    sym->isInitialized = 1;
    ASTNode structLit = valNode->children;
    if(structLit->nodeType == STRUCT_LIT){

    }

    return 1;
}

/**
 * @brief Validates if a variable initialization is valid for scalar types.
 */
int validateArrayInitialization(Symbol newSymbol, ASTNode initNode, DataType varType, int isConst,
                                TypeCheckContext context) {
    ASTNode valueNode = initNode;
    // initialization neededness happens on caller
    if (!valueNode || valueNode->nodeType != VALUE || !valueNode->children) {
        return 1;
    }

    ASTNode arrLit = valueNode->children;

    if (arrLit->nodeType == ARRAY_LIT) {
        // when const same size must be initialized, but when not const can be less (but not more) than declared
        if (!validateArrayLiteralInit(arrLit, varType, newSymbol->staticSize, isConst, context)) {
            return 0;
        }
        newSymbol->isInitialized = 1;
        return 1;
    }

    if (arrLit->nodeType == VARIABLE) {
        if (!validateArrayCopyInit(arrLit, newSymbol, context)) {
            return 0;
        }
        newSymbol->isInitialized = 1;
        return 1;
    }

    return 1;
}

int validateScalarInitialization(Symbol newSymbol, ASTNode node, 
                                        DataType varType, int isConst, int isMemRef,
                                        TypeCheckContext context) {
    ASTNode initExprForType = node->children->brothers->children;
    ASTNode initExpr = isMemRef ? 
        node->children->brothers->children->children : 
        node->children->brothers->children;
    
    if (initExpr && initExpr->nodeType == VARIABLE) {
        Symbol initSymbol = lookupSymbol(context->current, initExpr->start, initExpr->length);
        
        if (initSymbol) {
            if (initSymbol->isArray) {
                reportErrorWithText(ERROR_CANNOT_ASSIGN_ARRAY_TO_SCALAR, node, context,
                                  "Cannot assign array to scalar");
                return 0;
            }
            updateConstMemRef(newSymbol, initExpr, context);
        }
    } else if (initExpr && initExpr->nodeType == ARRAY_ACCESS) {
        updateConstMemRef(newSymbol, initExpr, context);
    }
    

    DataType initType = getExpressionType(initExprForType, context);
    if (initType == TYPE_UNKNOWN) {
        reportErrorWithText(ERROR_INTERNAL_TYPECHECKER_ERROR, node, context, "Cannot determine initialization type");
        return 0;
    }
    
    CompatResult compat = areCompatible(newSymbol->type, initType);
    if (compat == COMPAT_ERROR) {
        reportErrorWithText(variableErrorCompatibleHandling(varType, initType), 
                        node, context, "Type mismatch");
        return 0;
    } else if (compat == COMPAT_WARNING) {
        reportErrorWithText(ERROR_TYPE_MISMATCH_DOUBLE_TO_FLOAT, node, context,
                        "Precision loss warning");
    }
    
    // Pointer level validation
    if (newSymbol->isPointer && initExpr->nodeType == VARIABLE) {
        Symbol initSym = lookupSymbol(context->current, initExpr->start, initExpr->length);
        if (initSym && !validatePointerLevels(newSymbol, initSym, node, context, isMemRef)) {
            return 0;
        }
    }
    
    newSymbol->isInitialized = 1;
    
    // Track constant values for compile-time evaluation
    if (isConst && node->children->brothers->children->nodeType == LITERAL) {
        newSymbol->hasConstVal = 1;
        newSymbol->constVal = parseInt(node->children->brothers->children->start, 
                                    node->children->brothers->children->length);
    }
    
    return 1;
}

void freeErrorContext(ErrorContext * errCtx){
    if(errCtx){
        free(errCtx->source);
        free(errCtx);
    }
}