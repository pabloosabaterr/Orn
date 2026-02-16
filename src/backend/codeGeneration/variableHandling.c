#include <stdlib.h>
#include <string.h>
#include "codegen.h"

int isFloatingPoint(IrDataType type){
    return type == IR_TYPE_DOUBLE || type == IR_TYPE_FLOAT;
}

int getTypeSize(IrDataType type){
    switch(type){
        case IR_TYPE_BOOL:   return 1;
        case IR_TYPE_INT:    return 4;
        case IR_TYPE_FLOAT:  return 4;
        case IR_TYPE_DOUBLE: return 8;
        case IR_TYPE_STRING: return 8; // strings are pointers also
        case IR_TYPE_POINTER: return 8;
        default: return 8;
    }
}

const char *getParamIntReg(int index, IrDataType type) {
    static const char *regs32[] = {"%edi", "%esi", "%edx", "%ecx", "%r8d", "%r9d"};
    static const char *regs64[] = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
    
    if (index < 0 || index >= 6) return NULL;
    
    return (type == IR_TYPE_INT) ? regs32[index] : regs64[index];
}

const char *getIntSuffix(IrDataType type){
    switch (type) {
        case IR_TYPE_BOOL:  return "b";
        case IR_TYPE_INT:   return "l";
        default:            return "q";
    }
}

const char *getIntReg(const char *base, IrDataType type) {
    switch (type) {
        case IR_TYPE_BOOL:
            if (strcmp(base, "a") == 0)  return "%al";
            if (strcmp(base, "b") == 0)  return "%bl";
            if (strcmp(base, "c") == 0)  return "%cl";
            if (strcmp(base, "d") == 0)  return "%dl";
            if (strcmp(base, "di") == 0) return "%dil";
            if (strcmp(base, "si") == 0) return "%sil";
            break;
            
        case IR_TYPE_INT:
            if (strcmp(base, "a") == 0)  return "%eax";
            if (strcmp(base, "b") == 0)  return "%ebx";
            if (strcmp(base, "c") == 0)  return "%ecx";
            if (strcmp(base, "d") == 0)  return "%edx";
            if (strcmp(base, "di") == 0) return "%edi";
            if (strcmp(base, "si") == 0) return "%esi";
            break;
            
        default:
            if (strcmp(base, "a") == 0)  return "%rax";
            if (strcmp(base, "b") == 0)  return "%rbx";
            if (strcmp(base, "c") == 0)  return "%rcx";
            if (strcmp(base, "d") == 0)  return "%rdx";
            if (strcmp(base, "di") == 0) return "%rdi";
            if (strcmp(base, "si") == 0) return "%rsi";
            break;
    }
    return "%rax"; 
}

const char *getSSEReg(int num){
    static const char *regs[] = {
        "%xmm0", "%xmm1", "%xmm2", "%xmm3",
        "%xmm4", "%xmm5", "%xmm6", "%xmm7"
    };
    if (num >= 0 && num < 8) return regs[num];
    return "%xmm0";
}

const char *getSSESuffix(IrDataType type) {
    return (type == IR_TYPE_FLOAT) ? "ss" : "sd";
}

void addGlobalVar(CodeGenContext *ctx, const char *name, size_t len, IrDataType type) {
    VarLoc *existing = findVar(ctx, name, len);
    if (existing) return;
    
    VarLoc *var = malloc(sizeof(struct VarLoc));
    if (!var) return;
    
    int size = getTypeSize(type);
    ctx->globalStackOff -= size;
    int misalignment = (-ctx->globalStackOff) % size;
    if (misalignment != 0) {
        ctx->globalStackOff -= (size - misalignment);
    }
    
    var->name = name;
    var->nameLen = len;
    var->stackOffset = ctx->globalStackOff;
    var->type = type;
    var->next = ctx->globalVars;
    var->isAddresable = 0;
    var->arraySize = 0;
    ctx->globalVars = var;
}

void addLocalVar(CodeGenContext *ctx, const char *name, size_t len, IrDataType type) {
    if (!ctx->currentFn) {
        addGlobalVar(ctx, name, len, type);
        return;
    }
    
    VarLoc *loc = ctx->currentFn->locs;
    while (loc) {
        if (loc->nameLen == len && memcmp(loc->name, name, len) == 0) {
            return;
        }
        loc = loc->next;
    }
    
    VarLoc *var = malloc(sizeof(struct VarLoc));
    if (!var) return;
    
    int size = getTypeSize(type);
    ctx->currentFn->stackSize += size;
    int misalignment = (-ctx->globalStackOff) % size;
    if (misalignment != 0) {
        ctx->globalStackOff -= (size - misalignment);
    }
    
    var->name = name;
    var->nameLen = len;
    var->stackOffset = -ctx->currentFn->stackSize;
    var->type = type;
    var->next = ctx->currentFn->locs;
    var->isAddresable = 0;
    var->arraySize = 0;
    ctx->currentFn->locs = var;
}

