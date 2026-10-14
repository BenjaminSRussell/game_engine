/* Image, ListView, and TreeView. Not an Engine-linked test: Engine does not
 * build here (missing GLFW, then gpu_profiler.c). Memory and logger symbols
 * are stubbed, same as test_widget_focus. Does not touch the UI renderer.
 */

#include "core/logging/unified_logger.h"
#include "include/core/memory.h"
#include "ui/widgets/list_view.h"
#include "ui/widgets/tree_view.h"
#include "ui/widgets/ui_image.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static int g_list_selected = -1;

static void on_list_select(Widget *list_view, int index, void *user_data) {
  (void)list_view;
  g_list_selected = index;
  if (user_data) {
    *(int *)user_data = index;
  }
}

static int g_tree_selects = 0;

static void on_tree_select(Widget *tree_view, TreeNode *node, void *user_data) {
  (void)tree_view;
  (void)node;
  g_tree_selects++;
  if (user_data) {
    *(int *)user_data += 1;
  }
}

static Widget *ancestor_or_self(Widget *widget, Widget *target) {
  int guard = 0;
  while (widget && widget != target && guard++ < 32) {
    widget = widget->parent;
  }
  return widget;
}

static void test_image(void) {
  Widget *image = ui_image_create("icon", 7);
  Rect uv;
  Vec4 color;

  EXPECT(image, "image widget should be created");
  EXPECT(ui_image_get_texture_id(image) == 7, "texture id should be stored");
  uv = ui_image_get_uv_rect(image);
  EXPECT(uv.x == 0.0f && uv.y == 0.0f && uv.width == 1.0f && uv.height == 1.0f,
         "default UV should cover the texture");
  color = ui_image_get_color(image);
  EXPECT(color.x == 1.0f && color.y == 1.0f && color.z == 1.0f && color.w == 1.0f,
         "default tint should be white");
  EXPECT(image->size.x == 100.0f && image->size.y == 100.0f, "default size");
  EXPECT(image->background_color.w == 0.0f, "image background should be transparent");
  EXPECT(image->border_width == 0.0f, "image border should be zero");

  image->needs_redraw = false;
  ui_image_set_texture_id(image, 7);
  EXPECT(!image->needs_redraw, "same texture id should not invalidate");
  ui_image_set_texture_id(image, 9);
  ui_image_set_uv_rect(image, (Rect){0.1f, 0.2f, 0.3f, 0.4f});
  ui_image_set_color(image, (Vec4){0.5f, 0.6f, 0.7f, 0.8f});
  EXPECT(ui_image_get_texture_id(image) == 9, "texture id should update");
  uv = ui_image_get_uv_rect(image);
  EXPECT(uv.x == 0.1f && uv.y == 0.2f && uv.width == 0.3f && uv.height == 0.4f,
         "UV rect should update");
  color = ui_image_get_color(image);
  EXPECT(color.x == 0.5f && color.y == 0.6f && color.z == 0.7f && color.w == 0.8f,
         "tint should update");
  EXPECT(image->needs_redraw, "property changes should invalidate redraw");

  widget_destroy(image);
}

