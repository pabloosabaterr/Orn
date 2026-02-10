#include "interface.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *dataTypeToString(DataType type) {
    switch (type) {
    case TYPE_INT:
        return "int";
    case TYPE_FLOAT:
        return "float";
    case TYPE_DOUBLE:
        return "double";
    case TYPE_STRING:
        return "string";
    case TYPE_BOOL:
        return "bool";
    case TYPE_VOID:
        return "void";
    case TYPE_POINTER:
        return "ptr";
    default:
        return "unknown";
    }
}

DataType stringToDataType(const char *str) {
    if (!str) return TYPE_UNKNOWN;
    if (strcmp(str, "int") == 0) return TYPE_INT;
    if (strcmp(str, "float") == 0) return TYPE_FLOAT;
    if (strcmp(str, "double") == 0) return TYPE_DOUBLE;
    if (strcmp(str, "string") == 0) return TYPE_STRING;
    if (strcmp(str, "bool") == 0) return TYPE_BOOL;
    if (strcmp(str, "void") == 0) return TYPE_VOID;
    if (strcmp(str, "ptr") == 0) return TYPE_POINTER;
    if (str[0] == '*') return TYPE_POINTER;
    return TYPE_STRUCT;
}

static char *buildTypeString(DataType type, int pointerLevel) {
    const char *baseStr = dataTypeToString(type);

    size_t len = pointerLevel + strlen(baseStr) + 1;
    char *result = malloc(len);
    if (!result) return NULL;

    memset(result, '*', pointerLevel);
    strcpy(result + pointerLevel, baseStr);
    return result;
}

static char *buildParamSignature(FunctionParameter params) {
    if (!params) {
        return strdup("");
    }

    size_t totalLen = 0;
    FunctionParameter p = params;
    while (p) {
        totalLen += strlen(dataTypeToString(p->type)) + p->pointerLevel + 3;
        p = p->next;
    }

    char *result = malloc(totalLen + 1);
    if (!result) return NULL;
    result[0] = '\0';

    p = params;
    int first = 1;
    while (p) {
        if (!first) strcat(result, ", ");
        first = 0;

        // Handle pointer types
        DataType baseType = p->type;
        int ptrLevel = p->pointerLevel;

        char *typeStr = buildTypeString(baseType, ptrLevel);
        if (typeStr) {
            strcat(result, typeStr);
            free(typeStr);
        }
        p = p->next;
    }

    return result;
}

static ExportedFunction *createExportedFunction(ASTNode funcNode, TypeCheckContext ctx) {
    if (!funcNode || funcNode->nodeType != FUNCTION_DEFINITION) return NULL;

    ExportedFunction *ef = calloc(1, sizeof(ExportedFunction));
    if (!ef) return NULL;

    ef->name = strndup(funcNode->start, funcNode->length);

    Symbol funcSym = lookupSymbol(ctx->global, funcNode->start, funcNode->length);
    if (funcSym && funcSym->symbolType == SYMBOL_FUNCTION) {
        ef->signature = buildParamSignature(funcSym->parameters);
        DataType retType = funcSym->returnsPointer ? funcSym->returnBaseType : funcSym->type;
        int retPtrLevel = funcSym->returnPointerLevel;
        ef->returnType = buildTypeString(retType, retPtrLevel);
    }

    return ef;
}

static ExportedStruct *createExportedStruct(ASTNode structNode, TypeCheckContext ctx) {
    if (!structNode || structNode->nodeType != STRUCT_DEFINITION) return NULL;

    Symbol structSym = lookupSymbol(ctx->global, structNode->start, structNode->length);
    if (!structSym || structSym->symbolType != SYMBOL_TYPE || !structSym->structType) {
        return NULL;
    }

    ExportedStruct *es = calloc(1, sizeof(ExportedStruct));
    if (!es) return NULL;

    es->name = strndup(structNode->start, structNode->length);
    es->size = structSym->structType->size;
    es->fieldCount = structSym->structType->fieldCount;
    es->fields = NULL;
    es->next = NULL;

    StructField field = structSym->structType->fields;
    ExportedField *lastField = NULL;

    while (field) {
        ExportedField *ef = calloc(1, sizeof(ExportedField));
        if (!ef) {
            field = field->next;
            continue;
        }

        ef->name = strndup(field->nameStart, field->nameLength);
        ef->type = strdup(dataTypeToString(field->type));
        ef->offset = field->offset;
        ef->isPointer = 0;  // @todo: pointers in structs
        ef->pointerLevel = 0;
        ef->next = NULL;

        if (!es->fields) {
            es->fields = ef;
        } else {
            lastField->next = ef;
        }
        lastField = ef;

        field = field->next;
    }

    return es;
}

static StructType createStructTypeFromExport(ExportedStruct *es) {
    StructType st = malloc(sizeof(struct StructType));
    if (!st) return NULL;

    char *nameCopy = strdup(es->name);
    st->nameStart = nameCopy;
    st->nameLength = strlen(es->name);
    st->size = es->size;
    st->fieldCount = es->fieldCount;
    st->fields = NULL;

    ExportedField *ef = es->fields;
    StructField lastField = NULL;

    while (ef) {
        StructField sf = malloc(sizeof(struct StructField));
        if (!sf) {
            ef = ef->next;
            continue;
        }

        // Duplicar nombre del campo
        char *fieldNameCopy = strdup(ef->name);
        sf->nameStart = fieldNameCopy;
        sf->nameLength = strlen(ef->name);
        sf->type = stringToDataType(ef->type);
        sf->offset = ef->offset;
        sf->next = NULL;

        if (!st->fields) {
            st->fields = sf;
        } else {
            lastField->next = sf;
        }
        lastField = sf;

        ef = ef->next;
    }

    return st;
}

