#include "ir.h"
#include <stdlib.h>
#include <stdio.h>
#include "semantic.h"
#include "irHelpers.h"

IrContext *createIrContext(){
    IrContext *ctx = malloc(sizeof(IrContext));
    if(!ctx) return NULL;

    ctx->instructions = NULL;
    ctx->lastInstruction = NULL;
    ctx->instructionCount = 0;
    ctx->nextTempNum = 1;
    ctx->nextLabelNum = 1;
    ctx->pendingJumps = NULL;
    return ctx;
}

void freeIrContext(IrContext *ctx) {
    if (!ctx) return;
    
    IrInstruction *inst = ctx->instructions;
    while (inst) {
        IrInstruction *next = inst->next;
        free(inst);
        inst = next;
    }
    
    free(ctx);
}

IrOperand createTemp(IrContext *ctx, IrDataType type){
    return (IrOperand){
        .type =   OPERAND_TEMP,
        .dataType = type,
        .value.temp.tempNum = ctx->nextTempNum++
    };
}

IrOperand createVar(const char *name, size_t len, IrDataType type){
    return (IrOperand){
        .type = OPERAND_VAR,
        .dataType = type,
        .value.var.name = name,
        .value.var.nameLen = len
    };
}

IrOperand createConst(IrDataType type) {
    IrOperand op = {0};
    op.type = OPERAND_CONSTANT;
    op.dataType = type;
    return op;
}

IrOperand createSizedIntConst(int64_t val, IrDataType type) {
    IrOperand op = createConst(type);
    op.value.constant.intVal = val;
    return op;
}

IrOperand createFloatConst(float val){
    IrOperand op = createConst(IR_TYPE_FLOAT);
    op.value.constant.floatVal = val;
    return op;
}

IrOperand createDoubleConst(double val){
    IrOperand op = createConst(IR_TYPE_DOUBLE);
    op.value.constant.doubleVal = val;
    return op;
}

IrOperand createBoolConst(int val){
    IrOperand op = createConst(IR_TYPE_BOOL);
    op.value.constant.intVal = val ? 1 : 0;
    return op;
}

IrOperand createStringConst(const char* val, size_t len){
    IrOperand op = createConst(IR_TYPE_STRING);
    op.value.constant.str.stringVal = val;
    op.value.constant.str.len = len;
    return op;
}

IrOperand createLabel(int label){
    return (IrOperand){
        .type = OPERAND_LABEL,
        .dataType = IR_TYPE_VOID,
        .value.label.labelNum = label
    };
}

IrOperand createFn(const char *start, size_t len){
    return (IrOperand) {
        .type = OPERAND_FUNCTION,
        .value.fn.name = start,
        .value.fn.nameLen = len
    };
}

IrOperand createNone(){
    return (IrOperand){
        .type = OPERAND_NONE,
        .dataType = IR_TYPE_VOID
    };
}

IrOperand createNullConst() {
    IrOperand op = {0};
    op.type = OPERAND_CONSTANT;
    op.dataType = IR_TYPE_POINTER;
    op.value.constant.intVal = 0;
    return op;
}

void appendInstruction(IrContext *ctx, IrInstruction *inst){
    if(!ctx->instructions){
        ctx->instructions = inst;
        ctx->lastInstruction = inst;
    } else {
        ctx->lastInstruction->next = inst;
        inst->prev = ctx->lastInstruction;
        ctx->lastInstruction = inst;
    }
    ctx->instructionCount++;
}

IrInstruction *emitBinary(IrContext *ctx, IrOpCode op, IrOperand res, IrOperand ar1, IrOperand ar2){
    IrInstruction *inst = malloc(sizeof(IrInstruction));
    if(!inst) return NULL;

    inst->op = op;
    inst->result = res;
    inst->ar1 = ar1;
    inst->ar2 = ar2;
    inst->next = NULL;
    inst->prev = NULL;

    appendInstruction(ctx, inst);
    return inst;
}

IrInstruction *emitUnary(IrContext *ctx, IrOpCode op, IrOperand res, IrOperand ar1){
    IrOperand none = createNone();
    return emitBinary(ctx, op, res, ar1, none);
}

IrInstruction *emitCopy(IrContext *ctx, IrOperand res, IrOperand ar1){
    return emitUnary(ctx, IR_COPY, res, ar1);
}

IrInstruction *emitStore(IrContext *ctx, IrOperand ptr, IrOperand value) {
    IrOperand none = createNone();
    return emitBinary(ctx, IR_STORE, none, ptr, value);
}

IrInstruction *emitLabel(IrContext *ctx, int lab){
    IrOperand label = createLabel(lab);
    IrOperand none = createNone();
    return emitBinary(ctx, IR_LABEL, label, none, none);
}

IrInstruction *emitGoto(IrContext *ctx, int lab){
    IrOperand label = createLabel(lab);
    IrOperand none = createNone();
    return emitBinary(ctx, IR_GOTO, none, label, none);
}

IrInstruction *emitIfFalse(IrContext *ctx, IrOperand cond, int lab) {
    IrOperand label = createLabel(lab);
    IrOperand none = createNone();
    return emitBinary(ctx, IR_IF_FALSE, none, cond, label);
}

IrInstruction *emitReturn(IrContext *ctx, IrOperand ret) {
    IrOpCode op = (ret.type == OPERAND_NONE) ? IR_RETURN_VOID : IR_RETURN;
    IrOperand none = createNone();
    return emitBinary(ctx, op, none, ret, none);
}

