#include "../frontend.h"

void test_array_declaration(void) {
    assertPass("let arr: int[5];");
}