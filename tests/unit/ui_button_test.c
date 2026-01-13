// Button Widget Unit Tests
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>

// Include Common types
#include "include/common.h"

// Mock Memory System - Unified Allocator
// We must match signature from unified_memory_allocator.h
// void* unified_memory_alloc(size_t size, MemoryStrategy strategy, MemoryFlags flags, const char* file, int line, const char* function);

// We need these enums if we don't include unified_memory_allocator.h, but we do include core/memory.h which includes it.
// So we just implement the functions.

#include "include/core/memory.h"

void* unified_memory_alloc(u64 size, MemoryStrategy strategy, MemoryFlags flags,
                        const char* file, int line, const char* function) {
    return calloc(1, size);
}

void* unified_memory_realloc(void* ptr, u64 new_size, MemoryFlags flags,
                           const char* file, int line, const char* function) {
    return realloc(ptr, new_size);
}

void unified_memory_free(void* ptr, const char* file, int line, const char* function) {
    free(ptr);
}

// Mock Logger System
#include "include/core/logger/unified_logger.h"

// We need to implement unified_logger_log
void unified_logger_log(LogLevel level, LogCategory category, const char* file, int line,
                     const char* function, const char* format, ...) {
    // Basic print for debug
    va_list args;
    va_start(args, format);
    // vprintf(format, args); // Optional: print logs during test
    // printf("\n");
    va_end(args);
}

// Include UI headers
#include "ui/widgets/button.h"
#include "include/math/math.h"

// Simple assertion macros to avoid dependency on complex frameworks
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("FAIL: %s\n", message); \
            return false; \
        } \
    } while(0)

#define TEST_ASSERT_STR_EQ(a, b, message) \
    do { \
        if (strcmp((a), (b)) != 0) { \
            printf("FAIL: %s (expected '%s', got '%s')\n", message, (b), (a)); \
            return false; \
        } \
    } while(0)

#define TEST_ASSERT_INT_EQ(a, b, message) \
    do { \
        if ((a) != (b)) { \
            printf("FAIL: %s (expected %d, got %d)\n", message, (b), (a)); \
            return false; \
        } \
    } while(0)

#define TEST_ASSERT_FLOAT_EQ(a, b, message) \
    do { \
        if (fabsf((a) - (b)) > 0.001f) { \
            printf("FAIL: %s (expected %.3f, got %.3f)\n", message, (b), (a)); \
            return false; \
        } \
    } while(0)

typedef struct {
    const char* test_name;
    bool (*test_func)(void);
    bool passed;
} UITest;

static UITest g_tests[32];
static uint32_t g_test_count = 0;
static uint32_t g_tests_passed = 0;

void add_ui_test(const char* name, bool (*test_func)(void)) {
    if (g_test_count < 32) {
        g_tests[g_test_count].test_name = name;
        g_tests[g_test_count].test_func = test_func;
        g_tests[g_test_count].passed = false;
        g_test_count++;
    }
}

// Global state for callback testing
static bool g_callback_called = false;
static void* g_callback_user_data = NULL;

static bool test_callback(Widget* widget, UIEvent* event, void* user_data) {
    g_callback_called = true;
    g_callback_user_data = user_data;
    return true;
}

// Test 1: Button Creation
bool test_button_create(void) {
    printf("Testing button creation...\n");

    Button* btn = button_create("test_btn", "Test Label");
    TEST_ASSERT(btn != NULL, "Button creation failed");

    Widget* w = (Widget*)btn;
    TEST_ASSERT_STR_EQ(w->name, "test_btn", "Button name mismatch");
    TEST_ASSERT_STR_EQ(btn->label, "Test Label", "Button label mismatch");
    TEST_ASSERT(w->visible, "Button should be visible");
    TEST_ASSERT(w->enabled, "Button should be enabled");
    TEST_ASSERT_INT_EQ(w->state, WIDGET_STATE_NORMAL, "Initial state should be NORMAL");

    widget_destroy(w);
    return true;
}

// Test 2: Button Properties
bool test_button_properties(void) {
    printf("Testing button properties...\n");

    Button* btn = button_create("prop_btn", "Old Label");

    // Test label update
    button_set_label(btn, "New Label");
    TEST_ASSERT_STR_EQ(btn->label, "New Label", "Label update failed");

    // Test colors
    Vec4 normal = {1, 0, 0, 1};
    Vec4 hover = {0, 1, 0, 1};
    Vec4 pressed = {0, 0, 1, 1};
    Vec4 disabled = {0, 0, 0, 1};
    button_set_colors(btn, normal, hover, pressed, disabled);

    TEST_ASSERT_FLOAT_EQ(btn->normal_color.x, 1.0f, "Normal color not set");
    TEST_ASSERT_FLOAT_EQ(btn->hover_color.y, 1.0f, "Hover color not set");

    // Check initial background color (should be normal)
    Widget* w = (Widget*)btn;
    TEST_ASSERT_FLOAT_EQ(w->background_color.x, 1.0f, "Background color not updated to normal");

    widget_destroy(w);
    return true;
}