IrInstruction *emitPointerLoad(IrContext *ctx, IrOperand loadTo, IrOperand base, IrOperand off){
    return emitBinary(ctx, IR_POINTER_LOAD, loadTo, base, off);
}

IrInstruction *emitPointerStore(IrContext *ctx, IrOperand base, IrOperand off, IrOperand val){
    return emitBinary(ctx, IR_POINTER_STORE, base, off, val);
}

IrInstruction *emitCall(IrContext *ctx, IrOperand res, const char *fnName, 
                        size_t nameLen, int params) {
    IrOperand func = (IrOperand) {
        .type = OPERAND_FUNCTION,
        .value.fn.name = fnName,
        .value.fn.nameLen = nameLen
    };  
    
    IrOperand paramCount = createSizedIntConst(params, IR_TYPE_I32);
    
    return emitBinary(ctx, IR_CALL, res, func, paramCount);
}

IrInstruction *emitMemberStore(IrContext *ctx, IrOperand structVar, int offset, IrOperand val){
    IrInstruction *inst = malloc(sizeof(struct IrInstruction));
    if(!inst) return NULL;
    inst->op = IR_MEMBER_STORE;
    inst->result = structVar;
    inst->ar1 = createSizedIntConst(offset, IR_TYPE_I32);
    inst->ar2 = val;
    inst->next = NULL;
    inst->prev = NULL;

    appendInstruction(ctx, inst);
    return inst;
}

IrInstruction *emitMemberLoad(IrContext *ctx, IrOperand dest, IrOperand structVar, int offset){
    IrInstruction *inst = malloc(sizeof(struct IrInstruction));
    if(!inst) return NULL;
    inst->op = IR_MEMBER_LOAD;
    inst->result = dest;
    inst->ar1 = structVar;
    inst->ar2 = createSizedIntConst(offset, IR_TYPE_I32);
    inst->next = NULL;
    inst->prev = NULL;

    appendInstruction(ctx, inst);
    return inst;
}

IrInstruction *emitAllocStruct(IrContext *ctx, IrOperand dest, int size){
    IrInstruction *inst = malloc(sizeof(struct IrInstruction));
    if(!inst) return NULL;
    inst->op = IR_ALLOC_STRUCT;
    inst->result = dest;
    inst->ar1 = createSizedIntConst(size, IR_TYPE_I32);
    inst->ar2 = createNone();
    inst->next = NULL;
    inst->prev = NULL;

    appendInstruction(ctx, inst);
    return inst;
}

IrDataType symbolTypeToIrType(DataType type) {
    switch (type) {
        case TYPE_I8: return IR_TYPE_I8;
        case TYPE_I16: return IR_TYPE_I16;
        case TYPE_I32: return IR_TYPE_I32;
        case TYPE_I64: return IR_TYPE_I64;
        case TYPE_U8: return IR_TYPE_U8;
        case TYPE_U16: return IR_TYPE_U16;
        case TYPE_U32: return IR_TYPE_U32;
        case TYPE_U64: return IR_TYPE_U64;
        case TYPE_FLOAT: return IR_TYPE_FLOAT;
        case TYPE_DOUBLE: return IR_TYPE_DOUBLE;
        case TYPE_BOOL: return IR_TYPE_BOOL;
        case TYPE_STRING: return IR_TYPE_STRING;
        case TYPE_VOID: return IR_TYPE_VOID;
        case TYPE_POINTER: return IR_TYPE_POINTER;
        case TYPE_STRUCT: return IR_TYPE_POINTER;
        case TYPE_NULL: return IR_TYPE_POINTER;
        // todo: throw error instead of i32 def
        default: return IR_TYPE_I32;
    }
}

IrDataType nodeTypeToIrType(NodeTypes nodeType) {
    switch (nodeType) {
        case REF_I8:     return IR_TYPE_I8;
        case REF_I16:    return IR_TYPE_I16;
        case REF_I32:    return IR_TYPE_I32;
        case REF_I64:    return IR_TYPE_I64;
        case REF_U8:     return IR_TYPE_U8;
        case REF_U16:    return IR_TYPE_U16;
        case REF_U32:    return IR_TYPE_U32;
        case REF_U64:    return IR_TYPE_U64;
        case REF_FLOAT:  return IR_TYPE_FLOAT;
        case REF_DOUBLE: return IR_TYPE_DOUBLE;
        case REF_BOOL:   return IR_TYPE_BOOL;
        case REF_STRING: return IR_TYPE_STRING;
        case REF_VOID:   return IR_TYPE_VOID;
        case REF_CUSTOM:
        case STRUCT_VARIABLE_DEFINITION:
            return IR_TYPE_POINTER;
        // todo: throw error instead of defaulting to i32
        default:         return IR_TYPE_I32;
    }
}

