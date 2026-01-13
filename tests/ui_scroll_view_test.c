#include "../src/engine/ui/widgets/scroll_view.h"
#include "core/logger.h"
#include "core/memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <math.h>

// Mock Logger implementation
void unified_logger_log(LogLevel level, LogCategory category, const char* file, int line,
                     const char* function, const char* format, ...) {
    va_list args;
    va_start(args, format);
    // Only print errors or info
    if (level >= LOG_LEVEL_INFO) {
        vprintf(format, args);
        printf("\n");
    }
    va_end(args);
}

// Stub for structured logging
void unified_logger_log_structured(LogLevel level, LogCategory category, const char* file,
                              int line, const char* function, const char* message,
                              const char* context_json) {
    // No-op
}

// Mock Memory
void* unified_memory_alloc(size_t size, MemoryStrategy strategy, MemoryFlags flags,
                        const char* file, int line, const char* function) {
    return calloc(1, size);
}

void unified_memory_free(void* ptr, const char* file, int line, const char* function) {
    free(ptr);
}

void* unified_memory_realloc(void* ptr, size_t new_size, MemoryFlags flags,
                           const char* file, int line, const char* function) {
    return realloc(ptr, new_size);
}

// Mock content widget layout
void content_layout(Widget* widget, float w, float h) {
    // Content wants to be 1000x1000
    widget_set_size(widget, (Vec2){1000.0f, 1000.0f});
}

int main() {
    printf("Running ScrollView tests...\n");

    // Test 1: Creation
    Widget* sv = scroll_view_create("my_scroll_view", (Vec2){200.0f, 200.0f});
    if (!sv) {
        printf("Failed to create scroll view\n");
        return 1;
    }
    assert(sv->user_data != NULL); // ScrollViewData

    // Test 2: Add Content
    Widget* content = widget_create("content");
    content->layout = content_layout;
    scroll_view_set_content(sv, content);

    assert(scroll_view_get_content(sv) == content);

    // Test 3: Layout and initial state
    // Initial offset should be 0
    Vec2 offset = scroll_view_get_scroll_offset(sv);
    assert(offset.x == 0 && offset.y == 0);

    // Perform layout
    if (sv->layout) sv->layout(sv, 500, 500);

    // Content should have been sized
    assert(content->size.x == 1000.0f);
    assert(content->size.y == 1000.0f);

    // Content position should be 0,0 (offset is 0)
    assert(content->position.x == 0);
    assert(content->position.y == 0);

    // Test 4: Scrolling via API
    scroll_view_set_scroll_offset(sv, (Vec2){100.0f, 50.0f});

    // Re-layout to apply position
    if (sv->layout) sv->layout(sv, 500, 500);

    offset = scroll_view_get_scroll_offset(sv);
    assert(offset.x == 100.0f);
    assert(offset.y == 50.0f);

    assert(content->position.x == -100.0f);
    assert(content->position.y == -50.0f);

    // Test 5: Clamping
    scroll_view_set_scroll_offset(sv, (Vec2){2000.0f, 2000.0f});
    if (sv->layout) sv->layout(sv, 500, 500);

    offset = scroll_view_get_scroll_offset(sv);
    printf("Offset after clamping: %f, %f\n", offset.x, offset.y);

    // Scrollbar width is 12.
    // content_view_w = 200 - 12 = 188.
    // max scroll x = 1000 - 188 = 812.
    assert(fabs(offset.x - 812.0f) < 1.0f);
    assert(fabs(offset.y - 812.0f) < 1.0f);

    // Test 6: Event handling (Wheel)
    UIEvent* event = ui_event_create(UI_EVENT_MOUSE_WHEEL);
    event->mouse.wheel_delta = 1.0f;

    float old_y = offset.y;
    sv->handle_event(sv, event);

    offset = scroll_view_get_scroll_offset(sv);
    assert(offset.y == old_y - 20.0f);

    ui_event_destroy(event);

    widget_destroy(sv);

    printf("Tests passed!\n");
    return 0;
}
