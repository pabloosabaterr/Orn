#include "./ir.h"
#include <stdlib.h>
#include "./irHelpers.h"

int binaryConstant(IrInstruction *inst){
    return inst->ar1.type == OPERAND_CONSTANT && inst->ar2.type == OPERAND_CONSTANT;
}

int constantFolding(IrContext *ctx){
    int changed = 0;
    IrInstruction *inst = ctx->instructions;
    while(inst){
        if(binaryConstant(inst)){
            switch(inst->op){
                case IR_ADD: {
                    inst->op = IR_COPY;
                    switch(inst->result.dataType){
                        case IR_TYPE_INT: inst->ar1 = createIntConst(inst->ar1.value.constant.intVal + inst->ar2.value.constant.intVal); break;
                        case IR_TYPE_FLOAT: inst->ar1 = createFloatConst(inst->ar1.value.constant.floatVal + inst->ar2.value.constant.floatVal); break;
                        case IR_TYPE_DOUBLE: inst->ar1 = createDoubleConst(inst->ar1.value.constant.doubleVal + inst->ar2.value.constant.doubleVal); break;
                        default: break;
                    }
                    break;
                }
                case IR_SUB: {
                    inst->op = IR_COPY;
                    switch(inst->result.dataType){
                        case IR_TYPE_INT: inst->ar1 = createIntConst(inst->ar1.value.constant.intVal - inst->ar2.value.constant.intVal); break;
                        case IR_TYPE_FLOAT: inst->ar1 = createFloatConst(inst->ar1.value.constant.floatVal - inst->ar2.value.constant.floatVal); break;
                        case IR_TYPE_DOUBLE: inst->ar1 = createDoubleConst(inst->ar1.value.constant.doubleVal - inst->ar2.value.constant.doubleVal); break;
                        default: break;
                    }
                    break;
                }
                case IR_MUL: {
                    inst->op = IR_COPY;
                    switch(inst->result.dataType){
                        case IR_TYPE_INT: inst->ar1 = createIntConst(inst->ar1.value.constant.intVal * inst->ar2.value.constant.intVal); break;
                        case IR_TYPE_FLOAT: inst->ar1 = createFloatConst(inst->ar1.value.constant.floatVal * inst->ar2.value.constant.floatVal); break;
                        case IR_TYPE_DOUBLE: inst->ar1 = createDoubleConst(inst->ar1.value.constant.doubleVal * inst->ar2.value.constant.doubleVal); break;
                        default: break;
                    }
                    break;
                }
                case IR_DIV: {
                    inst->op = IR_COPY;
                    switch(inst->result.dataType){
                        case IR_TYPE_INT: inst->ar1 = createIntConst(inst->ar1.value.constant.intVal / inst->ar2.value.constant.intVal); break;
                        case IR_TYPE_FLOAT: inst->ar1 = createFloatConst(inst->ar1.value.constant.floatVal / inst->ar2.value.constant.floatVal); break;
                        case IR_TYPE_DOUBLE: inst->ar1 = createDoubleConst(inst->ar1.value.constant.doubleVal / inst->ar2.value.constant.doubleVal); break;
                        default: break;
                    }
                    break;
                }
                case IR_BIT_AND: {
                    inst->op = IR_COPY;
                    inst->ar1 = createIntConst(inst->ar1.value.constant.intVal & inst->ar2.value.constant.intVal);
                    break;
                }
                case IR_BIT_OR: {
                    inst->op = IR_COPY;
                    inst->ar1 = createIntConst(inst->ar1.value.constant.intVal | inst->ar2.value.constant.intVal);
                    break;
                }
                case IR_BIT_XOR: {
                    inst->op = IR_COPY;
                    inst->ar1 = createIntConst(inst->ar1.value.constant.intVal ^ inst->ar2.value.constant.intVal);
                    break;
                }
                case IR_SHL: {
                    inst->op = IR_COPY;
                    inst->ar1 = createIntConst(inst->ar1.value.constant.intVal << inst->ar2.value.constant.intVal);
                    break;
                }
                case IR_SHR: {
                    inst->op = IR_COPY;
                    inst->ar1 = createIntConst(inst->ar1.value.constant.intVal >> inst->ar2.value.constant.intVal);
                    break;
                }
                default: break;
            }
            inst->ar2 = createNone();
            changed = 1;
        }
        inst = inst->next;
    }
    return changed;
}

