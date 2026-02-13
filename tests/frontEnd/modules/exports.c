#include "../frontend.h"

void test_export_function(void) {
    assertPass("export fn add(a: int, b: int) -> int { return a + b; }");
}