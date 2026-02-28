#include <stddef.h>
#include <stdint.h>
#include "semantic.h"


typedef enum {
    IR_ADD,
    IR_SUB,
    IR_DIV,
    IR_MOD,
    IR_NEG,
    IR_MUL,

    IR_STRING_INIT,

    IR_BIT_AND,
    IR_BIT_OR,
    IR_BIT_XOR,
    IR_BIT_NOT,
    IR_SHL,
    IR_SHR,
    
    IR_AND,
    IR_OR,
    IR_NOT,

    IR_EQ,
    IR_NE,
    IR_LT,
    IR_LE,
    IR_GT,
    IR_GE,

    IR_COPY,
    IR_LOAD_PARAM,

    IR_REQ_MEM,
    IR_POINTER_LOAD,
    IR_POINTER_STORE,
    IR_ADDROF,
    IR_DEREF,
    IR_STORE,

    IR_ALLOC_STRUCT,
    IR_MEMBER_LOAD,
    IR_MEMBER_STORE,

    IR_LABEL,
    IR_GOTO,
    IR_IF_TRUE,
    IR_IF_FALSE,

    IR_PARAM,
    IR_CALL,
    IR_RETURN,
    IR_RETURN_VOID,

    IR_NOP,
    IR_FUNC_BEGIN,
    IR_FUNC_END,

    IR_CAST
} IrOpCode;

typedef struct {
    const char *baseName;
    size_t baseNameLen;
    int totalOffset;
    StructType finalStructType;
    DataType fieldType;
    int baseIsTemp; // comes from a temp variable e.g.: auto dereference of a struct pointer
    int baseTempNum;
} MemberAccessInfo;

typedef enum {
    OPERAND_NONE,
    OPERAND_TEMP,
    OPERAND_VAR,        
    OPERAND_CONSTANT,    
    OPERAND_LABEL,       
    OPERAND_FUNCTION,
} OperandType;

typedef enum {
    IR_TYPE_I8,
    IR_TYPE_I16,
    IR_TYPE_I32,
    IR_TYPE_I64,
    IR_TYPE_U8,
    IR_TYPE_U16,
    IR_TYPE_U32,
    IR_TYPE_U64,
    IR_TYPE_FLOAT,
    IR_TYPE_DOUBLE,
    IR_TYPE_BOOL,
    IR_TYPE_STRING,
    IR_TYPE_VOID,
    IR_TYPE_POINTER
} IrDataType;

typedef struct IrOperand {
    OperandType type;
    IrDataType dataType;
    union {
        struct{
            int tempNum;
        } temp;
        struct {
            const char *name;
            size_t nameLen;
        } var;
        struct {
            union {
                int64_t intVal;
                float floatVal;
                double doubleVal;
                struct {
                    const char *stringVal;
                    size_t len;
                } str;
            };
        } constant;
        struct {
            char* start;
            size_t len;
            int off;
        } mem;
        struct {
            int labelNum;
        } label;
        struct {
            const char *name;
            size_t nameLen;
        } fn;
    } value;
} IrOperand;

typedef struct IrInstruction{
    IrOpCode op;
    IrOperand result;
    IrOperand ar1;
    IrOperand ar2;
    struct IrInstruction *next;
    struct IrInstruction *prev;
} IrInstruction;

typedef struct IrContext {
    IrInstruction *instructions;
    IrInstruction *lastInstruction;         
    int instructionCount;
    
    int nextTempNum;                        
    int nextLabelNum;                      
    
    struct JumpPatch {
        IrInstruction *instruction;         
        int targetLabel;                    
        struct JumpPatch *next;
    } *pendingJumps;
    
} IrContext;

IrContext *createIrContext();
void freeIrContext(IrContext *ctx);

IrOperand createTemp(IrContext *ctx, IrDataType type);
IrOperand createVar(const char *name, size_t len, IrDataType type);
IrOperand createConst(IrDataType type);
IrOperand createSizedIntConst(int64_t val, IrDataType type);
IrOperand createFloatConst(float val);
IrOperand createDoubleConst(double val);
IrOperand createBoolConst(int val);
IrOperand createStringConst(const char* val, size_t len);
IrOperand createLabel(int label);
IrOperand createNone();

void appendInstruction(IrContext *ctx, IrInstruction *inst);
IrInstruction *emitBinary(IrContext *ctx, IrOpCode op, IrOperand res, IrOperand ar1, IrOperand ar2);
IrInstruction *emitUnary(IrContext *ctx, IrOpCode op, IrOperand res, IrOperand ar1);
IrInstruction *emitCopy(IrContext *ctx, IrOperand res, IrOperand ar1);
IrInstruction *emitLabel(IrContext *ctx, int lab);
IrInstruction *emitGoto(IrContext *ctx, int lab);
IrInstruction *emitIfFalse(IrContext *ctx, IrOperand cond, int lab);
IrInstruction *emitReturn(IrContext *ctx, IrOperand ret);
IrInstruction *emitCall(IrContext *ctx, IrOperand res, const char *fnName, size_t nameLen, int params);

IrDataType symbolTypeToIrType(DataType type);
IrDataType nodeTypeToIrType(NodeTypes nodeType);
IrOpCode astOpToIrOp(NodeTypes nodeType);

IrOperand generateExpressionIr(IrContext *ctx, ASTNode node, TypeCheckContext typeCtx, DataType expectedType);
void generateStatementIr(IrContext *ctx, ASTNode node, TypeCheckContext typeCtx, DataType expectedType);
IrContext *generateIr(ASTNode ast, TypeCheckContext typeCtx);

void printInstruction(IrInstruction *inst);
void printIR(IrContext *ctx);