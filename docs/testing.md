# Testing

Current tests looked up are:

## frontend

## Operators
- `test_comparison_returns_bool`
- `test_all_comparisons`
- `test_logical_and_or`

## Variables
- `test_const_int`
- `test_let_float`
- `test_let_double`
- `test_let_bool`
- `test_let_string`
- `test_const_without_init_fails`
- `test_type_mismatch_int_string`
- `test_type_mismatch_bool_int`
- `test_assign_to_const`
- `test_undefined_variable`
- `test_duplicate_variable`
- `test_let_reassignment`
- `test_type_mismatch_float_to_string_fails`
- `test_type_mismatch_string_to_float_fails`
- `test_type_mismatch_string_to_bool_fails`
- `test_type_mismatch_float_to_bool_fails`
- `test_type_mismatch_int_to_bool_fails`
- `test_type_mismatch_bool_to_string_fails`
- `test_type_mismatch_bool_to_float_fails`
- `test_type_mismatch_double_to_int_fails`
- `test_type_mismatch_double_to_float_fails`
- `test_type_mismatch_string_to_double_fails`
- `test_type_mismatch_bool_to_double_fails`
- `test_type_mismatch_double_to_bool_fails`
- `test_type_mismatch_double_to_string_fails`
- `test_variable_not_initialized_fails`
- `test_invalid_variable_name_fails`
- `test_undefined_symbol_fails`
- `test_symbol_not_variable_fails`
- `test_expected_type_fails`
- `test_expected_identifier_fails`

## Functions
- `test_function_basic`
- `test_function_void`
- `test_function_call`
- `test_function_wrong_arg_count`
- `test_function_wrong_arg_type`
- `test_return_type_mismatch`
- `test_void_return_with_value`
- `test_undefined_function`
- `test_function_uses_params`
- `test_function_multiple_calls`
- `test_function_missing_return_value`
- `test_function_call_expression_args`
- `test_expected_arrow_fails`
- `test_expected_return_fails`
- `test_expected_fn_fails`
- `test_expected_function_name_fails`
- `test_expected_parameter_name_fails`
- `test_expected_comma_or_paren_fails`
- `test_function_redefined_fails`
- `test_invalid_function_name_fails`
- `test_invalid_parameter_type_fails`
- `test_calling_non_function_fails`

## Arithmetic
- `test_arithmetic_int`
- `test_arithmetic_float`
- `test_arithmetic_sub_div_mod`
- `test_string_arithmetic_fails`
- `test_array_assign_size_mismatch_fails`
- `test_incompatible_binary_operands_fails`
- `test_invalid_operation_for_type_fails`
- `test_incompatible_operand_types_fails`
- `test_invalid_unary_operand_fails`
- `test_expression_type_unknown_lhs_fails`
- `test_expression_type_unknown_rhs_fails`
- `test_void_in_expression_fails`

## Assignment
- `test_increment`
- `test_decrement`
- `test_plus_assign`
- `test_minus_assign`
- `test_increment_bool_fails`
- `test_plus_assign_type_mismatch_fails`
- `test_invalid_assignment_target_fails`

## Casting
- `test_cast_int_to_float`
- `test_cast_float_to_int`
- `test_cast_string_to_int_fails`
- `test_cast_int_to_string_fails`
- `test_invalid_cast_target_fails`
- `test_forbidden_cast_fails`
- `test_cast_precision_loss_fails`

## Ternary
- `test_ternary_expression`
- `test_ternary_branch_type_mismatch_fails`
- `test_ternary_missing_true_branch_fails`
- `test_ternary_missing_false_branch_fails`
- `test_ternary_invalid_condition_fails`
- `test_expected_question_mark_fails`
- `test_expected_colon_fails`

## Control flow
- `test_if_statement`
- `test_if_else`
- `test_while_loop`
- `test_nested_if`

## Scope
- `test_block_scoping`
- `test_inner_scope_accesses_outer`
- `test_scope_variable_not_visible_outside_fails`

## Arrays
- `test_array_declaration`
- `test_array_index_non_int_fails`
- `test_array_index_out_of_bounds_const_fails`

## Pointers
- `test_pointer_declaration`
- `test_pointer_level_mismatch_assignment_fails`

## Structs
- `test_struct_definition`
- `test_struct_with_multiple_types`
- `test_struct_duplicate_fields_fails`

## Modules
- `test_export_function`

## Integration programs
- `test_fibonacci_like`
- `test_factorial_like`
- `test_multi_function_program`
- `test_mixed_types_program`

Test files are under `tests/frontEnd/`.