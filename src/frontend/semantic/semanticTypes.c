/**
 * @file semanticTypes.c
 * @brief Type comparison, inference, compatibility, and type utilities.
 *
 * Responsibilities:
 *   - Type equality and compatibility checks
 *   - Type inference for expressions
 *   - Numeric promotion rules
 *   - Cast validation
 *   - Operation result type determination
 *   - Type name strings
 *   - Stack size / alignment helpers
 *   - Pointer chain resolution
 *   - Member access type resolution
 *
 * Pure type logic — no AST walking for validation purposes.
 * (getExpressionType does traverse to *infer* types, not to validate.)
 */

#include "semanticInternal.h"

/**
 * helper
 */

NodeTypes symbolTypeToNodeType(DataType type){
    switch (type) {
        case TYPE_I8:     return REF_I8;
        case TYPE_I16:    return REF_I16;
        case TYPE_I32:    return REF_I32;
        case TYPE_I64:    return REF_I64;
        case TYPE_U8:     return REF_U8;
        case TYPE_U16:    return REF_U16;
        case TYPE_U32:    return REF_U32;
        case TYPE_U64:    return REF_U64;
        case TYPE_FLOAT:   return REF_FLOAT;
        case TYPE_DOUBLE:  return REF_DOUBLE;
        case TYPE_STRING:  return REF_STRING;
        case TYPE_BOOL:    return REF_BOOL;
        case TYPE_VOID:    return REF_VOID;
        case TYPE_STRUCT:  return REF_CUSTOM;
        case TYPE_POINTER: return POINTER;
        default:           return null_NODE; /* should not happen */
    }
}

