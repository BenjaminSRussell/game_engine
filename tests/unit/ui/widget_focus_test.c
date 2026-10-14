/* Proves widget_request_focus releases the previously focused widget.
 * Issue #127. Compiles against widget.c with allocator and logger stubs
 * because add_engine_test links Engine and a framework file that is not
 * in the tree.
 */

#include "core/logging/unified_logger.h"
#include "include/core/memory.h"
#include "ui/widgets/widget.h"

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

static void focusable(Widget *widget) { widget->focusable = true; }

static void test_request_sets_focus(void) {
  Widget *widget = widget_create("only");
  focusable(widget);

  EXPECT(widget_request_focus(widget), "focusable widget should take focus");
  EXPECT(widget->focused, "focused flag should be set");
  EXPECT(widget->state == WIDGET_STATE_FOCUSED, "state should be FOCUSED");

  widget_destroy(widget);
}

static void test_non_focusable_rejected(void) {
  Widget *widget = widget_create("plain");

  EXPECT(!widget_request_focus(widget), "non-focusable widget should be rejected");
  EXPECT(!widget->focused, "rejected widget must not be focused");
  EXPECT(!widget_request_focus(NULL), "NULL widget should be rejected");

  widget_destroy(widget);
}

static void test_sibling_releases_previous(void) {
  Widget *root = widget_create("root");
  Widget *first = widget_create("first");
  Widget *second = widget_create("second");
  focusable(first);
  focusable(second);
  widget_add_child(root, first);
  widget_add_child(root, second);

  EXPECT(widget_request_focus(first), "first sibling should take focus");
  EXPECT(widget_request_focus(second), "second sibling should take focus");
  EXPECT(!first->focused, "previous sibling focus should be released");
  EXPECT(first->state == WIDGET_STATE_NORMAL, "released sibling should be NORMAL");
  EXPECT(second->focused, "new sibling should be focused");
  EXPECT(second->state == WIDGET_STATE_FOCUSED, "new sibling state should be FOCUSED");

  widget_destroy(root);
}

static void test_nested_branch_releases_previous(void) {
  Widget *root = widget_create("root");
  Widget *left = widget_create("left");
  Widget *right = widget_create("right");
  Widget *deep = widget_create("deep");
  Widget *other = widget_create("other");
  focusable(deep);
  focusable(other);
  widget_add_child(root, left);
  widget_add_child(left, deep);
  widget_add_child(root, right);
  widget_add_child(right, other);

  EXPECT(widget_request_focus(deep), "nested widget should take focus");
  EXPECT(widget_request_focus(other), "widget in the other branch should take focus");
  EXPECT(!deep->focused, "focused widget in another branch should be released");
  EXPECT(other->focused, "newly requested nested widget should be focused");

  widget_destroy(root);
}

static void test_parent_releases_focused_child(void) {
  Widget *root = widget_create("root");
  Widget *parent = widget_create("parent");
  Widget *child = widget_create("child");
  focusable(parent);
  focusable(child);
  widget_add_child(root, parent);
  widget_add_child(parent, child);

  EXPECT(widget_request_focus(child), "child should take focus");
  EXPECT(widget_request_focus(parent), "parent should take focus from its child");
  EXPECT(!child->focused, "child focus should be released");
  EXPECT(parent->focused, "parent should be focused");

  widget_destroy(root);
}

static void test_refocus_same_widget(void) {
  Widget *root = widget_create("root");
  Widget *widget = widget_create("same");
  focusable(widget);
  widget_add_child(root, widget);

  EXPECT(widget_request_focus(widget), "initial focus should succeed");
  EXPECT(widget_request_focus(widget), "requesting focus again should succeed");
  EXPECT(widget->focused, "widget should stay focused");
  EXPECT(widget->state == WIDGET_STATE_FOCUSED, "state should stay FOCUSED");

  widget_release_focus(widget);
  EXPECT(!widget->focused, "explicit release should clear focus");
  EXPECT(widget->state == WIDGET_STATE_NORMAL, "released widget should be NORMAL");

  widget_destroy(root);
}

static void test_hidden_or_disabled_does_not_steal(void) {
  Widget *root = widget_create("root");
  Widget *current = widget_create("current");
  Widget *hidden = widget_create("hidden");
  Widget *disabled = widget_create("disabled");
  focusable(current);
  focusable(hidden);
  focusable(disabled);
  widget_add_child(root, current);
  widget_add_child(root, hidden);
  widget_add_child(root, disabled);
  widget_set_visible(hidden, false);
  widget_set_enabled(disabled, false);

  EXPECT(widget_request_focus(current), "current widget should take focus");
  EXPECT(!widget_request_focus(hidden), "hidden widget should be rejected");
  EXPECT(!widget_request_focus(disabled), "disabled widget should be rejected");
  EXPECT(current->focused, "existing focus should remain");
  EXPECT(!hidden->focused, "hidden widget must not be focused");
  EXPECT(!disabled->focused, "disabled widget must not be focused");

  widget_destroy(root);
}

int main(void) {
  test_request_sets_focus();
  test_non_focusable_rejected();
  test_sibling_releases_previous();
  test_nested_branch_releases_previous();
  test_parent_releases_focused_child();
  test_refocus_same_widget();
  test_hidden_or_disabled_does_not_steal();

  if (g_failed) {
    fprintf(stderr, "%d focus test(s) failed\n", g_failed);
    return 1;
  }
  printf("widget focus tests passed\n");
  return 0;
}