static void test_list_view(void) {
  Widget *list = list_view_create("list");
  Widget *a = widget_create("a");
  Widget *b = widget_create("b");
  Widget *c = widget_create("c");
  UIEvent click;
  int seen = -1;

  EXPECT(list, "list view should be created");
  EXPECT(list_view_get_item_count(list) == 0, "new list should be empty");
  EXPECT(list_view_get_selected_index(list) == -1, "new list has no selection");
  EXPECT(list->size.x == 200.0f && list->size.y == 300.0f, "default list size");

  widget_set_size(a, (Vec2){10.0f, 20.0f});
  widget_set_size(b, (Vec2){10.0f, 30.0f});
  widget_set_size(c, (Vec2){10.0f, 40.0f});
  list_view_add_item(list, a);
  list_view_add_item(list, b);
  list_view_add_item(list, c);
  EXPECT(list_view_get_item_count(list) == 3, "three items should be stored");
  EXPECT(list_view_get_item(list, 0) == a, "item 0");
  EXPECT(list_view_get_item(list, 1) == b, "item 1");
  EXPECT(list_view_get_item(list, 2) == c, "item 2");
  EXPECT(list_view_get_item(list, 3) == NULL, "out of range item is NULL");

  list_view_set_on_select(list, on_list_select, &seen);
  list_view_set_selected_index(list, 1);
  EXPECT(list_view_get_selected_index(list) == 1, "single selection index");
  EXPECT(b->state == WIDGET_STATE_SELECTED, "selected item state");
  EXPECT(seen == 1 && g_list_selected == 1, "selection callback");

  list_view_set_selected_index(list, 2);
  EXPECT(b->state == WIDGET_STATE_NORMAL, "previous item returns to normal");
  EXPECT(c->state == WIDGET_STATE_SELECTED, "new item is selected");
  EXPECT(seen == 2, "callback reports the new index");

  memset(&click, 0, sizeof(click));
  click.type = UI_EVENT_MOUSE_CLICK;
  EXPECT(widget_emit_event(a, &click), "item click should be handled");
  EXPECT(click.handled, "click event should be marked handled");
  EXPECT(list_view_get_selected_index(list) == 0, "click selects that item");
  EXPECT(a->state == WIDGET_STATE_SELECTED, "clicked item state");
  EXPECT(c->state == WIDGET_STATE_NORMAL, "previous selection cleared on click");

  EXPECT(list->layout != NULL, "list installs a layout");
  list->layout(list, list->size.x, list->size.y);
  EXPECT(a->position.y == 0.0f, "first item stacks at y=0");
  EXPECT(b->position.y == 20.0f, "second item stacks under the first");
  EXPECT(c->position.y == 50.0f, "third item stacks under the second");

  list_view_remove_item(list, 0);
  EXPECT(list_view_get_item_count(list) == 2, "remove drops one item");
  EXPECT(list_view_get_selected_index(list) == -1, "removing the selection clears it");
  EXPECT(list_view_get_item(list, 0) == b, "remaining items shift down");

  list_view_clear(list);
  EXPECT(list_view_get_item_count(list) == 0, "clear removes every item");
  EXPECT(list_view_get_selected_index(list) == -1, "clear drops selection");

  a = widget_create("only");
  list_view_add_item(list, a);
  list_view_set_selection_mode(list, SELECTION_MODE_NONE);
  list_view_set_selected_index(list, 0);
  EXPECT(list_view_get_selected_index(list) == -1, "none mode ignores selection");

  widget_destroy(list);
}

static void test_tree_view(void) {
  Widget *tree = tree_view_create("tree");
  TreeNode *root;
  TreeNode *child;
  Widget *root_label;
  Widget *child_label;
  int selects = 0;

  EXPECT(tree, "tree view should be created");
  EXPECT(tree->size.x == 200.0f && tree->size.y == 300.0f, "default tree size");

  root = tree_view_add_node(tree, NULL, "root");
  child = tree_view_add_node(tree, root, "child");
  EXPECT(root != NULL && child != NULL, "nodes should be created");
  EXPECT(tree_node_is_expanded(root), "new nodes start expanded");
  EXPECT(!tree_node_is_selected(root), "new nodes start unselected");

  root_label = tree_node_get_content_widget(root);
  child_label = tree_node_get_content_widget(child);
  EXPECT(root_label != NULL && child_label != NULL, "nodes expose a content widget");
  EXPECT(strcmp(root_label->name, "node_label") == 0, "content widget is the node label");
  EXPECT(ancestor_or_self(child_label, tree) == tree, "child node is parented in the tree");
  EXPECT(ancestor_or_self(root_label, tree) == tree, "root node is parented in the tree");

  tree_node_set_data(child, (void *)(uintptr_t)42);
  EXPECT(tree_node_get_data(child) == (void *)(uintptr_t)42, "node user data");

  g_tree_selects = 0;
  tree_view_set_on_select(tree, on_tree_select, &selects);
  tree_node_set_selected(child, true);
  EXPECT(tree_node_is_selected(child), "child becomes selected");
  EXPECT(selects == 1 && g_tree_selects == 1, "tree selection callback");
  EXPECT(child_label->parent->background_color.z == 0.8f, "selected header is tinted");

  tree_node_set_selected(root, true);
  EXPECT(tree_node_is_selected(root), "root becomes selected");
  EXPECT(!tree_node_is_selected(child), "previous node is deselected");
  EXPECT(selects == 2, "callback fires for the new node");

  tree_node_set_expanded(root, false);
  EXPECT(!tree_node_is_expanded(root), "node can collapse");
  tree_node_set_expanded(root, true);
  EXPECT(tree_node_is_expanded(root), "node can expand again");

  EXPECT(tree->layout != NULL, "tree installs a layout");
  tree->layout(tree, tree->size.x, tree->size.y);

  widget_destroy(tree);
}

int main(void) {
  test_image();
  test_list_view();
  test_tree_view();

  if (g_failed) {
    fprintf(stderr, "%d UI widget test(s) failed\n", g_failed);
    return 1;
  }
  printf("ui image/list/tree widget tests passed\n");
  return 0;
}