const char *getTypeName(DataType type) {
    switch (type) {
        case TYPE_I8:     return "i8";
        case TYPE_I16:    return "i16";
        case TYPE_I32:    return "i32";
        case TYPE_I64:    return "i64";
        case TYPE_U8:     return "u8";
        case TYPE_U16:    return "u16";
        case TYPE_U32:    return "u32";
        case TYPE_U64:    return "u64";
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
    if (isIntegerType(varType)) {
        switch (initType) {
            case TYPE_STRING: return ERROR_TYPE_MISMATCH_STRING_TO_INT;
            case TYPE_BOOL:   return ERROR_TYPE_MISMATCH_BOOL_TO_INT;
            case TYPE_FLOAT:  return ERROR_TYPE_MISMATCH_FLOAT_TO_INT;
            case TYPE_DOUBLE: return ERROR_TYPE_MISMATCH_DOUBLE_TO_INT;
            default:          return ERROR_INCOMPATIBLE_BINARY_OPERANDS;
        }
    }
    switch (varType) {
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
        case TYPE_STRING: {
            switch (initType) {
                case TYPE_BOOL:   return ERROR_TYPE_MISMATCH_BOOL_TO_STRING;
                case TYPE_FLOAT:  return ERROR_TYPE_MISMATCH_FLOAT_TO_STRING;
                case TYPE_DOUBLE: return ERROR_TYPE_MISMATCH_DOUBLE_TO_STRING;
                default:
                    if (isIntegerType(initType)) return ERROR_TYPE_MISMATCH_INT_TO_STRING;
                    return ERROR_INCOMPATIBLE_BINARY_OPERANDS;
            }
        }
        case TYPE_BOOL: {
            switch (initType) {
                case TYPE_STRING: return ERROR_TYPE_MISMATCH_STRING_TO_BOOL;
                case TYPE_FLOAT:  return ERROR_TYPE_MISMATCH_FLOAT_TO_BOOL;
                case TYPE_DOUBLE: return ERROR_TYPE_MISMATCH_DOUBLE_TO_BOOL;
                default:
                    if (isIntegerType(initType)) return ERROR_TYPE_MISMATCH_INT_TO_BOOL;
                    return ERROR_INCOMPATIBLE_BINARY_OPERANDS;
            }
        }
        default:
            return ERROR_INCOMPATIBLE_BINARY_OPERANDS;
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

int getStackSize(DataType type) {
    switch (type) {
        case TYPE_I8:     case TYPE_U8:     case TYPE_BOOL:   return 1;
        case TYPE_I16:    case TYPE_U16:                      return 2;
        case TYPE_I32:    case TYPE_U32:    
        case TYPE_FLOAT:                                      return 4;
        case TYPE_I64:    case TYPE_U64:    case TYPE_DOUBLE: 
        case TYPE_STRING: case TYPE_STRUCT: case TYPE_POINTER: return 8;
        default: return 4;
    }
}

/**
 * Type compat
 */

CompatResult areCompatible(DataType target, DataType source) {
    if (target == source) return COMPAT_OK;

    /* Pointer rules (unchanged) */
    if (target == TYPE_POINTER && source == TYPE_POINTER) return COMPAT_OK;
    if (source == TYPE_NULL && target == TYPE_POINTER) return COMPAT_OK;
    if (target == TYPE_NULL && source == TYPE_POINTER) return COMPAT_OK;
    if (source == TYPE_NULL && target == TYPE_NULL)    return COMPAT_OK;

    /* Integer widening: same signedness, source rank <= target rank */
    if (isIntegerType(target) && isIntegerType(source)) {
        if (isSignedInt(target) == isSignedInt(source)) {
            return getIntegerRank(source) <= getIntegerRank(target)
                ? COMPAT_OK : COMPAT_ERROR;
        }
        return COMPAT_ERROR;  /* mixed signedness needs explicit cast */
    }

    /* Float/double promotion (unchanged logic) */
    if (target == TYPE_FLOAT) {
        if (source == TYPE_DOUBLE) return COMPAT_WARNING;
        return isIntegerType(source) ? COMPAT_OK : COMPAT_ERROR;
    }
    if (target == TYPE_DOUBLE) {
        return (isIntegerType(source) || source == TYPE_FLOAT) 
            ? COMPAT_OK : COMPAT_ERROR;
    }

    /* Bool ↔ integer (unchanged) */
    if ((source == TYPE_BOOL && isIntegerType(target)) ||
        (isIntegerType(source) && target == TYPE_BOOL)) {
        return COMPAT_OK;
    }

    /* String, pointer, bool, struct: strict */
    return COMPAT_ERROR;
}

int isPrecisionLossCast(DataType source, DataType target) {
    if (source == TYPE_DOUBLE && target == TYPE_FLOAT) return 1;
    if ((source == TYPE_FLOAT || source == TYPE_DOUBLE) && isIntegerType(target)) return 1;
    if (isIntegerType(source) && target == TYPE_BOOL) return 1;
    
    /* Integer narrowing */
    if (isIntegerType(source) && isIntegerType(target)) {
        if (getIntegerRank(source) > getIntegerRank(target)) return 1;
    }
    /* Signed and unsigned of same size */
    if (isIntegerType(source) && isIntegerType(target) &&
        isSignedInt(source) != isSignedInt(target)) return 1;

    return 0;
}

int isNumType(DataType type) {
    return isIntegerType(type) || type == TYPE_FLOAT || type == TYPE_DOUBLE;
}

int isIntegerType(DataType type) {
    switch (type) {
        case TYPE_I8: case TYPE_I16: case TYPE_I32: case TYPE_I64:
        case TYPE_U8: case TYPE_U16: case TYPE_U32: case TYPE_U64:
            return 1;
        default:
            return 0;
    }
}

int isSignedInt(DataType type) {
    switch (type) {
        case TYPE_I8: case TYPE_I16: case TYPE_I32: case TYPE_I64:
            return 1;
        default:
            return 0;
    }
}

int isUnsignedInt(DataType type) {
    switch (type) {
        case TYPE_U8: case TYPE_U16: case TYPE_U32: case TYPE_U64:
            return 1;
        default:
            return 0;
    }
}

int getIntegerRank(DataType type) {
    switch (type) {
        case TYPE_I8:  case TYPE_U8:  return 1;
        case TYPE_I16: case TYPE_U16: return 2;
        case TYPE_I32: case TYPE_U32: return 3;
        case TYPE_I64: case TYPE_U64: return 4;
        default: return 0;
    }
}

CompatResult isCastAllowed(DataType target, DataType source) {
    CompatResult baseComp = areCompatible(target, source);
    if (baseComp != COMPAT_ERROR) return baseComp;

    /* All integer to integer casts are allowed */
    if (isIntegerType(source) && isIntegerType(target)) {
        return isPrecisionLossCast(source, target) ? COMPAT_WARNING : COMPAT_OK;
    }

    if (isNumType(source) && isNumType(target)) {
        return isPrecisionLossCast(source, target) ? COMPAT_WARNING : COMPAT_OK;
    }

    /* pointer to integer casts */
    if ((source == TYPE_POINTER && isIntegerType(target)) ||
        (isIntegerType(source) && target == TYPE_POINTER)) {
        return COMPAT_OK;
    }

    if (target == TYPE_BOOL || source == TYPE_BOOL) return COMPAT_OK;
    return COMPAT_ERROR;
}

/**
 * @brief Operation type
 */
DataType getOperationResultType(DataType left, DataType right, NodeTypes op) {
    switch (op) {
        case ADD_OP:
        case SUB_OP:
            if (left == TYPE_POINTER && isIntegerType(right)) return TYPE_POINTER;
            if (op == ADD_OP && isIntegerType(left) && right == TYPE_POINTER) return TYPE_POINTER;
            if (op == SUB_OP && left == TYPE_POINTER && right == TYPE_POINTER) return TYPE_I64;
            /* fall through */
        case MUL_OP:
        case DIV_OP:
        case MOD_OP:
            if (left == TYPE_DOUBLE || right == TYPE_DOUBLE) return TYPE_DOUBLE;
            if (left == TYPE_FLOAT  || right == TYPE_FLOAT)  return TYPE_FLOAT;
            /* Integer promotion: result is the wider type */
            if (isIntegerType(left) && isIntegerType(right)) {
                return getIntegerRank(left) >= getIntegerRank(right) ? left : right;
            }
            return TYPE_UNKNOWN;

        case EQUAL_OP: case NOT_EQUAL_OP:
        case LESS_EQUAL_OP: case GREATER_EQUAL_OP:
        case LESS_THAN_OP: case GREATER_THAN_OP:
            if (areCompatible(left, right) != COMPAT_ERROR ||
                areCompatible(right, left) != COMPAT_ERROR) return TYPE_BOOL;
            /* Allow comparison between any integers */
            if (isIntegerType(left) && isIntegerType(right)) return TYPE_BOOL;
            return TYPE_UNKNOWN;

        case LOGIC_AND: case LOGIC_OR:
            if (left == TYPE_BOOL && right == TYPE_BOOL) return TYPE_BOOL;
            return TYPE_UNKNOWN;

        default: return TYPE_UNKNOWN;
    }
}

/**
 * Member access
 */

int isAutoDerefeable(DataType type, StructType structType) {
    return structType != NULL && (type == TYPE_STRUCT || type == TYPE_POINTER);
}

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
        if(isAutoDerefeable(objResolved.type, objResolved.structType)) {
            structType = objResolved.structType;
        } else {
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
        if(isAutoDerefeable(objectSymbol->type, objectSymbol->structType)) {
            structType = objectSymbol->structType;
        }
        else {
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

DataType resolveIntLitType(DataType expectedType){
    assert(expectedType != TYPE_UNKNOWN && "Expected type must be known");
    int isValidInt = isIntegerType(expectedType);
    assert(isValidInt && "Expected type for int literal must be an integer type");
    return expectedType;
}

DataType getExpressionType(ASTNode node, TypeCheckContext context, DataType expectedType) {
    if (node == NULL) return TYPE_UNKNOWN;
    switch (node->nodeType) {
        case LITERAL: {
            if (node->children == NULL) {
                repError(ERROR_INTERNAL_PARSER_ERROR, "Invalid literal node: missing child");
                return TYPE_UNKNOWN;
            }
            switch (node->children->nodeType) {
                case REF_I8:     return TYPE_I8;
                case REF_I16:    return TYPE_I16;
                case REF_I32:    return TYPE_I32;
                case REF_I64:    return TYPE_I64;
                case REF_U8:     return TYPE_U8;
                case REF_U16:    return TYPE_U16;
                case REF_U32:    return TYPE_U32;
                case REF_U64:    return TYPE_U64;
                case REF_INT_UNRESOLVED: {
                    return resolveIntLitType(expectedType);
                }
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
                                    "Subscript on non-array variable");
                        return TYPE_UNKNOWN;
                    }

                    return arraySym->type;
                }
            }

            return TYPE_UNKNOWN;
        }

        case MEMADDRS: {
            if (!node->children) return TYPE_UNKNOWN;
            DataType innerType = getExpressionType(node->children, context, expectedType);
            if (innerType == TYPE_UNKNOWN) return TYPE_UNKNOWN;
            return TYPE_POINTER;
        }

        case VARIABLE: {
            Symbol sym = lookupSymbol(context->current, node->start, node->length);
            if (!sym) {
                reportErrorWithText(ERROR_UNDEFINED_VARIABLE, node, context, "Undefined variable");
                return TYPE_UNKNOWN;
            }
            return sym->type;
        }

        case ARRAY_ACCESS: {
            if (!validateArrayAccessNode(node, context)) return TYPE_UNKNOWN;
            ASTNode arrayNode = node->children;
            Symbol arraySym = lookupSymbol(context->current, arrayNode->start, arrayNode->length);
            if (!arraySym) return TYPE_UNKNOWN;
            if (arraySym->isPointer) return arraySym->baseType;
            return arraySym->type;
        }

        case UNARY_MINUS_OP:
        case UNARY_PLUS_OP: {
            DataType operandType = getExpressionType(node->children, context, expectedType);
            if (isNumType(operandType)) return operandType;
            REPORT_ERROR(ERROR_INVALID_UNARY_OPERAND, node, context,
                        "Unary +/- requires numeric operand");
            return TYPE_UNKNOWN;
        }

        case LOGIC_NOT: {
            DataType operandType = getExpressionType(node->children, context, expectedType);
            if (operandType == TYPE_BOOL) return TYPE_BOOL;
            REPORT_ERROR(ERROR_INVALID_UNARY_OPERAND, node, context,
                        "Logical NOT requires boolean operand");
            return TYPE_UNKNOWN;
        }

        case PRE_INCREMENT:
        case PRE_DECREMENT:
        case POST_INCREMENT:
        case POST_DECREMENT: {
            DataType operandType = getExpressionType(node->children, context, expectedType);
            if (isIntegerType(operandType) || operandType == TYPE_FLOAT || operandType == TYPE_DOUBLE) {
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

            DataType leftType = getExpressionType(node->children, context, expectedType);
            DataType rightType = getExpressionType(node->children->brothers, context, expectedType);

            if (!isIntegerType(leftType) || !isIntegerType(rightType)) {
                REPORT_ERROR(ERROR_INCOMPATIBLE_BINARY_OPERANDS, node, context,
                            "Bitwise operators require integer operands");
                return TYPE_UNKNOWN;
            }

            return getIntegerRank(leftType) >= getIntegerRank(rightType) ? leftType : rightType;
        }

        case BITWISE_NOT: {
            DataType opType = getExpressionType(node->children, context, expectedType);
            if (isIntegerType(opType)) return opType;
            REPORT_ERROR(ERROR_INVALID_UNARY_OPERAND, node, context,
                        "Bitwise NOT requires integer operand");
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

            DataType leftType = getExpressionType(node->children, context, expectedType);
            DataType rightType = getExpressionType(node->children->brothers, context, expectedType);
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

            if (getExpressionType(node->children, context, TYPE_BOOL) != TYPE_BOOL) {
                REPORT_ERROR(ERROR_INVALID_CONDITION_TYPE, node, context,
                            "Ternary condition must be boolean");
                return TYPE_UNKNOWN;
            }

            ASTNode trueBranchWrap = node->children->brothers;
            ASTNode falseBranchWrap = trueBranchWrap ? trueBranchWrap->brothers : NULL;

            ASTNode trueExpr = trueBranchWrap ? trueBranchWrap->children : NULL;
            ASTNode falseExpr = falseBranchWrap ? falseBranchWrap->children : NULL;

            if (!trueExpr || !falseExpr) return TYPE_UNKNOWN;

            DataType trueType = getExpressionType(trueExpr, context, expectedType);
            DataType falseType = getExpressionType(falseExpr, context, expectedType);

            if (areCompatible(trueType, falseType) != COMPAT_ERROR) return trueType;
            if (areCompatible(falseType, trueType) != COMPAT_ERROR) return falseType;

            REPORT_ERROR(ERROR_INCOMPATIBLE_BINARY_OPERANDS, node, context,
                        "Ternary branches must have compatible types");
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