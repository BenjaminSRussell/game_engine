#include "tree_view.h"
#include "core/logger.h"
#include "core/memory.h"
#include "scroll_view.h"
#include "ui_label.h"
#include <stdlib.h>
#include <string.h>

struct TreeNode {
  Widget *main_widget;        // Vertical container (The Node itself)
  Widget *header_widget;      // Horizontal row (Toggle + Content)
  Widget *toggle_button;      // Arrow button
  Widget *content_widget;     // Label
  Widget *children_container; // Vertical container for children

  struct TreeNode *parent;
  struct TreeNode **children;
  uint32_t child_count;
  uint32_t child_capacity;

  bool expanded;
  bool selected;
  void *user_data;
  Widget *tree_view;
};

typedef struct {
  Widget *scroll_view;
  Widget *root_container;

  TreeViewSelectCallback on_select;
  void *callback_user_data;
  TreeNode *selected_node;

  // Hidden root node data structure to manage top level children easily?
  // Or just use root_container children?
  // Let's use a dummy root node for consistency in API if needed, but API takes
  // NULL for root. We will store top level nodes in a list or just rely on
  // root_container. For recursion, we need to map Widgets back to TreeNodes or
  // store TreeNodes. Let's just store TreeNodes in `children` array of
  // TreeNode. For top level, we need a list of TreeNodes.
  TreeNode **root_nodes;
  uint32_t root_node_count;
  uint32_t root_node_capacity;
} TreeViewData;

static void tree_view_destroy(Widget *widget);
static void tree_view_layout(Widget *widget, float available_width,
                             float available_height);
static void vertical_stack_layout(Widget *widget, float available_width,
                                  float available_height);
static void horizontal_row_layout(Widget *widget, float available_width,
                                  float available_height);
static bool on_node_click(Widget *widget, UIEvent *event, void *user_data);
static bool on_toggle_click(Widget *widget, UIEvent *event, void *user_data);

static void destroy_tree_node_recursive(TreeNode *node);

Widget *tree_view_create(const char *name) {
  Widget *widget = widget_create(name);
  if (!widget)
    return NULL;

  TreeViewData *data = memory_alloc(sizeof(TreeViewData));
  if (!data) {
    LOG_ERROR("Failed to allocate TreeView data");
    widget_destroy(widget);
    return NULL;
  }

  memset(data, 0, sizeof(TreeViewData));

  // ScrollView
  data->scroll_view = scroll_view_create("tree_scroll", (Vec2){100, 100});
  widget_add_child(widget, data->scroll_view);

  // Root Container
  data->root_container = widget_create("tree_root");
  data->root_container->layout = vertical_stack_layout;
  scroll_view_set_content(data->scroll_view, data->root_container);

  data->root_nodes = NULL;
  data->root_node_count = 0;
  data->root_node_capacity = 0;

  widget->user_data = data;
  widget->destroy = tree_view_destroy;
  widget->layout = tree_view_layout;

  widget_set_size(widget, (Vec2){200.0f, 300.0f});

  return widget;
}

static void tree_view_destroy(Widget *widget) {
  if (widget && widget->user_data) {
    TreeViewData *data = (TreeViewData *)widget->user_data;

    // Destroy all nodes (structures)
    for (uint32_t i = 0; i < data->root_node_count; i++) {
      destroy_tree_node_recursive(data->root_nodes[i]);
    }
    if (data->root_nodes)
      memory_free(data->root_nodes);

    memory_free(data);
    widget->user_data = NULL;
  }
}

static void tree_view_layout(Widget *widget, float available_width,
                             float available_height) {
  TreeViewData *data = (TreeViewData *)widget->user_data;
  if (!data || !data->scroll_view)
    return;

  Vec2 size = widget_get_size(widget);
  widget_set_size(data->scroll_view, size);

  if (data->scroll_view->layout) {
    data->scroll_view->layout(data->scroll_view, size.x, size.y);
  }
}

