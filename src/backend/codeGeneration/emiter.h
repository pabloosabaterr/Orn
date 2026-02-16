#ifndef EMITER_H
#define EMITER_H

// Forward declaration - full definition in codegen.h
typedef struct CodeGenContext CodeGenContext;

void emitASMLabel(CodeGenContext *ctx, const char *label);
void emitDataLabel(CodeGenContext *ctx, int label);
void emitLabelNum(CodeGenContext *ctx, int labelNum);
void emitInstruction(CodeGenContext *ctx, const char *fmt, ...);
void emitComment(CodeGenContext *ctx, const char *comment);
void emitBlankLine(CodeGenContext *ctx);

#endif