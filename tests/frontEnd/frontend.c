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

void assertWarning(const char *src){
    resetErrorCount();
    TypeCheckContext ctx = compile(src);
    int hasWarning = (ctx != NULL) && (getWarningCount() > 0);
    TEST_ASSERT_TRUE_MESSAGE(hasWarning, "Expected compilation to emit a warning");
    if (ctx) freeTypeCheckContext(ctx);
}

int main() {
    UNITY_BEGIN();

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
    RUN_TEST(test_type_mismatch_float_to_string_fails);
    RUN_TEST(test_type_mismatch_string_to_float_fails);
    RUN_TEST(test_type_mismatch_string_to_bool_fails);
    RUN_TEST(test_type_mismatch_float_to_bool_fails);
    RUN_TEST(test_type_mismatch_int_to_bool_fails);
    RUN_TEST(test_type_mismatch_bool_to_string_fails);
    RUN_TEST(test_type_mismatch_bool_to_float_fails);
    RUN_TEST(test_type_mismatch_double_to_int_fails);
    RUN_TEST(test_type_mismatch_double_to_float_fails);
    RUN_TEST(test_type_mismatch_string_to_double_fails);
    RUN_TEST(test_type_mismatch_bool_to_double_fails);
    RUN_TEST(test_type_mismatch_double_to_bool_fails);
    RUN_TEST(test_type_mismatch_double_to_string_fails);
    RUN_TEST(test_variable_not_initialized_fails);
    RUN_TEST(test_invalid_variable_name_fails);
    RUN_TEST(test_undefined_symbol_fails);
    RUN_TEST(test_symbol_not_variable_fails);
    RUN_TEST(test_expected_type_fails);
    RUN_TEST(test_expected_identifier_fails);

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
    RUN_TEST(test_function_missing_return_value);
    RUN_TEST(test_function_call_expression_args);
    RUN_TEST(test_expected_arrow_fails);
    RUN_TEST(test_expected_return_fails);
    RUN_TEST(test_expected_fn_fails);
    RUN_TEST(test_expected_function_name_fails);
    RUN_TEST(test_expected_parameter_name_fails);
    RUN_TEST(test_expected_comma_or_paren_fails);
    RUN_TEST(test_function_redefined_fails);
    RUN_TEST(test_invalid_function_name_fails);
    RUN_TEST(test_invalid_parameter_type_fails);
    RUN_TEST(test_calling_non_function_fails);

    // Arithmetic expressions
    RUN_TEST(test_arithmetic_int);
    RUN_TEST(test_arithmetic_float);
    RUN_TEST(test_arithmetic_sub_div_mod);
    RUN_TEST(test_string_arithmetic_fails);
    RUN_TEST(test_array_assign_size_mismatch_fails);

    // Assignment expressions
    RUN_TEST(test_increment);
    RUN_TEST(test_decrement);
    RUN_TEST(test_plus_assign);
    RUN_TEST(test_minus_assign);
    RUN_TEST(test_increment_bool_fails);
    RUN_TEST(test_plus_assign_type_mismatch_fails);
    RUN_TEST(test_incompatible_binary_operands_fails);
    RUN_TEST(test_invalid_operation_for_type_fails);
    RUN_TEST(test_incompatible_operand_types_fails);
    RUN_TEST(test_invalid_unary_operand_fails);
    RUN_TEST(test_expression_type_unknown_lhs_fails);
    RUN_TEST(test_expression_type_unknown_rhs_fails);
    RUN_TEST(test_void_in_expression_fails);
    RUN_TEST(test_invalid_assignment_target_fails);

    // Casting expressions
    RUN_TEST(test_cast_int_to_float);
    RUN_TEST(test_cast_float_to_int);
    RUN_TEST(test_cast_string_to_int_fails);
    RUN_TEST(test_cast_int_to_string_fails);
    RUN_TEST(test_invalid_cast_target_fails);
    RUN_TEST(test_forbidden_cast_fails);
    RUN_TEST(test_cast_precision_loss_fails);

    // Ternary expressions
    RUN_TEST(test_ternary_expression);
    RUN_TEST(test_ternary_branch_type_mismatch_fails);
    RUN_TEST(test_ternary_missing_true_branch_fails);
    RUN_TEST(test_ternary_missing_false_branch_fails);
    RUN_TEST(test_ternary_invalid_condition_fails);
    RUN_TEST(test_expected_question_mark_fails);
    RUN_TEST(test_expected_colon_fails);

    // Control flow statements
    RUN_TEST(test_if_statement);
    RUN_TEST(test_if_else);
    RUN_TEST(test_while_loop);
    RUN_TEST(test_nested_if);

    // Scope tests
    RUN_TEST(test_block_scoping);
    RUN_TEST(test_inner_scope_accesses_outer);
    RUN_TEST(test_scope_variable_not_visible_outside_fails);

    // Pointers
    RUN_TEST(test_pointer_declaration);
    RUN_TEST(test_pointer_level_mismatch_assignment_fails);

    // Structs
    RUN_TEST(test_struct_definition);
    RUN_TEST(test_struct_with_multiple_types);
    RUN_TEST(test_struct_duplicate_fields_fails);

    // Module exports
    RUN_TEST(test_export_function);

    // Full program integration
    RUN_TEST(test_fibonacci_like);
    RUN_TEST(test_factorial_like);
    RUN_TEST(test_multi_function_program);
    RUN_TEST(test_mixed_types_program);

    // Arrays
    RUN_TEST(test_array_declaration);
    RUN_TEST(test_array_index_non_int_fails);
    RUN_TEST(test_array_index_out_of_bounds_const_fails);

    return UNITY_END();
}


