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

void test_function_missing_return_value(void) {
    assertFail("fn needsValue() -> int { return; }");
}

void test_function_call_expression_args(void) {
    assertPass(
        "fn add(a: int, b: int) -> int { return a + b; }\n"
        "const result: int = add(1 + 2, 3 * 4);"
    );
}

void test_expected_arrow_fails(void) {
    assertFail("fn f() int { return 1; }");
}

void test_expected_return_fails(void) {
    assertFail("fn f() -> int { 1; }");
}

void test_expected_fn_fails(void) {
    assertFail("function f() -> int { return 1; }");
}

void test_expected_function_name_fails(void) {
    assertFail("fn (a: int) -> int { return a; }");
}

void test_expected_parameter_name_fails(void) {
    assertFail("fn f(: int) -> int { return 1; }");
}

void test_expected_comma_or_paren_fails(void) {
    assertFail("fn f(a: int b: int) -> int { return a; }");
}

void test_function_redefined_fails(void) {
    assertFail("fn f() -> int { return 1; } fn f() -> int { return 2; }");
}

void test_invalid_function_name_fails(void) {
    assertFail("fn 1f() -> int { return 1; }");
}

void test_duplicate_parameter_name_fails(void) {
    assertFail("fn f(a: int, a: int) -> int { return a; }");
}

void test_invalid_parameter_type_fails(void) {
    assertFail("fn f(a: nope) -> int { return 1; }");
}

void test_calling_non_function_fails(void) {
    assertFail("let x: int = 1; const y: int = x();");
}