IrOpCode astOpToIrOp(NodeTypes nodeType) {
    switch (nodeType) {
        case ADD_OP:
        case COMPOUND_ADD_ASSIGN:
            return IR_ADD;
            
        case SUB_OP:
        case COMPOUND_SUB_ASSIGN:
            return IR_SUB;
            
        case MUL_OP:
        case COMPOUND_MUL_ASSIGN:
            return IR_MUL;
            
        case DIV_OP:
        case COMPOUND_DIV_ASSIGN:
            return IR_DIV;
            
        case MOD_OP:
            return IR_MOD;
            
        case BITWISE_AND:
        case COMPOUND_AND_ASSIGN:
            return IR_BIT_AND;
            
        case BITWISE_OR:
        case COMPOUND_OR_ASSIGN:
            return IR_BIT_OR;
            
        case BITWISE_XOR:
        case COMPOUND_XOR_ASSIGN:
            return IR_BIT_XOR;
            
        case BITWISE_LSHIFT:
        case COMPOUND_LSHIFT_ASSIGN:
            return IR_SHL;
            
        case BITWISE_RSHIFT:
        case COMPOUND_RSHIFT_ASSIGN:
            return IR_SHR;
            
        case EQUAL_OP:
            return IR_EQ;
            
        case NOT_EQUAL_OP:
            return IR_NE;
            
        case LESS_THAN_OP:
            return IR_LT;
            
        case LESS_EQUAL_OP:
            return IR_LE;
            
        case GREATER_THAN_OP:
            return IR_GT;
            
        case GREATER_EQUAL_OP:
            return IR_GE;
            
        case LOGIC_AND:
            return IR_AND;
            
        case LOGIC_OR:
            return IR_OR;
            
        case LOGIC_NOT:
            return IR_NOT;
            
        case UNARY_MINUS_OP:
            return IR_NEG;
        case BITWISE_NOT: 
            return IR_BIT_NOT;

        default:
            return IR_NOP;
    }
}

static MemberAccessInfo resolveMemberAccessChain(ASTNode node, TypeCheckContext typeCtx, IrContext *irCtx) {
    MemberAccessInfo info = { NULL, 0, 0, NULL, TYPE_UNKNOWN, 0, 0 };
    
    if (!node || node->nodeType != MEMBER_ACCESS) return info;
    
    ASTNode objectNode = node->children;
    ASTNode fieldNode = objectNode->brothers;
    
    if (objectNode->nodeType == MEMBER_ACCESS) {
        info = resolveMemberAccessChain(objectNode, typeCtx, irCtx);
        if (!info.baseName || !info.finalStructType) return info;
        
        if(info.fieldType == TYPE_POINTER && info.finalStructType){
            IrOperand temp = createTemp(irCtx, IR_TYPE_POINTER);
            IrOperand base;
            if(info.baseIsTemp){
                base = (IrOperand){
                    .type = OPERAND_TEMP,
                    .dataType = IR_TYPE_POINTER,
                    .value.temp.tempNum = info.baseTempNum
                };
            } else {
                base = createVar(info.baseName, info.baseNameLen, IR_TYPE_POINTER);
            }

            emitMemberLoad(irCtx, temp, base, info.totalOffset);

            info.baseIsTemp = 1;
            info.baseTempNum = temp.value.temp.tempNum;
            info.totalOffset = 0;
        }

        StructField field = info.finalStructType->fields;
        while (field) {
            if (bufferEqual(fieldNode->start, fieldNode->length, field->nameStart, field->nameLength)) {
                info.totalOffset += field->offset;
                info.finalStructType = field->structType;
                info.fieldType = field->type;
                return info;
            }
            field = field->next;
        }
        info.baseName = NULL;
        return info;
        
    } else if (objectNode->nodeType == VARIABLE) {
        Symbol structSym = lookupSymbol(typeCtx->current, 
                                        objectNode->start, objectNode->length);
        if (!structSym || !structSym->structType) return info;
        
        info.baseName = objectNode->start;
        info.baseNameLen = objectNode->length;
        
        StructField field = structSym->structType->fields;
        while (field) {
            if (bufferEqual(fieldNode->start, fieldNode->length, field->nameStart, field->nameLength)) {
                info.totalOffset = field->offset;
                info.finalStructType = field->structType;
                info.fieldType = field->type;
                return info;
            }
            field = field->next;
        }
    }
    
    return info;
}

/**
 * @brief Generated Ir instructtions for a function
 * @details ar1 is the function name, ar2 is 1 if exported, 0 otherwise and ar3 it is used to indicate if the function returns a data container (struct)
 */
static void generateFunctionIr(IrContext *ctx, ASTNode node, TypeCheckContext typeCtx, int isExported) {
    ASTNode paramList = node->children;
    ASTNode returnType = paramList->brothers;
    ASTNode body = returnType->brothers;

    Symbol fnSymbol = lookupSymbol(typeCtx->current, node->start, node->length);
    if (!fnSymbol) return;
    
    Symbol oldFunction = typeCtx->currentFunction;
    SymbolTable oldScope = typeCtx->current;
    
    typeCtx->currentFunction = fnSymbol;
    if (fnSymbol->functionScope) {
        typeCtx->current = fnSymbol->functionScope;
    }
    
    IrOperand funcName = createFn(node->start, node->length);
    IrOperand exportFlag = createSizedIntConst(isExported, IR_TYPE_I32);
    int returnsDataContainerFlag = fnSymbol->type == TYPE_STRUCT;
    IrOperand returnsDataContainer = createSizedIntConst(returnsDataContainerFlag, IR_TYPE_I32);
    IrOperand none = createNone();
    emitBinary(ctx, IR_FUNC_BEGIN, funcName, exportFlag, returnsDataContainer);

    if (fnSymbol && fnSymbol->parameters) {
        FunctionParameter param = fnSymbol->parameters;
        int paramIndex = returnsDataContainerFlag ? 1 : 0; // Adjust for hidden struct return param
        if(returnsDataContainerFlag){
            IrOperand hiddenPtr = createVar("__hidden_ptr", 13, IR_TYPE_POINTER);
            emitBinary(ctx, IR_LOAD_PARAM, hiddenPtr, createNone(), createSizedIntConst(0, IR_TYPE_I32));
        }
        while (param) {
            IrDataType irType = symbolTypeToIrType(param->type);
            if (param->isPointer) {
                irType = IR_TYPE_POINTER;
            }
            IrOperand paramVar = createVar(param->nameStart, param->nameLength, irType);
            IrOperand indexOp = createSizedIntConst(paramIndex, IR_TYPE_I32);

            emitBinary(ctx, IR_LOAD_PARAM, paramVar, createNone(), indexOp);

            param = param->next;
            paramIndex++;
        }
    }
    generateStatementIr(ctx, body, typeCtx, symbolTypeToNodeType(fnSymbol->type));
    
    emitBinary(ctx, IR_FUNC_END, funcName, none, none);
    typeCtx->currentFunction = oldFunction;
    typeCtx->current = oldScope;
}

