/**
 * Unit Test for String Utilities
 * Tests string manipulation and utility functions
 */

#include "../../../src/engine/core/string_utils.h"
#include "../../../tests/test_framework_unified.h"
#include <string.h>
#include <stdio.h>

// Test string duplication
static TestResult test_string_duplicate(void) {
    const char* original = "Hello, World!";
    char* copy = string_duplicate(original);
    
    TEST_ASSERT_NOT_NULL(copy, "String duplicated");
    TEST_ASSERT_STRING_EQ(copy, original, "Strings match");
    TEST_ASSERT(copy != original, "Different memory addresses");
    
    free(copy);
    return TEST_PASS;
}

// Test string comparison (case insensitive)
static TestResult test_string_compare_case_insensitive(void) {
    TEST_ASSERT(string_compare_ci("Hello", "hello") == 0, "Case insensitive comparison");
    TEST_ASSERT(string_compare_ci("ABC", "abc") == 0, "Uppercase vs lowercase");
    TEST_ASSERT(string_compare_ci("Test", "Different") != 0, "Different strings");
    
    return TEST_PASS;
}

// Test string starts with
static TestResult test_string_starts_with(void) {
    TEST_ASSERT(string_starts_with("HelloWorld", "Hello"), "Starts with 'Hello'");
    TEST_ASSERT(!string_starts_with("HelloWorld", "World"), "Does not start with 'World'");
    TEST_ASSERT(string_starts_with("Test", "Test"), "Exact match");
    TEST_ASSERT(!string_starts_with("Short", "LongerString"), "Prefix longer than string");
    
    return TEST_PASS;
}

// Test string ends with
static TestResult test_string_ends_with(void) {
    TEST_ASSERT(string_ends_with("HelloWorld", "World"), "Ends with 'World'");
    TEST_ASSERT(!string_ends_with("HelloWorld", "Hello"), "Does not end with 'Hello'");
    TEST_ASSERT(string_ends_with("Test.txt", ".txt"), "File extension check");
    
    return TEST_PASS;
}

// Test string trim
static TestResult test_string_trim(void) {
    char str1[] = "  Hello  ";
    string_trim(str1);
    TEST_ASSERT_STRING_EQ(str1, "Hello", "Leading and trailing spaces removed");
    
    char str2[] = "NoSpaces";
    string_trim(str2);
    TEST_ASSERT_STRING_EQ(str2, "NoSpaces", "String without spaces unchanged");
    
    char str3[] = "  ";
    string_trim(str3);
    TEST_ASSERT_STRING_EQ(str3, "", "Only spaces becomes empty");
    
    return TEST_PASS;
}

// Test string split
static TestResult test_string_split(void) {
    const char* input = "one,two,three";
    int count = 0;
    char** parts = string_split(input, ',', &count);
    
    TEST_ASSERT_NOT_NULL(parts, "Split result not null");
    TEST_ASSERT_EQ(count, 3, "Three parts");
    TEST_ASSERT_STRING_EQ(parts[0], "one", "First part");
    TEST_ASSERT_STRING_EQ(parts[1], "two", "Second part");
    TEST_ASSERT_STRING_EQ(parts[2], "three", "Third part");
    
    for (int i = 0; i < count; i++) {
        free(parts[i]);
    }
    free(parts);
    
    return TEST_PASS;
}

// Test string replace
static TestResult test_string_replace(void) {
    char* result = string_replace("Hello World", "World", "Universe");
    TEST_ASSERT_NOT_NULL(result, "Replace result not null");
    TEST_ASSERT_STRING_EQ(result, "Hello Universe", "String replaced");
    free(result);
    
    char* no_match = string_replace("Hello", "Goodbye", "Hi");
    TEST_ASSERT_STRING_EQ(no_match, "Hello", "No match returns original");
    free(no_match);
    
    return TEST_PASS;
}

// Test string to integer
static TestResult test_string_to_int(void) {
    int value = 0;
    
    TEST_ASSERT(string_to_int("42", &value), "Parse '42'");
    TEST_ASSERT_EQ(value, 42, "Value is 42");
    
    TEST_ASSERT(string_to_int("-123", &value), "Parse negative");
    TEST_ASSERT_EQ(value, -123, "Value is -123");
    
    TEST_ASSERT(!string_to_int("abc", &value), "Invalid string fails");
    
    return TEST_PASS;
}

// Test string to float
static TestResult test_string_to_float(void) {
    float value = 0.0f;
    
    TEST_ASSERT(string_to_float("3.14", &value), "Parse '3.14'");
    TEST_ASSERT_FLOAT_EQ(value, 3.14f, 0.001f, "Value is 3.14");
    
    TEST_ASSERT(string_to_float("-2.5", &value), "Parse negative");
    TEST_ASSERT_FLOAT_EQ(value, -2.5f, 0.001f, "Value is -2.5");
    
    return TEST_PASS;
}

// Test string concatenation
static TestResult test_string_concat(void) {
    char* result = string_concat("Hello", " World");
    TEST_ASSERT_NOT_NULL(result, "Concat result not null");
    TEST_ASSERT_STRING_EQ(result, "Hello World", "Strings concatenated");
    free(result);
    
    return TEST_PASS;
}

// Register all tests
int main(void) {
    test_init();
    
    test_register("Core:StringUtils", "Duplicate", test_string_duplicate, NULL, NULL);
    test_register("Core:StringUtils", "Compare CI", test_string_compare_case_insensitive, NULL, NULL);
    test_register("Core:StringUtils", "Starts With", test_string_starts_with, NULL, NULL);
    test_register("Core:StringUtils", "Ends With", test_string_ends_with, NULL, NULL);
    test_register("Core:StringUtils", "Trim", test_string_trim, NULL, NULL);
    test_register("Core:StringUtils", "Split", test_string_split, NULL, NULL);
    test_register("Core:StringUtils", "Replace", test_string_replace, NULL, NULL);
    test_register("Core:StringUtils", "To Int", test_string_to_int, NULL, NULL);
    test_register("Core:StringUtils", "To Float", test_string_to_float, NULL, NULL);
    test_register("Core:StringUtils", "Concat", test_string_concat, NULL, NULL);
    
    TestStats stats = test_run_all();
    
    printf("\n");
    printf("════════════════════════════════════════════════════════\n");
    printf("  String Utils Test Results\n");
    printf("════════════════════════════════════════════════════════\n");
    printf("  Total:   %u\n", stats.total);
    printf("  Passed:  %u\n", stats.passed);
    printf("  Failed:  %u\n", stats.failed);
    printf("  Skipped: %u\n", stats.skipped);
    printf("  Time:    %.2f ms\n", stats.duration_ms);
    printf("════════════════════════════════════════════════════════\n");
    
    test_cleanup();
    
    return stats.failed > 0 ? 1 : 0;
}
