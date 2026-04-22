/**
 * @file build.h
 * @brief Multi-module build system API.
 *
 * Exposes Module, BuildContext structures and functions for
 * discovering, sorting, compiling, and linking Orn modules.
 * Include from main.c or any driver that orchestrates builds.
 */

#ifndef BUILD_H
#define BUILD_H

#include "interface.h"

typedef struct Module {
    char *name;
    char *path;
    char **imports;
    int importCount;
    int importCapacity;
    ModuleInterface *interface;
} Module;

typedef struct BuildContext {
    Module *modules;
    int moduleCount;
    int moduleCapacity;
    char *basePath;
} BuildContext;

char **extractImports(ASTNode ast, int *count);

/**
 * @brief Resolve module path from import name
 * "math" -> "/path/to/math.orn"
 */
char *resolveModulePath(const char *basePath, const char *moduleName);

/**
 * @brief Discover all modules starting from entry file
 */
int findModules(BuildContext *ctx, const char *entryPath);

/**
 * @brief Topological sort modules (dependencies first)
 * Returns array of module indices in compilation order
 */
int *topoSortModules(BuildContext *ctx, int *outCount);

/**
 * @brief Build entire project from entry file
 */
int buildProject(const char *entryPath, const char *outputPath, int optLevel, int verbose,int showAST, int showIR);

/**
 * @brief Find module by name
 */
Module *findModule(BuildContext *ctx, const char *name);

/**
 * @brief Free build context
 */
void freeBuildContext(BuildContext *ctx);

#endif //BUILD_H

