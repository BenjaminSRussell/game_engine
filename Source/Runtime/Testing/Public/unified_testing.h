#pragma once

#include <stdbool.h>
#include <stdint.h>

// Forward declarations
typedef struct TestSuite TestSuite;
typedef struct DebugDraw DebugDraw;

// Unit testing
typedef void (*TestFunction)(void);

TestSuite *test_suite_create(const char *name);
void test_suite_add_test(TestSuite *suite, const char *test_name,
                         TestFunction test_func);
bool test_suite_run(TestSuite *suite);
void test_suite_destroy(TestSuite *suite);

// Assertions
#define TEST_ASSERT(condition)                                                 \
  test_assert_impl(condition, #condition, __FILE__, __LINE__)
#define TEST_ASSERT_EQ(a, b)                                                   \
  test_assert_eq_impl(a, b, #a, #b, __FILE__, __LINE__)

void test_assert_impl(bool condition, const char *expr, const char *file,
                      int line);
void test_assert_eq_impl(int a, int b, const char *expr_a, const char *expr_b,
                         const char *file, int line);

// Debug drawing
DebugDraw *debug_draw_create(void);
void debug_draw_line(DebugDraw *draw, float x1, float y1, float z1, float x2,
                     float y2, float z2, uint32_t color);
void debug_draw_box(DebugDraw *draw, float x, float y, float z, float width,
                    float height, float depth, uint32_t color);
void debug_draw_sphere(DebugDraw *draw, float x, float y, float z, float radius,
                       uint32_t color);
void debug_draw_text(DebugDraw *draw, float x, float y, float z,
                     const char *text);
void debug_draw_clear(DebugDraw *draw);
void debug_draw_destroy(DebugDraw *draw);

// Debug utilities
void debug_log_memory_stats(void);
void debug_log_performance_stats(void);
void debug_break_on_error(bool enabled);
