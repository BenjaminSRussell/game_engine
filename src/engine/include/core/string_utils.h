// include/core/string_utils.h
//
// Purpose: Provides a comprehensive set of utility functions for common string
// manipulation tasks. These functions enhance C-style string handling by offering
// capabilities like duplication, substring extraction, various comparisons,
// prefix/suffix checking, trimming whitespace, splitting strings by delimiters,
// converting strings to numeric types, formatting, case conversion, length calculation,
// and in-place character replacement.
//
// Public APIs:
// - `string_duplicate`: Creates a dynamically allocated copy of a string.
// - `string_substring`: Extracts a portion of a string.
// - `string_compare_case`: Performs a case-insensitive comparison of two strings.
// - `string_starts_with`, `string_ends_with`: Checks if a string begins or ends with a specific sequence.
// - `string_trim`: Returns a new string with leading/trailing whitespace removed.
// - `string_split`: Splits a string into an array of substrings based on a delimiter.
// - `string_split_free`: Frees memory allocated by `string_split`.
// - `string_to_int`, `string_to_float`: Converts string representations to integer or float values.
// - `string_format`: Formats a string using a `printf`-like syntax.
// - `string_to_lower`, `string_to_upper`: Converts a string to lowercase or uppercase in-place.
// - `string_length`: Calculates the length of a null-terminated string.
// - `string_replace`: Replaces all occurrences of one character with another within a string.
//
// Ownership: Functions that return `char*` (e.g., `string_duplicate`, `string_substring`, `string_trim`, `string_format`, `string_split`)
// typically allocate new memory that the caller is responsible for freeing.
// Functions that modify strings in-place (e.g., `string_to_lower`, `string_to_upper`, `string_replace`)
// require a mutable string buffer as input.
//
// Invariants:
// - Input strings to functions are generally expected to be null-terminated C strings.
// - Memory management for dynamically allocated strings returned by these functions is crucial to avoid leaks.
// - `string_split` and `string_split_free` must be used in tandem.
//
#ifndef STRING_UTILS_H
#define STRING_UTILS_H


#include "include/common.h"

char *string_duplicate(const char *str);
char *string_substring(const char *str, u32 start, u32 length);
int string_compare_case(const char *a, const char *b);
bool string_starts_with(const char *str, const char *prefix);
bool string_ends_with(const char *str, const char *suffix);
char *string_trim(const char *str);
char **string_split(const char *str, const char *delim, u32 *count);
void string_split_free(char **parts, u32 count);
bool string_to_int(const char *str, int *out);
bool string_to_float(const char *str, f32 *out);
char *string_format(const char *format, ...);
void string_to_lower(char *str);
void string_to_upper(char *str);
u32 string_length(const char *str);
void string_replace(char *str, char from, char to);

#endif
