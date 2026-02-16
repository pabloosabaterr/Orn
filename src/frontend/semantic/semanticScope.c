#include "semanticInternal.h"

/**
 * Scope queues are being used by the IR later on
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