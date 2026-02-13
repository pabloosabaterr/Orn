#ifndef MATHORN_H
#define MATHORN_H

void assertPass(const char *src);
void assertFail(const char *src);

void test_arithmetic_int(void);
void test_arithmetic_float(void);
void test_arithmetic_sub_div_mod(void);
void test_comparison_returns_bool(void);
void test_all_comparisons(void);
void test_logical_and_or(void);
void test_string_arithmetic_fails(void);
void test_ternary_expression(void);

#endif //MATHORN_H