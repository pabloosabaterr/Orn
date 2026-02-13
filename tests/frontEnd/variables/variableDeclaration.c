#include "variable.h"
#include "unity.h"

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
    assertPass("let s: string = \"hello\";");
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