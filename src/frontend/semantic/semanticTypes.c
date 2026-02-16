#include "semanticInternal.h"

/**
 * helper
 */

const char *getTypeName(DataType type) {
    switch (type) {
        case TYPE_INT:     return "int";
        case TYPE_FLOAT:   return "float";
        case TYPE_DOUBLE:  return "double";
        case TYPE_STRING:  return "string";
        case TYPE_BOOL:    return "bool";
        case TYPE_VOID:    return "void";
        case TYPE_POINTER: return "pointer";
        case TYPE_STRUCT:  return "struct";
        case TYPE_NULL:    return "null";
        default:           return "unknown";
    }
}

/* Compatibility error handling */
ErrorCode variableErrorCompatibleHandling(DataType varType, DataType initType) {
    switch (varType) {
        case TYPE_INT: {
            switch (initType) {
                case TYPE_STRING: return ERROR_TYPE_MISMATCH_STRING_TO_INT;
                case TYPE_BOOL:   return ERROR_TYPE_MISMATCH_BOOL_TO_INT;
                case TYPE_FLOAT:  return ERROR_TYPE_MISMATCH_FLOAT_TO_INT;
                case TYPE_DOUBLE: return ERROR_TYPE_MISMATCH_DOUBLE_TO_INT;
                default:          return ERROR_INCOMPATIBLE_BINARY_OPERANDS;
            }
        }
        case TYPE_FLOAT: {
            switch (initType) {
                case TYPE_STRING: return ERROR_TYPE_MISMATCH_STRING_TO_FLOAT;
                case TYPE_BOOL:   return ERROR_TYPE_MISMATCH_BOOL_TO_FLOAT;
                case TYPE_DOUBLE: return ERROR_TYPE_MISMATCH_DOUBLE_TO_FLOAT;
                default:          return ERROR_INCOMPATIBLE_BINARY_OPERANDS;
            }
        }
        case TYPE_DOUBLE: {
            switch (initType) {
                case TYPE_STRING: return ERROR_TYPE_MISMATCH_STRING_TO_DOUBLE;
                case TYPE_BOOL:   return ERROR_TYPE_MISMATCH_BOOL_TO_DOUBLE;
                default:          return ERROR_INCOMPATIBLE_BINARY_OPERANDS;
            }
        }
        case TYPE_BOOL: {
            switch (initType) {
                case TYPE_STRING: return ERROR_TYPE_MISMATCH_STRING_TO_BOOL;
                case TYPE_INT:    return ERROR_TYPE_MISMATCH_INT_TO_BOOL;
                case TYPE_FLOAT:  return ERROR_TYPE_MISMATCH_FLOAT_TO_BOOL;
                case TYPE_DOUBLE: return ERROR_TYPE_MISMATCH_DOUBLE_TO_BOOL;
                default:          return ERROR_INCOMPATIBLE_BINARY_OPERANDS;
            }
        }
        case TYPE_STRING: {
            switch (initType) {
                case TYPE_INT:    return ERROR_TYPE_MISMATCH_INT_TO_STRING;
                case TYPE_FLOAT:  return ERROR_TYPE_MISMATCH_FLOAT_TO_STRING;
                case TYPE_DOUBLE: return ERROR_TYPE_MISMATCH_DOUBLE_TO_STRING;
                case TYPE_BOOL:   return ERROR_TYPE_MISMATCH_BOOL_TO_STRING;
                default:          return ERROR_INCOMPATIBLE_BINARY_OPERANDS;
            }
        }
        default:
            return ERROR_INVALID_OPERATION_FOR_TYPE;
    }
}

/* Pointer */

ASTNode getBaseTypeFromPointerChain(ASTNode typeRefNode, int *outPointerLevel) {
    int ptrLevel = 0;
    ASTNode current = typeRefNode;

    while (current && current->nodeType == POINTER) {
        ptrLevel++;
        current = current->children;
    }

    if (outPointerLevel) {
        *outPointerLevel = ptrLevel;
    }

    return current;
}

/**
 * Stack info
 */

typedef enum {
    STACK_SIZE_INT    = 4,
    STACK_SIZE_FLOAT  = 4,
    STACK_SIZE_BOOL   = 1,
    STACK_SIZE_STRING = 8,
    STACK_SIZE_DOUBLE = 8,
    ALIGNMENT         = 16
} StackSizeEnum;