static void vertical_stack_layout(Widget *widget, float available_width,
                                  float available_height) {
  float y_offset = 0.0f;
  float max_width = 0.0f;

  for (uint32_t i = 0; i < widget->child_count; i++) {
    Widget *child = widget->children[i];
    if (!widget_is_visible(child))
      continue;

    widget_set_position(child, (Vec2){0.0f, y_offset});

    Vec2 child_size = widget_get_size(child);
    if (available_width > 0) {
      child_size.x = available_width;
      // widget_set_size(child, child_size); // Don't force width on children
      // unless needed? For tree nodes, we want them to stretch? Yes, usually.
      widget_set_size(child, (Vec2){available_width,
                                    child_size.y > 0 ? child_size.y : 20.0f});
    }

    // Recurse
    if (child->layout) {
      child->layout(child, available_width, child_size.y);
      // Re-read size in case layout changed it
      child_size = widget_get_size(child);
    }

    y_offset += child_size.y;
    if (child_size.x > max_width)
      max_width = child_size.x;
  }

  widget_set_size(widget, (Vec2){max_width, y_offset});
}

static void horizontal_row_layout(Widget *widget, float available_width,
                                  float available_height) {
  // Layout header: Toggle (Fixed) + Content (Flex)
  float x_offset = 0.0f;
  float height = 20.0f; // Default height

  for (uint32_t i = 0; i < widget->child_count; i++) {
    Widget *child = widget->children[i];
    if (!widget_is_visible(child))
      continue;

    widget_set_position(child, (Vec2){x_offset, 0.0f});
    Vec2 size = widget_get_size(child);

    if (i == 1) { // Content widget, stretch it
      float remaining = available_width - x_offset;
      if (remaining > 0)
        size.x = remaining;
      widget_set_size(child, size);
    }

    x_offset += size.x;
    if (size.y > height)
      height = size.y;
  }

  widget_set_size(widget, (Vec2){available_width, height});
}

TreeNode *tree_view_add_node(Widget *tree_view, TreeNode *parent,
                             const char *text) {
  TreeViewData *data = (TreeViewData *)tree_view->user_data;
  if (!data)
    return NULL;

  TreeNode *node = memory_alloc(sizeof(TreeNode));
  if (!node)
    return NULL;
  memset(node, 0, sizeof(TreeNode));

  node->tree_view = tree_view;
  node->parent = parent;
  node->expanded = true; // Default expanded? Or collapsed. Let's say expanded.

  // Main widget (Vertical)
  node->main_widget = widget_create("tree_node");
  node->main_widget->layout = vertical_stack_layout;

  // Header (Horizontal)
  node->header_widget = widget_create("node_header");
  node->header_widget->layout = horizontal_row_layout;
  widget_set_size(node->header_widget, (Vec2){100.0f, 20.0f});
  widget_add_child(node->main_widget, node->header_widget);

  // Toggle Button
  node->toggle_button = widget_create("toggle");
  widget_set_size(node->toggle_button, (Vec2){16.0f, 16.0f});
  widget_set_background_color(
      node->toggle_button,
      (Vec4){0.5f, 0.5f, 0.5f, 1.0f}); // Placeholder visual
  widget_add_child(node->header_widget, node->toggle_button);
  widget_add_event_handler(node->toggle_button, UI_EVENT_MOUSE_CLICK,
                           on_toggle_click, node);

  // Content Label
  UILabel *label = ui_label_create("node_label", text);
  node->content_widget = ui_label_get_widget(label);
  widget_add_child(node->header_widget, node->content_widget);

  // Make header clickable for selection
  widget_add_event_handler(node->header_widget, UI_EVENT_MOUSE_CLICK,
                           on_node_click, node);

  // Children Container
  node->children_container = widget_create("node_children");
  node->children_container->layout = vertical_stack_layout;
  widget_set_padding(node->children_container,
                     (BoxEdges){0.0f, 0.0f, 0.0f, 16.0f}); // Indent
  widget_add_child(node->main_widget, node->children_container);

  // Add to parent
  if (parent) {
    if (parent->child_count >= parent->child_capacity) {
      uint32_t new_cap =
          parent->child_capacity == 0 ? 4 : parent->child_capacity * 2;
      parent->children =
          memory_realloc(parent->children, sizeof(TreeNode *) * new_cap);
      parent->child_capacity = new_cap;
    }
    parent->children[parent->child_count++] = node;
    widget_add_child(parent->children_container, node->main_widget);
  } else {
    // Add to root
    if (data->root_node_count >= data->root_node_capacity) {
      uint32_t new_cap =
          data->root_node_capacity == 0 ? 4 : data->root_node_capacity * 2;
      data->root_nodes =
          memory_realloc(data->root_nodes, sizeof(TreeNode *) * new_cap);
      data->root_node_capacity = new_cap;
    }
    data->root_nodes[data->root_node_count++] = node;
    widget_add_child(data->root_container, node->main_widget);
  }

  widget_invalidate_layout(tree_view);
  return node;
}

