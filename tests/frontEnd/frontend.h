#ifndef FRONTEND_H
#define FRONTEND_H

#include "typeChecker.h"

// Setup and teardown
void setUp(void);
void tearDown(void);

// Helper functions
TypeCheckContext compile(const char *src);
void assertPass(const char *src);
void assertFail(const char *src);

// Operators
void test_comparison_returns_bool(void);
void test_all_comparisons(void);
void test_logical_and_or(void);

// Variables
void test_const_int(void);
void test_let_float(void);
void test_let_double(void);
void test_let_bool(void);
void test_let_string(void);
void test_const_without_init_fails(void);
void test_type_mismatch_int_string(void);
void test_type_mismatch_bool_int(void);
void test_assign_to_const(void);
void test_undefined_variable(void);
void test_duplicate_variable(void);
void test_let_reassignment(void);

// Functions
void test_function_basic(void);
void test_function_void(void);
void test_function_call(void);
void test_function_wrong_arg_count(void);
void test_function_wrong_arg_type(void);
void test_return_type_mismatch(void);
void test_void_return_with_value(void);
void test_undefined_function(void);
void test_function_uses_params(void);
void test_function_multiple_calls(void);
void test_function_missing_return_value(void);
void test_function_call_expression_args(void);

// Arithmetic
void test_arithmetic_int(void);
void test_arithmetic_float(void);
void test_arithmetic_sub_div_mod(void);
void test_string_arithmetic_fails(void);
void test_array_assign_size_mismatch_fails(void);

// Assignment
void test_increment(void);
void test_decrement(void);
void test_plus_assign(void);
void test_minus_assign(void);
void test_increment_bool_fails(void);
void test_plus_assign_type_mismatch_fails(void);

// Casting
void test_cast_int_to_float(void);
void test_cast_float_to_int(void);
void test_cast_string_to_int_fails(void);
void test_cast_int_to_string_fails(void);

// Ternary
void test_ternary_expression(void);
void test_ternary_branch_type_mismatch_fails(void);

// Control flow
void test_if_statement(void);
void test_if_else(void);
void test_while_loop(void);
void test_nested_if(void);

// Scope
void test_block_scoping(void);
void test_inner_scope_accesses_outer(void);
void test_scope_variable_not_visible_outside_fails(void);

// Arrays
void test_array_declaration(void);
void test_array_index_non_int_fails(void);
void test_array_index_out_of_bounds_const_fails(void);

// Pointers
void test_pointer_declaration(void);
void test_pointer_level_mismatch_assignment_fails(void);

// Structs
void test_struct_definition(void);
void test_struct_with_multiple_types(void);
void test_struct_duplicate_fields_fails(void);

// Modules
void test_export_function(void);

// Integration programs
void test_fibonacci_like(void);
void test_factorial_like(void);
void test_multi_function_program(void);
void test_mixed_types_program(void);

#endif // FRONTEND_H