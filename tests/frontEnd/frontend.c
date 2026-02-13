#include "unity.h"
#include "lexer.h"
#include "parser.h"
#include "typeChecker.h"
#include "frontend.h"

void setUp(void) { resetErrorCount(); setSilentMode(1); }
void tearDown(void) {setSilentMode(0);}

TypeCheckContext compile(const char *src) {
    TokenList *tokens = lex(src, "test");
    if (!tokens) return NULL;
    ASTContext *ast = ASTGenerator(tokens);
    if (!ast || !ast->root) return NULL;
    return typeCheckAST(ast->root, src, "test", NULL);
}

void assertPass(const char *src) {
    resetErrorCount();
    TypeCheckContext ctx = compile(src);
    TEST_ASSERT_NOT_NULL_MESSAGE(ctx, "Compilation returned NULL");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, getErrorCount(), "Expected no errors");
    freeTypeCheckContext(ctx);
}

void assertFail(const char *src) {
    resetErrorCount();
    TypeCheckContext ctx = compile(src);
    int failed = (ctx == NULL) || (getErrorCount() > 0);
    TEST_ASSERT_TRUE_MESSAGE(failed, "Expected compilation to fail");
    if (ctx) freeTypeCheckContext(ctx);
}

int main() {
    UNITY_BEGIN();

    // Basic operators
    RUN_TEST(test_comparison_returns_bool);
    RUN_TEST(test_all_comparisons);
    RUN_TEST(test_logical_and_or);

    // Variable declarations
    RUN_TEST(test_const_int);
    RUN_TEST(test_let_float);
    RUN_TEST(test_let_double);
    RUN_TEST(test_let_bool);
    RUN_TEST(test_let_string);
    RUN_TEST(test_const_without_init_fails);
    RUN_TEST(test_type_mismatch_int_string);
    RUN_TEST(test_type_mismatch_bool_int);
    RUN_TEST(test_assign_to_const);
    RUN_TEST(test_undefined_variable);
    RUN_TEST(test_duplicate_variable);
    RUN_TEST(test_let_reassignment);

    // Functions
    RUN_TEST(test_function_basic);
    RUN_TEST(test_function_void);
    RUN_TEST(test_function_call);
    RUN_TEST(test_function_wrong_arg_count);
    RUN_TEST(test_function_wrong_arg_type);
    RUN_TEST(test_return_type_mismatch);
    RUN_TEST(test_void_return_with_value);
    RUN_TEST(test_undefined_function);
    RUN_TEST(test_function_uses_params);
    RUN_TEST(test_function_multiple_calls);

    // Arithmetic expressions
    RUN_TEST(test_arithmetic_int);
    RUN_TEST(test_arithmetic_float);
    RUN_TEST(test_arithmetic_sub_div_mod);
    RUN_TEST(test_string_arithmetic_fails);

    // Assignment expressions
    RUN_TEST(test_increment);
    RUN_TEST(test_decrement);
    RUN_TEST(test_plus_assign);
    RUN_TEST(test_minus_assign);

    // Casting expressions
    RUN_TEST(test_cast_int_to_float);
    RUN_TEST(test_cast_float_to_int);

    // Ternary expressions
    RUN_TEST(test_ternary_expression);

    // Control flow statements
    RUN_TEST(test_if_statement);
    RUN_TEST(test_if_else);
    RUN_TEST(test_while_loop);
    RUN_TEST(test_nested_if);

    // Scope tests
    RUN_TEST(test_block_scoping);
    RUN_TEST(test_inner_scope_accesses_outer);

    // Pointers
    RUN_TEST(test_pointer_declaration);

    // Structs
    RUN_TEST(test_struct_definition);
    RUN_TEST(test_struct_with_multiple_types);

    // Module exports
    RUN_TEST(test_export_function);

    // Full program integration
    RUN_TEST(test_fibonacci_like);
    RUN_TEST(test_factorial_like);
    RUN_TEST(test_multi_function_program);
    RUN_TEST(test_mixed_types_program);

    // Arrays
    RUN_TEST(test_array_declaration);

    return UNITY_END();
}


