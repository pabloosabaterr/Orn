#include "parserInternal.h"

NodeTypes getDecType(TokenType type){
    switch(type){
        case TK_INT:    return REF_INT;
        case TK_FLOAT:  return REF_FLOAT;
        case TK_DOUBLE: return REF_DOUBLE;
        case TK_BOOL:   return REF_BOOL;
        case TK_STRING: return REF_STRING;
        default:        return null_NODE;
    }
}

NodeTypes getTypeNodeFromToken(TokenType type){
    switch(type){
        case TK_INT:    return REF_INT;
        case TK_STRING: return REF_STRING;
        case TK_FLOAT:  return REF_FLOAT;
        case TK_BOOL:   return REF_BOOL;
        case TK_VOID:   return REF_VOID;
        case TK_DOUBLE: return REF_DOUBLE;
        case TK_LIT:    return REF_CUSTOM;
        default:        return null_NODE;
    }
}

int isTypeToken(TokenType type){
    return (type == TK_INT ||
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