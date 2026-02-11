//
// Created by pablo on 08/09/2025.
//

#include "typeChecker.h"

#include "builtIns.h"


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "../IR/irHelpers.h"

#include "errorHandling.h"
#include "semanticHelpers.h"

typedef enum {
    STACK_SIZE_INT = 4,      
    STACK_SIZE_FLOAT = 4,    
    STACK_SIZE_BOOL = 1,     
    STACK_SIZE_STRING = 8,   
    STACK_SIZE_DOUBLE = 8, 
    ALIGNMENT = 16
} StackSize;

StackSize getStackSize(DataType type) {
    switch (type) {
        case TYPE_INT: return STACK_SIZE_INT;
        case TYPE_FLOAT: return STACK_SIZE_FLOAT;
        case TYPE_BOOL: return STACK_SIZE_BOOL;
        case TYPE_STRING: return STACK_SIZE_STRING;
        case TYPE_STRUCT: return STACK_SIZE_STRING;
        case TYPE_DOUBLE: return STACK_SIZE_DOUBLE;
        default: return STACK_SIZE_INT;
    }
}

/**
 * @brief Creates and initializes a new type checking context.
 *
 * Allocates memory for a type checking context and creates the global
 * symbol table. Initializes both global and current scope pointers
 * to the global table for the start of type checking.
 *
 * @return Newly allocated TypeCheckContext or NULL on allocation failure
 *
 * @note The caller is responsible for freeing the context with
 *       freeTypeCheckContext(). Reports specific error codes for
 *       allocation failures.
 */
TypeCheckContext createTypeCheckContext(const char *sourceCode, const char *filename) {
    TypeCheckContext context = malloc(sizeof(struct TypeCheckContext));
    if (context == NULL) {
        repError(ERROR_MEMORY_ALLOCATION_FAILED, "Failed to allocate type check context");
        return NULL;
    };

    context->global = createSymbolTable(NULL);
    if (context->global == NULL) {
        free(context);
        repError(ERROR_SYMBOL_TABLE_CREATION_FAILED, "Failed to create global symbol table");
        return NULL;
    }
    context->current = context->global;
    context->currentFunction = NULL;
    context->sourceFile = sourceCode;
    context->filename = filename;
    context->blockScopesHead = NULL;
    context->blockScopesTail = NULL;

    initBuiltIns(context->global);

    return context;
}

/**
 * @brief Frees a type checking context and all associated symbol tables.
 *
 * Performs complete cleanup of the type checking context including
 * the global symbol table and all nested symbol tables that may
 * have been created during type checking.
 *
 * @param context Type checking context to free (can be NULL)
 *
 * @note This function only frees the global symbol table. Nested
 *       symbol tables should be freed as scopes are exited during
 *       the type checking process.
 */
void freeTypeCheckContext(TypeCheckContext context) {
    if (context == NULL) return;
    if (context->global != NULL) freeSymbolTable(context->global);
    BlockScopeNode node = context->blockScopesHead;
    while (node) {
        BlockScopeNode next = node->next;
        free(node);
        node = next;
    }
    free(context);
}

/**
 * @brief Checks if two data types are compatible for assignment operations.
 *
 * Implements the type compatibility rules for the language. Supports
 * identity compatibility (same types) and limited implicit conversions.
 * The only implicit conversion allowed is int to float.
 *
 * @param target Target type for assignment
 * @param source Source type being assigned
 * @return 1 if compatible (assignment allowed), 0 if incompatible
 *
 * Compatibility rules:
 * - Same types are always compatible
 * - int can be assigned to float (implicit conversion)
 * - All other type combinations are incompatible
 */
