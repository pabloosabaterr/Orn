#include "../frontend.h"

void test_fibonacci_like(void) {
    assertPass(
        "fn fib(n: int) -> int {\n"
        "    if (n <= 1) { return n; };\n"
        "    return fib(n - 1) + fib(n - 2);\n"
        "}"
    );
}

void test_factorial_like(void) {
    assertPass(
        "fn fact(n: int) -> int {\n"
        "    if (n <= 1) { return 1; };\n"
        "    return n * fact(n - 1);\n"
        "}"
    );
}

void test_multi_function_program(void) {
    assertPass(
        "fn double(x: int) -> int { return x * 2; }\n"
        "fn triple(x: int) -> int { return x * 3; }\n"
        "const a: int = double(5);\n"
        "const b: int = triple(a);"
    );
}

void test_mixed_types_program(void) {
    assertPass(
        "let i: int = 10;\n"
        "let f: float = 3.14f;\n"
        "let b: bool = true;\n"
        "let s: string = \"hello\";\n"
        "if (b) { i = i + 1; }"
    );
}