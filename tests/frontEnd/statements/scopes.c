#include "../frontend.h"

void test_block_scoping(void) {
    assertPass(
        "let x: int = 1;\n"
        "if (true) { let y: int = x + 1; }"
    );
}

void test_inner_scope_accesses_outer(void) {
    assertPass(
        "let x: int = 10;\n"
        "if (x > 0) { let y: int = x * 2; }"
    );
}