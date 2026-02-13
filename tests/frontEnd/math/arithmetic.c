#include "mathOrn.h"

void test_arithmetic_int(void) {
    assertPass("const x: int = 1 + 2 * 3;");
}

void test_arithmetic_float(void) {
    assertPass("const x: float = 1.0f + 2.5f;");
}

void test_arithmetic_sub_div_mod(void) {
    assertPass("const a: int = 10 - 3; const b: int = 10 / 2; const c: int = 10 % 3;");
}

void test_comparison_returns_bool(void) {
    assertPass("const b: bool = 1 < 2;");
}

void test_all_comparisons(void) {
    assertPass(
        "const a: bool = 1 < 2;\n"
        "const b: bool = 1 > 2;\n"
        "const c: bool = 1 <= 2;\n"
        "const d: bool = 1 >= 2;\n"
        "const e: bool = 1 == 2;\n"
        "const f: bool = 1 != 2;"
    );
}

void test_logical_and_or(void) {
    assertPass("const a: bool = true && false;");
    assertPass("const b: bool = true || false;");
}

void test_string_arithmetic_fails(void) {
    assertFail("const x: int = \"a\" + \"b\";");
}

void test_ternary_expression(void) {
    assertPass("let x: int = 5; const r: int = x > 0 ? 1 : 0;");
}