int operandsEqual(IrOperand a, IrOperand b) {
    if (a.type != b.type) return 0;
    switch (a.type) {
        case OPERAND_TEMP: return a.value.temp.tempNum == b.value.temp.tempNum;
        case OPERAND_VAR: return bufferEqual(a.value.var.name, a.value.var.nameLen, b.value.var.name, b.value.var.nameLen);
        case OPERAND_LABEL: return a.value.label.labelNum == b.value.label.labelNum;
        default:
            return 0;
    }
}

int isReplaceable(IrOperand op){
    return op.type == OPERAND_VAR || op.type == OPERAND_TEMP;
}

int copyProp(IrContext *ctx){
    int changed = 0;
    IrInstruction *inst = ctx->instructions;
    while (inst){
        if((inst->op == IR_COPY || inst->op == IR_CAST) && isReplaceable(inst->result) && ((inst->ar1.type == OPERAND_CONSTANT) || isReplaceable(inst->ar1))){
            IrInstruction *scan = inst->next;
            while (scan && !(scan->op == IR_FUNC_BEGIN || scan->op == IR_FUNC_END)) {
                if (isReplaceable(scan->ar1) && operandsEqual(scan->ar1, inst->result)) {
                    scan->ar1 = inst->ar1;
                    changed = 1;
                }
                if (isReplaceable(scan->ar2) && operandsEqual(scan->ar2, inst->result)) {
                    scan->ar2 = inst->ar1;
                    changed = 1;
                }
                if (isReplaceable(scan->result) && operandsEqual(scan->result, inst->result)) {
                    break;
                }
                
                scan = scan->next;
            }
        }
        inst = inst->next;
    }
    return changed;
}

int deadCodeElimination(IrContext *ctx) {
    int changed;

    do {
        changed = 0;
        IrInstruction *inst = ctx->instructions;

        while (inst) {
            if ((inst->result.type == OPERAND_TEMP || inst->result.type == OPERAND_VAR) &&
                inst->op != IR_CALL && inst->op != IR_PARAM && 
                inst->op != IR_RETURN && inst->op != IR_RETURN_VOID) {

                int isUsed = 0;
                IrInstruction *scan = inst->next;

                while (scan && !(scan->op == IR_FUNC_BEGIN || scan->op == IR_FUNC_END)) {
                    if (operandsEqual(scan->ar1, inst->result)) {
                        isUsed = 1;
                        break;
                    }

                    if (operandsEqual(scan->ar2, inst->result)) {
                        isUsed = 1;
                        break;
                    }

                    if (operandsEqual(scan->result, inst->result)) {
                        break;
                    }
                    scan = scan->next;
                }

                if (!isUsed) {
                    IrInstruction *toDelete = inst;
                    inst = inst->next;

                    if (toDelete->prev) {
                        toDelete->prev->next = toDelete->next;
                    } else {
                        ctx->instructions = toDelete->next;
                    }

                    if (toDelete->next) {
                        toDelete->next->prev = toDelete->prev;
                    } else {
                        ctx->lastInstruction = toDelete->prev;
                    }

                    free(toDelete);
                    ctx->instructionCount--;
                    changed = 1;
                    continue;
                }
            }

            inst = inst->next;
        }
    } while (changed);
    return changed;
}

void optimizeIR(IrContext *ctx, int optLevel) {
    if (optLevel == 0) return;
    
    // int maxPasses;
    // switch (optLevel) {
    //     case 1: maxPasses = 3; break;
    //     case 2: maxPasses = 5; break;
    //     case 3: maxPasses = 10; break;
    //     default: maxPasses = 0; break;
    // }
    
    int changed = 0;
    while (1) {    
        changed |= constantFolding(ctx);
        changed |= copyProp(ctx);
        changed |= constantFolding(ctx);
        changed |= deadCodeElimination(ctx);
        if (!changed) break;
    }
}
