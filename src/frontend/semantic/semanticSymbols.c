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
        case REF_INT:    return TYPE_INT;
        case REF_FLOAT:  return TYPE_FLOAT;
        case REF_STRING: return TYPE_STRING;
        case REF_BOOL:   return TYPE_BOOL;
        case REF_DOUBLE: return TYPE_DOUBLE;
        case REF_CUSTOM: return TYPE_STRUCT;
        case REF_VOID:   return TYPE_VOID;
        default:         return TYPE_UNKNOWN;
    }
}