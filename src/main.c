#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "modules/build.h"

void printUsage(const char* programName) {
    printf("Orn Lang Compiler\n\n");
    printf("OPTIONS:\n");
    printf("    -o <file>    Write output to <file>\n");
    printf("    --verbose    Show build steps\n");
    printf("    --ir         Show intermediate representation for all modules\n");
    printf("    --ast        Show AST for all modules\n");
    printf("    -O0          No optimization (default)\n");
    printf("    -O1          Basic optimization (3 passes)\n");
    printf("    -O2          Moderate optimization (5 passes)\n");
    printf("    -O3          Aggressive optimization (10 passes)\n");
    printf("    -Ox          Extremely aggressive optimizations (30 passes)\n");
    printf("    --help       Show this help message\n\n");
    printf("EXAMPLES:\n");
    printf("    %s program.orn                   Compile to ./program\n", programName);
    printf("    %s --ast program.orn             Show AST for all modules\n", programName);
    printf("    %s -O2 -o myapp program.orn      Optimize and output to myapp\n", programName);
}

int main(int argc, char* argv[]) {
    const char* inputFile = NULL;
    const char* outputFile = NULL;
    int verbose = 0;
    int showAST = 0;
    int showIR = 0;
    int optLvl = 0;

    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            printUsage(argv[0]);
            return 0;
        }
        else if (strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        }
        else if (strcmp(argv[i], "--ast") == 0) {
            showAST = 1;
        }
        else if (strcmp(argv[i], "--ir") == 0) {
            showIR = 1;
        }
        else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc) {
                outputFile = argv[++i];
            } else {
                fprintf(stderr, "Error: -o requires an argument\n");
                return 1;
            }
        }
        else if (strncmp(argv[i], "-O", 2) == 0) {
            char level = argv[i][2];
            if (level == 'x') // -Ox
                optLvl = 4;
            else if (level >= '0' && level <= '3')
                optLvl = level - '0';
            else {
                fprintf(stderr, "Invalid optimization level: %s (use -O0 to -O3)\n", argv[i]);
                return 1;
            }
        }
        else if (argv[i][0] != '-') {
            inputFile = argv[i];
        }
        else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            return 1;
        }
    }

    if (!inputFile) {
        fprintf(stderr, "Error: No input file specified\n");
        printUsage(argv[0]);
        return 1;
    }

    // Determine output filename
    char exeFile[256];
    if (outputFile) {
        snprintf(exeFile, sizeof(exeFile), "%s", outputFile);
    } else {
        const char *dot = strrchr(inputFile, '.');
        const char *slash = strrchr(inputFile, '/');
        const char *baseName = slash ? slash + 1 : inputFile;
        size_t baseLen = dot && dot > baseName ? (size_t)(dot - baseName) : strlen(baseName);
        snprintf(exeFile, sizeof(exeFile), "%.*s", (int)baseLen, baseName);
    }

    // Build project
    if (!buildProject(inputFile, exeFile, optLvl, verbose, showAST, showIR)) {
        return 1;
    }

    if (!verbose && !showAST && !showIR) {
        printf("Compiled '%s' -> '%s'\n", inputFile, exeFile);
    }

    return 0;
}
