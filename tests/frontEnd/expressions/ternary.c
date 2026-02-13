#include "../frontend.h"

void test_ternary_expression(void) {
    assertPass("let x: int = 5; const r: int = x > 0 ? 1 : 0;");
}