IrOperand generateExpressionIr(IrContext *ctx, ASTNode node, TypeCheckContext typeCtx, NodeTypes expectedType) {
    if(!node) return createNone();
    switch (node->nodeType){

    case NULL_LIT:
        return createNullConst();

    case LITERAL: {
        if (!node->children) {
            return createNone();
        }
        switch (node->children->nodeType) {
            case REF_I8:  return createSizedIntConst(parseInt(node->start, node->length), IR_TYPE_I8);
            case REF_I16: return createSizedIntConst(parseInt(node->start, node->length), IR_TYPE_I16);
            case REF_I32: return createSizedIntConst(parseInt(node->start, node->length), IR_TYPE_I32);
            case REF_I64: return createSizedIntConst(parseInt(node->start, node->length), IR_TYPE_I64);
            case REF_U8:  return createSizedIntConst(parseInt(node->start, node->length), IR_TYPE_U8);
            case REF_U16: return createSizedIntConst(parseInt(node->start, node->length), IR_TYPE_U16);
            case REF_U32: return createSizedIntConst(parseInt(node->start, node->length), IR_TYPE_U32);
            case REF_U64: return createSizedIntConst(parseInt(node->start, node->length), IR_TYPE_U64);

            case REF_FLOAT:
                return createFloatConst(parseFloat(node->start, node->length));

            case REF_DOUBLE:
                return createDoubleConst(parseFloat(node->start, node->length));

            case REF_BOOL:
                return createBoolConst(matchLit(node->start, node->length, "true") ? 1 : 0);

            case REF_STRING:
                return createStringConst(node->start, node->length);
            default:
                return createNone();
        }
        break;
    }

    case VARIABLE: {
        Symbol sym = lookupSymbol(typeCtx->current, node->start, node->length);
        if(!sym) return createNone();
        
        IrDataType type;
        if (sym->isPointer || sym->type == TYPE_POINTER) {
            type = IR_TYPE_POINTER;
        } else if (sym->type == TYPE_STRUCT) {
            type = IR_TYPE_POINTER;
        } else {
            type = symbolTypeToIrType(sym->type);
        }
        
        return createVar(node->start, node->length, type);
    }

    case ADD_OP:
    case SUB_OP:
    case MUL_OP:
    case DIV_OP:
    case MOD_OP:
    case BITWISE_AND:
    case BITWISE_OR:
    case BITWISE_XOR:
    case BITWISE_LSHIFT:
    case BITWISE_RSHIFT:
    case EQUAL_OP:
    case NOT_EQUAL_OP:
    case LESS_THAN_OP:
    case LESS_EQUAL_OP:
    case GREATER_THAN_OP:
    case GREATER_EQUAL_OP:
    case LOGIC_AND:
    case LOGIC_OR: {
        ASTNode left = node->children;
        ASTNode right = left ? left->brothers : NULL;
        if (!left || !right) return createNone();

        IrOperand leftOp = generateExpressionIr(ctx, left, typeCtx, expectedType);
        IrOperand rightOp = generateExpressionIr(ctx, right, typeCtx, expectedType);

        IrDataType resultType = symbolTypeToIrType(getExpressionType(node, typeCtx, expectedType));

        IrOperand res = createTemp(ctx, resultType);
        IrOpCode op = astOpToIrOp(node->nodeType);
        emitBinary(ctx, op, res, leftOp, rightOp);
        return res;
    }

    case UNARY_MINUS_OP:
    case LOGIC_NOT:
    case BITWISE_NOT: {
        ASTNode operand = node->children;
        IrOperand operandOp = generateExpressionIr(ctx, operand, typeCtx, expectedType);
        IrOperand res = createTemp(ctx, operandOp.dataType);

        IrOpCode irOp = astOpToIrOp(node->nodeType);
        emitUnary(ctx, irOp, res, operandOp);
        return res;
    }
    case MEMBER_ACCESS: {
        MemberAccessInfo info = resolveMemberAccessChain(node, typeCtx, ctx);
        
        if (!info.baseName && !info.baseIsTemp) {
            return createNone();
        }
        
        IrOperand temp = createTemp(ctx, symbolTypeToIrType(info.fieldType));
        IrOperand base;
        if(info.baseIsTemp){
            base = (IrOperand){
                .type = OPERAND_TEMP,
                .dataType = IR_TYPE_POINTER,
                .value.temp.tempNum = info.baseTempNum
            };
        }else {
            base = createVar(info.baseName, info.baseNameLen, IR_TYPE_POINTER);
        }
        emitMemberLoad(ctx, temp, base, info.totalOffset);
        
        return temp;
    }
    case MEMADDRS: {
        ASTNode target = node->children;
        if (!target) return createNone();

        if (target->nodeType == ARRAY_ACCESS) {
            // &buf[index]: compute base + index * elemSize
            ASTNode arrNode = target->children;
            ASTNode indexNode = arrNode->brothers;
            
            IrOperand indexOp = generateExpressionIr(ctx, indexNode, typeCtx, REF_I32);
            Symbol arraySym = lookupSymbol(typeCtx->current, arrNode->start, arrNode->length);
            if (!arraySym) return createNone();
            
            IrOperand base = createVar(arrNode->start, arrNode->length, IR_TYPE_POINTER);
            IrOperand result = createTemp(ctx, IR_TYPE_POINTER);
            
            // Emit: result = &base[index] (leaq + offset computation)
            emitBinary(ctx, IR_ADDROF, result, base, indexOp);
            return result;
        }

        // &variable
        Symbol targetSym = lookupSymbol(typeCtx->current, target->start, target->length);
        if (!targetSym) return createNone();
        IrDataType targetType = symbolTypeToIrType(targetSym->type);
        IrOperand targetVar = createVar(target->start, target->length, targetType);
        IrOperand result = createTemp(ctx, IR_TYPE_POINTER);
        emitUnary(ctx, IR_ADDROF, result, targetVar);
        return result;
    }

    case POINTER: {
        // Handle *ptr (dereference)
        ASTNode ptrNode = node->children;
        if (!ptrNode) return createNone();

        // Generate the pointer expression
        IrOperand ptrOp = generateExpressionIr(ctx, ptrNode, typeCtx, expectedType);

        // Determine the type of the dereferenced value
        Symbol ptrSym = NULL;
        if (ptrNode->nodeType == VARIABLE) {
            ptrSym = lookupSymbol(typeCtx->current, ptrNode->start, ptrNode->length);
        }

        IrDataType derefType = ptrSym ? symbolTypeToIrType(ptrSym->type) : IR_TYPE_I32;

        // Create temp to hold dereferenced value
        IrOperand result = createTemp(ctx, derefType);

        // Emit: result = *ptr
        emitUnary(ctx, IR_DEREF, result, ptrOp);

        return result;
    }
    case PRE_INCREMENT:
    case PRE_DECREMENT: {
        ASTNode operand = node->children;
        IrOperand var = generateExpressionIr(ctx, operand, typeCtx, REF_I32);
        
        IrOperand one;
        if (var.dataType == IR_TYPE_FLOAT) {
            one = createFloatConst(1.0f);
        } else if (var.dataType == IR_TYPE_DOUBLE) {
            one = createDoubleConst(1.0);
        } else {
            one = createSizedIntConst(1, var.dataType);
        }
        
        IrOperand temp = createTemp(ctx, var.dataType);
        
        IrOpCode op = (node->nodeType == PRE_INCREMENT) ? IR_ADD : IR_SUB;
        emitBinary(ctx, op, temp, var, one);
        
        emitCopy(ctx, var, temp);
        return var;
    }

    case POST_INCREMENT:
    case POST_DECREMENT: {
        ASTNode operand = node->children;
        IrOperand var = generateExpressionIr(ctx, operand, typeCtx, REF_I32);
        IrOperand oldValue = createTemp(ctx, var.dataType);
        emitCopy(ctx, oldValue, var);
        
        IrOperand one;
        if (var.dataType == IR_TYPE_FLOAT) {
            one = createFloatConst(1.0f);
        } else if (var.dataType == IR_TYPE_DOUBLE) {
            one = createDoubleConst(1.0);
        } else {
            one = createSizedIntConst(1, var.dataType);
        }
        
        IrOperand newValue = createTemp(ctx, var.dataType);
        
        IrOpCode op = (node->nodeType == POST_INCREMENT) ? IR_ADD : IR_SUB;
        emitBinary(ctx, op, newValue, var, one);
        
        emitCopy(ctx, var, newValue);
        
        return oldValue;
    }

    case FUNCTION_CALL: {
        Symbol funcSymbol = lookupSymbol(typeCtx->current, node->start, node->length);
        int paramCount = 0;
        if(funcSymbol && funcSymbol->type != TYPE_VOID && funcSymbol->type == TYPE_STRUCT){
            ++paramCount;
        }
        ASTNode argList = node->children;
        if (argList && argList->nodeType == ARGUMENT_LIST) {
            ASTNode arg = argList->children;
            while (arg) {
                IrOperand argOp = generateExpressionIr(ctx, arg, typeCtx, funcSymbol->parameters + paramCount);
                IrOperand none = createNone();
                emitBinary(ctx, IR_PARAM, none, argOp, none);
                paramCount++;
                arg = arg->brothers;
            }
        }

        IrDataType retType = funcSymbol && funcSymbol->type != TYPE_STRUCT ? symbolTypeToIrType(funcSymbol->type) : IR_TYPE_VOID;
        
        IrOperand result = (retType == IR_TYPE_VOID) ? createNone() : createTemp(ctx, retType);
        emitCall(ctx, result, node->start, node->length, paramCount);
        return result;
    }

    case ASSIGNMENT:
    case COMPOUND_ADD_ASSIGN:
    case COMPOUND_SUB_ASSIGN:
    case COMPOUND_MUL_ASSIGN:
    case COMPOUND_DIV_ASSIGN:
    case COMPOUND_AND_ASSIGN:
    case COMPOUND_OR_ASSIGN:
    case COMPOUND_XOR_ASSIGN:
    case COMPOUND_LSHIFT_ASSIGN:
    case COMPOUND_RSHIFT_ASSIGN: {
        ASTNode left = node->children;
        ASTNode right = left ? left->brothers : NULL;
        if (!left || !right) return createNone();
        Symbol leftSym = lookupSymbol(typeCtx->current, left->children->start, left->children->length);
        NodeTypes leftTypeExpected = left->nodeType;
        IrOperand rightOp = generateExpressionIr(ctx, right, typeCtx, leftTypeExpected);
        IrOperand leftOp;

        if (left->nodeType == ARRAY_ACCESS) {
            leftOp = generateExpressionIr(ctx, left->children, typeCtx, leftSym->type);
            ASTNode target = left->children->brothers;
            emitPointerStore(ctx, leftOp, generateExpressionIr(ctx, target, typeCtx, leftSym->type), rightOp);
        } else if (left->nodeType == POINTER) {
            // Handle *ptr = value
            ASTNode ptrNode = left->children;
            IrOperand ptrOp = generateExpressionIr(ctx, ptrNode, typeCtx, leftSym->type);

            emitStore(ctx, ptrOp, rightOp);

            return ptrOp;
        }else if (left->nodeType == MEMBER_ACCESS) {
            MemberAccessInfo info = resolveMemberAccessChain(left, typeCtx, ctx);
            
            if (!info.baseName && !info.baseIsTemp) {
                return createNone();
            }
            
            IrOperand base;
            if(info.baseIsTemp){
                base = (IrOperand){
                    .type = OPERAND_TEMP,
                    .dataType = IR_TYPE_POINTER,
                    .value.temp.tempNum = info.baseTempNum
                };
            } else {
                base = createVar(info.baseName, info.baseNameLen, IR_TYPE_POINTER);
            }
            
            if (node->nodeType != ASSIGNMENT) {
                // Compound assignment
                IrOperand temp1 = createTemp(ctx, symbolTypeToIrType(info.fieldType));
                emitMemberLoad(ctx, temp1, base, info.totalOffset);
                IrOperand t2 = createTemp(ctx, temp1.dataType);
                IrOpCode op = astOpToIrOp(node->nodeType);
                emitBinary(ctx, op, t2, temp1, rightOp);
                emitMemberStore(ctx, base, info.totalOffset, t2);
            } else {
                emitMemberStore(ctx, base, info.totalOffset, rightOp);
            }
        }else {
            leftOp = generateExpressionIr(ctx, left, typeCtx, leftSym->type);
            if (node->nodeType != ASSIGNMENT) {
                IrDataType resultType = leftOp.dataType;
                IrOperand temp = createTemp(ctx, resultType);

                IrOpCode op = astOpToIrOp(node->nodeType);

                emitBinary(ctx, op, temp, leftOp, rightOp);
                emitCopy(ctx, leftOp, temp);

                return leftOp;
            }

            emitCopy(ctx, leftOp, rightOp);
        }

        return leftOp;
    }

    case CAST_EXPRESSION: {
        ASTNode sourceExpr = node->children;
        ASTNode targetType = sourceExpr->brothers;

        IrOperand source = generateExpressionIr(ctx, sourceExpr, typeCtx, targetType->nodeType);
        IrDataType target = nodeTypeToIrType(targetType->nodeType);

        IrOperand res = createTemp(ctx, target);
        emitUnary(ctx, IR_CAST, res, source);
        return res;
    }

    case ARRAY_ACCESS:{
        ASTNode arrNode = node->children;
        ASTNode index = arrNode->brothers;
        IrOperand indexOp = generateExpressionIr(ctx, index, typeCtx, REF_I32);

        Symbol arraySym = lookupSymbol(typeCtx->current, arrNode->start, arrNode->length);
        IrDataType elemType = symbolTypeToIrType(arraySym->type);

        IrOperand arrayBase = createVar(arrNode->start, arrNode->length, IR_TYPE_POINTER);

        IrOperand result = createTemp(ctx, elemType);
        emitPointerLoad(ctx, result, arrayBase, indexOp);
        return result;
    }

    default: return createNone();
    }
}


