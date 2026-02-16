#include "semanticInternal.h"

static BuiltInFunction builtInFunctions[] = {
    {
        .name = "print",
        .returnType = TYPE_VOID,
        .paramTypes = NULL,
        .paramNames = NULL,
        .paramCount = 1,
        .id = BUILTIN_PRINT_INT
    },
    {
        .name = "print",
        .returnType = TYPE_VOID,
        .paramTypes = NULL,
        .paramNames = NULL,
        .paramCount = 1,
        .id = BUILTIN_PRINT_STRING
    },
    {
        .name = "print",
        .returnType = TYPE_VOID,
        .paramTypes = NULL,
        .paramNames = NULL,
        .paramCount = 1,
        .id = BUILTIN_PRINT_FLOAT
    },
    {
        .name = "print",
        .returnType = TYPE_VOID,
        .paramTypes = NULL,
        .paramNames = NULL,
        .paramCount = 1,
        .id = BUILTIN_PRINT_BOOL
    },
    {
        .name = "print",
        .returnType = TYPE_VOID,
        .paramTypes = NULL,
        .paramNames = NULL,
        .paramCount = 1,
        .id = BUILTIN_PRINT_DOUBLE
    },
};

static int builtInFnCount = sizeof(builtInFunctions) / sizeof(BuiltInFunction);
static int builtInsInit = 0;

static void initBuiltInsParams(void) {
    if (builtInsInit) return;

    builtInFunctions[0].paramTypes = malloc(sizeof(DataType));
    builtInFunctions[0].paramTypes[0] = TYPE_INT;
    builtInFunctions[0].paramNames = malloc(sizeof(char *));
    builtInFunctions[0].paramNames[0] = strdup("value");

    builtInFunctions[1].paramTypes = malloc(sizeof(DataType));
    builtInFunctions[1].paramTypes[0] = TYPE_STRING;
    builtInFunctions[1].paramNames = malloc(sizeof(char *));
    builtInFunctions[1].paramNames[0] = strdup("value");

    builtInFunctions[2].paramTypes = malloc(sizeof(DataType));
    builtInFunctions[2].paramTypes[0] = TYPE_FLOAT;
    builtInFunctions[2].paramNames = malloc(sizeof(char *));
    builtInFunctions[2].paramNames[0] = strdup("value");

    builtInFunctions[3].paramTypes = malloc(sizeof(DataType));
    builtInFunctions[3].paramTypes[0] = TYPE_BOOL;
    builtInFunctions[3].paramNames = malloc(sizeof(char *));
    builtInFunctions[3].paramNames[0] = strdup("value");

    builtInFunctions[4].paramTypes = malloc(sizeof(DataType));
    builtInFunctions[4].paramTypes[0] = TYPE_DOUBLE;
    builtInFunctions[4].paramNames = malloc(sizeof(char *));
    builtInFunctions[4].paramNames[0] = strdup("value");

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