int getStackSize(DataType type) {
    switch (type) {
        case TYPE_INT:    return STACK_SIZE_INT;
        case TYPE_FLOAT:  return STACK_SIZE_FLOAT;
        case TYPE_BOOL:   return STACK_SIZE_BOOL;
        case TYPE_STRING: return STACK_SIZE_STRING;
        case TYPE_STRUCT: return STACK_SIZE_STRING;
        case TYPE_DOUBLE: return STACK_SIZE_DOUBLE;
        default:          return STACK_SIZE_INT;
    }
}

/**
 * Type compat
 */

CompatResult areCompatible(DataType target, DataType source) {
    if (target == TYPE_POINTER && source == TYPE_POINTER) {
        return COMPAT_OK;
    }

    if (target == source) return COMPAT_OK;

    if (source == TYPE_NULL && target == TYPE_POINTER) return COMPAT_OK;
    if (target == TYPE_NULL && source == TYPE_POINTER) return COMPAT_OK;
    if (source == TYPE_NULL && target == TYPE_NULL)    return COMPAT_OK;

    switch (target) {
        case TYPE_STRING:
        case TYPE_BOOL:
        case TYPE_INT:
        case TYPE_POINTER:
            return COMPAT_ERROR;
        case TYPE_FLOAT: {
            if (source == TYPE_DOUBLE) return COMPAT_WARNING;
            return source == TYPE_INT ? COMPAT_OK : COMPAT_ERROR;
        }
        case TYPE_DOUBLE:
            return source == TYPE_INT || source == TYPE_FLOAT ? COMPAT_OK : COMPAT_ERROR;
        default:
            return COMPAT_ERROR;
    }
}

int isPrecisionLossCast(DataType source, DataType target) {
    if (source == TYPE_DOUBLE && target == TYPE_FLOAT) return 1;
    if ((source == TYPE_FLOAT || source == TYPE_DOUBLE) && target == TYPE_INT) return 1;
    if (source == TYPE_INT && target == TYPE_BOOL) return 1;
    return 0;
}

int isNumType(DataType type) {
    return type == TYPE_INT || type == TYPE_FLOAT || type == TYPE_DOUBLE;
}

CompatResult isCastAllowed(DataType target, DataType source) {
    CompatResult baseComp = areCompatible(target, source);
    if (baseComp != COMPAT_ERROR) {
        return baseComp;
    }
    if (isNumType(source) && isNumType(target)) {
        return isPrecisionLossCast(source, target) ? COMPAT_WARNING : COMPAT_OK;
    }
    if ((source == TYPE_BOOL && isNumType(target)) || (isNumType(source) && target == TYPE_BOOL)) {
        return COMPAT_OK;
    }
    return COMPAT_ERROR;
}

/**
 * @brief Operation type
 */
DataType getOperationResultType(DataType left, DataType right, NodeTypes op) {
    switch (op) {
        case ADD_OP:
        case SUB_OP:
            /* Pointer arithmetic */
            if (left == TYPE_POINTER && right == TYPE_INT) return TYPE_POINTER;
            if (op == ADD_OP && left == TYPE_INT && right == TYPE_POINTER) return TYPE_POINTER;
            if (op == SUB_OP && left == TYPE_POINTER && right == TYPE_POINTER) return TYPE_INT;
            /* fall through */
        case MUL_OP:
        case DIV_OP:
        case MOD_OP:
            if (left == TYPE_DOUBLE || right == TYPE_DOUBLE) return TYPE_DOUBLE;
            if (left == TYPE_FLOAT  || right == TYPE_FLOAT)  return TYPE_FLOAT;
            if (left == TYPE_INT    && right == TYPE_INT)     return TYPE_INT;
            return TYPE_UNKNOWN;
        case EQUAL_OP:
        case NOT_EQUAL_OP:
        case LESS_EQUAL_OP:
        case GREATER_EQUAL_OP:
        case LESS_THAN_OP:
        case GREATER_THAN_OP:
            if (areCompatible(left, right) != COMPAT_ERROR ||
                areCompatible(right, left) != COMPAT_ERROR) return TYPE_BOOL;
            return TYPE_UNKNOWN;
        case LOGIC_AND:
        case LOGIC_OR:
            if (left == TYPE_BOOL && right == TYPE_BOOL) return TYPE_BOOL;
            return TYPE_UNKNOWN;
        default: return TYPE_UNKNOWN;
    }
}

