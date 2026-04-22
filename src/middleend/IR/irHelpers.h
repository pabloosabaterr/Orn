/**
 * @file irHelpers.h
 * @brief Utility functions for parsing literals and buffer comparison.
 *
 * Exposes parseInt(), parseFloat(), matchLit(), and bufferEqual()
 * used during IR generation to interpret AST node content.
 */

#ifndef IRHELPERS_H
#define IRHELPERS_H

#include <stddef.h>

int parseInt(const char *start, size_t len);
double parseFloat(const char *start, size_t len);

int matchLit(const char *start, size_t len, const char *lit);
int bufferEqual(const char *start1, size_t len1, const char *start2, size_t len2);

#endif