ModuleInterface *extractExportsWithContext(ASTNode ast, const char *moduleName,
                                           TypeCheckContext ctx) {
    if (!ast || ast->nodeType != PROGRAM || !ctx) return NULL;

    ModuleInterface *iface = calloc(1, sizeof(ModuleInterface));
    if (!iface) return NULL;

    iface->moduleName = strdup(moduleName);
    iface->functions = NULL;
    iface->functionCount = 0;
    iface->structs = NULL;
    iface->structCount = 0;

    ASTNode stmt = ast->children;
    ExportedFunction *lastFunc = NULL;
    ExportedStruct *lastStruct = NULL;

    while (stmt) {
        if (stmt->nodeType == EXPORTDEC && stmt->children) {
            ASTNode child = stmt->children;

            if (child->nodeType == FUNCTION_DEFINITION) {
                ExportedFunction *ef = createExportedFunction(child, ctx);
                if (ef) {
                    if (!iface->functions) {
                        iface->functions = ef;
                    } else if (lastFunc) {
                        lastFunc->next = ef;
                    }
                    lastFunc = ef;
                    iface->functionCount++;
                }
            }
            else if (child->nodeType == STRUCT_DEFINITION) {
                ExportedStruct *es = createExportedStruct(child, ctx);
                if (es) {
                    if (!iface->structs) {
                        iface->structs = es;
                    } else if (lastStruct) {
                        lastStruct->next = es;
                    }
                    lastStruct = es;
                    iface->structCount++;
                }
            }
        }
        stmt = stmt->brothers;
    }

    return iface;
}

static int countPointerStars(const char **str) {
    int count = 0;
    while (**str == '*') {
        count++;
        (*str)++;
    }
    return count;
}

static FunctionParameter parseParamSignature(const char *sig) {
    if (!sig || !*sig) return NULL;

    FunctionParameter first = NULL;
    FunctionParameter last = NULL;

    const char *p = sig;
    while (*p) {
        // Skip whitespace and commas
        while (*p == ' ' || *p == ',') p++;
        if (!*p) break;

        // Count pointer level
        int ptrLevel = countPointerStars(&p);

        // Read type name
        const char *typeStart = p;
        while (*p && *p != ',' && *p != ' ') p++;

        char *typeName = strndup(typeStart, p - typeStart);
        DataType type = stringToDataType(typeName);
        free(typeName);

        // Create parameter (name is NULL for imported functions - we only need types)
        FunctionParameter param = createParameter(NULL, 0, type);
        if (param) {
            param->pointerLevel = ptrLevel;
            param->isPointer = (ptrLevel > 0);
            if (ptrLevel > 0) {
                param->type = TYPE_POINTER;
            }

            if (!first) {
                first = param;
            } else if (last) {
                last->next = param;
            }
            last = param;
        }
    }

    return first;
}

int addImportsToSymbolTable(SymbolTable table, ModuleInterface *iface) {
    if (!table || !iface) return 0;

    ExportedStruct *es = iface->structs;
    while (es) {
        Symbol existing = lookupSymbolCurrentOnly(table, es->name, strlen(es->name));
        if (!existing) {
            StructType structType = createStructTypeFromExport(es);
            if (structType) {
                Symbol sym = addSymbol(table, structType->nameStart, structType->nameLength,
                                       TYPE_STRUCT, 0, 0);
                if (sym) {
                    sym->symbolType = SYMBOL_TYPE;
                    sym->structType = structType;
                }
            }
        }
        es = es->next;
    }

    ExportedFunction *func = iface->functions;
    while (func) {
        const char *retTypeStr = func->returnType;
        int returnPtrLevel = countPointerStars(&retTypeStr);
        DataType returnType = stringToDataType(retTypeStr);

        FunctionParameter params = parseParamSignature(func->signature);

        int paramCount = 0;
        FunctionParameter p = params;
        while (p) {
            paramCount++;
            p = p->next;
        }

        Symbol funcSym =
            addFunctionSymbolFromString(table, func->name, returnType, params, paramCount, 0, 0);
        if (funcSym) {
            funcSym->returnPointerLevel = returnPtrLevel;
            funcSym->returnsPointer = (returnPtrLevel > 0);
            if (returnPtrLevel > 0) {
                funcSym->returnBaseType = returnType;
                funcSym->type = TYPE_POINTER;
            }
        }

        func = func->next;
    }

    return 1;
}

static void freeExportedFields(ExportedField *field) {
    while (field) {
        ExportedField *next = field->next;
        free(field->name);
        free(field->type);
        free(field);
        field = next;
    }
}

static void freeExportedStructs(ExportedStruct *es) {
    while (es) {
        ExportedStruct *next = es->next;
        free(es->name);
        freeExportedFields(es->fields);
        free(es);
        es = next;
    }
}

void freeModuleInterface(ModuleInterface *iface) {
    if (!iface) return;

    free(iface->moduleName);

    ExportedFunction *func = iface->functions;
    while (func) {
        ExportedFunction *next = func->next;
        free(func->name);
        free(func->signature);
        free(func->returnType);
        free(func);
        func = next;
    }
    freeExportedStructs(iface->structs);

    free(iface);
}