static void destroy_tree_node_recursive(TreeNode *node) {
  if (!node)
    return;

  // Children nodes
  for (uint32_t i = 0; i < node->child_count; i++) {
    destroy_tree_node_recursive(node->children[i]);
  }
  if (node->children)
    memory_free(node->children);

  // Widgets are destroyed by widget_destroy hierarchy, but we allocated
  // TreeNode struct
  memory_free(node);
}

void tree_view_remove_node(Widget *tree_view, TreeNode *node) {
  if (!tree_view || !node)
    return;
  // Implementation requires finding node in parent list and removing it
  // TODO: Implement removal logic (remove widget from parent, remove node from
  // parent children list, free node)
}

void tree_view_clear(Widget *tree_view) {
  // TODO
}

void tree_node_set_expanded(TreeNode *node, bool expanded) {
  if (!node || node->expanded == expanded)
    return;
  node->expanded = expanded;
  widget_set_visible(node->children_container, expanded);
  widget_invalidate_layout(node->tree_view);
}

bool tree_node_is_expanded(const TreeNode *node) {
  return node ? node->expanded : false;
}

void tree_node_set_selected(TreeNode *node, bool selected) {
  if (!node || !node->tree_view)
    return;
  TreeViewData *data = (TreeViewData *)node->tree_view->user_data;

  if (selected) {
    // Deselect current
    if (data->selected_node && data->selected_node != node) {
      data->selected_node->selected = false;
      widget_set_background_color(data->selected_node->header_widget,
                                  (Vec4){0, 0, 0, 0}); // Transparent
    }

    node->selected = true;
    data->selected_node = node;
    widget_set_background_color(
        node->header_widget, (Vec4){0.2f, 0.4f, 0.8f, 1.0f}); // Selection color

    if (data->on_select) {
      data->on_select(node->tree_view, node, data->callback_user_data);
    }
  } else {
    if (node->selected) {
      node->selected = false;
      widget_set_background_color(node->header_widget, (Vec4){0, 0, 0, 0});
      if (data->selected_node == node)
        data->selected_node = NULL;
    }
  }
}

bool tree_node_is_selected(const TreeNode *node) {
  return node ? node->selected : false;
}

Widget *tree_node_get_content_widget(TreeNode *node) {
  return node ? node->content_widget : NULL;
}

void tree_node_set_data(TreeNode *node, void *data) {
  if (node)
    node->user_data = data;
}

void *tree_node_get_data(const TreeNode *node) {
  return node ? node->user_data : NULL;
}

void tree_view_set_on_select(Widget *tree_view, TreeViewSelectCallback callback,
                             void *user_data) {
  TreeViewData *data = (TreeViewData *)tree_view->user_data;
  if (data) {
    data->on_select = callback;
    data->callback_user_data = user_data;
  }
}

static bool on_node_click(Widget *widget, UIEvent *event, void *user_data) {
  if (event->type == UI_EVENT_MOUSE_CLICK) {
    TreeNode *node = (TreeNode *)user_data;
    tree_node_set_selected(node, true);
    return true;
  }
  return false;
}

static bool on_toggle_click(Widget *widget, UIEvent *event, void *user_data) {
  if (event->type == UI_EVENT_MOUSE_CLICK) {
    TreeNode *node = (TreeNode *)user_data;
    tree_node_set_expanded(node, !node->expanded);
    event->handled = true;
    event->stop_propagation = true; // Don't trigger selection
    return true;
  }
  return false;
}
