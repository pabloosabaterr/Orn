#include "../frontend.h"

void test_pointer_declaration(void) {
    assertPass("let x: int = 42; let p: *int = &x;");
}

void test_pointer_level_mismatch_assignment_fails(void) {
    assertFail("let x: int = 42; let p: *int = &x; let pp: **int = p;");
}