/**
 * @file semanticBuiltins.c
 * @brief Built-in type and function registration.
 *
 * Responsibilities:
 *   - Static built-in function table (defined ONLY here)
 *   - Built-in parameter initialization
 *   - Populating the initial global symbol table with built-ins
 *   - Overload resolution for built-in functions
 *   - Built-in function name checking
 *
 * All built-in data tables are static and defined ONLY in this file.
 */

#include "semanticInternal.h"

static BuiltInFunction builtInFunctions[] = {
    {
        .name = "syscall",
        .returnType = TYPE_I64,
        .paramTypes = NULL,
        .paramNames = NULL,
        .paramCount = 7,
        .id = BUILTIN_SYSCALL

    }
};

static int builtInFnCount = sizeof(builtInFunctions) / sizeof(BuiltInFunction);
static int builtInsInit = 0;

static void initBuiltInsParams(void) {
    if (builtInsInit) return;

    builtInFunctions[0].paramTypes = malloc(sizeof(DataType) * 7);
    builtInFunctions[0].paramTypes[0] = TYPE_I64;
    builtInFunctions[0].paramTypes[1] = TYPE_I64;
    builtInFunctions[0].paramTypes[2] = TYPE_I64;
    builtInFunctions[0].paramTypes[3] = TYPE_I64;
    builtInFunctions[0].paramTypes[4] = TYPE_I64;
    builtInFunctions[0].paramTypes[5] = TYPE_I64;
    builtInFunctions[0].paramTypes[6] = TYPE_I64;

    builtInFunctions[0].paramNames = malloc(sizeof(char *) * 7);
    builtInFunctions[0].paramNames[0] = strdup("a");
    builtInFunctions[0].paramNames[1] = strdup("b");
    builtInFunctions[0].paramNames[2] = strdup("c");
    builtInFunctions[0].paramNames[3] = strdup("d");
    builtInFunctions[0].paramNames[4] = strdup("e");
    builtInFunctions[0].paramNames[5] = strdup("f");
    builtInFunctions[0].paramNames[6] = strdup("g");

    builtInsInit = 1;
}

static FunctionParameter createParameterList(char **names, DataType *types, int count) {
    if (count == 0) return NULL;

    FunctionParameter first = NULL;
    FunctionParameter last = NULL;

    for (int i = 0; i < count; i++) {
        FunctionParameter param = createParameter(names[i], strlen(names[i]), types[i]);
        if (param == NULL) {
            freeParamList(first);
            return NULL;
        }

        if (first == NULL) {
            first = param;
        } else {
            last->next = param;
        }
        last = param;
    }

    return first;
}

/* Public */

void initBuiltIns(SymbolTable globTable) {
    if (globTable == NULL) return;

    initBuiltInsParams();

    for (int i = 0; i < builtInFnCount; i++) {
        BuiltInFunction *builtin = &builtInFunctions[i];

        FunctionParameter params = createParameterList(
            builtin->paramNames,
            builtin->paramTypes,
            builtin->paramCount
        );
        addFunctionSymbolFromString(
            globTable,
            builtin->name,
            builtin->returnType,
            params,
            builtin->paramCount,
            0, 0
        );
    }
}

BuiltInId resolveOverload(const char *nameStart, size_t nameLength, DataType arg[], int argCount) {
    if (nameStart == NULL || nameLength == 0) return BUILTIN_UNKNOWN;

    for (int i = 0; i < builtInFnCount; i++) {
        BuiltInFunction *builtin = &builtInFunctions[i];

        size_t builtinNameLen = strlen(builtin->name);
        if (nameLength != builtinNameLen ||
            memcmp(nameStart, builtin->name, nameLength) != 0)
            continue;

        if (builtin->paramCount != argCount) continue;

        // syscall accepts any types - kernel sees raw 64-bit values
        if (builtin->id == BUILTIN_SYSCALL) return builtin->id;

        int typesMatch = 1;
        for (int j = 0; j < argCount; j++) {
            if (!areCompatible(builtin->paramTypes[j], arg[j])) {
                typesMatch = 0;
                break;
            }
        }
        if (typesMatch) return builtin->id;
    }
    return BUILTIN_UNKNOWN;
}

int isBuiltinFunction(const char *nameStart, size_t nameLength) {
    if (nameStart == NULL || nameLength == 0) return 0;

    for (int i = 0; i < builtInFnCount; i++) {
        size_t builtinNameLen = strlen(builtInFunctions[i].name);
        if (nameLength == builtinNameLen &&
            memcmp(nameStart, builtInFunctions[i].name, nameLength) == 0) {
            return 1;
        }
    }
    return 0;
}