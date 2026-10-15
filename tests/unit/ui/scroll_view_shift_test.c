/* Issue #128. Holding Shift while scrolling the wheel moves a scroll_view
 * horizontally, and a plain wheel still scrolls vertically. Compiles
 * scroll_view.c and widget.c with allocator and logger stubs, because
 * add_engine_test links Engine, which does not build on origin/main.
 */

#include "../../../src/engine/ui/widgets/scroll_view.h"
#include "core/logger.h"
#include "core/memory.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

void *unified_memory_alloc(size_t size, MemoryStrategy strategy, MemoryFlags flags,
                           const char *file, int line, const char *function) {
  (void)strategy;
  (void)flags;
  (void)file;
  (void)line;
  (void)function;
  return calloc(1, size ? size : 1);
}

void *unified_memory_realloc(void *ptr, size_t new_size, MemoryFlags flags,
                             const char *file, int line, const char *function) {
  (void)flags;
  (void)file;
  (void)line;
  (void)function;
  return realloc(ptr, new_size);
}

void unified_memory_free(void *ptr, const char *file, int line,
                         const char *function) {
  (void)file;
  (void)line;
  (void)function;
  free(ptr);
}

void unified_logger_log(LogLevel level, LogCategory category, const char *file,
                        int line, const char *function, const char *format, ...) {
  (void)level;
  (void)category;
  (void)file;
  (void)line;
  (void)function;
  (void)format;
}

static int g_failed = 0;

#define EXPECT(cond, msg)                                                      \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg);            \
      g_failed++;                                                              \
      return;                                                                  \
    }                                                                          \
  } while (0)

#define EXPECT_NEAR(actual, expected, msg)                                     \
  EXPECT(fabsf((actual) - (expected)) < 0.001f, msg)

/* Content is 1000x1000 and the viewport is 200x200 minus a 12px scrollbar on
 * each axis, so the scroll range on both axes is 1000 - 188 = 812. */
static void content_layout(Widget *widget, float available_width,
                           float available_height) {
  (void)available_width;
  (void)available_height;
  widget_set_size(widget, (Vec2){1000.0f, 1000.0f});
}

static Widget *make_scroll_view(Widget **content_out) {
  Widget *scroll_view = scroll_view_create("scroll_view", (Vec2){200.0f, 200.0f});
  Widget *content = widget_create("content");
  content->layout = content_layout;
  scroll_view_set_content(scroll_view, content);
  scroll_view->layout(scroll_view, 500.0f, 500.0f);
  *content_out = content;
  return scroll_view;
}

/* Calls the widget's handler directly with one mouse wheel event. */
static bool send_wheel(Widget *scroll_view, float wheel_delta,
                       UIModifiers modifiers) {
  UIEvent *event = ui_event_create(UI_EVENT_MOUSE_WHEEL);
  event->mouse.wheel_delta = wheel_delta;
  event->modifiers = modifiers;
  bool handled = scroll_view->handle_event(scroll_view, event);
  ui_event_destroy(event);
  return handled;
}

static void test_shift_wheel_scrolls_x_not_y(void) {
  Widget *content = NULL;
  Widget *scroll_view = make_scroll_view(&content);
  scroll_view_set_scroll_offset(scroll_view, (Vec2){100.0f, 50.0f});

  EXPECT(send_wheel(scroll_view, 1.0f, UI_MODIFIER_SHIFT),
         "shift+wheel should be consumed when the offset changes");
  Vec2 offset = scroll_view_get_scroll_offset(scroll_view);
  EXPECT_NEAR(offset.x, 80.0f, "shift+wheel should move x by one wheel step");
  EXPECT_NEAR(offset.y, 50.0f, "shift+wheel should leave y unchanged");

  scroll_view->layout(scroll_view, 500.0f, 500.0f);
  EXPECT_NEAR(content->position.x, -80.0f, "content should move with x");
  EXPECT_NEAR(content->position.y, -50.0f, "content y should not move");

  widget_destroy(scroll_view);
}

static void test_plain_wheel_still_scrolls_y(void) {
  Widget *content = NULL;
  Widget *scroll_view = make_scroll_view(&content);
  scroll_view_set_scroll_offset(scroll_view, (Vec2){100.0f, 50.0f});

  EXPECT(send_wheel(scroll_view, 1.0f, UI_MODIFIER_NONE),
         "plain wheel should be consumed when the offset changes");
  Vec2 offset = scroll_view_get_scroll_offset(scroll_view);
  EXPECT_NEAR(offset.y, 30.0f, "plain wheel should move y by one wheel step");
  EXPECT_NEAR(offset.x, 100.0f, "plain wheel should leave x unchanged");

  widget_destroy(scroll_view);
}

static void test_shift_wheel_stops_at_horizontal_end(void) {
  Widget *content = NULL;
  Widget *scroll_view = make_scroll_view(&content);
  scroll_view_set_scroll_offset(scroll_view, (Vec2){810.0f, 0.0f});

  /* A negative delta with Shift moves x past the end of the range. */
  EXPECT(send_wheel(scroll_view, -1.0f, UI_MODIFIER_SHIFT),
         "shift+wheel should be consumed at the horizontal edge");
  scroll_view->layout(scroll_view, 500.0f, 500.0f);
  Vec2 offset = scroll_view_get_scroll_offset(scroll_view);
  EXPECT_NEAR(offset.x, 812.0f, "layout should clamp x to the horizontal range");
  EXPECT_NEAR(offset.y, 0.0f, "shift+wheel should leave y unchanged at the edge");

  widget_destroy(scroll_view);
}

int main(void) {
  test_shift_wheel_scrolls_x_not_y();
  test_plain_wheel_still_scrolls_y();
  test_shift_wheel_stops_at_horizontal_end();

  if (g_failed) {
    fprintf(stderr, "%d scroll_view shift-wheel check(s) failed\n", g_failed);
    return 1;
  }
  printf("scroll_view shift-wheel tests passed\n");
  return 0;
}
