#include "build.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "lexer.h"
#include "codegen.h"
#include "optimization.h"

static char *readFile(const char *fileName){
    FILE *file = fopen(fileName, "r");
    if(!file){
        fprintf(stderr, "Error: Cannot open file '%s'\n", fileName);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *content = malloc(fileSize + 1);
    if(!content){
        fprintf(stderr, "Error: Cannot allocate memory for file '%s'\n", fileName);
        fclose(file);
        return NULL;
    }

    size_t bytesRead = fread(content, 1, fileSize, file);
    content[bytesRead] = '\0';
    fclose(file);
    return content;
}

static char *extractModuleName(const char *path){
    const char *base = strrchr(path, '/');
    base = base ? base + 1 : path;
    const char *dot = strrchr(base, '.');
    size_t len = dot ? (size_t)(dot - base) : strlen(base);

    char *name = malloc(len + 1);
    if(name){
        memcpy(name, base, len);
        name[len] = '\0';
    };
    return name;
}


static char *extractBasePath(const char *path){
    const char *lastSlash = strrchr(path, '/');
    if(!lastSlash){
        return strdup(".");
    }
    size_t len = (size_t)(lastSlash - path);
    char *basePath = malloc(len + 1);
    if(basePath){
        memcpy(basePath, path, len);
        basePath[len] = '\0';
    }
    return basePath;
}

char **extractImports(ASTNode ast, int *count){
    *count = 0;
    if(!ast || ast->nodeType != PROGRAM) return NULL;
    int importCount = 0;
    ASTNode stmt = ast->children;
    while(stmt){
        if(stmt->nodeType == IMPORTDEC){
            importCount++;
        }
        stmt = stmt->brothers;
    }

    if(importCount == 0) return NULL;

    char**imports = malloc(sizeof(char*) * importCount);
    if(!imports) return NULL;

    stmt = ast->children;
    int idx = 0;
    while(stmt){
        if(stmt->nodeType == IMPORTDEC){
            const char *start = stmt->start;
            size_t length = stmt->length;

            if(length>=2&&start[0]=='"'&&start[length-1]=='"'){
                start++;
                length-=2;
            }

            imports[idx] = strndup(start, length);
            idx++;
        }
        stmt = stmt->brothers;
    }
    *count = importCount;
    return imports;
}

char *resolveModulePath(const char *basePath, const char *moduleName){
    size_t len = strlen(basePath) + 1 + strlen(moduleName) + 4 + 1;
    char *rawPath = malloc(len);
    if(!rawPath) return NULL;
    
    snprintf(rawPath, len, "%s/%s.orn", basePath, moduleName);

    char* resolved = realpath(rawPath, NULL);
    free(rawPath);
    return resolved;
}

Module *findModule(BuildContext *ctx, const char *path){
    for(int i=0;i<ctx->moduleCount; ++i){
        if(strcmp(ctx->modules[i].path, path)==0){
            return &ctx->modules[i];
        }
    }
    return NULL;
}

static Module *addModule(BuildContext *ctx, const char *name, const char *path){
    Module *existing = findModule(ctx, path);
    if(existing) return existing;
    if(ctx->moduleCount >= ctx->moduleCapacity){
        int newCap = ctx->moduleCapacity == 0 ? 8 : ctx->moduleCapacity * 2;
        Module *newMods = realloc(ctx->modules, sizeof(Module) * newCap);
        if(!newMods) return NULL;
        ctx->modules = newMods;
        ctx->moduleCapacity = newCap;
    }

    Module *mod = &ctx->modules[ctx->moduleCount++];
    memset(mod, 0, sizeof(Module));
    mod->name = strdup(name);
    mod->path = strdup(path);
    mod->imports = NULL;
    mod->importCount = 0;
    mod->importCapacity = 0;
    mod->interface = NULL;
    return mod;
}

static int findModulesRec(BuildContext *ctx, const char *path){
    char *name = extractModuleName(path);
    if(!name) return 0;

    char* resPath = realpath(path, NULL);
    if(!resPath){
        fprintf(stderr, "Error: Cannot resolve path '%s'\n", path);
        free(name);
        return 0;
    }
    
    if(findModule(ctx, resPath)){
        free(name);
        free(resPath);
        return 1;
    }

    char *source = readFile(path);
    if (!source) {
        fprintf(stderr, "Error: Cannot read module '%s' at '%s'\n", name, path);
        free(name);
        free(resPath);
        return 0;
    }

    TokenList *tokens = lex(source, resPath);
    if (!tokens) {
        fprintf(stderr, "Error: Failed to lex module '%s'\n", name);
        free(source);
        free(name);
        free(resPath);
        return 0;
    }

    ASTContext *ast = ASTGenerator(tokens);
    if (!ast || !ast->root) {
        fprintf(stderr, "Error: Failed to parse module '%s'\n", name);
        freeTokens(tokens);
        free(source);
        free(name);
        free(resPath);
        return 0;
    }

    Module *mod = addModule(ctx, name, resPath);
    if(!mod){
        fprintf(stderr, "Error: Failed to add module '%s'\n", name);
        freeASTContext(ast);
        freeTokens(tokens);
        free(source);
        free(name);
        free(resPath);
        return 0;
    }

    int importCount;
    char **imports = extractImports(ast->root, &importCount);

    char* basePath = extractBasePath(resPath);
    
    for(int i = 0; i<importCount; ++i){
        if(mod->importCount >= mod->importCapacity){
            int newCap = mod->importCapacity == 0 ? 4 : mod->importCapacity * 2;
            char **newImports = realloc(mod->imports, sizeof(char*) * newCap);
            if(!newImports){
                fprintf(stderr, "Error: Failed to allocate imports for module '%s'\n", name);
                freeASTContext(ast);
                freeTokens(tokens);
                free(source);
                free(name);
                free(resPath);
                free(basePath);
                return 0;
            }

            mod->imports = newImports;
            mod->importCapacity = newCap;
        }

        char *importPath = resolveModulePath(basePath, imports[i]);
        if(!importPath){
            fprintf(stderr, "Error: Failed to resolve import '%s' for module '%s'\n", imports[i], name);
            free(imports);
            freeASTContext(ast);
            freeTokens(tokens);
            free(source);
            free(name);
            free(resPath);
            free(basePath);
            return 0;
        }
        mod->imports[mod->importCount++] = strdup(importPath);  // store resolved path
        free(imports[i]);

        if(!findModulesRec(ctx, importPath)){
            fprintf(stderr, "Error: Failed to process import '%s' for module '%s'\n", imports[i], name);
            free(importPath);
            free(imports);
            freeASTContext(ast);
            freeTokens(tokens);
            free(source);
            free(name);
            free(resPath);
            free(basePath);
            return 0;
        }
        free(importPath);
    }

    free(basePath);
    free(imports);
    freeASTContext(ast);
    freeTokens(tokens);
    free(source);
    free(name);
    return 1;
}

int findModules(BuildContext *ctx, const char *entryPath){
    ctx->modules = NULL;
    ctx->moduleCount = 0;
    ctx->moduleCapacity = 0;
    ctx->basePath = extractBasePath(entryPath);
    return findModulesRec(ctx, entryPath);
}

int *topoSortModules(BuildContext *ctx, int *outCount) {
    int n = ctx->moduleCount;
    int *inDegree = calloc(n, sizeof(int));
    int *result = malloc(n * sizeof(int));
    int *queue = malloc(n * sizeof(int));
    int qHead = 0, qTail = 0;
    int resultCount = 0;
    
    // Actually: in-degree[i] = number of modules that i depends on
    // We want modules with no dependencies first
    for (int i = 0; i < n; i++) {
        Module *mod = &ctx->modules[i];
        inDegree[i] = mod->importCount;
        
        // But only count imports that are actual modules we're building
        inDegree[i] = 0;
        for (int j = 0; j < mod->importCount; j++) {
            if (findModule(ctx, mod->imports[j])) {
                inDegree[i]++;
            }
        }
        
        if (inDegree[i] == 0) {
            queue[qTail++] = i;
        }
    }
    
    // Process queue
    while (qHead < qTail) {
        int curr = queue[qHead++];
        result[resultCount++] = curr;
        
        // For each module that imports this one, decrease its in-degree
        for (int i = 0; i < n; i++) {
            Module *mod = &ctx->modules[i];
            for (int j = 0; j < mod->importCount; j++) {
                if (strcmp(mod->imports[j], ctx->modules[curr].path) == 0) {
                    inDegree[i]--;
                    if (inDegree[i] == 0) {
                        queue[qTail++] = i;
                    }
                }
            }
        }
    }
    
    free(inDegree);
    free(queue);
    
    // Check for cycle
    if (resultCount != n) {
        fprintf(stderr, "Error: Circular dependency detected\n");
        free(result);
        *outCount = 0;
        return NULL;
    }
    
    *outCount = resultCount;
    return result;
}

static int compileModule(BuildContext *ctx, Module *mod, int optLevel, 
                        int verbose, int showAST, int showIR) {
    if (verbose) {
        printf("  Compiling %s...\n", mod->name);
    }
    
    // Read source
    char *source = readFile(mod->path);
    if (!source) {
        fprintf(stderr, "Error: Cannot read '%s'\n", mod->path);
        return 0;
    }

    if (showAST || showIR) {
        printf("\n=== MODULE: %s ===\n", mod->name);
        printf("Source: %s\n", mod->path);
    }
    
    // Lex
    TokenList *tokens = lex(source, mod->path);
    if (!tokens) {
        free(source);
        return 0;
    }
    
    // Parse
    ASTContext *ast = ASTGenerator(tokens);
    if (!ast || !ast->root) {
        freeTokens(tokens);
        free(source);
        return 0;
    }

    if (showAST) {
        printf("\n--- AST: %s ---\n", mod->name);
        printAST(ast->root, 0);
        printf("\n");
    }
    
    // Create type check context
    TypeCheckContext typeCtx = createTypeCheckContext(source, mod->path);
    if (!typeCtx) {
        freeASTContext(ast);
        freeTokens(tokens);
        free(source);
        return 0;
    }
    
    // Load imports into symbol table
    for (int i = 0; i < mod->importCount; i++) {
        Module *imported = findModule(ctx, mod->imports[i]);
        if (imported && imported->interface) {
            addImportsToSymbolTable(typeCtx->global, imported->interface);
        }
    }
    // Type check
    typeCheckAST(ast->root, source, mod->path, typeCtx);
    // Extract exports for dependents
    mod->interface = extractExportsWithContext(ast->root, mod->name, typeCtx);
    // Generate IR
    IrContext *ir = generateIr(ast->root, typeCtx);
    if (!ir) {
        freeTypeCheckContext(typeCtx);
        freeASTContext(ast);
        freeTokens(tokens);
        free(source);
        return 0;
    }
    
    // Optimize
    if (optLevel > 0) {
        optimizeIR(ir, optLevel);
    }

    if (showIR) {
        printf("\n--- IR: %s ---\n", mod->name);
        printIR(ir);
        printf("\n");
    }

    // Build array of imported interfaces for codegen
    ModuleInterface **imports = NULL;
    int importCount = 0;

    if (mod->importCount > 0) {
        imports = malloc(mod->importCount * sizeof(ModuleInterface*));
        if (imports) {
            for (int i = 0; i < mod->importCount; i++) {
                Module *imported = findModule(ctx, mod->imports[i]);
                if (imported && imported->interface) {
                    imports[importCount++] = imported->interface;
                }
            }
        }
    }
    
    // Generate assembly
    char *assembly = generateAssembly(ir, mod->name, imports, importCount);
    free(imports);

    if (!assembly) {
        freeIrContext(ir);
        freeTypeCheckContext(typeCtx);
        freeASTContext(ast);
        freeTokens(tokens);
        free(source);
        return 0;
    }

    /* for debug */
    // printf("%s\n", assembly);
    
    // Write assembly file
    char asmPath[512];
    snprintf(asmPath, sizeof(asmPath), "%s/%s.s", ctx->basePath, mod->name);
    if (!writeAssemblyToFile(assembly, asmPath)) {
        free(assembly);
        freeIrContext(ir);
        freeTypeCheckContext(typeCtx);
        freeASTContext(ast);
        freeTokens(tokens);
        free(source);
        return 0;
    }
    
    // Assemble to .o
    char objPath[512];
    char cmd[2048];
    snprintf(objPath, sizeof(objPath), "%s/%s.o", ctx->basePath, mod->name);
    snprintf(cmd, sizeof(cmd), "gcc -c -o %s %s 2>&1", objPath, asmPath);
    
    int result = system(cmd);
    if (result != 0) {
        fprintf(stderr, "Error: Failed to assemble '%s'\n", asmPath);
        free(assembly);
        freeIrContext(ir);
        freeTypeCheckContext(typeCtx);
        freeASTContext(ast);
        freeTokens(tokens);
        free(source);
        return 0;
    }
    
    // Cleanup assembly file
    remove(asmPath);
    
    // Cleanup
    free(assembly);
    freeIrContext(ir);
    freeTypeCheckContext(typeCtx);
    freeASTContext(ast);
    freeTokens(tokens);
    free(source);
    
    return 1;
}

static int linkModules(BuildContext *ctx, const char *outputPath, int verbose) {
    if (verbose) {
        printf("  Linking...\n");
    }
    char cmd[4096];
    int pos = snprintf(cmd, sizeof(cmd), "gcc -no-pie -nostdlib -o %s", outputPath);
    
    for (int i = 0; i < ctx->moduleCount; i++) {
        pos += snprintf(cmd + pos, sizeof(cmd) - pos, " %s/%s.o", 
                        ctx->basePath, ctx->modules[i].name);
    }
    
    pos += snprintf(cmd + pos, sizeof(cmd) - pos, " ./runtime.s 2>&1");
    
    int result = system(cmd);
    
    // Cleanup .o files
    for (int i = 0; i < ctx->moduleCount; i++) {
        char objPath[512];
        snprintf(objPath, sizeof(objPath), "%s/%s.o", ctx->basePath, ctx->modules[i].name);
        remove(objPath);
    }
    
    return result == 0;
}

int buildProject(const char *entryPath, const char *outputPath, int optLevel, 
                 int verbose, int showAST, int showIR) {
    BuildContext ctx = {0};
    
    if (verbose || showAST || showIR) {
        printf("=== BUILD ===\n");
        printf("Entry: %s\n", entryPath);
        printf("Optimization: -O%d\n", optLevel);
    }
    
    // 1. Discover all modules
    if (verbose) printf("Discovering modules...\n");
    if (!findModules(&ctx, entryPath)) {
        freeBuildContext(&ctx);
        return 0;
    }
    
    if (verbose) {
        printf("Found %d module(s):\n", ctx.moduleCount);
        for (int i = 0; i < ctx.moduleCount; i++) {
            printf("  - %s (%s)\n", ctx.modules[i].name, ctx.modules[i].path);
        }
    }
    
    // 2. Topological sort
    int sortedCount;
    int *sorted = topoSortModules(&ctx, &sortedCount);
    if (!sorted) {
        freeBuildContext(&ctx);
        return 0;
    }
    
    if (verbose) {
        printf("Compilation order: ");
        for (int i = 0; i < sortedCount; i++) {
            printf("%s ", ctx.modules[sorted[i]].name);
        }
        printf("\n");
    }
    
    // 3. Compile each module in order
    if (verbose) printf("Compiling...\n");
    for (int i = 0; i < sortedCount; i++) {
        Module *mod = &ctx.modules[sorted[i]];
        if (!compileModule(&ctx, mod, optLevel, verbose, showAST, showIR)) {
            fprintf(stderr, "Error: Failed to compile module '%s'\n", mod->name);
            free(sorted);
            freeBuildContext(&ctx);
            return 0;
        }
    }
    
    free(sorted);
    
    // 4. Link
    if (verbose) printf("Linking...\n");
    if (!linkModules(&ctx, outputPath, verbose)) {
        fprintf(stderr, "Error: Linking failed\n");
        freeBuildContext(&ctx);
        return 0;
    }
    
    if (verbose || showAST || showIR) {
        printf("\n=== BUILD SUCCESSFUL ===\n");
        printf("Output: %s\n", outputPath);
    }
    
    freeBuildContext(&ctx);
    return 1;
}

void freeBuildContext(BuildContext *ctx) {
    for (int i = 0; i < ctx->moduleCount; i++) {
        Module *mod = &ctx->modules[i];
        free(mod->name);
        free(mod->path);
        for (int j = 0; j < mod->importCount; j++) {
            free(mod->imports[j]);
        }
        free(mod->imports);
        if (mod->interface) {
            freeModuleInterface(mod->interface);
        }
    }
    free(ctx->modules);
    free(ctx->basePath);
}