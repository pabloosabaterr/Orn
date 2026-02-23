/**
 * @file semanticCheck.c
 * @brief AST semantic validation logic.
 *
 * Responsibilities:
 *   - Variable declaration validation
 *   - Assignment validation
 *   - Variable usage validation
 *   - Function definition & call validation
 *   - Return statement validation
 *   - Cast expression validation
 *   - Struct definition & variable validation
 *   - Array/scalar/pointer initialization validation
 *   - Const violation checking
 *
 * Uses: semanticTypes, semanticSymbols, semanticScope
 * Equivalent to parserStatement.c + parserExpression.c in the parser subsystem.
 */

#include "semanticInternal.h"

#include <assert.h>

/**
 * HELPERS
 */

int checkConstViolation(Symbol sym, ASTNode node, TypeCheckContext context, int isPointerDeref) {
    if (!sym) return 0;

    if (!isPointerDeref && sym->isConst) {
        reportErrorWithText(ERROR_CONSTANT_REASSIGNMENT, node, context, "Cannot modify const");
        return 1;
    }

    if (isPointerDeref && sym->hasConstMemRef) {
        REPORT_ERROR(ERROR_CONSTANT_REASSIGNMENT, node, context, "Cannot modify through pointer to const");
        return 1;
    }

    return 0;
}

/**
 * arrays
 */

