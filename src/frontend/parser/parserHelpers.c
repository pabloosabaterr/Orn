/**
 * @file parserHelpers.c
 * @brief Operator tables and lookup utilities for the Pratt parser.
 *
 * Responsibilities:
 *   - Operator precedence / associativity table (operators[])
 *   - getOperatorInfo(): binary operator lookup
 *   - getUnaryOpType(): prefix-operator mapping
 *
 */

#include "parserInternal.h"

static const OperatorInfo operators[] = {
    /* Assignment (right-associative) */
    {TK_ASSIGN, ASSIGNMENT, PREC_ASSIGN, 1},
    {TK_PLUS_ASSIGN, COMPOUND_ADD_ASSIGN, PREC_ASSIGN, 1},
    {TK_MINUS_ASSIGN, COMPOUND_SUB_ASSIGN, PREC_ASSIGN, 1},
    {TK_STAR_ASSIGN, COMPOUND_MUL_ASSIGN, PREC_ASSIGN, 1},
    {TK_SLASH_ASSIGN, COMPOUND_DIV_ASSIGN, PREC_ASSIGN, 1},
    {TK_AND_ASSIGN, COMPOUND_AND_ASSIGN, PREC_ASSIGN, 1},
    {TK_OR_ASSIGN, COMPOUND_OR_ASSIGN, PREC_ASSIGN, 1},
    {TK_XOR_ASSIGN, COMPOUND_XOR_ASSIGN, PREC_ASSIGN, 1},
    {TK_LSHIFT_ASSIGN, COMPOUND_LSHIFT_ASSIGN, PREC_ASSIGN, 1},
    {TK_RSHIFT_ASSIGN, COMPOUND_RSHIFT_ASSIGN, PREC_ASSIGN, 1},

    /* Logical */
    {TK_OR, LOGIC_OR, PREC_OR, 0},
    {TK_AND, LOGIC_AND, PREC_AND, 0},

    /* Bitwise */
    {TK_BIT_OR, BITWISE_OR, PREC_BITWISE_OR, 0},
    {TK_BIT_XOR, BITWISE_XOR, PREC_BITWISE_XOR, 0},
    {TK_AMPERSAND, BITWISE_AND, PREC_BITWISE_AND, 0},
    {TK_LSHIFT, BITWISE_LSHIFT, PREC_SHIFT, 0},
    {TK_RSHIFT, BITWISE_RSHIFT, PREC_SHIFT, 0},

    /* Equality / comparison */
    {TK_EQ, EQUAL_OP, PREC_EQUALITY, 0},
    {TK_NOT_EQ, NOT_EQUAL_OP, PREC_EQUALITY, 0},
    {TK_LESS, LESS_THAN_OP, PREC_COMPARISON, 0},
    {TK_GREATER, GREATER_THAN_OP, PREC_COMPARISON, 0},
    {TK_LESS_EQ, LESS_EQUAL_OP, PREC_COMPARISON, 0},
    {TK_GREATER_EQ, GREATER_EQUAL_OP, PREC_COMPARISON, 0},

    /* Arithmetic */
    {TK_PLUS, ADD_OP, PREC_TERM, 0},
    {TK_MINUS, SUB_OP, PREC_TERM, 0},
    {TK_STAR, MUL_OP, PREC_FACTOR, 0},
    {TK_SLASH, DIV_OP, PREC_FACTOR, 0},
    {TK_MOD, MOD_OP, PREC_FACTOR, 0},

    /* Cast */
    {TK_AS, CAST_EXPRESSION, PREC_CAST, 0},

    /* Sentinel */
    {TK_NULL, null_NODE, PREC_NONE, 0}
};

const OperatorInfo *getOperatorInfo(TokenType type) {
    for (int i = 0; operators[i].token != TK_NULL; i++) {
        if (operators[i].token == type) return &operators[i];
    }
    return NULL;
}

NodeTypes getUnaryOpType(TokenType t) {
    switch (t) {
    case TK_MINUS:
        return UNARY_MINUS_OP;
    case TK_PLUS:
        return UNARY_PLUS_OP;
    case TK_NOT:
        return LOGIC_NOT;
    case TK_BIT_NOT:
        return BITWISE_NOT;
    case TK_INCR:
        return PRE_INCREMENT;
    case TK_DECR:
        return PRE_DECREMENT;
    default:
        return null_NODE;
    }
}