/**
 * @file semanticSymbols.c
 * @brief Symbol resolution and identifier-related helpers.
 *
 * Responsibilities:
 *   - Identifier resolution with error reporting
 *   - AST node-type → DataType conversion
 *
 * No type compatibility logic lives here.
 */

#include "semanticInternal.h"

/* This are just two helpers */

Symbol lookupSymbolOrError(TypeCheckContext context, ASTNode node) {
    Symbol sym = lookupSymbol(context->current, node->start, node->length);
    if (!sym) {
        reportErrorWithText(ERROR_UNDEFINED_VARIABLE, node, context, "Undefined variable");
    }
    return sym;
}

DataType getDataTypeFromNode(NodeTypes nodeType) {
    switch (nodeType) {
        case REF_I8:    return TYPE_I8;
        case REF_I16:   return TYPE_I16;
        case REF_I32:   return TYPE_I32;
        case REF_I64:   return TYPE_I64;
        case REF_U8:    return TYPE_U8;
        case REF_U16:   return TYPE_U16;
        case REF_U32:   return TYPE_U32;
        case REF_U64:   return TYPE_U64;
        case REF_FLOAT:  return TYPE_FLOAT;
        case REF_STRING: return TYPE_STRING;
        case REF_BOOL:   return TYPE_BOOL;
        case REF_DOUBLE: return TYPE_DOUBLE;
        case REF_CUSTOM: return TYPE_STRUCT;
        case REF_VOID:   return TYPE_VOID;
        default:         return TYPE_UNKNOWN;
    }
}