void generateStatementIr(IrContext *ctx, ASTNode node, TypeCheckContext typeCtx, NodeTypes expectedType) {
    switch(node->nodeType){
        case PROGRAM: {
            ASTNode child = node->children;
            while(child){
                generateStatementIr(ctx, child, typeCtx, expectedType);
                child = child->brothers;
            }
            break;
        }
        case BLOCK_STATEMENT:
        case BLOCK_EXPRESSION: {
            // IMPORTANT: Dequeue the block scope that was created during type checking
            // This ensures symbol lookups work correctly for block-local variables
            
            SymbolTable oldScope = typeCtx->current;
            SymbolTable blockScope = dequeueBlockScope(typeCtx);
            
            if (blockScope) {
                typeCtx->current = blockScope;
            }
            
            ASTNode child = node->children;
            while(child){
                generateStatementIr(ctx, child, typeCtx, expectedType);
                child = child->brothers;
            }
            
            // Restore the previous scope
            typeCtx->current = oldScope;
            break;
        }
        case LET_DEC:
        case CONST_DEC:
        if (node->children) {
            ASTNode varDef = node->children;
            Symbol sym = lookupSymbol(typeCtx->current, varDef->start, varDef->length);
            if(sym->type == TYPE_STRUCT){
                IrOperand var = createVar(varDef->start, varDef->length, IR_TYPE_POINTER);
                int totalSize = sym->structType->size;
                if(typeCtx->currentFunction && typeCtx->currentFunction->returnedVar == sym){
                    emitCopy(ctx, var, createVar("__hidden_ptr", 13, IR_TYPE_POINTER));
                }else{
                    emitAllocStruct(ctx, var, totalSize);
                    if(varDef->children->brothers && varDef->children->brothers->children->nodeType == FUNCTION_CALL){
                        IrOperand temp = createTemp(ctx, IR_TYPE_POINTER);
                        emitUnary(ctx, IR_ADDROF, temp, var);
                        emitBinary(ctx, IR_PARAM, createNone(), temp, createNone());
                    }
                }
                
                if (varDef->children && varDef->children->brothers) {
                    ASTNode initValue = varDef->children->brothers->children;
                    IrOperand srcOp = generateExpressionIr(ctx, initValue, typeCtx, sym->type);
                    
                    if(varDef->children->brothers->children->nodeType != FUNCTION_CALL){
                        StructField field = sym->structType->fields;
                        while (field) {
                            IrOperand temp = createTemp(ctx, symbolTypeToIrType(field->type));
                            emitMemberLoad(ctx, temp, srcOp, field->offset);
                            emitMemberStore(ctx, var, field->offset, temp);
                            field = field->next;
                        }
                    }
                }
                }else{
                    generateStatementIr(ctx, node->children, typeCtx, expectedType);
                }
            }
            break;
        case VAR_DEFINITION: {
            if (node->children && node->children->brothers) {
                Symbol sym = lookupSymbol(typeCtx->current, node->children->start, node->children->length);
                assert(sym && "Variable symbol should exist in symbol table at this point");
                IrOperand val = generateExpressionIr(ctx, node->children->brothers->children, typeCtx, sym->type);

                ASTNode typeRefChild = node->children->children;
                IrDataType type;
                if (typeRefChild && typeRefChild->nodeType == POINTER) {
                    type = IR_TYPE_POINTER;
                } else {
                    type = nodeTypeToIrType(typeRefChild ? typeRefChild->nodeType : REF_I32);
                }

                IrOperand var = createVar(node->start, node->length, type);
                emitCopy(ctx, var, val);
            }
            break;
        }
        case ARRAY_VARIABLE_DEFINITION:{
            if(node->children){
                // todo: use symbol table instead of node values, even if ast is supposed to be correct after sem pass
                ASTNode typeref = node->children;
                IrDataType type = nodeTypeToIrType(typeref->children->nodeType);
                ASTNode staticSizeNode = typeref->brothers;
                IrOperand arr = createVar(node->start, node->length, type);
                ASTNode valNode = staticSizeNode->brothers;
                int staticSize;
                if(staticSizeNode->nodeType == LITERAL){
                    staticSize = parseInt(staticSizeNode->start, staticSizeNode->length);
                }else{
                    Symbol arrSym = lookupSymbol(typeCtx->current, staticSizeNode->start, staticSizeNode->length);
                    staticSize = arrSym->constVal;
                }
                IrOperand sizeOp = createSizedIntConst(staticSize, IR_TYPE_I32);
                emitUnary(ctx, IR_REQ_MEM, arr, sizeOp);
                if(valNode){
                    if (valNode->children->nodeType == ARRAY_LIT) {
                        ASTNode arrLitVal = valNode->children->children;
                        for (int i = 0; i < staticSize; ++i) {
                            IrOperand val = generateExpressionIr(ctx, arrLitVal, typeCtx, type);
                            IrOperand off = createSizedIntConst(i, IR_TYPE_I32);
                            emitPointerStore(ctx, arr, off, val);
                            arrLitVal = arrLitVal->brothers;
                        }
                    } else {
                        emitCopy(ctx, arr, generateExpressionIr(ctx, valNode->children, typeCtx, type));
                    }
                }
            }
            break;
        }

        case IF_CONDITIONAL: {
            ASTNode cond = node->children;
            ASTNode trueBranchWrap = cond->brothers;
            ASTNode elseBranchWrap = trueBranchWrap->brothers;

            int elseLab = ctx->nextLabelNum++;
            int endLab = ctx->nextLabelNum++;

            IrOperand condOp = generateExpressionIr(ctx, cond, typeCtx, REF_BOOL);

            if(elseBranchWrap){
                emitIfFalse(ctx, condOp, elseLab);
            }else {
                emitIfFalse(ctx, condOp, endLab);
            }

            generateStatementIr(ctx, trueBranchWrap->children, typeCtx, REF_VOID);
            if(elseBranchWrap){
                emitGoto(ctx, endLab);
                emitLabel(ctx, elseLab);
                generateStatementIr(ctx, elseBranchWrap->children, typeCtx, REF_VOID);
            }
            emitLabel(ctx, endLab);
            break;
        }

        case LOOP_STATEMENT: {
            ASTNode cond = node->children;
            ASTNode body = cond->brothers;

            int startLab = ctx->nextLabelNum++;
            int endLab = ctx->nextLabelNum++;

            emitLabel(ctx, startLab);

            IrOperand condOp = generateExpressionIr(ctx, cond, typeCtx, REF_BOOL);
            emitIfFalse(ctx, condOp, endLab);
            generateStatementIr(ctx, body, typeCtx, REF_VOID);
            emitGoto(ctx, startLab);
            emitLabel(ctx, endLab);

            break;
        }

        case RETURN_STATEMENT: {
            if (node->children && typeCtx->currentFunction->type != TYPE_STRUCT) {
                IrOperand retVal = generateExpressionIr(ctx, node->children, typeCtx, typeCtx->currentFunction->type);
                emitReturn(ctx, retVal);
            } else {
                IrOperand none = createNone();
                emitReturn(ctx, none);
            }
            break;
        }

        case EXPORTDEC: {
            if (node->children && node->children->nodeType == FUNCTION_DEFINITION) {
                generateFunctionIr(ctx, node->children, typeCtx, 1);
            }
            break;
        }

        case FUNCTION_DEFINITION: {
            generateFunctionIr(ctx, node, typeCtx, 0);
            break;
        }


        default: generateExpressionIr(ctx, node, typeCtx, expectedType);
    }
}