CompatResult areCompatible(DataType target, DataType source) {
    if (target == TYPE_POINTER && source == TYPE_POINTER) {
        return COMPAT_OK;
    }
    
    if (target == source) return COMPAT_OK;

    if (source == TYPE_NULL && target == TYPE_POINTER) return COMPAT_OK;
    if (target == TYPE_NULL && source == TYPE_POINTER) return COMPAT_OK;
    if (source == TYPE_NULL && target == TYPE_NULL) return COMPAT_OK;

    switch (target) {
        case TYPE_STRING:
        case TYPE_BOOL:
        case TYPE_INT: 
        case TYPE_POINTER:
            return COMPAT_ERROR;
        case TYPE_FLOAT: {
            if(source == TYPE_DOUBLE) return COMPAT_WARNING;
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
 * @brief Determines the result type of binary operations.
 *
 * Implements type promotion and result type determination for binary
 * operators. Follows C-style type promotion rules for arithmetic
 * operations and validates operand types for logical and comparison
 * operations.
 *
 * @param left Left operand type
 * @param right Right operand type
 * @param op Binary operation node type
 * @return Result type of the operation or TYPE_UNKNOWN for invalid operations
 *
 * Type promotion rules:
 * - Arithmetic: float + int = float, int + int = int
 * - Comparison: operands must be compatible, result is bool
 * - Logical: operands must be bool, result is bool
 */
DataType getOperationResultType(DataType left, DataType right, NodeTypes op) {
    switch (op) {
        case ADD_OP:
        case SUB_OP:
            // Pointer arithmetic
            if (left == TYPE_POINTER && right == TYPE_INT) return TYPE_POINTER;
            if (op == ADD_OP && left == TYPE_INT && right == TYPE_POINTER) return TYPE_POINTER;
            if (op == SUB_OP && left == TYPE_POINTER && right == TYPE_POINTER) return TYPE_INT;
            // fall through
        case MUL_OP:
        case DIV_OP:
        case MOD_OP:
            if (left == TYPE_DOUBLE || right == TYPE_DOUBLE) return TYPE_DOUBLE;
            if (left == TYPE_FLOAT || right == TYPE_FLOAT) return TYPE_FLOAT;
            if (left == TYPE_INT && right == TYPE_INT) return TYPE_INT;
            return TYPE_UNKNOWN;
        case EQUAL_OP:
        case NOT_EQUAL_OP:
        case LESS_EQUAL_OP:
        case GREATER_EQUAL_OP:
        case LESS_THAN_OP:
        case GREATER_THAN_OP:
            if (areCompatible(left, right) != COMPAT_ERROR || areCompatible(right, left) != COMPAT_ERROR) return TYPE_BOOL;
            return TYPE_UNKNOWN;
        case LOGIC_AND:
        case LOGIC_OR:
            if (left == TYPE_BOOL && right == TYPE_BOOL) return TYPE_BOOL;
            return TYPE_UNKNOWN;
        default: return TYPE_UNKNOWN;
    } 
}

/**
 * @brief Resolves the type of a member access expression recursively.
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

/**
 * @brief Validates member access and returns its DataType.
 */
DataType validateMemberAccess(ASTNode node, TypeCheckContext context) {
    ResolvedType resolved = resolveMemberAccessType(node, context);
    return resolved.type;
}

/**
 * @brief Infers the data type of an expression AST node.
 *
 * Recursively analyzes expression nodes to determine their result types.
 * Handles literals, variables, unary operations, binary operations, and
 * validates type correctness for all operations. Performs symbol table
 * lookups for variable references.
 *
 * @param node Expression AST node to analyze
 * @param context Type checking context for symbol resolution
 * @return Inferred DataType or TYPE_UNKNOWN for invalid expressions
 *
 * @note Reports specific error codes for type violations and undefined
 *       variables. The function performs comprehensive type checking
 *       during type inference.
 */
DataType getExpressionType(ASTNode node, TypeCheckContext context) {
    if (node == NULL) return TYPE_UNKNOWN;
    switch (node->nodeType) {
        case LITERAL: {
            if (node->children == NULL) {
                repError(ERROR_INTERNAL_PARSER_ERROR, "Invalid literal node: missing child");
                return TYPE_UNKNOWN;
            }
            switch(node->children->nodeType){
            case REF_INT:
                return TYPE_INT;
            case REF_FLOAT:
                return TYPE_FLOAT;
            case REF_BOOL:
                return TYPE_BOOL;
            case REF_DOUBLE:
                return TYPE_DOUBLE;
            default:
                return TYPE_STRING;
            }
        }
        case NULL_LIT: return TYPE_NULL;
        case POINTER: { // Dereference: *ptr, **pp, *arr[i], etc.
            ASTNode ptrNode = node->children;
            if (!ptrNode) return TYPE_UNKNOWN;

            // Count dereference depth and find the base expression
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
            
            // TYPE_POINTER alone doesn't carry base type info
            // For complex expressions, we'd need a richer type system
            // For now, this is a limitation - return TYPE_UNKNOWN
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
                // Check if it's a const variable with known value
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
                char * tempText = extractText(node->start, node->length);
                REPORT_ERROR(ERROR_UNDEFINED_VARIABLE, node, context, tempText);
                free(tempText);
                return TYPE_UNKNOWN;
            }
            return symbol->type;
        }
        case REF_INT:
            return TYPE_INT;
        case REF_FLOAT:
            return TYPE_FLOAT;
        case REF_BOOL:
            return TYPE_BOOL;
        case REF_DOUBLE:
            return TYPE_DOUBLE;
        case REF_STRING:
            return TYPE_STRING;
        case UNARY_MINUS_OP:
        case UNARY_PLUS_OP: {
            DataType opType = getExpressionType(node->children, context);
            if (opType == TYPE_INT || opType == TYPE_FLOAT || opType == TYPE_DOUBLE) {
                return opType;
            }
            REPORT_ERROR(ERROR_INVALID_UNARY_OPERAND, node, context, "Arithmetic unary operators require numeric operands");
            return TYPE_UNKNOWN;
        }
        case LOGIC_NOT: {
            DataType opType = getExpressionType(node->children, context);
            if (opType == TYPE_BOOL) return TYPE_BOOL;
            REPORT_ERROR(ERROR_INVALID_UNARY_OPERAND, node, context, "Logical NOT requires boolean operand");
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
            REPORT_ERROR(ERROR_INVALID_UNARY_OPERAND, node, context, "Increment/decrement operators require numeric operands");
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
            if(opType == TYPE_INT) return TYPE_INT;
            if(opType != TYPE_INT) {
                REPORT_ERROR(ERROR_INVALID_UNARY_OPERAND, node, context, "Bitwise NOT requires integer operand");
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
                REPORT_ERROR(ERROR_INCOMPATIBLE_BINARY_OPERANDS, node, context, "Incompatible types in binary operation");
            }
            return resultType;
        }
        case CAST_EXPRESSION:
            if(!node->children || !node->children->brothers) return TYPE_UNKNOWN;
            ASTNode targetTypeNode = node->children->brothers;
            return getDataTypeFromNode(targetTypeNode->nodeType);
        case FUNCTION_CALL: {
            Symbol funcSymbol = lookupSymbol(context->current, node->start, node->length);
            if (funcSymbol != NULL && funcSymbol->symbolType == SYMBOL_FUNCTION) {
                return funcSymbol->type;
            }
            return TYPE_UNKNOWN;
        }
        case MEMBER_ACCESS:
            //should be another function the one who handles this
            return validateMemberAccess(node, context);
        case TERNARY_CONDITIONAL: {
            if (!node->children || !node->children->brothers) return TYPE_UNKNOWN;

            ASTNode trueBranchWrap = node->children->brothers;
            ASTNode falseBranchWrap = trueBranchWrap ? trueBranchWrap->brothers : NULL;

            if (!trueBranchWrap || !falseBranchWrap) return TYPE_UNKNOWN;

            ASTNode trueExpr = trueBranchWrap->children;
            ASTNode falseExpr = falseBranchWrap->children;

            if (!trueExpr || !falseExpr) return TYPE_UNKNOWN;

            DataType trueType = getExpressionType(trueExpr, context);
            DataType falseType = getExpressionType(falseExpr, context);

            if (trueType == falseType) return trueType;

            // Type promotion for numbers
            if ((trueType == TYPE_DOUBLE || falseType == TYPE_DOUBLE)) return TYPE_DOUBLE;
            if ((trueType == TYPE_FLOAT || falseType == TYPE_FLOAT)) return TYPE_FLOAT;

            return TYPE_UNKNOWN;
        }
        default:
            return TYPE_UNKNOWN;
    }
}

int validateFunctionCall(ASTNode node, TypeCheckContext context) {
    if (node == NULL || node->nodeType != FUNCTION_CALL || node->start == NULL) {
        repError(ERROR_INTERNAL_PARSER_ERROR, "Invalid function call node");
        return 0;
    }

    ASTNode argListNode = node->children;
    if (argListNode == NULL || argListNode->nodeType != ARGUMENT_LIST) {
        repError(ERROR_INTERNAL_PARSER_ERROR,"Function call missing argument list");
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
        char * tempText = extractText(node->start, node->length);
        REPORT_ERROR(ERROR_UNDEFINED_FUNCTION, node, context, tempText);
        free(tempText);
        return 0;
    }

    if (funcSymbol->symbolType != SYMBOL_FUNCTION) {
        REPORT_ERROR(ERROR_CALLING_NON_FUNCTION, node, context, "Attempting to call non-function");
        return 0;
    }

    ASTNode argListNode = node->children;

    // Count arguments
    int argCount = 0;
    ASTNode arg = argListNode->children;
    while (arg != NULL) {
        argCount++;
        arg = arg->brothers;
    }

    // Check argument count
    if (argCount != funcSymbol->paramCount) {
        REPORT_ERROR(ERROR_FUNCTION_ARG_COUNT_MISMATCH, node, context, "Function call argument count mismatch" );
        return 0;
    }

    // Stream validate argument types
    FunctionParameter param = funcSymbol->parameters;
    arg = argListNode->children;

    while (param != NULL && arg != NULL) {
        DataType argType = getExpressionType(arg, context);
        if (argType == TYPE_UNKNOWN) {
            return 0; // Error already reported
        }
        CompatResult compat = areCompatible(param->type, argType);
        if (compat == COMPAT_ERROR) {
            char * tempText = extractText(param->nameStart, param->nameLength);
            REPORT_ERROR(variableErrorCompatibleHandling(param->type, argType), node, context, tempText);
            free(tempText);
            return 0;
        } else if (compat == COMPAT_WARNING) {
            char * tempText = extractText(param->nameStart, param->nameLength);
            REPORT_ERROR(ERROR_TYPE_MISMATCH_DOUBLE_TO_FLOAT, node, context, tempText);
            free(tempText);
        }

        param = param->next;
        arg = arg->brothers;
    }

    return 1;
}


/**
 * @brief Determines appropriate error code for variable type mismatch scenarios.
 *
 * Maps type mismatch combinations to specific error codes for detailed
 * error reporting. Provides precise error messages for each possible
 * type mismatch scenario in variable assignments.
 *
 * @param varType Expected variable type
 * @param initType Actual initialization/assignment type
 * @return Specific ErrorCode for the type mismatch scenario
 *
 * @note This function assumes the types are incompatible and should
 *       only be called after compatibility checking has failed.
 */
ErrorCode variableErrorCompatibleHandling(DataType varType, DataType initType) {
    switch (varType) {
        case TYPE_INT: {
            switch (initType) {
                case TYPE_STRING: return ERROR_TYPE_MISMATCH_STRING_TO_INT;
                case TYPE_BOOL: return ERROR_TYPE_MISMATCH_BOOL_TO_INT;
                case TYPE_FLOAT: return ERROR_TYPE_MISMATCH_FLOAT_TO_INT;
                case TYPE_DOUBLE: return ERROR_TYPE_MISMATCH_DOUBLE_TO_INT;
                default: return ERROR_INCOMPATIBLE_BINARY_OPERANDS; // Generic error
            }
        }
        case TYPE_FLOAT: {
            switch (initType) {
                case TYPE_STRING: return ERROR_TYPE_MISMATCH_STRING_TO_FLOAT;
                case TYPE_BOOL: return ERROR_TYPE_MISMATCH_BOOL_TO_FLOAT;
                case TYPE_DOUBLE: return ERROR_TYPE_MISMATCH_DOUBLE_TO_FLOAT;
                default: return ERROR_INCOMPATIBLE_BINARY_OPERANDS;
            }
        }
        case TYPE_DOUBLE: {
            switch (initType) {
                case TYPE_STRING: return ERROR_TYPE_MISMATCH_STRING_TO_DOUBLE;
                case TYPE_BOOL: return ERROR_TYPE_MISMATCH_BOOL_TO_DOUBLE;
                default: return ERROR_INCOMPATIBLE_BINARY_OPERANDS;
            }
        }
        case TYPE_BOOL: {
            switch (initType) {
                case TYPE_STRING: return ERROR_TYPE_MISMATCH_STRING_TO_BOOL;
                case TYPE_INT: return ERROR_TYPE_MISMATCH_INT_TO_BOOL;
                case TYPE_FLOAT: return ERROR_TYPE_MISMATCH_FLOAT_TO_BOOL;
                case TYPE_DOUBLE: return ERROR_TYPE_MISMATCH_DOUBLE_TO_BOOL;
                default: return ERROR_INCOMPATIBLE_BINARY_OPERANDS;
            }
        }
        case TYPE_STRING: {
            switch (initType) {
                case TYPE_INT: return ERROR_TYPE_MISMATCH_INT_TO_STRING;
                case TYPE_FLOAT: return ERROR_TYPE_MISMATCH_FLOAT_TO_STRING;
                case TYPE_DOUBLE: return ERROR_TYPE_MISMATCH_DOUBLE_TO_STRING;
                case TYPE_BOOL: return ERROR_TYPE_MISMATCH_BOOL_TO_STRING;
                default: return ERROR_INCOMPATIBLE_BINARY_OPERANDS;
            }
        } 
        default:
            return ERROR_INVALID_OPERATION_FOR_TYPE; // Unsupported type
    }
}

const char* getTypeName(DataType type) {
    switch (type) {
        case TYPE_INT: return "int";
        case TYPE_FLOAT: return "float";
        case TYPE_DOUBLE: return "double";
        case TYPE_STRING: return "string";
        case TYPE_BOOL: return "bool";
        case TYPE_VOID: return "void";
        case TYPE_POINTER: return "pointer";
        case TYPE_STRUCT: return "struct";
        case TYPE_NULL: return "null";
        default: return "unknown";
    }
}

static ASTNode getBaseTypeFromPointerChain(ASTNode typeRefNode, int* outPointerLevel) {
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
 * @brief Validates if a declaration is properly formed and adds it to the symbol table.
 * @param node AST node representing the variable declaration
 * @param context Type checking context for symbol resolution
 * @param isConst Flag indicating if the variable is declared as const
 * @return 1 if the declaration is valid and added, 0 otherwise
*/
int validateVariableDeclaration(ASTNode node, TypeCheckContext context, int isConst) {
    // Basic validation
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
    
    // Extract type information
    int pointerLevel = 0;
    ASTNode typeref = getBaseTypeFromPointerChain(node->children->children, &pointerLevel);
    DataType varType = getDataTypeFromNode(typeref->nodeType);
    Symbol structSymbol = NULL;
    if (varType == TYPE_UNKNOWN) {
        repError(ERROR_INTERNAL_PARSER_ERROR, "Unknown variable type in declaration");
        return 0;
        //checks if the custom type aka struct currently exists
    }else if(varType == TYPE_STRUCT){
        structSymbol = lookupSymbol(context->current, typeref->start, typeref->length);
        if(structSymbol == NULL || structSymbol->symbolType != SYMBOL_TYPE){
            REPORT_ERROR(ERROR_UNDEFINED_SYMBOL, typeref, context, "Undefined struct type in variable declaration");
            return 0;
        }
    }
    
    // Check for redeclaration
    Symbol existing = lookupSymbolCurrentOnly(context->current, node->start, node->length);
    if (existing != NULL) {
        reportErrorWithText(ERROR_VARIABLE_REDECLARED, node, context, "Variable redeclared");
        return 0;
    }
    
    // Create new symbol
    Symbol newSymbol = addSymbolFromNode(context->current, node, varType);
    if (!newSymbol) {
        repError(ERROR_SYMBOL_TABLE_CREATION_FAILED, "Failed to add symbol");
        return 0;
    }

    if(varType == TYPE_STRUCT && structSymbol){
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

    // Handle array-specific validation
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
    
    // Handle pointer initialization with const tracking
    if (newSymbol->isPointer && node->children->brothers) {
        ASTNode valueNode = node->children->brothers;
        if (valueNode->children && valueNode->children->nodeType == MEMADDRS) {
            ASTNode target = valueNode->children->children;
            updateConstMemRef(newSymbol, target, context);
        }
    }
    
    ASTNode initNode = isArr ? node->children->brothers->brothers : node->children->brothers;
    if (node->children && node->children->brothers) {
        int isMemRef = (initNode->children && initNode->children->nodeType == MEMADDRS);
        
        // Validate address-of operator if used
        if (isMemRef) {
            ASTNode addrTarget = initNode->children->children;
            if (!validateAddressOf(addrTarget, context)) {
                return 0;
            }
        }
        
        // Branch on array vs scalar initialization
        if (isArr) {
            if (!validateArrayInitialization(newSymbol, initNode, varType, isConst, context)) {
                return 0;
            }
        }else if(isStruct){
            assert(0 && "Struct initialization validation not implemented yet");
        } else {
            if (!validateScalarInitialization(newSymbol, node, varType, isConst, isMemRef, context)) {
                return 0;
            }
        }
    } else if (isConst) {
        // Const variables must be initialized
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
    
    // Check for pointer dereference to const
    if (isPointerDeref) {
        ASTNode derefTarget = left->children;

        // Case 1: *ptr where ptr is a variable
        if (derefTarget->nodeType == VARIABLE) {
            Symbol ptrSym = lookupSymbol(context->current, derefTarget->start, derefTarget->length);
            if (checkConstViolation(ptrSym, node, context, isPointerDeref)) {
                return 0;
            }
        }
        // Case 2: *arr[i] where arr is an array of pointers
        else if (derefTarget->nodeType == ARRAY_ACCESS) {
            ASTNode arrayNode = derefTarget->children;
            if (arrayNode && arrayNode->nodeType == VARIABLE) {
                Symbol arraySym =
                    lookupSymbol(context->current, arrayNode->start, arrayNode->length);
                // Check if array itself is const or has const memory reference
                if (arraySym && arraySym->isConst) {
                    REPORT_ERROR(ERROR_CONSTANT_REASSIGNMENT, node, context,
                                 "Cannot modify through const array element");
                    return 0;
                }
                if (arraySym && arraySym->hasConstMemRef) {
                    REPORT_ERROR(ERROR_CONSTANT_REASSIGNMENT, node, context,
                                 "Cannot modify through pointer to const");
                    return 0;
                }
            }
        }
    }

    // Normalize node types for easier processing
    if (left->nodeType == POINTER) left = left->children;
    if (right->nodeType == MEMADDRS) right = right->children;
    
    // Validate left-hand side is assignable
    if (left->nodeType != VARIABLE && 
        left->nodeType != MEMBER_ACCESS && 
        left->nodeType != ARRAY_ACCESS && 
        left->nodeType != POINTER) {
        REPORT_ERROR(ERROR_INVALID_ASSIGNMENT_TARGET, node, context, 
                    "Left side must be a variable or member access");
        return 0;
    }

    // Handle variable assignment
    if (left->nodeType == VARIABLE) {
        Symbol sym = lookupSymbolOrError(context, left);
        if (!sym) return 0;
        
        if (checkConstViolation(sym, node, context, isPointerDeref)) {
            return 0;
        }
        
        // Check for array assignment issues
        if (right->nodeType == VARIABLE) {
            Symbol rightSym = lookupSymbol(context->current, right->start, right->length);
            if (rightSym) {
                // Scalar = Array (error)
                if (!sym->isArray && rightSym->isArray) {
                    reportErrorWithText(ERROR_CANNOT_ASSIGN_ARRAY_TO_SCALAR, node, context,
                                      "Cannot assign array to scalar");
                    return 0;
                }
                // Array = Array (check sizes)
                if (sym->isArray && rightSym->isArray) {
                    if (sym->staticSize != rightSym->staticSize) {
                        char msg[100];
                        snprintf(msg, sizeof(msg), 
                                "Cannot assign array of size %d to array of size %d",
                                rightSym->staticSize, sym->staticSize);
                        REPORT_ERROR(ERROR_ARRAY_SIZE_MISMATCH, node, context, msg);
                        return 0;
                    }
                }
            }
        }
    }
    
    // Handle array access assignment
    if (left->nodeType == ARRAY_ACCESS) {
        if (!validateArrayAccessNode(left, context)) {
            return 0;
        }
        
        // Additional const checking for array access
        ASTNode baseNode = left->children;
        Symbol arraySym = lookupSymbol(context->current, baseNode->start, baseNode->length);
        if (arraySym) {
            if (arraySym->isConst) {
                reportErrorWithText(ERROR_CONSTANT_REASSIGNMENT, node, context,
                                  "Cannot modify const array");
                return 0;
            }
            if (arraySym->isPointer && arraySym->hasConstMemRef) {
                REPORT_ERROR(ERROR_CONSTANT_REASSIGNMENT, node, context,
                            "Cannot modify through pointer to const");
                return 0;
            }
        }
    }
    // Type compatibility checking
    DataType leftType = getExpressionType(leftForType, context);
    if (leftType == TYPE_UNKNOWN) {
        REPORT_ERROR(ERROR_EXPRESSION_TYPE_UNKNOWN_LHS, leftForType, context, 
                    "Cannot determine type of left-hand side");
        return 0;
    }
    
    DataType rightType = getExpressionType(rightForType, context);
    if (rightType == TYPE_UNKNOWN) {
        REPORT_ERROR(ERROR_EXPRESSION_TYPE_UNKNOWN_RHS, rightForType, context, 
                    "Cannot determine type of right-hand side");
        return 0;
    }
    
    CompatResult compat = areCompatible(leftType, rightType);
    if (compat == COMPAT_ERROR) {
        REPORT_ERROR(variableErrorCompatibleHandling(leftType, rightType), node, context, 
                    "Type mismatch in assignment");
        return 0;
    } else if (compat == COMPAT_WARNING) {
        REPORT_ERROR(ERROR_TYPE_MISMATCH_DOUBLE_TO_FLOAT, node, context, 
                    "Type mismatch in assignment");
    }
    
    // Handle address-of with const tracking
    if (left->nodeType == VARIABLE && right->nodeType == MEMADDRS) {
        Symbol leftSym = lookupSymbol(context->current, left->start, left->length);
        if (leftSym) {
            updateConstMemRef(leftSym, right->children, context);
        }
    }
    
    // Mark variable as initialized and handle pointer level validation
    if (left->nodeType == VARIABLE) {
        Symbol symbol = lookupSymbol(context->current, left->start, left->length);
        if (symbol && node->nodeType == ASSIGNMENT) {
            symbol->isInitialized = 1;
            
            // Pointer level validation
            if (symbol->isPointer && right->nodeType == VARIABLE) {
                Symbol rightSym = lookupSymbol(context->current, right->start, right->length);
                if (rightSym && rightSym->isPointer) {
                    if (symbol->pointerLvl != rightSym->pointerLvl) {
                        char msg[100];
                        snprintf(msg, sizeof(msg),
                                "Cannot assign pointer of level %d to pointer of level %d",
                                rightSym->pointerLvl, symbol->pointerLvl);
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

/**
 * @brief Validates variable usage for declaration and initialization status.
 *
 * Ensures that variables are properly declared and initialized before use.
 * Performs symbol table lookup and validates initialization status to
 * prevent use of uninitialized variables.
 *
 * @param node Variable usage AST node
 * @param context Type checking context
 * @return 1 if variable usage is valid, 0 if validation failed
 *
 * @note This function enforces the language rule that variables must
 *       be initialized before first use. Variables are considered
 *       initialized if they have an initializer in their declaration
 *       or have been assigned a value.
 */
int validateVariableUsage(ASTNode node, TypeCheckContext context) {
    if (node == NULL || node->start == NULL) {
        repError(ERROR_INTERNAL_PARSER_ERROR, "Variable usage node is null or has no name");
        return 0;
    };

    Symbol symbol = lookupSymbol(context->current, node->start, node->length);
    if (symbol == NULL) {
        char * tempText = extractText(node->start, node->length);
        REPORT_ERROR(ERROR_UNDEFINED_VARIABLE, node, context, tempText);
        free(tempText);
        return 0;
    }

    if (!symbol->isInitialized) {
        char * tempText = extractText(node->start, node->length);
        REPORT_ERROR(ERROR_VARIABLE_NOT_INITIALIZED, node, context, tempText);
        free(tempText); 
        return 0;
    }

    return 1;
}

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

int validateFunctionDef(ASTNode node, TypeCheckContext context) {
    if (node == NULL || node->nodeType != FUNCTION_DEFINITION || node->start == NULL || node->children == NULL) {
        repError(ERROR_INTERNAL_PARSER_ERROR,"Invalid function definition node");
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
        char * tempText = extractText(node->start, node->length);
        REPORT_ERROR(ERROR_VARIABLE_REDECLARED, node, context, tempText);
        free(tempText);
        freeParamList(parameters);
        return 0;
    }
    if(returnType == TYPE_STRUCT) {
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
        Symbol paramSymbol = addSymbol(context->current, param->nameStart, 
                                        param->nameLength, param->type, 
                                        node->line, node->column);
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
        success = typeCheckNode(bodyNode, context);
    }

    context->current = oldScope;
    context->currentFunction = oldFunction;

    return success;
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

    // Handle void return
    if (node->children == NULL) {
        if (expectedType != TYPE_VOID) {
            repError(ERROR_MISSING_RETURN_VALUE, "Non-void function must return a value");
            return 0;
        }
        return 1;
    }

    // Get actual return type
    DataType returnType = getExpressionType(node->children, context);
    if (returnType == TYPE_UNKNOWN) {
        return 0;
    }

    // Check void mismatch
    if (expectedType == TYPE_VOID) {
        repError(ERROR_UNEXPECTED_RETURN_VALUE, "Void function cannot return a value");
        return 0;
    }

    // Special handling for pointer returns
    if (expectedType == TYPE_POINTER || returnType == TYPE_POINTER) {
        // Both must be pointers
        if (expectedType != TYPE_POINTER || returnType != TYPE_POINTER) {
            repError(ERROR_RETURN_TYPE_MISMATCH, "Cannot return pointer from non-pointer function or vice versa");
            return 0;
        }
        
        // Check pointer levels and base type
        ASTNode retExpr = node->children;
        if (retExpr->nodeType == VARIABLE) {
            Symbol retSym = lookupSymbol(context->current, retExpr->start, retExpr->length);
            if (retSym && retSym->isPointer) {
                // Check pointer levels match
                if (funcSym->returnPointerLevel != retSym->pointerLvl) {
                    char msg[100];
                    snprintf(msg, sizeof(msg),
                            "Pointer level mismatch: expected %d, got %d",
                            funcSym->returnPointerLevel, retSym->pointerLvl);
                    REPORT_ERROR(ERROR_RETURN_TYPE_MISMATCH, node, context, msg);
                    return 0;
                }
                
                // Check base types match
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
    
    // For non-pointer types, standard compatibility check
    CompatResult compat = areCompatible(expectedType, returnType);
    if (compat == COMPAT_ERROR) {
        repError(ERROR_RETURN_TYPE_MISMATCH, "return");
        return 0;
    }

    funcSym->returnedVar = lookupSymbol(context->current, node->children->start, node->children->length);
    printf("return type is %s, expected %s\n", getTypeName(returnType), getTypeName(expectedType));

    return 1;
}

/**
 * @brief Enqueues a block scope for later use by IR generation.
 */
void enqueueBlockScope(TypeCheckContext context, SymbolTable scope) {
    if (!context || !scope) return;
    
    BlockScopeNode node = malloc(sizeof(struct BlockScopeNode));
    if (!node) return;
    
    node->scope = scope;
    node->next = NULL;
    
    if (context->blockScopesTail) {
        context->blockScopesTail->next = node;
    } else {
        context->blockScopesHead = node;
    }
    context->blockScopesTail = node;
}

/**
 * @brief Dequeues the next block scope for IR generation.
 * 
 * @return The next scope, or NULL if queue is empty.
 */
SymbolTable dequeueBlockScope(TypeCheckContext context) {
    if (!context || !context->blockScopesHead) return NULL;
    
    BlockScopeNode node = context->blockScopesHead;
    SymbolTable scope = node->scope;
    
    context->blockScopesHead = node->next;
    if (!context->blockScopesHead) {
        context->blockScopesTail = NULL;
    }
    
    free(node);
    return scope;
}


/**
 * @brief Recursively type checks all children of an AST node.
 *
 * Traverses all child nodes (linked as siblings) and performs type
 * checking on each one. Accumulates success status to ensure all
 * children pass validation.
 *
 * @param node Parent AST node
 * @param context Type checking context
 * @return 1 if all children passed type checking, 0 if any failed
 *
 * @note This function continues checking all children even if some
 *       fail, allowing for comprehensive error reporting rather than
 *       stopping at the first error.
 */
int typeCheckChildren(ASTNode node, TypeCheckContext context) {
    if (node == NULL) return 1;

    int success = 1;
    ASTNode child = node->children;
    while (child != NULL) {
        if (!typeCheckNode(child, context)) {
            success = 0;
        }
        child = child->brothers;
    }

    return success;
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
                    // has to free previously allocated fields
                    free(structType);
                    free(structField);
                    return NULL;
                }
                int pointerLevel = 0;
                ASTNode baseTypeNode = getBaseTypeFromPointerChain(field->children->children, &pointerLevel);
                DataType type =  getDataTypeFromNode(baseTypeNode->nodeType);
                structField->nameStart = field->start;
                structField->nameLength = field->length;       
                structField->type = type;
                // @todo: this can be extracted to a createStructField
                if(type == TYPE_STRUCT) {
                    Symbol structSymbol = lookupSymbol(context->current, field->children->start, field->children->length);
                    if(!structSymbol || structSymbol->symbolType != SYMBOL_TYPE){
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
                    if (check->nameLength == structField->nameLength && memcmp(check->nameStart, structField->nameStart, check->nameLength) == 0) {
                        REPORT_ERROR(ERROR_VARIABLE_REDECLARED, node, context, "duplicate field on struct");
                        free(structField);
                        return NULL;
                    }
                    check = check->next;
                }
                structType->fieldCount++;
                if(!structType->fields){
                    structType->fields = structField;
                }else {
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
        char * tempText = extractText(node->start, node->length);
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
    if(canItBeCasted == COMPAT_ERROR){
        REPORT_ERROR(ERROR_FORBIDDEN_CAST, node, context, "Cannot cast between these types");
        return 0;
    }
    // just a warning
    if (isPrecisionLossCast(sourceType, targetType)) {
        REPORT_ERROR(ERROR_CAST_PRECISION_LOSS, node, context, "Cast may lose precision");
    }
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
        char * tempText = extractText(node->start, node->length);
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

/**
 * @brief Recursively type checks a single AST node and its subtree.
 *
 * Performs comprehensive type checking based on the node type. Handles
 * different categories of nodes including declarations, assignments,
 * expressions, statements, and control flow constructs. Manages scope
 * creation and cleanup for block statements.
 *
 * @param node AST node to type check
 * @param context Type checking context
 * @return 1 if type checking passed, 0 if errors occurred
 *
 * @note This is the main dispatch function for type checking. It handles
 *       scope management for blocks and delegates specific validation to
 *       specialized functions for different node types.
 */
int typeCheckNode(ASTNode node, TypeCheckContext context) {
    if (node == NULL) return 1;

    int success = 1;
    switch (node->nodeType) {
        case PROGRAM:
            success = typeCheckChildren(node, context);
            break;
        case ASSIGNMENT:                  
        case COMPOUND_ADD_ASSIGN:         
        case COMPOUND_SUB_ASSIGN:         
        case COMPOUND_MUL_ASSIGN:        
        case COMPOUND_DIV_ASSIGN:
        case COMPOUND_AND_ASSIGN:        
        case COMPOUND_OR_ASSIGN:
        case COMPOUND_XOR_ASSIGN:
        case COMPOUND_LSHIFT_ASSIGN:
        case COMPOUND_RSHIFT_ASSIGN:
            success = validateAssignment(node, context);
            break;
        case LET_DEC:
        case CONST_DEC : {
            ASTNode varDef = node->children;
            if(!varDef){
                repError(ERROR_INTERNAL_PARSER_ERROR, "Declaration wrapper has no child");
                return 0;
            }
            int isConst = node->nodeType == CONST_DEC;
            success = validateVariableDeclaration(varDef, context, isConst);
            break;
        }
        case FUNCTION_DEFINITION:
            success = validateFunctionDef(node, context);
            break;

        case FUNCTION_CALL:
            success = validateFunctionCall(node, context);
            break;

        case RETURN_STATEMENT:
            success = validateReturnStatement(node, context);
            break;
        case PARAMETER_LIST:
        case PARAMETER:
        case ARGUMENT_LIST:
        case RETURN_TYPE:
            success = typeCheckChildren(node, context);
            break;
        case BLOCK_STATEMENT:
        case BLOCK_EXPRESSION: {
            SymbolTable oldScope = context->current;
            SymbolTable blockScope = createSymbolTable(oldScope);

            if (blockScope == NULL) {
                repError(ERROR_SYMBOL_TABLE_CREATION_FAILED, "Failed to create new scope for block");
                success = 0;
                break;
            }

            context->current = blockScope;
            
            // Enqueue the scope for IR generation to use later
            enqueueBlockScope(context, blockScope);

            success = typeCheckChildren(node, context);
            
            context->current = oldScope;
            break;
        }
        case TERNARY_CONDITIONAL:
        case TERNARY_IF_EXPR:
        case TERNARY_ELSE_EXPR:
        case IF_CONDITIONAL:
        case LOOP_STATEMENT:
        case IF_TRUE_BRANCH:
        case ELSE_BRANCH:
            success = typeCheckChildren(node, context);
            break;

        case VARIABLE:
            success = validateVariableUsage(node, context);
            break;

        case ADD_OP:
        case SUB_OP:
        case MUL_OP:
        case DIV_OP:
        case MOD_OP:
        case BITWISE_AND:
        case BITWISE_OR:
        case BITWISE_XOR:
        case BITWISE_LSHIFT:
        case BITWISE_RSHIFT:
        case EQUAL_OP:
        case NOT_EQUAL_OP:
        case LESS_THAN_OP:
        case GREATER_THAN_OP:
        case LESS_EQUAL_OP:
        case GREATER_EQUAL_OP:
        case LOGIC_AND:
        case LOGIC_OR: {
            success = typeCheckChildren(node, context);
            if (success) {
                DataType resultType = getExpressionType(node, context);
                if (resultType == TYPE_UNKNOWN) {
                    success = 0;
                }
            }
            break;
        }
        case CAST_EXPRESSION:
            success = validateCastExpression(node, context);
            break;
        case UNARY_MINUS_OP:
        case UNARY_PLUS_OP:
        case LOGIC_NOT:
        case PRE_INCREMENT:
        case PRE_DECREMENT:
        case POST_INCREMENT:
        case POST_DECREMENT:
        case BITWISE_NOT:
            success = typeCheckChildren(node, context);
            if (success) {
                DataType resultType = getExpressionType(node, context);
                if (resultType == TYPE_UNKNOWN) {
                    success = 0;
                }
            }
            break;

        case LITERAL: break;
        case STRUCT_DEFINITION:
            success = validateStructDef(node, context);
            break;
        case STRUCT_VARIABLE_DEFINITION:
            success = validateStructVarDec(node, context);
            break;
        default:
            success = typeCheckChildren(node, context);
            break;
    }
    return success;
}

TypeCheckContext typeCheckAST(ASTNode ast, const char *sourceCode, const char *filename, TypeCheckContext ref) {
    TypeCheckContext context;
    if(ref){
        context = ref;
    }else {
        context = createTypeCheckContext(sourceCode, filename);
    }
    if (context == NULL) {
        repError(ERROR_CONTEXT_CREATION_FAILED, "Failed to create type check context");
        return 0;
    }
    if (ast && ast->nodeType == PROGRAM && ast->children == NULL) {
        repError(ERROR_NO_ENTRY_POINT, "Empty program");
        freeTypeCheckContext(context);
        return NULL;
    }
    int success = typeCheckNode(ast, context);
    if (!success) {
        freeTypeCheckContext(context);
        return NULL;
    }
    return context;
}