/**
 * Member access
 */

ResolvedType resolveMemberAccessType(ASTNode node, TypeCheckContext context) {
    ResolvedType result = { TYPE_UNKNOWN, NULL };

    if (!node || node->nodeType != MEMBER_ACCESS) return result;

    ASTNode objectNode = node->children;
    ASTNode fieldNode = objectNode ? objectNode->brothers : NULL;

    if (!objectNode || !fieldNode) {
        repError(ERROR_INTERNAL_PARSER_ERROR, "Invalid member access structure");
        return result;
    }

    StructType structType = NULL;

    if (objectNode->nodeType == MEMBER_ACCESS) {
        ResolvedType objResolved = resolveMemberAccessType(objectNode, context);
        if (objResolved.type != TYPE_STRUCT || !objResolved.structType) {
            REPORT_ERROR(ERROR_INVALID_OPERATION_FOR_TYPE, node, context,
                        "Member access on non-struct type");
            return result;
        }
        structType = objResolved.structType;

    } else if (objectNode->nodeType == VARIABLE) {
        Symbol objectSymbol = lookupSymbol(context->current, objectNode->start, objectNode->length);
        if (!objectSymbol) {
            REPORT_ERROR(ERROR_UNDEFINED_VARIABLE, objectNode, context,
                        "Undefined variable in member access");
            return result;
        }
        if (objectSymbol->type != TYPE_STRUCT || !objectSymbol->structType) {
            REPORT_ERROR(ERROR_INVALID_OPERATION_FOR_TYPE, node, context,
                        "Member access on non-struct type");
            return result;
        }
        structType = objectSymbol->structType;
    } else {
        REPORT_ERROR(ERROR_INVALID_OPERATION_FOR_TYPE, node, context,
                    "Member access requires a variable or nested access");
        return result;
    }

    StructField field = structType->fields;
    while (field) {
        if (field->nameLength == fieldNode->length &&
            memcmp(field->nameStart, fieldNode->start, fieldNode->length) == 0) {
            result.type = field->type;
            result.structType = field->structType;
            return result;
        }
        field = field->next;
    }

    REPORT_ERROR(ERROR_UNDEFINED_VARIABLE, fieldNode, context,
                "Struct '%.*s' has no field '%.*s'");
    return result;
}

DataType validateMemberAccess(ASTNode node, TypeCheckContext context) {
    ResolvedType resolved = resolveMemberAccessType(node, context);
    return resolved.type;
}

/**
 * type inference
 */

