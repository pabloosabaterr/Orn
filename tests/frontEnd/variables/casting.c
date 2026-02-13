#include "variable.h"

void test_cast_int_to_float(void) {
    assertPass("let x: int = 42; let y: float = x as float;");
}

void test_cast_float_to_int(void) {
    assertPass("let x: float = 3.14f; let y: int = x as int;");
}