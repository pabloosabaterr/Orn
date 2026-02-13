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