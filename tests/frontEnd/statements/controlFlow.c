#include "../frontend.h"

void test_if_statement(void) {
    assertPass(
        "let x: int = 5;\n"
        "if (x > 0) { x = 1; }"
    );
}

void test_if_else(void) {
    assertPass(
        "let x: int = 5;\n"
        "if (x > 0) { x = 1; } else { x = 0; }"
    );
}

void test_while_loop(void) {
    assertPass(
        "let x: int = 10;\n"
        "while (x > 0) { x = x - 1; }"
    );
}

void test_nested_if(void) {
    assertPass(
        "let x: int = 5;\n"
        "if (x > 0) { if (x < 10) { x = 1; } }"
    );
}