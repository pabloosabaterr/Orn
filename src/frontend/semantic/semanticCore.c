/**
 * @file semanticCore.c
 * @brief Entry point and orchestration for semantic analysis.
 *
 * Responsibilities:
 *   - Create / destroy TypeCheckContext
 *   - Register built-ins
 *   - Walk the AST via typeCheckNode (main dispatch)
 *   - Delegate to specialised check functions
 *
 * Equivalent to parserCore.c in the parser.
 */

#include "semanticInternal.h"

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
        case CONST_DEC: {
            ASTNode varDef = node->children;
            if (!varDef) {
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

            /* Enqueue the scope for IR generation to use later */
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

/**
 * @brief Recursively type checks all children of an AST node.
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

TypeCheckContext typeCheckAST(ASTNode ast, const char *sourceCode, const char *filename, TypeCheckContext ref) {
    TypeCheckContext context;
    if (ref) {
        context = ref;
    } else {
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