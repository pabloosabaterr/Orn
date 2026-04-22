/**
 * @file dataPool.h
 * @brief Constant-data pool types and API for code generation.
 *
 * Exposes StringEntry, DoubleEntry, FloatEntry structures and
 * functions for interning string, double, and float literals
 * into the .data section during assembly generation.
 */

#ifndef DATA_POOL_H
#define DATA_POOL_H

#include <stddef.h>

typedef struct CodeGenContext CodeGenContext;


typedef struct StringEntry {
    const char *str;
    size_t len;
    int labelNum;
    struct StringEntry *next;
} StringEntry;

typedef struct DoubleEntry {
    double d;
    int label;
    struct DoubleEntry *next;
} DoubleEntry;

typedef struct FloatEntry {
    double f;
    int label;
    struct FloatEntry *next;
} FloatEntry;

int addStringLit(CodeGenContext *ctx, const char *str, size_t len);
int addDoubleLit(CodeGenContext *ctx, double d);
int addFloatLit(CodeGenContext *ctx, float f);
StringEntry *findStringLit(CodeGenContext *ctx, const char *str, size_t len);

#endif