DataType getExpressionType(ASTNode node, TypeCheckContext context) {
    if (node == NULL) return TYPE_UNKNOWN;
    switch (node->nodeType) {
        case LITERAL: {
            if (node->children == NULL) {
                repError(ERROR_INTERNAL_PARSER_ERROR, "Invalid literal node: missing child");
                return TYPE_UNKNOWN;
            }
            switch (node->children->nodeType) {
                case REF_INT:    return TYPE_INT;
                case REF_FLOAT:  return TYPE_FLOAT;
                case REF_BOOL:   return TYPE_BOOL;
                case REF_DOUBLE: return TYPE_DOUBLE;
                default:         return TYPE_STRING;
            }
        }
        case NULL_LIT: return TYPE_NULL;
        case POINTER: {
            ASTNode ptrNode = node->children;
            if (!ptrNode) return TYPE_UNKNOWN;

            int derefCount = 1;
            ASTNode current = ptrNode;

            while (current && current->nodeType == POINTER) {
                derefCount++;
                current = current->children;
            }

            if (!current) return TYPE_UNKNOWN;

            if (current->nodeType == VARIABLE) {
                Symbol ptrSym = lookupSymbol(context->current, current->start, current->length);
                if (!ptrSym || !ptrSym->isPointer) {
                    REPORT_ERROR(ERROR_INVALID_OPERATION_FOR_TYPE, node, context,
                                "Cannot dereference non-pointer");
                    return TYPE_UNKNOWN;
                }

                int remainingLevel = ptrSym->pointerLvl - derefCount;

                if (remainingLevel < 0) {
                    REPORT_ERROR(ERROR_INVALID_OPERATION_FOR_TYPE, node, context,
                                "Too many dereference operations");
                    return TYPE_UNKNOWN;
                } else if (remainingLevel > 0) {
                    return TYPE_POINTER;
                } else {
                    return ptrSym->baseType;
                }
            }

            if (current->nodeType == ARRAY_ACCESS) {
                ASTNode arrayNode = current->children;
                if (arrayNode && arrayNode->nodeType == VARIABLE) {
                    Symbol arraySym = lookupSymbol(context->current, arrayNode->start, arrayNode->length);

                    if (!arraySym) {
                        REPORT_ERROR(ERROR_UNDEFINED_VARIABLE, current, context,
                                    "Undefined array variable");
                        return TYPE_UNKNOWN;
                    }

                    if (!arraySym->isArray) {
                        REPORT_ERROR(ERROR_INVALID_OPERATION_FOR_TYPE, current, context,
                                    "Subscript on non-array type");
                        return TYPE_UNKNOWN;
                    }

                    if (arraySym->isPointer) {
                        int remainingLevel = arraySym->pointerLvl - derefCount;

                        if (remainingLevel < 0) {
                            REPORT_ERROR(ERROR_INVALID_OPERATION_FOR_TYPE, node, context,
                                        "Too many dereference operations");
                            return TYPE_UNKNOWN;
                        } else if (remainingLevel > 0) {
                            return TYPE_POINTER;
                        } else {
                            return arraySym->baseType;
                        }
                    } else {
                        REPORT_ERROR(ERROR_INVALID_OPERATION_FOR_TYPE, node, context,
                                    "Cannot dereference non-pointer array element");
                        return TYPE_UNKNOWN;
                    }
                }
            }
            DataType innerType = getExpressionType(current, context);
            if (innerType != TYPE_POINTER) {
                REPORT_ERROR(ERROR_INVALID_OPERATION_FOR_TYPE, node, context,
                            "Cannot dereference non-pointer expression");
                return TYPE_UNKNOWN;
            }

            return TYPE_UNKNOWN;
        }
        case ARRAY_ACCESS: {
            ASTNode arrayNode = node->children;
            ASTNode indexNode = arrayNode ? arrayNode->brothers : NULL;

            if (!arrayNode || !indexNode) {
                repError(ERROR_INTERNAL_PARSER_ERROR, "Invalid array access structure");
                return TYPE_UNKNOWN;
            }

            if (arrayNode->nodeType != VARIABLE) {
                REPORT_ERROR(ERROR_INVALID_OPERATION_FOR_TYPE, node, context,
                            "Array access requires variable");
                return TYPE_UNKNOWN;
            }

            Symbol sym = lookupSymbol(context->current, arrayNode->start, arrayNode->length);
            if (!sym) {
                char *tempText = extractText(arrayNode->start, arrayNode->length);
                REPORT_ERROR(ERROR_UNDEFINED_VARIABLE, node, context, tempText);
                free(tempText);
                return TYPE_UNKNOWN;
            }

            if (!sym->isArray) {
                REPORT_ERROR(ERROR_INVALID_OPERATION_FOR_TYPE, node, context,
                            "Subscript on non-array type");
                return TYPE_UNKNOWN;
            }

            DataType indexType = getExpressionType(indexNode, context);
            if (indexType != TYPE_INT) {
                REPORT_ERROR(ERROR_ARRAY_INDEX_NOT_INTEGER, indexNode, context,
                            "Array index must be integer type");
                return TYPE_UNKNOWN;
            }

            if (indexNode->nodeType == LITERAL) {
                int indexValue = parseInt(indexNode->start, indexNode->length);
                if (indexValue < 0 || indexValue >= sym->staticSize) {
                    char msg[100];
                    snprintf(msg, sizeof(msg), "Array index %d out of bounds [0, %d)", indexValue,
                            sym->staticSize);
                    REPORT_ERROR(ERROR_ARRAY_INDEX_OUT_OF_BOUNDS, indexNode, context, msg);
                    return TYPE_UNKNOWN;
                }
            } else if (indexNode->nodeType == VARIABLE) {
                Symbol indexSym =
                    lookupSymbol(context->current, indexNode->start, indexNode->length);
                if (indexSym && indexSym->isConst && indexSym->hasConstVal) {
                    if (indexSym->constVal < 0 || indexSym->constVal >= sym->staticSize) {
                        char msg[100];
                        snprintf(msg, sizeof(msg), "Array index %d out of bounds [0, %d)",
                                indexSym->constVal, sym->staticSize);
                        REPORT_ERROR(ERROR_ARRAY_INDEX_OUT_OF_BOUNDS, indexNode, context, msg);
                        return TYPE_UNKNOWN;
                    }
                }
            }

            return sym->type;
        }
        case MEMADDRS: {
            ASTNode target = node->children;
            if (!target) return TYPE_UNKNOWN;
            if (target->nodeType == VARIABLE) {
                Symbol sym = lookupSymbol(context->current, target->start, target->length);
                if (!sym) {
                    return TYPE_UNKNOWN;
                }
            }
            return TYPE_POINTER;
        }
        case VARIABLE: {
            Symbol symbol = lookupSymbol(context->current, node->start, node->length);
            if (symbol == NULL) {
                char *tempText = extractText(node->start, node->length);
                REPORT_ERROR(ERROR_UNDEFINED_VARIABLE, node, context, tempText);
                free(tempText);
                return TYPE_UNKNOWN;
            }
            return symbol->type;
        }
        case REF_INT:    return TYPE_INT;
        case REF_FLOAT:  return TYPE_FLOAT;
        case REF_BOOL:   return TYPE_BOOL;
        case REF_DOUBLE: return TYPE_DOUBLE;
        case REF_STRING: return TYPE_STRING;
        case UNARY_MINUS_OP:
        case UNARY_PLUS_OP: {
            DataType opType = getExpressionType(node->children, context);
            if (opType == TYPE_INT || opType == TYPE_FLOAT || opType == TYPE_DOUBLE) {
                return opType;
            }
            REPORT_ERROR(ERROR_INVALID_UNARY_OPERAND, node, context,
                        "Arithmetic unary operators require numeric operands");
            return TYPE_UNKNOWN;
        }
        case LOGIC_NOT: {
            DataType opType = getExpressionType(node->children, context);
            if (opType == TYPE_BOOL) return TYPE_BOOL;
            REPORT_ERROR(ERROR_INVALID_UNARY_OPERAND, node, context,
                        "Logical NOT requires boolean operand");
            return TYPE_UNKNOWN;
        }
        case PRE_INCREMENT:
        case PRE_DECREMENT:
        case POST_INCREMENT:
        case POST_DECREMENT: {
            DataType operandType = getExpressionType(node->children, context);
            if (operandType == TYPE_INT || operandType == TYPE_FLOAT) {
                return operandType;
            }
            REPORT_ERROR(ERROR_INVALID_UNARY_OPERAND, node, context,
                        "Increment/decrement operators require numeric operands");
            return TYPE_UNKNOWN;
        }
        case BITWISE_AND:
        case BITWISE_OR:
        case BITWISE_XOR:
        case BITWISE_LSHIFT:
        case BITWISE_RSHIFT: {
            if (node->children == NULL || node->children->brothers == NULL) {
                repError(ERROR_INTERNAL_PARSER_ERROR, "Binary operation missing operands");
                return TYPE_UNKNOWN;
            }

            DataType leftType = getExpressionType(node->children, context);
            DataType rightType = getExpressionType(node->children->brothers, context);

            if (leftType != TYPE_INT || rightType != TYPE_INT) {
                REPORT_ERROR(ERROR_INCOMPATIBLE_BINARY_OPERANDS, node, context,
                            "Bitwise operators require integer operands");
                return TYPE_UNKNOWN;
            }

            return TYPE_INT;
        }

        case BITWISE_NOT: {
            DataType opType = getExpressionType(node->children, context);
            if (opType == TYPE_INT) return TYPE_INT;
            if (opType != TYPE_INT) {
                REPORT_ERROR(ERROR_INVALID_UNARY_OPERAND, node, context,
                            "Bitwise NOT requires integer operand");
                return TYPE_UNKNOWN;
            }
            return TYPE_UNKNOWN;
        }

        case ADD_OP:
        case SUB_OP:
        case MUL_OP:
        case DIV_OP:
        case MOD_OP:
        case EQUAL_OP:
        case NOT_EQUAL_OP:
        case LESS_THAN_OP:
        case GREATER_THAN_OP:
        case LESS_EQUAL_OP:
        case GREATER_EQUAL_OP:
        case LOGIC_AND:
        case LOGIC_OR: {
            if (node->children == NULL || node->children->brothers == NULL) {
                repError(ERROR_INTERNAL_PARSER_ERROR, "Binary operation missing operands");
                return TYPE_UNKNOWN;
            }

            DataType leftType = getExpressionType(node->children, context);
            DataType rightType = getExpressionType(node->children->brothers, context);
            DataType resultType = getOperationResultType(leftType, rightType, node->nodeType);
            if (resultType == TYPE_UNKNOWN) {
                REPORT_ERROR(ERROR_INCOMPATIBLE_BINARY_OPERANDS, node, context,
                            "Incompatible types in binary operation");
            }
            return resultType;
        }
        case CAST_EXPRESSION:
            if (!node->children || !node->children->brothers) return TYPE_UNKNOWN;
            if (!validateCastExpression(node, context)) return TYPE_UNKNOWN;
            {
                ASTNode targetTypeNode = node->children->brothers;
                return getDataTypeFromNode(targetTypeNode->nodeType);
            }
        case FUNCTION_CALL: {
            if (!validateFunctionCall(node, context)) {
                return TYPE_UNKNOWN;
            }

            Symbol funcSymbol = lookupSymbol(context->current, node->start, node->length);
            if (funcSymbol != NULL && funcSymbol->symbolType == SYMBOL_FUNCTION) {
                return funcSymbol->type;
            }
            return TYPE_UNKNOWN;
        }
        case MEMBER_ACCESS:
            return validateMemberAccess(node, context);
        case TERNARY_CONDITIONAL: {
            if (!node->children || !node->children->brothers) return TYPE_UNKNOWN;

            if (getExpressionType(node->children, context) != TYPE_BOOL) {
                REPORT_ERROR(ERROR_INVALID_CONDITION_TYPE, node, context,
                            "Ternary condition must be boolean");
                return TYPE_UNKNOWN;
            }

            ASTNode trueBranchWrap = node->children->brothers;
            ASTNode falseBranchWrap = trueBranchWrap ? trueBranchWrap->brothers : NULL;

            if (!trueBranchWrap || !falseBranchWrap) return TYPE_UNKNOWN;
            if (trueBranchWrap->nodeType != TERNARY_IF_EXPR ||
                falseBranchWrap->nodeType != TERNARY_ELSE_EXPR) {
                repError(ERROR_INTERNAL_PARSER_ERROR, "Invalid ternary expression structure");
                return TYPE_UNKNOWN;
            }

            ASTNode trueExpr = trueBranchWrap->children;
            ASTNode falseExpr = falseBranchWrap->children;

            if (!trueExpr || !falseExpr) return TYPE_UNKNOWN;

            DataType trueType = getExpressionType(trueExpr, context);
            DataType falseType = getExpressionType(falseExpr, context);

            if (trueType == falseType) return trueType;

            /* Type promotion for numbers */
            if ((trueType == TYPE_DOUBLE || falseType == TYPE_DOUBLE)) return TYPE_DOUBLE;
            if ((trueType == TYPE_FLOAT  || falseType == TYPE_FLOAT))  return TYPE_FLOAT;

            return TYPE_UNKNOWN;
        }
        default:
            return TYPE_UNKNOWN;
    }
}

/* return type */

DataType getReturnTypeFromNode(ASTNode returnTypeNode, int *outPointerLevel) {
    if (returnTypeNode == NULL || returnTypeNode->nodeType != RETURN_TYPE) {
        if (outPointerLevel) *outPointerLevel = 0;
        return TYPE_VOID;
    }

    if (returnTypeNode->children != NULL) {
        ASTNode typeRef = getBaseTypeFromPointerChain(
            returnTypeNode->children, outPointerLevel);
        return getDataTypeFromNode(typeRef->nodeType);
    }

    if (outPointerLevel) *outPointerLevel = 0;
    return TYPE_VOID;
}