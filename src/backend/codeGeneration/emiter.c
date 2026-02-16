#include <stdarg.h>
#include <stdio.h>
#include "codegen.h"

void emitASMLabel(CodeGenContext *ctx, const char *label){
    sbAppendf(&ctx->text, "%s:\n", label);
}

void emitDataLabel(CodeGenContext *ctx, int label){
    sbAppendf(&ctx->data, ".LC%d:\n", label);
}

void emitLabelNum(CodeGenContext *ctx, int labelNum){
    sbAppendf(&ctx->text, ".L%d:\n", labelNum);
}

void emitInstruction(CodeGenContext *ctx, const char *fmt, ...){
    sbAppend(&ctx->text, "    ");
    va_list args;
    va_start(args, fmt);
    va_list argsCopy;
    va_copy(argsCopy, args);
    int needed = vsnprintf(NULL, 0, fmt, argsCopy);
    va_end(argsCopy);

    if(needed > 0){
        sbEnsureCapacity(&ctx->text, (size_t)needed);
        vsnprintf(ctx->text.data + ctx->text.len, ctx->text.cap - ctx->text.len, fmt, args);
        ctx->text.len += needed;
    }

    va_end(args);
    sbAppend(&ctx->text, "\n");
}

void emitComment(CodeGenContext *ctx, const char *comment) {
    sbAppendf(&ctx->text, "    # %s\n", comment);
}

void emitBlankLine(CodeGenContext *ctx) {
    sbAppend(&ctx->text, "\n");
}