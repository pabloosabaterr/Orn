#include "../frontend.h"

void test_ternary_expression(void) {
    assertPass("let x: int = 5; const r: int = x > 0 ? 1 : 0;");
}

void test_ternary_branch_type_mismatch_fails(void) {
    assertFail("let b: bool = true; const x: int = b ? 1 : \"nope\";");
}