// Test 3: Button State Transitions
bool test_button_states(void) {
    printf("Testing button state transitions...\n");

    Button* btn = button_create("state_btn", "Click Me");
    Widget* w = (Widget*)btn;
    w->size = (Vec2){100.0f, 50.0f};

    // Test Mouse Enter
    UIEvent* enter_event = ui_event_create(UI_EVENT_MOUSE_ENTER);
    enter_event->mouse.position = (Vec2){10, 10}; // Inside
    widget_handle_event(w, enter_event);
    TEST_ASSERT_INT_EQ(w->state, WIDGET_STATE_HOVER, "State should be HOVER after mouse enter");
    ui_event_destroy(enter_event);

    // Test Mouse Leave
    UIEvent* leave_event = ui_event_create(UI_EVENT_MOUSE_LEAVE);
    widget_handle_event(w, leave_event);
    TEST_ASSERT_INT_EQ(w->state, WIDGET_STATE_NORMAL, "State should be NORMAL after mouse leave");
    ui_event_destroy(leave_event);

    // Test Mouse Down
    UIEvent* down_event = ui_event_create_mouse(UI_EVENT_MOUSE_DOWN, (Vec2){10, 10}, UI_MOUSE_BUTTON_LEFT);
    widget_handle_event(w, down_event);
    TEST_ASSERT_INT_EQ(w->state, WIDGET_STATE_PRESSED, "State should be PRESSED after mouse down");
    ui_event_destroy(down_event);

    // Test Mouse Up (Click)
    UIEvent* up_event = ui_event_create_mouse(UI_EVENT_MOUSE_UP, (Vec2){10, 10}, UI_MOUSE_BUTTON_LEFT);
    widget_handle_event(w, up_event);
    TEST_ASSERT_INT_EQ(w->state, WIDGET_STATE_HOVER, "State should return to HOVER after click inside");
    ui_event_destroy(up_event);

    widget_destroy(w);
    return true;
}

// Test 4: Button Callbacks
bool test_button_callback(void) {
    printf("Testing button callbacks...\n");

    Button* btn = button_create("cb_btn", "Click Me");
    Widget* w = (Widget*)btn;
    w->size = (Vec2){100.0f, 50.0f};

    // Reset test state
    g_callback_called = false;
    g_callback_user_data = NULL;
    int data = 123;

    button_set_on_click(btn, test_callback, &data);

    // Simulate Click Sequence
    // 1. Mouse Down
    UIEvent* down_event = ui_event_create_mouse(UI_EVENT_MOUSE_DOWN, (Vec2){10, 10}, UI_MOUSE_BUTTON_LEFT);
    widget_handle_event(w, down_event);
    ui_event_destroy(down_event);

    // 2. Mouse Up
    UIEvent* up_event = ui_event_create_mouse(UI_EVENT_MOUSE_UP, (Vec2){10, 10}, UI_MOUSE_BUTTON_LEFT);
    widget_handle_event(w, up_event);
    ui_event_destroy(up_event);

    TEST_ASSERT(g_callback_called, "Callback should have been called");
    TEST_ASSERT(g_callback_user_data == &data, "User data mismatch");

    widget_destroy(w);
    return true;
}

int main(void) {
    printf("Button Widget Unit Test Suite\n");
    printf("=============================\n\n");

    add_ui_test("Button Creation", test_button_create);
    add_ui_test("Button Properties", test_button_properties);
    add_ui_test("Button State Transitions", test_button_states);
    add_ui_test("Button Callbacks", test_button_callback);

    // Run tests
    for (uint32_t i = 0; i < g_test_count; i++) {
        printf("\n--- Test %u: %s ---\n", i + 1, g_tests[i].test_name);

        bool passed = g_tests[i].test_func();
        g_tests[i].passed = passed;

        if (passed) {
            printf("PASS\n");
            g_tests_passed++;
        } else {
            printf("FAIL\n");
        }
    }

    // Summary
    printf("\n=== Test Summary ===\n");
    printf("Tests passed: %u/%u (%.1f%%)\n",
           g_tests_passed, g_test_count,
           (float)g_tests_passed / g_test_count * 100.0f);

    return (g_tests_passed == g_test_count) ? 0 : 1;
}
