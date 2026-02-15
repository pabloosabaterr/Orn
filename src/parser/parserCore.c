#include "parserInternal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const StatementHandler statementHandlers[] = {
    {TK_IMPORT,  parseImport},
    {TK_EXPORT,  parseExportFunction},
    {TK_FN,      parseFunction},
    {TK_RETURN,  parseReturnStatement},
    {TK_WHILE,   parseLoop},
    {TK_LBRACE,  parseBlock},
    {TK_STRUCT,  parseStruct},
    {TK_IF,      parseIf},
    {TK_FOR,     parseForLoop},
    {TK_NULL,    NULL}
};

/**
 * @brief Creates an error context
 */
ErrorContext* createErrorContextFromParser(TokenList* list, size_t* pos){
    static ErrorContext ctx;
    static char* lastSourceLine = NULL;

    if(!list || *pos >= list->count) return NULL;

    size_t tempPos = list->tokens[*pos].type != TK_SEMI ? *pos-1 : *pos;
    Token* token = &list->tokens[tempPos];

    if(lastSourceLine){
        free(lastSourceLine);
        lastSourceLine = NULL;
    }

    lastSourceLine= extractSourceLineForToken(list, token);

    ctx.file = list->filename ? list->filename : "source";
    ctx.line = token->line;
    ctx.column = token->column;
    ctx.source = lastSourceLine;
    ctx.startColumn = token->column;
    ctx.length = token->length;

    return &ctx;
}

static ASTContext* buildASTContextFromTokenList(TokenList* list){
    ASTContext* ctx = malloc(sizeof(struct ASTContext));
    ctx->buffer = list->buffer;
    ctx->filename = list->filename;
    return ctx;
}

/**
 * PUBLIC
 */

/**
 * @brief Parses a list of tokens into an AST.
 */
ASTContext* ASTGenerator(TokenList* tokenList){
    if(!tokenList || tokenList->count == 0) return NULL;

    ASTContext* astContext = buildASTContextFromTokenList(tokenList);
    ASTNode programNode;
    
    size_t pos = 0;
    CREATE_NODE_OR_FAIL(programNode, NULL, PROGRAM, tokenList, &pos);
    astContext->root = programNode;

    ASTNode lastStatement = NULL;
    size_t lastPos = (size_t)-1;

    while(pos < tokenList->count){
        if(tokenList->tokens[pos].type == TK_EOF) break;
        if(pos == lastPos){
            reportError(ERROR_PARSER_STUCK, createErrorContextFromParser(tokenList, &pos), "Parser is stuck at token");
            pos++;
            continue;
        }
        lastPos = pos;
        ASTNode currentStatement = parseStatement(tokenList, &pos);
        if(currentStatement){
            if(!programNode->children){
                programNode->children = currentStatement;
            }else{
                lastStatement->brothers = currentStatement;
            }

            ASTNode tail = currentStatement;
            while(tail && tail->brothers) tail = tail->brothers;
            lastStatement = tail;
        }
    }

    return astContext;
}

/**
 * PRINTING AST
 */

void printASTTree(ASTNode node, char *prefix, int isLast) {
    if (node == NULL) return;

    const char *nodeTypeStr = getNodeTypeName(node->nodeType);

    printf("%s%s%s", prefix, isLast ? "┗ " : "┣ ", nodeTypeStr);
    if (node->start && node->length > 0) {
        char *str = extractText(node->start, node->length);
        if (str) {
            printf(": %s", str);
            free(str);
        }
    }
    printf("\n");

    char newPrefix[256];
    sprintf(newPrefix, "%s%s", prefix, isLast ? "    " : "┃   ");

    ASTNode child = node->children;
    while (child != NULL) {
        printASTTree(child, newPrefix, child->brothers == NULL);
        child = child->brothers;
    }
}

void printAST(ASTNode node, int depth) {
    (void)depth;
    if (node == NULL ||
        (node->nodeType != PROGRAM && node->nodeType != null_NODE)) {
        printf("Empty or invalid AST.\n");
        return;
    }
    printf("AST:\n");
    ASTNode child = node->children;
    while (child != NULL) {
        printASTTree(child, "", child->brothers == NULL);
        child = child->brothers;
    }
}

/**
 * FREES
 */


void freeAST(ASTNode node) {
    if (node == NULL) return;
    freeAST(node->children);
    freeAST(node->brothers);
    free(node);
}

void freeASTContext(ASTContext *ctx) {
    if (ctx) {
        if (ctx->root) freeAST(ctx->root);
        free(ctx);
    }
}