int validateArrayAccessNode(ASTNode arrayAccess, TypeCheckContext context) {
    if (arrayAccess->nodeType != ARRAY_ACCESS) return 1;

    ASTNode baseNode = arrayAccess->children;
    if (!baseNode || baseNode->nodeType != VARIABLE) return 1;

    Symbol arraySym = lookupSymbolOrError(context, baseNode);
    if (!arraySym) return 0;

    if (!arraySym->isArray && !arraySym->isPointer) {
        reportErrorWithText(ERROR_INVALID_OPERATION_FOR_TYPE, baseNode, context, "Array subscript requires array or pointer type");
        return 0;
    }

    ASTNode indexNode = baseNode->brothers;
    if (!indexNode) return 1;

    DataType indexType = getExpressionType(indexNode, context);
    if (indexType == TYPE_UNKNOWN) {
        REPORT_ERROR(ERROR_ARRAY_INDEX_INVALID_EXPR, indexNode, context, "Invalid array index expression");
        return 0;
    }

    if (indexType != TYPE_INT) {
        REPORT_ERROR(ERROR_ARRAY_INDEX_NOT_INTEGER, indexNode, context, "Array index must be integer type");
        return 0;
    }

    if (arraySym->isArray && indexNode->nodeType == LITERAL) {
        int indexValue = parseInt(indexNode->start, indexNode->length);
        if (indexValue < 0 || indexValue >= arraySym->staticSize) {
            char msg[100];
            snprintf(msg, sizeof(msg), "Array index %d out of bounds [0, %d)", indexValue, arraySym->staticSize);
            REPORT_ERROR(ERROR_INVALID_EXPRESSION, indexNode, context, msg);
            return 0;
        }
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

    /* let dec can have initialization with less elements than declared, but not more */
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

int validateArrayCopyInit(ASTNode sourceVarNode, Symbol targetSym, TypeCheckContext context) {
    Symbol sourceSym = lookupSymbol(context->current, sourceVarNode->start, sourceVarNode->length);

    if (!sourceSym) {
        reportErrorWithText(ERROR_UNDEFINED_VARIABLE, sourceVarNode, context, "Undefined variable");
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

/**
 * Pointers, addressof ,reference and dereference
 */

int validateAddressOf(ASTNode addrNode, TypeCheckContext context) {
    if (addrNode->nodeType == LITERAL) {
        REPORT_ERROR(ERROR_CANNOT_TAKE_ADDRESS_OF_LITERAL, addrNode, context, "Cannot take address of literal");
        return 0;
    }

    if (addrNode->nodeType != VARIABLE && addrNode->nodeType != ARRAY_ACCESS) {
        REPORT_ERROR(ERROR_CANNOT_TAKE_ADDRESS_OF_TEMPORARY, addrNode, context, "Cannot take address of temporary expression");
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

int validatePointerLevels(Symbol targetSym, Symbol sourceSym, ASTNode node, TypeCheckContext context, int isMemRef) {
    if (!targetSym->isPointer || !sourceSym) return 1;

    int expectedLevel;
    if (isMemRef) {
        expectedLevel = sourceSym->isPointer ? sourceSym->pointerLvl + 1 : 1;
    } else {
        expectedLevel = sourceSym->isPointer ? sourceSym->pointerLvl : 0;
    }

    if (targetSym->pointerLvl != expectedLevel) {
        char msg[100];
        snprintf(msg, sizeof(msg), "Cannot initialize pointer of level %d with pointer of level %d", targetSym->pointerLvl, expectedLevel);
        REPORT_ERROR(ERROR_INVALID_EXPRESSION, node, context, msg);
        return 0;
    }

    return 1;
}

/**
 * structs
 */

int validateStructInlineInitialization(Symbol sym, ASTNode init, DataType type, int isConst, TypeCheckContext ctx) {
    ASTNode valNode = init;
    /* initialization neededness happens on caller */
    if (!valNode || valNode->nodeType != VALUE || !valNode->children) {
        return 1;
    }
    sym->isInitialized = 1;
    ASTNode structLit = valNode->children;
    if (structLit->nodeType == STRUCT_LIT) {
        /* placeholder */
    }

    return 1;
}

int validateArrayInitialization(Symbol newSymbol, ASTNode initNode, DataType varType, int isConst, TypeCheckContext context) {
    ASTNode valueNode = initNode;
    /* initialization neededness happens on caller */
    if (!valueNode || valueNode->nodeType != VALUE || !valueNode->children) {
        return 1;
    }

    ASTNode arrLit = valueNode->children;

    if (arrLit->nodeType == ARRAY_LIT) {
        /* when const same size must be initialized, but when not const can be less (but not more) */
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

static size_t alignTo(size_t offset, size_t alignment) {
    return (offset + alignment - 1) & ~(alignment - 1);
}

StructType createStructType(ASTNode node, TypeCheckContext context) {
    if (!node || node->nodeType != STRUCT_DEFINITION) return NULL;
    StructType structType = malloc(sizeof(struct StructType));
    if (!structType) {
        repError(ERROR_MEMORY_ALLOCATION_FAILED, "failed to create struct type");
        return NULL;
    };
    structType->nameStart = node->start;
    structType->nameLength = node->length;
    structType->fields = NULL;
    structType->fieldCount = 0;
    structType->size = 0;

    ASTNode fieldList = node->children;
    if (fieldList && fieldList->nodeType == STRUCT_FIELD_LIST) {
        ASTNode field = fieldList->children;
        StructField last = NULL;

        while (field) {
            if (field->nodeType == STRUCT_FIELD && field->children) {
                StructField structField = malloc(sizeof(struct StructField));
                if (!structField) {
                    free(structType);
                    free(structField);
                    return NULL;
                }
                int pointerLevel = 0;
                ASTNode baseTypeNode = getBaseTypeFromPointerChain(field->children->children, &pointerLevel);
                DataType type = getDataTypeFromNode(baseTypeNode->nodeType);
                structField->nameStart = field->start;
                structField->nameLength = field->length;
                structField->type = type;
                if (type == TYPE_STRUCT) {
                    Symbol structSymbol = lookupSymbol(context->current, field->children->start, field->children->length);
                    if (!structSymbol || structSymbol->symbolType != SYMBOL_TYPE) {
                        REPORT_ERROR(ERROR_UNDEFINED_SYMBOL, field->children, context, "Undefined struct type in field declaration");
                        free(structField);
                        free(structType);
                        return NULL;
                    }
                    structField->structType = structSymbol->structType;
                }
                structField->isPointer = (pointerLevel > 0);
                structField->pointerLevel = pointerLevel;
                if (pointerLevel > 0) {
                    structField->type = TYPE_POINTER;
                }
                structField->next = NULL;

                size_t fieldSize = getStackSize(structField->type);
                size_t alignment = getStackSize(structField->type);
                structType->size = alignTo(structType->size, alignment);
                structField->offset = structType->size;

                structType->size += fieldSize;

                StructField check = structType->fields;
                while (check) {
                    if (check->nameLength == structField->nameLength &&
                        memcmp(check->nameStart, structField->nameStart, check->nameLength) == 0) {
                        REPORT_ERROR(ERROR_VARIABLE_REDECLARED, node, context, "duplicate field on struct");
                        free(structField);
                        return NULL;
                    }
                    check = check->next;
                }
                structType->fieldCount++;
                if (!structType->fields) {
                    structType->fields = structField;
                } else {
                    last->next = structField;
                }
                last = structField;
            }
            field = field->brothers;
        }
        size_t maxAlignment = 1;
        StructField f = structType->fields;
        while (f) {
            size_t align = getStackSize(f->type);
            if (align > maxAlignment) maxAlignment = align;
            f = f->next;
        }
        structType->size = alignTo(structType->size, maxAlignment);
    }
    return structType;
}

int validateStructDef(ASTNode node, TypeCheckContext context) {
    if (!node || node->nodeType != STRUCT_DEFINITION) return 0;

    Symbol exists = lookupSymbolCurrentOnly(context->current, node->start, node->length);
    if (exists) {
        char *tempText = extractText(node->start, node->length);
        REPORT_ERROR(ERROR_VARIABLE_REDECLARED, node, context, tempText);
        free(tempText);
        return 0;
    }
    StructType structType = createStructType(node, context);
    if (!structType) {
        REPORT_ERROR(ERROR_INVALID_EXPRESSION, node, context, "Failed to create struct type");
        return 0;
    };
    Symbol structSymbol = addSymbolFromNode(context->current, node, TYPE_STRUCT);
    if (!structSymbol) {
        free(structType);
        return 0;
    }
    structSymbol->structType = structType;
    structSymbol->symbolType = SYMBOL_TYPE;
    return 1;
}

int validateStructVarDec(ASTNode node, TypeCheckContext context) {
    if (!node || node->nodeType != STRUCT_VARIABLE_DEFINITION) return 0;

    ASTNode typeRef = node->children;
    if (!typeRef || typeRef->nodeType != REF_CUSTOM) {
        repError(ERROR_INTERNAL_PARSER_ERROR, "Invalid struct variable declaration");
        return 0;
    }

    Symbol structSymbol = lookupSymbol(context->current, typeRef->start, typeRef->length);
    if (!structSymbol) {
        REPORT_ERROR(ERROR_UNDEFINED_VARIABLE, node, context, "Undefined struct type");
        return 0;
    }

    Symbol exists = lookupSymbolCurrentOnly(context->current, node->start, node->length);
    if (exists) {
        char *tempText = extractText(node->start, node->length);
        REPORT_ERROR(ERROR_VARIABLE_REDECLARED, node, context, tempText);
        free(tempText);
        return 0;
    }

    Symbol symbol = addSymbolFromNode(context->current, node, TYPE_STRUCT);
    if (!symbol) {
        repError(ERROR_SYMBOL_TABLE_CREATION_FAILED, "Failed to add struct vairable to symbol table");
        return 0;
    }

    symbol->structType = structSymbol->structType;
    if (typeRef->brothers) {
        symbol->isInitialized = 1;
    }

    return 1;
}

/* scalars */


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
        reportErrorWithText(ERROR_INTERNAL_TYPECHECKER_ERROR, node, context,
                          "Cannot determine initialization type");
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

    /* Pointer level validation */
    if (newSymbol->isPointer && initExpr->nodeType == VARIABLE) {
        Symbol initSym = lookupSymbol(context->current, initExpr->start, initExpr->length);
        if (initSym && !validatePointerLevels(newSymbol, initSym, node, context, isMemRef)) {
            return 0;
        }
    }

    newSymbol->isInitialized = 1;

    /* Track constant values for compile-time evaluation */
    if (isConst && node->children->brothers->children->nodeType == LITERAL) {
        newSymbol->hasConstVal = 1;
        newSymbol->constVal = parseInt(node->children->brothers->children->start,
                                    node->children->brothers->children->length);
    }

    return 1;
}

/**
 * variable declaration and assignment validation
 */

int validateVariableDeclaration(ASTNode node, TypeCheckContext context, int isConst) {
    /* Basic validation */
    if (node == NULL || node->start == NULL) {
        repError(ERROR_INTERNAL_PARSER_ERROR, "Variable declaration node is null");
        return 0;
    }

    if (node->children == NULL || node->children->children == NULL) {
        repError(ERROR_INTERNAL_PARSER_ERROR, "Variable missing type information");
        return 0;
    }

    int isArr = (node->nodeType == ARRAY_VARIABLE_DEFINITION);
    int isStruct = node->children->children->nodeType == REF_CUSTOM;

    /* Extract type information */
    int pointerLevel = 0;
    ASTNode typeref = getBaseTypeFromPointerChain(node->children->children, &pointerLevel);
    DataType varType = getDataTypeFromNode(typeref->nodeType);
    Symbol structSymbol = NULL;
    if (varType == TYPE_UNKNOWN) {
        repError(ERROR_INTERNAL_PARSER_ERROR, "Unknown variable type in declaration");
        return 0;
    } else if (varType == TYPE_STRUCT) {
        structSymbol = lookupSymbol(context->current, typeref->start, typeref->length);
        if (structSymbol == NULL || structSymbol->symbolType != SYMBOL_TYPE) {
            REPORT_ERROR(ERROR_UNDEFINED_SYMBOL, typeref, context,
                        "Undefined struct type in variable declaration");
            return 0;
        }
    }

    /* Check for redeclaration */
    Symbol existing = lookupSymbolCurrentOnly(context->current, node->start, node->length);
    if (existing != NULL) {
        reportErrorWithText(ERROR_VARIABLE_REDECLARED, node, context, "Variable redeclared");
        return 0;
    }

    /* Create new symbol */
    Symbol newSymbol = addSymbolFromNode(context->current, node, varType);
    if (!newSymbol) {
        repError(ERROR_SYMBOL_TABLE_CREATION_FAILED, "Failed to add symbol");
        return 0;
    }

    if (varType == TYPE_STRUCT && structSymbol) {
        newSymbol->structType = structSymbol->structType;
    }

    newSymbol->isPointer = (pointerLevel > 0);
    newSymbol->pointerLvl = pointerLevel;
    newSymbol->isConst = isConst;

    if (pointerLevel > 0) {
        newSymbol->baseType = varType;
        newSymbol->type = TYPE_POINTER;
        varType = TYPE_POINTER;
    }

    /* Handle array-specific validation */
    if (isArr) {
        ASTNode sizeNode = node->children->brothers;
        if (!sizeNode || (sizeNode->nodeType != LITERAL && sizeNode->nodeType != VARIABLE)) {
            REPORT_ERROR(ERROR_ARRAY_SIZE_INVALID_SPEC, node, context,
                        "invalid static size for array");
            return 0;
        }

        int arraySize;
        if (sizeNode->nodeType == LITERAL) {
            arraySize = parseInt(sizeNode->start, sizeNode->length);
        } else {
            Symbol sizeSym = lookupSymbol(context->current, sizeNode->start, sizeNode->length);
            if (isConst && (!sizeSym || !sizeSym->isConst || !sizeSym->hasConstVal)) {
                REPORT_ERROR(ERROR_ARRAY_SIZE_NOT_CONSTANT, sizeNode, context,
                            "Array size must be compile-time constant");
                return 0;
            }
            arraySize = sizeSym->constVal;
        }

        if (arraySize <= 0) {
            REPORT_ERROR(ERROR_ARRAY_SIZE_NOT_POSITIVE, sizeNode, context,
                        "Array size must be positive");
            return 0;
        }

        newSymbol->isArray = 1;
        newSymbol->staticSize = arraySize;
    }

    /* Handle pointer initialization with const tracking */
    if (newSymbol->isPointer && node->children->brothers) {
        ASTNode valueNode = node->children->brothers;
        if (valueNode->children && valueNode->children->nodeType == MEMADDRS) {
            ASTNode target = valueNode->children->children;
            updateConstMemRef(newSymbol, target, context);
        }
    }

    ASTNode initNode = isArr ? node->children->brothers->brothers : node->children->brothers;
    if (initNode) {
        int isMemRef = (initNode->children && initNode->children->nodeType == MEMADDRS);

        /* Validate address-of operator if used */
        if (isMemRef) {
            ASTNode addrTarget = initNode->children->children;
            if (!validateAddressOf(addrTarget, context)) {
                return 0;
            }
        }

        /* Branch on array vs scalar initialization */
        if (isArr) {
            if (!validateArrayInitialization(newSymbol, initNode, varType, isConst, context)) {
                return 0;
            }
        } else if (isStruct) {
            assert(0 && "Struct initialization validation not implemented yet");
            if (!validateStructInlineInitialization(newSymbol, initNode, varType, isConst, context)) {
                return 0;
            }
        } else {
            if (!validateScalarInitialization(newSymbol, node, varType, isConst, isMemRef, context)) {
                return 0;
            }
        }
    } else if (isConst) {
        /* Const variables must be initialized */
        reportErrorWithText(ERROR_CONST_MUST_BE_INITIALIZED, node, context, "Const must be initialized");
        return 0;
    }

    return 1;
}

int validateAssignment(ASTNode node, TypeCheckContext context) {
    if (node == NULL || node->children == NULL || node->children->brothers == NULL) {
        repError(ERROR_INTERNAL_PARSER_ERROR, "Assignment node missing operands");
        return 0;
    }

    ASTNode left = node->children;
    ASTNode right = node->children->brothers;
    ASTNode leftForType = left;
    ASTNode rightForType = right;
    int isPointerDeref = (left->nodeType == POINTER);

    /* Check for pointer dereference to const */
    if (isPointerDeref) {
        ASTNode derefTarget = left->children;

        /* Case 1: *ptr where ptr is a variable */
        if (derefTarget->nodeType == VARIABLE) {
            Symbol ptrSym = lookupSymbol(context->current, derefTarget->start, derefTarget->length);
            if (checkConstViolation(ptrSym, node, context, isPointerDeref)) {
                return 0;
            }
        }
        /* Case 2: *arr[i] where arr is an array of pointers */
        else if (derefTarget->nodeType == ARRAY_ACCESS) {
            ASTNode arrayNode = derefTarget->children;
            if (arrayNode && arrayNode->nodeType == VARIABLE) {
                Symbol arraySym =
                    lookupSymbol(context->current, arrayNode->start, arrayNode->length);
                if (arraySym && arraySym->isConst) {
                    REPORT_ERROR(ERROR_CONSTANT_REASSIGNMENT, node, context, "Cannot modify through const array element");
                    return 0;
                }
                if (arraySym && arraySym->hasConstMemRef) {
                    REPORT_ERROR(ERROR_CONSTANT_REASSIGNMENT, node, context, "Cannot modify through pointer to const");
                    return 0;
                }
            }
        }
    }

    /* Normalize node types for easier processing */
    if (left->nodeType == POINTER) left = left->children;
    if (right->nodeType == MEMADDRS) right = right->children;

    /* Validate left-hand side is assignable */
    if (left->nodeType != VARIABLE &&
        left->nodeType != MEMBER_ACCESS &&
        left->nodeType != ARRAY_ACCESS &&
        left->nodeType != POINTER) {
        REPORT_ERROR(ERROR_INVALID_ASSIGNMENT_TARGET, node, context, "Left side must be a variable or member access");
        return 0;
    }

    /* Handle variable assignment */
    if (left->nodeType == VARIABLE) {
        Symbol sym = lookupSymbolOrError(context, left);
        if (!sym) return 0;

        if (sym->symbolType == SYMBOL_FUNCTION) {
            REPORT_ERROR(ERROR_INVALID_ASSIGNMENT_TARGET, node, context, "Cannot assign to function name");
            return 0;
        }

        if (checkConstViolation(sym, node, context, isPointerDeref)) {
            return 0;
        }

        /* Check for array assignment issues */
        if (right->nodeType == VARIABLE) {
            Symbol rightSym = lookupSymbol(context->current, right->start, right->length);
            if (rightSym) {
                /* Scalar = Array (error) */
                if (!sym->isArray && rightSym->isArray) {
                    reportErrorWithText(ERROR_CANNOT_ASSIGN_ARRAY_TO_SCALAR, node, context, "Cannot assign array to scalar");
                    return 0;
                }
                /* Array = Array (check sizes) */
                if (sym->isArray && rightSym->isArray) {
                    if (sym->staticSize != rightSym->staticSize) {
                        char msg[100];
                        snprintf(msg, sizeof(msg), "Cannot assign array of size %d to array of size %d", rightSym->staticSize, sym->staticSize);
                        REPORT_ERROR(ERROR_ARRAY_SIZE_MISMATCH, node, context, msg);
                        return 0;
                    }
                }
            }
        }
    }

    /* Handle array access assignment */
    if (left->nodeType == ARRAY_ACCESS) {
        if (!validateArrayAccessNode(left, context)) {
            return 0;
        }

        /* Additional const checking for array access */
        ASTNode baseNode = left->children;
        Symbol arraySym = lookupSymbol(context->current, baseNode->start, baseNode->length);
        if (arraySym) {
            if (arraySym->isConst) {
                reportErrorWithText(ERROR_CONSTANT_REASSIGNMENT, node, context, "Cannot modify const array");
                return 0;
            }
            if (arraySym->isPointer && arraySym->hasConstMemRef) {
                REPORT_ERROR(ERROR_CONSTANT_REASSIGNMENT, node, context, "Cannot modify through pointer to const");
                return 0;
            }
        }
    }

    /* Type compatibility checking */
    DataType leftType = getExpressionType(leftForType, context);
    if (leftType == TYPE_UNKNOWN) {
        REPORT_ERROR(ERROR_EXPRESSION_TYPE_UNKNOWN_LHS, leftForType, context, "Cannot determine type of left-hand side");
        return 0;
    }

    DataType rightType = getExpressionType(rightForType, context);
    if (rightType == TYPE_UNKNOWN) {
        REPORT_ERROR(ERROR_EXPRESSION_TYPE_UNKNOWN_RHS, rightForType, context, "Cannot determine type of right-hand side");
        return 0;
    }

    CompatResult compat = areCompatible(leftType, rightType);
    if (compat == COMPAT_ERROR) {
        REPORT_ERROR(variableErrorCompatibleHandling(leftType, rightType), node, context, "Type mismatch in assignment");
        return 0;
    } else if (compat == COMPAT_WARNING) {
        REPORT_ERROR(ERROR_TYPE_MISMATCH_DOUBLE_TO_FLOAT, node, context, "Type mismatch in assignment");
    }

    /* Handle address-of with const tracking */
    if (left->nodeType == VARIABLE && right->nodeType == MEMADDRS) {
        Symbol leftSym = lookupSymbol(context->current, left->start, left->length);
        if (leftSym) {
            updateConstMemRef(leftSym, right->children, context);
        }
    }

    /* Mark variable as initialized and handle pointer level validation */
    if (left->nodeType == VARIABLE) {
        Symbol symbol = lookupSymbol(context->current, left->start, left->length);
        if (symbol && node->nodeType == ASSIGNMENT) {
            symbol->isInitialized = 1;

            /* Pointer level validation */
            if (symbol->isPointer && right->nodeType == VARIABLE) {
                Symbol rightSym = lookupSymbol(context->current, right->start, right->length);
                if (rightSym && rightSym->isPointer) {
                    if (symbol->pointerLvl != rightSym->pointerLvl) {
                        char msg[100];
                        snprintf(msg, sizeof(msg), "Cannot assign pointer of level %d to pointer of level %d", rightSym->pointerLvl, symbol->pointerLvl);
                        REPORT_ERROR(ERROR_INVALID_EXPRESSION, node, context, msg);
                        return 0;
                    }

                    if (rightSym->hasConstMemRef) {
                        symbol->hasConstMemRef = 1;
                    }
                }
            }
        }
    }
    return 1;
}

int validateVariableUsage(ASTNode node, TypeCheckContext context) {
    if (node == NULL || node->start == NULL) {
        repError(ERROR_INTERNAL_PARSER_ERROR, "Variable usage node is null or has no name");
        return 0;
    };

    Symbol symbol = lookupSymbol(context->current, node->start, node->length);
    if (symbol == NULL) {
        char *tempText = extractText(node->start, node->length);
        REPORT_ERROR(ERROR_UNDEFINED_VARIABLE, node, context, tempText);
        free(tempText);
        return 0;
    }

    if (!symbol->isInitialized) {
        char *tempText = extractText(node->start, node->length);
        REPORT_ERROR(ERROR_VARIABLE_NOT_INITIALIZED, node, context, tempText);
        free(tempText);
        return 0;
    }

    return 1;
}

/**
 * Functions
 */

FunctionParameter extractParameters(ASTNode paramListNode) {
    if (paramListNode == NULL || paramListNode->nodeType != PARAMETER_LIST) return NULL;

    FunctionParameter firstParam = NULL;
    FunctionParameter lastParam = NULL;

    ASTNode paramNode = paramListNode->children;
    while (paramNode != NULL) {
        if (paramNode->nodeType == PARAMETER &&
            paramNode->length > 0 &&
            paramNode->start != NULL &&
            paramNode->children != NULL &&
            paramNode->children->children != NULL) {

            int pointerLevel = 0;
            ASTNode baseTypeNode = getBaseTypeFromPointerChain(
                paramNode->children->children, &pointerLevel);
            DataType paramType = getDataTypeFromNode(baseTypeNode->nodeType);

            if (paramType == TYPE_STRUCT) {
                Symbol structSym = lookupSymbol(NULL, baseTypeNode->start, baseTypeNode->length);
                if (structSym && structSym->symbolType == SYMBOL_TYPE) {
                    paramType = TYPE_STRUCT;
                } else {
                    repError(ERROR_UNDEFINED_STRUCT, "Unknown struct type in parameter");
                    return NULL;
                }
            }

            FunctionParameter param = createParameter(paramNode->start, paramNode->length, paramType);
            if (param == NULL) {
                freeParamList(firstParam);
                return NULL;
            }
            param->isPointer = (pointerLevel > 0);
            param->pointerLevel = pointerLevel;
            if (pointerLevel > 0) {
                param->type = TYPE_POINTER;
            }
            if (firstParam == NULL) {
                firstParam = param;
            } else {
                lastParam->next = param;
            }
            lastParam = param;
        }
        paramNode = paramNode->brothers;
    }
    return firstParam;
}

int containsReturnStatement(ASTNode node) {
    if (node == NULL) return 0;
    if (node->nodeType == RETURN_STATEMENT) return 1;

    ASTNode child = node->children;
    while (child != NULL) {
        if (containsReturnStatement(child)) {
            return 1;
        }
        child = child->brothers;
    }
    return 0;
}

int validateFunctionDef(ASTNode node, TypeCheckContext context) {
    if (node == NULL || node->nodeType != FUNCTION_DEFINITION || node->start == NULL || node->children == NULL) {
        repError(ERROR_INTERNAL_PARSER_ERROR, "Invalid function definition node");
        return 0;
    }

    ASTNode paramListNode = node->children;
    ASTNode returnTypeNode = paramListNode ? paramListNode->brothers : NULL;
    ASTNode bodyNode = returnTypeNode ? returnTypeNode->brothers : NULL;

    if (paramListNode == NULL || paramListNode->nodeType != PARAMETER_LIST) {
        repError(ERROR_INTERNAL_PARSER_ERROR, "Function missing parameter list");
        return 0;
    }

    FunctionParameter parameters = extractParameters(paramListNode);
    int returnPointerLevel = 0;
    DataType returnType = getReturnTypeFromNode(returnTypeNode, &returnPointerLevel);

    int paramCount = 0;
    FunctionParameter param = parameters;
    while (param != NULL) {
        paramCount++;
        param = param->next;
    }

    Symbol funcSymbol = addFunctionSymbolFromNode(context->current, node, returnType, parameters, paramCount);
    if (funcSymbol == NULL) {
        char *tempText = extractText(node->start, node->length);
        REPORT_ERROR(ERROR_VARIABLE_REDECLARED, node, context, tempText);
        free(tempText);
        freeParamList(parameters);
        return 0;
    }
    if (returnType == TYPE_STRUCT) {
        funcSymbol->structType = lookupSymbol(context->current, returnTypeNode->children->start, returnTypeNode->children->length)->structType;
    }
    funcSymbol->returnsPointer = (returnPointerLevel > 0);
    funcSymbol->returnPointerLevel = returnPointerLevel;
    if (returnPointerLevel > 0) {
        funcSymbol->returnBaseType = returnType;
        funcSymbol->type = TYPE_POINTER;
    }

    SymbolTable oldScope = context->current;
    Symbol oldFunction = context->currentFunction;

    SymbolTable funcScope = createSymbolTable(oldScope);
    if (funcScope == NULL) {
        repError(ERROR_SYMBOL_TABLE_CREATION_FAILED, "Failed to create function scope");
        return 0;
    }
    funcSymbol->functionScope = funcScope;
    context->current = funcScope;
    context->currentFunction = funcSymbol;

    if (context->current == NULL) {
        repError(ERROR_SYMBOL_TABLE_CREATION_FAILED, "Failed to create function scope");
        context->current = oldScope;
        context->currentFunction = oldFunction;
        return 0;
    }

    ASTNode paramNode = paramListNode->children;
    param = parameters;
    while (param != NULL && paramNode != NULL) {
        Symbol paramSymbol = addSymbol(context->current, param->nameStart, param->nameLength, param->type, node->line, node->column);
        if (paramSymbol != NULL) {
            paramSymbol->isInitialized = 1;

            if (paramNode->children && paramNode->children->children) {
                int pointerLevel = 0;
                ASTNode baseType = getBaseTypeFromPointerChain(paramNode->children->children, &pointerLevel);

                paramSymbol->isPointer = (pointerLevel > 0);
                paramSymbol->pointerLvl = pointerLevel;
                paramSymbol->baseType = getDataTypeFromNode(baseType->nodeType);

                if (paramSymbol->baseType == TYPE_STRUCT || paramSymbol->type == TYPE_STRUCT) {
                    Symbol structTypeSymbol = lookupSymbol(context->current, baseType->start, baseType->length);
                    if (structTypeSymbol && structTypeSymbol->symbolType == SYMBOL_TYPE) {
                        paramSymbol->structType = structTypeSymbol->structType;
                    }
                }
            }
        }
        param = param->next;
        paramNode = paramNode->brothers;
    }

    int success = 1;
    if (bodyNode != NULL) {
        if (!containsReturnStatement(bodyNode) && returnType != TYPE_VOID) {
            REPORT_ERROR(ERROR_MISSING_RETURN_VALUE, node, context, "Non-void function missing return statement");
            success = 0;
        } else {
            success = typeCheckNode(bodyNode, context);
        }
    }

    context->current = oldScope;
    context->currentFunction = oldFunction;

    return success;
}

int validateFunctionCall(ASTNode node, TypeCheckContext context) {
    if (node == NULL || node->nodeType != FUNCTION_CALL || node->start == NULL) {
        repError(ERROR_INTERNAL_PARSER_ERROR, "Invalid function call node");
        return 0;
    }

    ASTNode argListNode = node->children;
    if (argListNode == NULL || argListNode->nodeType != ARGUMENT_LIST) {
        repError(ERROR_INTERNAL_PARSER_ERROR, "Function call missing argument list");
        return 0;
    }

    if (isBuiltinFunction(node->start, node->length)) {
        return validateBuiltinFunctionCall(node, context);
    }

    return validateUserDefinedFunctionCall(node, context);
}

int validateBuiltinFunctionCall(ASTNode node, TypeCheckContext context) {
    ASTNode argListNode = node->children;

    int argCount = 0;
    ASTNode arg = argListNode->children;
    while (arg != NULL) {
        argCount++;
        arg = arg->brothers;
    }

    DataType *argTypes = NULL;
    if (argCount > 0) {
        argTypes = malloc(argCount * sizeof(DataType));
        if (!argTypes) {
            repError(ERROR_MEMORY_ALLOCATION_FAILED, "Failed to allocate argument types array");
            return 0;
        }

        arg = argListNode->children;
        for (int i = 0; i < argCount && arg != NULL; i++) {
            DataType argType = getExpressionType(arg, context);
            if (argType == TYPE_UNKNOWN) {
                free(argTypes);
                return 0;
            }
            argTypes[i] = argType;
            arg = arg->brothers;
        }
    }

    BuiltInId builtinId = resolveOverload(node->start, node->length, argTypes, argCount);
    int result = (builtinId != BUILTIN_UNKNOWN);

    if (!result) {
        REPORT_ERROR(ERROR_FUNCTION_NO_OVERLOAD_MATCH, node, context, "No matching overload for built-in function");
    }

    if (argTypes != NULL) {
        free(argTypes);
    }

    return result;
}

int validateUserDefinedFunctionCall(ASTNode node, TypeCheckContext context) {
    Symbol funcSymbol = lookupSymbol(context->current, node->start, node->length);
    if (funcSymbol == NULL) {
        char *tempText = extractText(node->start, node->length);
        REPORT_ERROR(ERROR_UNDEFINED_FUNCTION, node, context, tempText);
        free(tempText);
        return 0;
    }

    if (funcSymbol->symbolType != SYMBOL_FUNCTION) {
        REPORT_ERROR(ERROR_CALLING_NON_FUNCTION, node, context, "Attempting to call non-function");
        return 0;
    }

    ASTNode argListNode = node->children;

    /* Count arguments */
    int argCount = 0;
    ASTNode arg = argListNode->children;
    while (arg != NULL) {
        argCount++;
        arg = arg->brothers;
    }

    /* Check argument count */
    if (argCount != funcSymbol->paramCount) {
        REPORT_ERROR(ERROR_FUNCTION_ARG_COUNT_MISMATCH, node, context, "Function call argument count mismatch");
        return 0;
    }

    /* Stream validate argument types */
    FunctionParameter param = funcSymbol->parameters;
    arg = argListNode->children;

    while (param != NULL && arg != NULL) {
        DataType argType = getExpressionType(arg, context);
        if (argType == TYPE_UNKNOWN) {
            return 0;
        }
        CompatResult compat = areCompatible(param->type, argType);
        if (compat == COMPAT_ERROR) {
            char *tempText = extractText(param->nameStart, param->nameLength);
            REPORT_ERROR(variableErrorCompatibleHandling(param->type, argType), node, context, tempText);
            free(tempText);
            return 0;
        } else if (compat == COMPAT_WARNING) {
            char *tempText = extractText(param->nameStart, param->nameLength);
            REPORT_ERROR(ERROR_TYPE_MISMATCH_DOUBLE_TO_FLOAT, node, context, tempText);
            free(tempText);
        }

        param = param->next;
        arg = arg->brothers;
    }

    return 1;
}

int validateReturnStatement(ASTNode node, TypeCheckContext context) {
    if (node == NULL || node->nodeType != RETURN_STATEMENT) {
        repError(ERROR_INTERNAL_PARSER_ERROR, "Invalid return statement node");
        return 0;
    }

    if (context->currentFunction == NULL) {
        repError(ERROR_INVALID_EXPRESSION, "Return statement outside function");
        return 0;
    }

    Symbol funcSym = context->currentFunction;
    DataType expectedType = funcSym->type;

    /* Handle void return */
    if (node->children == NULL) {
        if (expectedType != TYPE_VOID) {
            repError(ERROR_MISSING_RETURN_VALUE, "Non-void function must return a value");
            return 0;
        }
        return 1;
    }

    /* Get actual return type */
    DataType returnType = getExpressionType(node->children, context);
    if (returnType == TYPE_UNKNOWN) {
        return 0;
    }

    /* Check void mismatch */
    if (expectedType == TYPE_VOID) {
        repError(ERROR_UNEXPECTED_RETURN_VALUE, "Void function cannot return a value");
        return 0;
    }

    /* Special handling for pointer returns */
    if (expectedType == TYPE_POINTER || returnType == TYPE_POINTER) {
        if (expectedType != TYPE_POINTER || returnType != TYPE_POINTER) {
            repError(ERROR_RETURN_TYPE_MISMATCH, "Cannot return pointer from non-pointer function or vice versa");
            return 0;
        }

        ASTNode retExpr = node->children;
        if (retExpr->nodeType == VARIABLE) {
            Symbol retSym = lookupSymbol(context->current, retExpr->start, retExpr->length);
            if (retSym && retSym->isPointer) {
                if (funcSym->returnPointerLevel != retSym->pointerLvl) {
                    char msg[100];
                    snprintf(msg, sizeof(msg),
                            "Pointer level mismatch: expected %d, got %d",
                            funcSym->returnPointerLevel, retSym->pointerLvl);
                    REPORT_ERROR(ERROR_RETURN_TYPE_MISMATCH, node, context, msg);
                    return 0;
                }

                if (funcSym->returnBaseType != retSym->baseType) {
                    char msg[100];
                    snprintf(msg, sizeof(msg),
                            "Base type mismatch: expected *%s, got *%s",
                            getTypeName(funcSym->returnBaseType),
                            getTypeName(retSym->baseType));
                    REPORT_ERROR(ERROR_RETURN_TYPE_MISMATCH, node, context, msg);
                    return 0;
                }
            }
        }
    }

    /* For non-pointer types, standard compatibility check */
    CompatResult compat = areCompatible(expectedType, returnType);
    if (compat == COMPAT_ERROR) {
        repError(ERROR_RETURN_TYPE_MISMATCH, "return");
        return 0;
    }

    funcSym->returnedVar = lookupSymbol(context->current, node->children->start, node->children->length);

    return 1;
}

/**
 * Casting
 */

int validateCastExpression(ASTNode node, TypeCheckContext context) {
    if (!node || !node->children || !node->children->brothers) {
        repError(ERROR_INTERNAL_PARSER_ERROR, "Invalid cast expression structure");
        return 0;
    }
    ASTNode sourceExpr = node->children;
    ASTNode targetTypeNode = node->children->brothers;
    DataType sourceType = getExpressionType(sourceExpr, context);
    if (sourceType == TYPE_UNKNOWN) return 0;
    DataType targetType = getDataTypeFromNode(targetTypeNode->nodeType);
    if (targetType == TYPE_UNKNOWN) {
        REPORT_ERROR(ERROR_INVALID_CAST_TARGET, node, context, "Invalid cast target type");
        return 0;
    }
    CompatResult canItBeCasted = isCastAllowed(sourceType, targetType);
    if (canItBeCasted == COMPAT_ERROR) {
        REPORT_ERROR(ERROR_FORBIDDEN_CAST, node, context, "Cannot cast between these types");
        return 0;
    }
    /* just a warning */
    if (isPrecisionLossCast(sourceType, targetType)) {
        REPORT_ERROR(ERROR_CAST_PRECISION_LOSS, node, context, "Cast may lose precision");
    }
    return 1;
}

