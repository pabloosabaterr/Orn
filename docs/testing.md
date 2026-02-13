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

## Arithmetic
- `test_arithmetic_int`
- `test_arithmetic_float`
- `test_arithmetic_sub_div_mod`
- `test_string_arithmetic_fails`
- `test_array_assign_size_mismatch_fails`

## Assignment
- `test_increment`
- `test_decrement`
- `test_plus_assign`
- `test_minus_assign`
- `test_increment_bool_fails`
- `test_plus_assign_type_mismatch_fails`

## Casting
- `test_cast_int_to_float`
- `test_cast_float_to_int`
- `test_cast_string_to_int_fails`
- `test_cast_int_to_string_fails`

## Ternary
- `test_ternary_expression`
- `test_ternary_branch_type_mismatch_fails`

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