IrContext *generateIr(ASTNode ast, TypeCheckContext typeCtx){
    IrContext *ctx = createIrContext();
    if(!ctx)  return NULL;
    generateStatementIr(ctx, ast, typeCtx, REF_VOID);
    return ctx;
}

// printing stuff

static const char *opCodeToString(IrOpCode op) {
    switch (op) {
        case IR_ADD: return "ADD";
        case IR_SUB: return "SUB";
        case IR_MUL: return "MUL";
        case IR_DIV: return "DIV";
        case IR_MOD: return "MOD";
        case IR_NEG: return "NEG";
        case IR_BIT_AND: return "BIT_AND";
        case IR_BIT_OR: return "BIT_OR";
        case IR_BIT_XOR: return "BIT_XOR";
        case IR_BIT_NOT: return "BIT_NOT";
        case IR_SHL: return "SHL";
        case IR_SHR: return "SHR";
        case IR_AND: return "AND";
        case IR_OR: return "OR";
        case IR_NOT: return "NOT";
        case IR_EQ: return "EQ";
        case IR_NE: return "NE";
        case IR_LT: return "LT";
        case IR_LE: return "LE";
        case IR_GT: return "GT";
        case IR_GE: return "GE";
        case IR_COPY: return "COPY";
        case IR_STORE: return "STORE";
        case IR_LABEL: return "LABEL";
        case IR_GOTO: return "GOTO";
        case IR_IF_TRUE: return "IF_TRUE";
        case IR_IF_FALSE: return "IF_FALSE";
        case IR_PARAM: return "PARAM";
        case IR_CALL: return "CALL";
        case IR_RETURN: return "RETURN";
        case IR_RETURN_VOID: return "RETURN_VOID";
        case IR_NOP: return "NOP";
        case IR_FUNC_BEGIN: return "FUNC_BEGIN";
        case IR_FUNC_END: return "FUNC_END";
        case IR_CAST: return "CAST";
        case IR_POINTER_LOAD: return "PTRLD";
        case IR_POINTER_STORE: return "PTRST";
        case IR_REQ_MEM: return "REQMEM";
        case IR_DEREF: return "DEREF";
        case IR_ADDROF: return "ADDROF";
        case IR_LOAD_PARAM: return "LOAD_PARAM";
        case IR_MEMBER_LOAD: return "MEM_LOAD";
        case IR_MEMBER_STORE: return "MEM_STORE";
        case IR_ALLOC_STRUCT: return "ALLOC_STRUCT";
        default: return "UNKNOWN";
    }
}

