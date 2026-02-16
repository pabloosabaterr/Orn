#include "semanticInternal.h"

void freeSymbol(Symbol symbol) {
    if (symbol == NULL) return;
    if (symbol->symbolType == SYMBOL_FUNCTION && symbol->parameters) {
        freeParamList(symbol->parameters);
    }
    free(symbol);
}

/**
 * Parammeter helpers
 */

FunctionParameter createParameter(const char *nameStart, size_t nameLen, DataType type) {
    FunctionParameter param = malloc(sizeof(struct FunctionParameter));
    if (param == NULL) return NULL;

    param->nameStart = nameStart;
    param->nameLength = nameLen;
    param->type = type;
    param->next = NULL;
    param->isPointer = 0;
    param->pointerLevel = 0;
    return param;
}

void freeParamList(FunctionParameter paramList) {
    FunctionParameter current = paramList;
    while (current != NULL) {
        FunctionParameter next = current->next;
        free(current);
        current = next;
    }
}

/**
 * Symbol table
 */

SymbolTable createSymbolTable(SymbolTable parent) {
    SymbolTable table = malloc(sizeof(struct SymbolTable));
    if (table == NULL) return NULL;

    table->symbols = NULL;
    table->parent = parent;
    table->child = NULL;
    table->brother = NULL;
    table->scope = (parent == NULL) ? 0 : parent->scope + 1;
    table->symbolCount = 0;

    /* Register as child of parent */
    if (parent != NULL) {
        table->brother = parent->child;
        parent->child = table;
    }

    return table;
}

void freeSymbolTable(SymbolTable table) {
    if (table == NULL) return;

    /* Free all child scopes first */
    SymbolTable child = table->child;
    while (child != NULL) {
        SymbolTable nextChild = child->brother;
        freeSymbolTable(child);
        child = nextChild;
    }

    /* Free symbols in this scope */
    Symbol current = table->symbols;
    while (current != NULL) {
        Symbol next = current->next;
        freeSymbol(current);
        current = next;
    }

    free(table);
}

/** 
 * Look up table
 */

Symbol lookupSymbolCurrentOnly(SymbolTable table, const char *nameStart, size_t nameLength) {
    if (table == NULL || nameStart == NULL) return NULL;

    Symbol current = table->symbols;
    while (current != NULL) {
        if (current->nameLength == nameLength &&
            memcmp(current->nameStart, nameStart, nameLength) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

Symbol lookupSymbol(SymbolTable table, const char *nameStart, size_t nameLength) {
    if (table == NULL || nameStart == NULL) return NULL;

    Symbol current = table->symbols;
    while (current != NULL) {
        if (current->nameLength == nameLength &&
            memcmp(current->nameStart, nameStart, nameLength) == 0) {
            return current;
        }
        current = current->next;
    }

    if (table->parent != NULL) {
        return lookupSymbol(table->parent, nameStart, nameLength);
    }

    return NULL;
}

/**
 * add symbols
 */

Symbol addSymbol(SymbolTable table, const char *nameStart, size_t nameLength, DataType type, int line, int column) {
    if (table == NULL || nameStart == NULL) return NULL;

    Symbol existing = lookupSymbolCurrentOnly(table, nameStart, nameLength);
    if (existing != NULL) return NULL;

    Symbol newSymbol = malloc(sizeof(struct Symbol));
    if (newSymbol == NULL) return NULL;
    memset(newSymbol, 0, sizeof(struct Symbol));

    newSymbol->nameStart = nameStart;
    newSymbol->nameLength = nameLength;
    newSymbol->symbolType = SYMBOL_VARIABLE;
    newSymbol->type = type;
    newSymbol->line = line;
    newSymbol->column = column;
    newSymbol->scope = table->scope;
    newSymbol->isInitialized = 0;
    newSymbol->parameters = NULL;
    newSymbol->paramCount = 0;
    newSymbol->baseType = type;
    newSymbol->next = table->symbols;
    table->symbols = newSymbol;
    table->symbolCount++;

    return newSymbol;
}

Symbol addSymbolFromNode(SymbolTable table, ASTNode node, DataType type) {
    if (!node) return NULL;
    return addSymbol(table, node->start, node->length, type, node->line, node->column);
}

static Symbol addFunctionSymbol(SymbolTable symbolTable, const char *nameStart, size_t nameLength, DataType returnType, FunctionParameter parameters, int paramCount, int line, int column) {
    if (symbolTable == NULL || nameStart == NULL) return NULL;

    Symbol exists = lookupSymbol(symbolTable, nameStart, nameLength);
    if (exists != NULL) return NULL;

    Symbol newSymbol = malloc(sizeof(struct Symbol));
    if (newSymbol == NULL) return NULL;
    memset(newSymbol, 0, sizeof(struct Symbol));

    newSymbol->nameStart = nameStart;
    newSymbol->nameLength = nameLength;
    newSymbol->symbolType = SYMBOL_FUNCTION;
    newSymbol->type = returnType;
    newSymbol->line = line;
    newSymbol->column = column;
    newSymbol->scope = symbolTable->scope;
    newSymbol->isInitialized = 1;
    newSymbol->parameters = parameters;
    newSymbol->paramCount = paramCount;
    newSymbol->functionScope = NULL;

    newSymbol->next = symbolTable->symbols;
    symbolTable->symbols = newSymbol;
    symbolTable->symbolCount++;

    return newSymbol;
}

Symbol addFunctionSymbolFromNode(SymbolTable symbolTable, ASTNode node, DataType returnType, FunctionParameter parameters, int paramCount) {
    if (!node) return NULL;
    return addFunctionSymbol(symbolTable, node->start, node->length, returnType, parameters, paramCount, node->line, node->column);
}

Symbol addFunctionSymbolFromString(SymbolTable symbolTable, const char *name, DataType returnType, FunctionParameter parameters, int paramCount, int line, int column) {
    if (!name) return NULL;
    return addFunctionSymbol(symbolTable, name, strlen(name), returnType, parameters, paramCount, line, column);
}