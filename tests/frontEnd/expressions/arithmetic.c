#include "../frontend.h"

void test_arithmetic_int(void) {
    assertPass("const x: int = 1 + 2 * 3;");
}

void test_arithmetic_float(void) {
    assertPass("const x: float = 1.0f + 2.5f;");
}

void test_arithmetic_sub_div_mod(void) {
    assertPass("const a: int = 10 - 3; const b: int = 10 / 2; const c: int = 10 % 3;");
}

void test_string_arithmetic_fails(void) {
    assertFail("const x: int = \"a\" + \"b\";");
}

void test_array_assign_size_mismatch_fails(void) {
    assertFail("let a: int[2]; let b: int[3]; a = b;");
}

void test_incompatible_binary_operands_fails(void) {
    assertFail("const x: int = 1 + \"a\";");
}

void test_invalid_operation_for_type_fails(void) {
    assertFail("const x: bool = true - false;");
}

void test_incompatible_operand_types_fails(void) {
    assertFail("const x: int = 1 < \"a\";");
}

void test_invalid_unary_operand_fails(void) {
    assertFail("const x: int = -\"a\";");
}

void test_expression_type_unknown_lhs_fails(void) {
    assertFail("const x: int = unknown + 1;");
}

void test_expression_type_unknown_rhs_fails(void) {
    assertFail("const x: int = 1 + unknown;");
}

void test_void_in_expression_fails(void) {
    assertFail("fn f() -> void { } const x: int = f();");
}