static void printOperand(IrOperand op) {
    switch (op.type) {
        case OPERAND_TEMP:
            printf("t%d", op.value.temp.tempNum);
            break;
        case OPERAND_VAR:
            printf("%.*s", (int)op.value.var.nameLen, op.value.var.name);
            break;
        case OPERAND_CONSTANT:
            if (op.dataType == IR_TYPE_POINTER) {
                if (op.value.constant.intVal == 0) {
                    printf("null");
                } else {
                    printf("0x%lx", (unsigned long)op.value.constant.intVal);
                }
            } else if (op.dataType == IR_TYPE_I32 || op.dataType == IR_TYPE_BOOL) {
                printf("%ld", op.value.constant.intVal);
            } else if (op.dataType == IR_TYPE_STRING) {
                printf("%.*s", (int)op.value.constant.str.len, op.value.constant.str.stringVal);
            } else if (op.dataType == IR_TYPE_FLOAT) {
                printf("%g", op.value.constant.floatVal);
            } else {
                printf("%f", op.value.constant.doubleVal);
            }
            break;
        case OPERAND_LABEL:
            printf("L%d", op.value.label.labelNum);
            break;
        case OPERAND_FUNCTION:
            printf("%.*s", (int)op.value.fn.nameLen, op.value.fn.name);
            break;
        case OPERAND_NONE:
            printf("-");
            break;
    }
}

void printInstruction(IrInstruction *inst) {
    if (!inst) return;

    printf("%-12s ", opCodeToString(inst->op));

    if (inst->result.type != OPERAND_NONE) {
        printOperand(inst->result);
    }

    if (inst->ar1.type != OPERAND_NONE) {
        if(inst->result.type!=OPERAND_NONE){
            printf(", ");
        }
        printOperand(inst->ar1);
    }

    if (inst->ar2.type != OPERAND_NONE) {
        printf(", ");
        printOperand(inst->ar2);
    }
}

void printIR(IrContext *ctx) {
    if (!ctx) return;
    printf("Total instructions: %d\n", ctx->instructionCount);
    printf("Temporaries used: t1 - t%d\n", ctx->nextTempNum - 1);
    printf("Labels used: L1 - L%d\n\n", ctx->nextLabelNum - 1);

    IrInstruction *inst = ctx->instructions;
    int count = 0;

    while (inst) {
        printf("%4d: ", count++);
        printInstruction(inst);
        printf("\n");
        inst = inst->next;
    }
}
