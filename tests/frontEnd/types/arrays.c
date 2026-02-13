#include "../frontend.h"

void test_array_declaration(void) {
    assertPass("let arr: int[5];");
}

void test_array_index_non_int_fails(void) {
    assertFail("let arr: int[3]; let idx: float = 1.0f; let x: int = arr[idx];");
}

void test_array_index_out_of_bounds_const_fails(void) {
    assertFail("let arr: int[2]; const idx: int = 2; let x: int = arr[idx];");
}