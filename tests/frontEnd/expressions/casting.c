#include "../frontend.h"

void test_cast_int_to_float(void) {
    assertPass("let x: int = 42; let y: float = x as float;");
}

void test_cast_float_to_int(void) {
    assertPass("let x: float = 3.14f; let y: int = x as int;");
}

void test_cast_string_to_int_fails(void) {
    assertFail("let s: string = \"42\"; let x: int = s as int;");
}

void test_cast_int_to_string_fails(void) {
    assertFail("let s: int = 42; let x: string = s as string;");
}

void test_invalid_cast_target_fails(void) {
    assertFail("const x: int = 1 as nope;");
}

void test_forbidden_cast_fails(void) {
    assertFail("const x: bool = \"true\" as bool;");
}

void test_cast_precision_loss_fails(void) {
    assertWarning("const x: int = 1.9f as int;");
}