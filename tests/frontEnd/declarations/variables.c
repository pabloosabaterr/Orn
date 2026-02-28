#include "../frontend.h"

void test_const_int(void) {
    assertPass("const x: int = 42;");
}

void test_let_float(void) {
    assertPass("let y: float = 3.14f;");
}

void test_let_double(void) {
    assertPass("let d: double = 3.14;");
}

void test_let_bool(void) {
    assertPass("let b: bool = true;");
}

void test_let_string(void) {
    assertPass("let s: str = \"hello\";");
}

void test_const_without_init_fails(void) {
    assertFail("const x: int;");
}

void test_type_mismatch_int_string(void) {
    assertFail("const x: int = \"hello\";");
}

void test_type_mismatch_bool_int(void) {
    assertFail("const b: bool = 42;");
}

void test_assign_to_const(void) {
    assertFail("const x: int = 1; x = 2;");
}

void test_undefined_variable(void) {
    assertFail("let x: int = y;");
}

void test_duplicate_variable(void) {
    assertFail("let x: int = 1; let x: int = 2;");
}

void test_let_reassignment(void) {
    assertPass("let x: int = 1; x = 2;");
}

void test_type_mismatch_float_to_string_fails(void) {
    assertFail("const s: str = 1.5f;");
}

void test_type_mismatch_string_to_float_fails(void) {
    assertFail("const f: float = \"1.5\";");
}

void test_type_mismatch_string_to_bool_fails(void) {
    assertFail("const b: bool = \"true\";");
}

void test_type_mismatch_float_to_bool_fails(void) {
    assertFail("const b: bool = 0.0f;");
}

void test_type_mismatch_int_to_bool_fails(void) {
    assertFail("const b: bool = 1;");
}

void test_type_mismatch_bool_to_string_fails(void) {
    assertFail("const s: str = false;");
}

void test_type_mismatch_bool_to_float_fails(void) {
    assertFail("const f: float = true;");
}

void test_type_mismatch_double_to_int_fails(void) {
    assertFail("const x: int = 3.14;");
}

void test_type_mismatch_double_to_float_fails(void) {
    assertWarning("const f: float = 3.14;");
}

void test_type_mismatch_string_to_double_fails(void) {
    assertFail("const d: double = \"3.14\";");
}

void test_type_mismatch_bool_to_double_fails(void) {
    assertFail("const d: double = true;");
}

void test_type_mismatch_double_to_bool_fails(void) {
    assertFail("const b: bool = 3.14;");
}

void test_type_mismatch_double_to_string_fails(void) {
    assertFail("const s: string = 3.14;");
}

void test_variable_not_initialized_fails(void) {
    assertFail("const x: int;");
}

void test_invalid_variable_name_fails(void) {
    assertFail("let 1x: int = 1;");
}

void test_undefined_symbol_fails(void) {
    assertFail("const x: int = missing;");
}

void test_symbol_not_variable_fails(void) {
    assertFail("fn foo() -> int { return 1; } foo = 3;");
}

void test_expected_type_fails(void) {
    assertFail("let x = 1;");
}

void test_expected_identifier_fails(void) {
    assertFail("let : int = 1;");
}