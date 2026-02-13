#include "../frontend.h"

void test_increment(void) {
    assertPass("let x: int = 0; x++;");
}

void test_decrement(void) {
    assertPass("let x: int = 10; x--;");
}

void test_plus_assign(void) {
    assertPass("let x: int = 1; x += 5;");
}

void test_minus_assign(void) {
    assertPass("let x: int = 10; x -= 3;");
}

void test_increment_bool_fails(void) {
    assertFail("let b: bool = true; b++;");
}

void test_plus_assign_type_mismatch_fails(void) {
    assertFail("let x: int = 1; x += \"hello\";");
}