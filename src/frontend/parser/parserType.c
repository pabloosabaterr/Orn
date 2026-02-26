/**
 * @file parserType.c
 * @brief Type parsing and type-token classification.
 *
 * Responsibilities:
 *   - parseType(): parse a full type expression (pointers, references, base)
 *   - getDecType() / getTypeNodeFromToken(): map token → NodeTypes for types
 *   - isTypeToken(): predicate for built-in type keywords
 *
 */

#include "parserInternal.h"

NodeTypes getDecType(TokenType type){
    switch(type){
        case TK_I8:     return REF_I8;
        case TK_I16:    return REF_I16;
        case TK_I32:    return REF_I32;
        case TK_I64:    return REF_I64;
        case TK_U8:     return REF_U8;
        case TK_U16:    return REF_U16;
        case TK_U32:    return REF_U32;
        case TK_U64:    return REF_U64;
        case TK_FLOAT:  return REF_FLOAT;
        case TK_DOUBLE: return REF_DOUBLE;
        case TK_BOOL:   return REF_BOOL;
        case TK_STRING: return REF_STRING;
        default:        return null_NODE;
    }
}

NodeTypes getTypeNodeFromToken(TokenType type){
    switch(type){
        case TK_I8:     return REF_I8;
        case TK_I16:    return REF_I16;
        case TK_I32:    return REF_I32;
        case TK_I64:    return REF_I64;
        case TK_U8:     return REF_U8;
        case TK_U16:    return REF_U16;
        case TK_U32:    return REF_U32;
        case TK_U64:    return REF_U64;
        case TK_STRING: return REF_STRING;
        case TK_FLOAT:  return REF_FLOAT;
        case TK_BOOL:   return REF_BOOL;
        case TK_VOID:   return REF_VOID;
        case TK_DOUBLE: return REF_DOUBLE;
        case TK_LIT:    return REF_CUSTOM;
        default:        return null_NODE;
    }
}

int isIntTypeToken(TokenType type){
    return (type == TK_I8 ||
            type == TK_I16 ||
            type == TK_I32 ||
            type == TK_I64 ||
            type == TK_U8 ||
            type == TK_U16 ||
            type == TK_U32 ||
            type == TK_U64);
}

int isIntTypeNode(NodeTypes nodeType){
    return (nodeType == REF_I8 ||
            nodeType == REF_I16 ||
            nodeType == REF_I32 ||
            nodeType == REF_I64 ||
            nodeType == REF_U8 ||
            nodeType == REF_U16 ||
            nodeType == REF_U32 ||
            nodeType == REF_U64 || 
            nodeType == REF_INT_UNRESOLVED);
}

int isTypeToken(TokenType type){
    return (type == TK_I8 ||
            type == TK_I16 ||
            type == TK_I32 ||
            type == TK_I64 ||
            type == TK_U8 ||
            type == TK_U16 ||
            type == TK_U32 ||
            type == TK_U64 ||
            type == TK_STRING ||
            type == TK_FLOAT ||
            type == TK_BOOL ||
            type == TK_DOUBLE ||
            type == TK_LIT ||
            type == TK_VOID);
}

ASTNode parseType(TokenList* list, size_t* pos){
    if(*pos >= list->count) return NULL;

    int pointerCount = 0;
    int isReference = 0;

    while(*pos < list->count){
        Token* token = &list->tokens[*pos];
        if (token->type == TK_STAR) {
            pointerCount++;
            ADVANCE_TOKEN(list, pos);
        } else if (token->type == TK_AMPERSAND && !isReference) {
            isReference = 1;
            ADVANCE_TOKEN(list, pos);
        } else {
            break;
        }
    }

    Token* typeToken = &list->tokens[*pos];
    ASTNode typeNode;

    if(isTypeToken(typeToken->type)){
        NodeTypes baseType = getTypeNodeFromToken(typeToken->type);
        CREATE_NODE_OR_FAIL(typeNode, typeToken, baseType, list, pos);
        ADVANCE_TOKEN(list, pos);
    } else if(detectLitType(typeToken, list, pos) == VARIABLE){
        CREATE_NODE_OR_FAIL(typeNode, typeToken, REF_CUSTOM, list, pos);
        ADVANCE_TOKEN(list, pos);
    } else {
        reportError(ERROR_EXPECTED_TYPE, createErrorContextFromParser(list, pos), "Expected valid type");
        return NULL;
    }

    /* Wrap in POINTER nodes */
    for(int i = 0; i < pointerCount; ++i){
        ASTNode pointerNode;
        CREATE_NODE_OR_FAIL(pointerNode, typeToken, POINTER, list, pos);
        pointerNode->children = typeNode;
        typeNode = pointerNode;
    }

    /* Wrap in MEMADDRS node if its a reference */
    if(isReference){
        ASTNode refNode;
        CREATE_NODE_OR_FAIL(refNode, typeToken, MEMADDRS, list, pos);
        refNode->children = typeNode;
        typeNode = refNode;
    }

    return typeNode;
}