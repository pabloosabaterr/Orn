#include "../frontend.h"

void test_pointer_declaration(void) {
    assertPass("let x: int = 42; let p: *int = &x;");
}