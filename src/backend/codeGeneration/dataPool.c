#include <stdlib.h>
#include <string.h>
#include "codegen.h"
#include "emiter.h"

StringEntry *findStringLit(CodeGenContext *ctx, const char *str, size_t len){
    StringEntry *entry = ctx->stringPool;
    while(entry){
        if(entry->len == len && memcmp(entry->str, str, len) == 0){
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}

int addStringLit(CodeGenContext *ctx, const char *str, size_t len){
    if (len >= 2 && str[0] == '"' && str[len-1] == '"') {
        str++;
        len -= 2;
    }
    StringEntry *exists = findStringLit(ctx, str, len);
    if(exists) return exists->labelNum;

    StringEntry *entry = malloc(sizeof(struct StringEntry));
    if(!entry) return -1;

    entry->str = str;
    entry->len = len;
    entry->labelNum = ctx->nextLab++;
    entry->next = ctx->stringPool;
    ctx->stringPool = entry;

    emitDataLabel(ctx, entry->labelNum);
    sbAppendf(&ctx->data, "    .string \"");
    for (size_t i = 0; i < len; i++) {
        char c = str[i];
        if (c == '"')
            sbAppend(&ctx->data, "\\\"");
        else if (c == '\\' && i + 1 < len) {
            char next = str[i + 1];
            if (next == 'n' || next == 't' || next == 'r' || next == '\\' || next == '"' ||
                next == '0') {
                sbAppendChar(&ctx->data, '\\');
                sbAppendChar(&ctx->data, next);
                i++;
            } else {
                sbAppend(&ctx->data, "\\\\");
            }
        } else if (c == '\n')
            sbAppend(&ctx->data, "\\n");
        else
            sbAppendChar(&ctx->data, c);
    }
    sbAppend(&ctx->data, "\"\n");

    return entry->labelNum;
}

int addDoubleLit(CodeGenContext *ctx, double d){
    DoubleEntry *exists = ctx->doublePool;
    while(exists){
        if(exists->d == d){
            return exists->label;
        }
        exists = exists->next;
    }

    DoubleEntry *newEntry = malloc(sizeof(struct DoubleEntry));
    newEntry->d = d;
    newEntry->label = ctx->nextLab++;
    newEntry->next = ctx->doublePool;
    ctx->doublePool = newEntry;

    emitDataLabel(ctx, newEntry->label);
    sbAppendf(&ctx->data, "    .double %.17g\n", d);
    return newEntry->label;
}

int addFloatLit(CodeGenContext *ctx, float f){
    FloatEntry *exists = ctx->floatPool;
    while(exists){
        if(exists->f == f){
            return exists->label;
        }
        exists = exists->next;
    }

    FloatEntry *newEntry = malloc(sizeof(struct FloatEntry));
    newEntry->f = f;
    newEntry->label = ctx->nextLab++;
    newEntry->next = ctx->floatPool;
    ctx->floatPool = newEntry;

    emitDataLabel(ctx, newEntry->label);
    sbAppendf(&ctx->data, "    .float %.9g\n", f);
    return newEntry->label;
}
