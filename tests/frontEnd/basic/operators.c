#include "../frontend.h"

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