#ifndef VARIABLE_H
#define VARIABLE_H

void assertPass(const char *src);
void assertFail(const char *src);

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
void test_cast_int_to_float(void);
void test_cast_float_to_int(void);

#endif //VARIABLE_H
