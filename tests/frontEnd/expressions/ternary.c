#include "../frontend.h"

void test_ternary_expression(void) {
    assertPass("let x: int = 5; const r: int = x > 0 ? 1 : 0;");
}

void test_ternary_branch_type_mismatch_fails(void) {
    assertFail("let b: bool = true; const x: int = b ? 1 : \"nope\";");
}

void test_ternary_missing_true_branch_fails(void) {
    assertFail("const x: int = true ? : 2;");
}

void test_ternary_missing_false_branch_fails(void) {
    assertFail("const x: int = true ? 1 : ;");
}

void test_ternary_invalid_condition_fails(void) {
    assertFail("const x: int = 1 ? 2 : 3;");
}

void test_expected_question_mark_fails(void) {
    assertFail("const x: int = true 1 : 2;");
}

void test_expected_colon_fails(void) {
    assertFail("const x: int = true ? 1 2;");
}