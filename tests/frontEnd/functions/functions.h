#ifndef FUNCTIONS_H
#define FUNCTIONS_H

void assertPass(const char *src);
void assertFail(const char *src);

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

#endif //FUNCTIONS_H