VarLoc *findVar(CodeGenContext *ctx, const char *name, size_t len) {
    if (ctx->currentFn) {
        VarLoc *loc = ctx->currentFn->locs;
        while (loc) {
            if (loc->nameLen == len && memcmp(loc->name, name, len) == 0) {
                return loc;
            }
            loc = loc->next;
        }
    }
    
    VarLoc *loc = ctx->globalVars;
    while (loc) {
        if (loc->nameLen == len && memcmp(loc->name, name, len) == 0) {
            return loc;
        }
        loc = loc->next;
    }
    
    return NULL;
}

void markVarAsAddresable(CodeGenContext *ctx, const char *name, size_t len, int arraySize) {
    VarLoc *var = findVar(ctx, name, len);
    if (!var) return;
    
    var->isAddresable = 1;
    var->arraySize = arraySize;
    
    int elemSize = getTypeSize(var->type);
    int totalSize = elemSize * arraySize;
    int currentSize = elemSize; 
    int additionalSize = totalSize - currentSize;
    
    if (ctx->inFn && ctx->currentFn) {
        var->stackOffset -= additionalSize;
        ctx->currentFn->stackSize += additionalSize;
    } else {
        ctx->globalStackOff -= additionalSize;
        var->stackOffset = ctx->globalStackOff;
    }
}


//just a wrap
int getVarOffset(CodeGenContext *ctx, const char *name, size_t len) {
    VarLoc *var = findVar(ctx, name, len);
    if (var) return var->stackOffset;
    return 0;
}

TempLoc *findTemp(CodeGenContext *ctx, int tempNum){
    if (ctx->currentFn) {
        TempLoc *temp = ctx->currentFn->temps;
        while (temp) {
            if (temp->tempNum == tempNum) {
                return temp;
            }
            temp = temp->next;
        }
    }
    TempLoc *temp = ctx->globalTemps;
    while (temp) {
        if (temp->tempNum == tempNum) {
            return temp;
        }
        temp = temp->next;
    }
    
    return NULL;
}

// min 4 bytes size alignement

void addTemp(CodeGenContext *ctx, int tempNum, IrDataType type) {
    TempLoc *exist = findTemp(ctx, tempNum);
    if (exist) return;
    
    TempLoc *temp = malloc(sizeof(struct TempLoc));
    if (!temp) return;
    
    int size = getTypeSize(type);
    if (size < 4) size = 4;
    
    temp->tempNum = tempNum;
    temp->type = type;
    
    if (ctx->inFn && ctx->currentFn) {
        ctx->currentFn->stackSize += size;
        if (ctx->currentFn->stackSize % size != 0) {
            ctx->currentFn->stackSize += size - (ctx->currentFn->stackSize % size);
        }
        temp->stackOff = -ctx->currentFn->stackSize;
        temp->next = ctx->currentFn->temps;
        ctx->currentFn->temps = temp;
    } else {
        ctx->globalStackOff -= size;
        if (ctx->globalStackOff % size != 0) {
            ctx->globalStackOff -= (-ctx->globalStackOff % size);
        }
        temp->stackOff = ctx->globalStackOff;
        temp->next = ctx->globalTemps;
        ctx->globalTemps = temp;
    }
    
    if (tempNum > ctx->maxTempNum) {
        ctx->maxTempNum = tempNum;
    }
}

int getTempOffset(CodeGenContext *ctx, int tempNum, IrDataType type) {
    TempLoc *temp = findTemp(ctx, tempNum);
    if (temp) {
        return temp->stackOff;
    }
    
    addTemp(ctx, tempNum, type);
    temp = findTemp(ctx, tempNum);
    return temp->stackOff;
}

void freeVarList(VarLoc *list) {
    while (list) {
        VarLoc *next = list->next;
        free(list);
        list = next;
    }
}

void freeTempList(TempLoc *list) {
    while (list) {
        TempLoc *next = list->next;
        free(list);
        list = next;
    }
}