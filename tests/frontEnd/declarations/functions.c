#include "../frontend.h"

void test_function_basic(void) {
    assertPass("fn add(a: int, b: int) -> int { return a + b; }");
}

void test_function_void(void) {
    assertPass("fn doNothing() -> void { }");
}

void test_function_call(void) {
    assertPass(
        "fn add(a: int, b: int) -> int { return a + b; }\n"
        "const result: int = add(1, 2);"
    );
}

void test_function_wrong_arg_count(void) {
    assertFail(
        "fn add(a: int, b: int) -> int { return a + b; }\n"
        "const result: int = add(1);"
    );
}

void test_function_wrong_arg_type(void) {
    assertFail(
        "fn add(a: int, b: int) -> int { return a + b; }\n"
        "const result: int = add(1, \"hello\");"
    );
}

void test_return_type_mismatch(void) {
    assertFail("fn getNum() -> int { return \"not a number\"; }");
}

void test_void_return_with_value(void) {
    assertFail("fn doNothing() -> void { return 42; }");
}

void test_undefined_function(void) {
    assertFail("const x: int = unknown(1);");
}

void test_function_uses_params(void) {
    assertPass(
        "fn square(n: int) -> int { return n * n; }\n"
        "const r: int = square(5);"
    );
}

void test_function_multiple_calls(void) {
    assertPass(
        "fn add(a: int, b: int) -> int { return a + b; }\n"
        "const x: int = add(1, 2);\n"
        "const y: int = add(x, 3);"
    );
}