#include "semanticInternal.h"

void freeSymbol(Symbol symbol) {
    if (symbol == NULL) return;
    if (symbol->symbolType == SYMBOL_FUNCTION && symbol->parameters) {
        freeParamList(symbol->parameters);
    }
    free(symbol);
}

void freeSymbolTable(SymbolTable table) {
    if (!table) return;

    SymbolTable child = table->child;
    while (child) {
        SymbolTable nextChild = child->brother;
        freeSymbolTable(child);
        child = nextChild;
    }

    for (int i = 0; i < table->bucketCount; i++) {
        Symbol current = table->symbols[i];
        while (current) {
            Symbol next = current->next;
            freeSymbol(current);
            current = next;
        }
    }

    free(table->symbols);
    free(table);
}

/**
 * symbols Hashtable operations 
 */

/* hash documentation http://www.isthe.com/chongo/tech/comp/fnv/ */
static uint32_t hashName(const char* name, size_t len){
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < len; i++) {
        hash ^= (unsigned char)name[i];
        hash *= 16777619u;
    }
    return hash;
}

static int rehashTable(SymbolTable table){
    int newCount = table->bucketCount * 2;
    Symbol* newBuckets = calloc(newCount, sizeof(Symbol));
    if(!newBuckets) return 0;

    for(int i = 0; i < table->bucketCount; ++i){
        Symbol current = table->symbols[i];
        while(current){
            Symbol next = current->next;
            uint32_t index = hashName(current->nameStart, current->nameLength) & (newCount - 1);
            current->next = newBuckets[index];
            newBuckets[index] = current;
            current = next;
        }
    }

    free(table->symbols);
    table->symbols = newBuckets;
    table->bucketCount = newCount;
    return 1;
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

    table->symbols = calloc(SYMBOL_TABLE_BUCKETS, sizeof(Symbol));
    if (!table->symbols) {
        free(table);
        return NULL;
    }
    table->bucketCount = SYMBOL_TABLE_BUCKETS;
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

/** 
 * Look up table
 */

Symbol lookupSymbolCurrentOnly(SymbolTable table, const char *nameStart, size_t nameLength) {
    if (!table || !nameStart) return NULL;

    uint32_t index = hashName(nameStart, nameLength) & (table->bucketCount - 1);
    Symbol current = table->symbols[index];
    while (current) {
        if (current->nameLength == nameLength &&
            memcmp(current->nameStart, nameStart, nameLength) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

Symbol lookupSymbol(SymbolTable table, const char *nameStart, size_t nameLength) {
    if (!table || !nameStart) return NULL;

    uint32_t index = hashName(nameStart, nameLength) & (table->bucketCount - 1);
    Symbol current = table->symbols[index];
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
    if (!table || !nameStart) return NULL;

    Symbol existing = lookupSymbolCurrentOnly(table, nameStart, nameLength);
    if (existing) return NULL;

    if(table->symbolCount >= table->bucketCount * SYMBOL_TABLE_LOAD_FACTOR){
        rehashTable(table);
    }

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

    uint32_t index = hashName(nameStart, nameLength) & (table->bucketCount - 1);
    newSymbol->next = table->symbols[index];
    table->symbols[index] = newSymbol;
    table->symbolCount++;

    return newSymbol;
}

Symbol addSymbolFromNode(SymbolTable table, ASTNode node, DataType type) {
    if (!node) return NULL;
    return addSymbol(table, node->start, node->length, type, node->line, node->column);
}

static Symbol addFunctionSymbol(SymbolTable symbolTable, const char *nameStart, size_t nameLength, DataType returnType, FunctionParameter parameters, int paramCount, int line, int column) {
    if (!symbolTable || !nameStart) return NULL;

    Symbol exists = lookupSymbol(symbolTable, nameStart, nameLength);
    if (exists) return NULL;

    if(symbolTable->symbolCount >= symbolTable->bucketCount * SYMBOL_TABLE_LOAD_FACTOR){
        rehashTable(symbolTable);
    }

    Symbol newSymbol = malloc(sizeof(struct Symbol));
    if (!newSymbol) return NULL;
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

    uint32_t index = hashName(nameStart, nameLength) & (symbolTable->bucketCount - 1);
    newSymbol->next = symbolTable->symbols[index];
    symbolTable->symbols[index] = newSymbol;
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