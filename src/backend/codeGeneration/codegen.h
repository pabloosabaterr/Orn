#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "stringBuffer.h"
#include "dataPool.h"
#include "ir.h"
#include "variableHandling.h"
#include "interface.h"

typedef struct FuncInfo {
    const char *name;
    size_t nameLen;
    int stackSize;
    int paramCount;
    VarLoc *locs;
    TempLoc *temps;
} FuncInfo;

typedef struct CodeGenContext {
    StringBuffer data;
    StringBuffer text;

    StringEntry *stringPool;
    DoubleEntry *doublePool;
    FloatEntry *floatPool;
    int nextLab;

    VarLoc *globalVars;
    TempLoc *globalTemps;
    int globalStackOff;

    FuncInfo *currentFn;
    int inFn;
    
    int maxTempNum;
    IrDataType lastParamType;

    const char *moduleName;
    ModuleInterface **imports;
    int importCount;
} CodeGenContext;

CodeGenContext *createCodeGenContext(void);
void freeCodeGenContext(CodeGenContext *ctx);

CodeGenContext *createCodeGenContext(void);
void freeCodeGenContext(CodeGenContext *ctx);

void loadOp(CodeGenContext *ctx, IrOperand *op, const char *reg);
void storeOp(CodeGenContext *ctx, const char *reg, IrOperand *op);
void genBinaryOp(CodeGenContext *ctx, IrInstruction *inst);
void genUnaryOp(CodeGenContext *ctx, IrInstruction *inst);
void genCopy(CodeGenContext *ctx, IrInstruction *inst);
void genGoto(CodeGenContext *ctx, IrInstruction *inst);
void genIfFalse(CodeGenContext *ctx, IrInstruction *inst);
void genReturn(CodeGenContext *ctx, IrInstruction *inst);
void genParam(CodeGenContext *ctx, IrInstruction *inst, int paramNum);
void genCall(CodeGenContext *ctx, IrInstruction *inst);
void genFuncBegin(CodeGenContext *ctx, IrInstruction *inst);
void genFuncEnd(CodeGenContext *ctx, IrInstruction *inst);
void genComparison(CodeGenContext *ctx, IrInstruction *inst);
void genLogical(CodeGenContext *ctx, IrInstruction *inst);
void genCast(CodeGenContext *ctx, IrInstruction *inst);
void generateInstruction(CodeGenContext *ctx, IrInstruction *inst, int *paramCount);

char *generateAssembly(IrContext *ir, const char *moduleName, ModuleInterface **imports, int importCount);
int writeAssemblyToFile(const char *assembly, const char *filename);

#endif