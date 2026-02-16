#ifndef VARIABLE_HANDLING_H
#define VARIABLE_HANDLING_H

#include <stddef.h>

typedef struct CodeGenContext CodeGenContext;

typedef struct VarLoc {
    const char *name;
    size_t nameLen;
    int stackOffset;
    IrDataType type;
    struct VarLoc *next;
    int isAddresable;
    int arraySize;
} VarLoc;

typedef struct TempLoc {
    int tempNum;
    int stackOff;
    IrDataType type;
    struct TempLoc *next;
} TempLoc;

int getTempOffset(CodeGenContext *ctx, int tempNum, IrDataType type);
void addTemp(CodeGenContext *ctx, int tempNum, IrDataType type);
TempLoc *findTemp(CodeGenContext *ctx, int tempNum);
int getVarOffset(CodeGenContext *ctx, const char *name, size_t len);
VarLoc *findVar(CodeGenContext *ctx, const char *name, size_t len);
void addLocalVar(CodeGenContext *ctx, const char *name, size_t len, IrDataType type);
void addGlobalVar(CodeGenContext *ctx, const char *name, size_t len, IrDataType type);
const char *getIntReg(const char *base, IrDataType type) ;
const char *getSSESuffix(IrDataType type);
const char *getSSEReg(int num);
int getTypeSize(IrDataType type);
const char *getIntSuffix(IrDataType type);
int isFloatingPoint(IrDataType type);
void freeVarList(VarLoc *list);
void freeTempList(TempLoc *list);
const char *getParamIntReg(int index, IrDataType type);
void markVarAsAddresable(CodeGenContext *ctx, const char *name, size_t len, int arraySize);

#endif // VARIABLE_HANDLING_H