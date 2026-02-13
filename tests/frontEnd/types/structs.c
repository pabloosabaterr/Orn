#include "../frontend.h"

void test_struct_definition(void) {
    assertPass("struct Point { x: int; y: int; }");
}

void test_struct_with_multiple_types(void) {
    assertPass("struct Entity { name: string; x: float; y: float; alive: bool; }");
}

void test_struct_duplicate_fields_fails(void) {
    assertFail("struct Bad { x: int; x: int; }");
}