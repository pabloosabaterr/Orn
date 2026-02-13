#include "unity.h"
#include "parser.h"
#include "typeChecker.h"
#include "variable.h"
#include "mathOrn.h"
#include "functions.h"

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

int main(){
    UNITY_BEGIN();

    // variable

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

    //casting
    RUN_TEST(test_cast_int_to_float);
    RUN_TEST(test_cast_float_to_int);

    // math

    RUN_TEST(test_arithmetic_int);
    RUN_TEST(test_arithmetic_float);
    RUN_TEST(test_arithmetic_sub_div_mod);
    RUN_TEST(test_comparison_returns_bool);
    RUN_TEST(test_all_comparisons);
    RUN_TEST(test_logical_and_or);
    RUN_TEST(test_string_arithmetic_fails);
    RUN_TEST(test_ternary_expression);

    // functions
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

    return UNITY_END();
}

