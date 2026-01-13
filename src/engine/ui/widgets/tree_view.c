#include "tree_view.h"
#include "core/logger.h"
#include "core/memory.h"
#include <stdlib.h>
#include <string.h>

// Internal structures
struct TreeNode {
    char* text;
    bool expanded;
    bool selected;
    void* user_data;

    struct TreeNode* parent;
    struct TreeNode* first_child;
    struct TreeNode* next_sibling;
    struct TreeNode* last_child;

    uint32_t child_count;
};

typedef struct {
    TreeNode* root_dummy; // Invisible root holding top-level nodes
    TreeNode* selected_node;

    float row_height;
    float indent_width;

    TreeViewNodeCallback on_select;
    void* callback_user_data;
} TreeViewData;

// Forward declarations of internal functions
static void tree_view_destroy_node_recursive(TreeNode* node);
static void tree_view_calculate_layout(Widget* widget);
static void tree_view_render_node(Widget* widget, TreeViewData* data, TreeNode* node, int depth, float* y_offset);

// Widget overrides
static void tree_view_destroy(Widget* widget) {
    if (!widget || !widget->user_data) return;

    TreeViewData* data = (TreeViewData*)widget->user_data;

    if (data->root_dummy) {
        tree_view_destroy_node_recursive(data->root_dummy);
    }

    memory_free(data);
    widget->user_data = NULL;
}

static void tree_view_layout(Widget* widget, float available_width, float available_height) {
    if (!widget || !widget->user_data) return;
    tree_view_calculate_layout(widget);
}

static void tree_view_render(Widget* widget) {
    if (!widget || !widget->visible || !widget->user_data) return;

    TreeViewData* data = (TreeViewData*)widget->user_data;
    float y_offset = 0.0f;

    // Start rendering from the first child of the dummy root
    TreeNode* child = data->root_dummy->first_child;
    while (child) {
        tree_view_render_node(widget, data, child, 0, &y_offset);
        child = child->next_sibling;
    }

    // Draw border/background if handled by base widget?
    // Widget base class doesn't draw automatically, usually the renderer does.
    // Since we are simulating, we log the structure.
    // LOG_INFO(LOG_CAT_EDITOR, "TreeView '%s' rendered, height: %.1f", widget->name, y_offset);
}

static TreeNode* tree_view_hit_test_recursive(TreeNode* node, float target_y, float* current_y, float row_height) {
    if (*current_y <= target_y && target_y < *current_y + row_height) {
        return node;
    }
    *current_y += row_height;

    if (node->expanded) {
        TreeNode* child = node->first_child;
        while (child) {
            TreeNode* hit = tree_view_hit_test_recursive(child, target_y, current_y, row_height);
            if (hit) return hit;
            child = child->next_sibling;
        }
    }
    return NULL;
}

static bool tree_view_handle_event(Widget* widget, UIEvent* event) {
    if (!widget || !widget->enabled || !widget->visible || !widget->user_data) return false;

    TreeViewData* data = (TreeViewData*)widget->user_data;

    if (event->type == UI_EVENT_MOUSE_DOWN && event->mouse.button == UI_MOUSE_BUTTON_LEFT) {
        // Convert global to local (widget system handles this?)
        // Widget.c: widget_handle_event calls widget_propagate_event which calls widget->handle_event.
        // It says: "Convert to widget-local coordinates ... event->mouse.position = local_point;"
        // So event->mouse.position is local.

        float hit_y = event->mouse.position.y;
        float current_y = 0.0f;

        TreeNode* hit_node = NULL;
        TreeNode* child = data->root_dummy->first_child;
        while (child) {
            hit_node = tree_view_hit_test_recursive(child, hit_y, &current_y, data->row_height);
            if (hit_node) break;
            child = child->next_sibling;
        }

        if (hit_node) {
            // Determine depth to check for expander click
            int depth = 0;
            TreeNode* p = hit_node->parent;
            while (p && p != data->root_dummy) {
                depth++;
                p = p->parent;
            }

            float indent_x = depth * data->indent_width;
            float expander_size = data->row_height; // Assume square expander area

            if (event->mouse.position.x >= indent_x && event->mouse.position.x < indent_x + expander_size) {
                // Toggle expansion
                tree_node_set_expanded(widget, hit_node, !hit_node->expanded);
            } else {
                // Select node
                tree_node_set_selected(widget, hit_node, true);

                // Trigger callback
                if (data->on_select) {
                    data->on_select(widget, hit_node, data->callback_user_data);
                }
            }
            return true; // Handled
        }
    }

    return false;
}

// Public API implementation

Widget* tree_view_create(const char* name) {
    Widget* widget = widget_create(name);
    if (!widget) return NULL;

    TreeViewData* data = memory_alloc(sizeof(TreeViewData));
    if (!data) {
        // cleanup widget? widget_destroy(widget);
        return NULL;
    }

    memset(data, 0, sizeof(TreeViewData));
    data->row_height = 20.0f;
    data->indent_width = 20.0f;

    // Create dummy root
    data->root_dummy = memory_alloc(sizeof(TreeNode));
    memset(data->root_dummy, 0, sizeof(TreeNode));
    data->root_dummy->expanded = true; // Always expanded

    widget->user_data = data;
    widget->destroy = tree_view_destroy;
    widget->layout = tree_view_layout;
    widget->render = tree_view_render;
    widget->handle_event = tree_view_handle_event;

    return widget;
}

TreeNode* tree_view_add_node(Widget* tree, TreeNode* parent, const char* text) {
    if (!tree || !tree->user_data) return NULL;
    TreeViewData* data = (TreeViewData*)tree->user_data;

    if (!parent) {
        parent = data->root_dummy;
    }

    TreeNode* node = memory_alloc(sizeof(TreeNode));
    if (!node) return NULL;

    memset(node, 0, sizeof(TreeNode));
    node->text = text ? strdup(text) : NULL;
    node->parent = parent;

    if (parent->last_child) {
        parent->last_child->next_sibling = node;
        parent->last_child = node;
    } else {
        parent->first_child = node;
        parent->last_child = node;
    }

    parent->child_count++;

    tree->needs_layout = true;
    tree->dirty = true;

    return node;
}

void tree_view_remove_node(Widget* tree, TreeNode* node) {
    if (!tree || !node) return;
    TreeViewData* data = (TreeViewData*)tree->user_data;

    // Cannot remove dummy root directly
    if (node == data->root_dummy) return;

    TreeNode* parent = node->parent;
    if (parent) {
        if (parent->first_child == node) {
            parent->first_child = node->next_sibling;
            if (parent->last_child == node) {
                parent->last_child = NULL;
            }
        } else {
            TreeNode* prev = parent->first_child;
            while (prev && prev->next_sibling != node) {
                prev = prev->next_sibling;
            }
            if (prev) {
                prev->next_sibling = node->next_sibling;
                if (parent->last_child == node) {
                    parent->last_child = prev;
                }
            }
        }
        parent->child_count--;
    }

    if (data->selected_node == node) {
        data->selected_node = NULL;
    }

    tree_view_destroy_node_recursive(node);
    tree->needs_layout = true;
    tree->dirty = true;
}

void tree_view_clear(Widget* tree) {
    if (!tree || !tree->user_data) return;
    TreeViewData* data = (TreeViewData*)tree->user_data;

    TreeNode* child = data->root_dummy->first_child;
    while (child) {
        TreeNode* next = child->next_sibling;
        tree_view_destroy_node_recursive(child);
        child = next;
    }

    data->root_dummy->first_child = NULL;
    data->root_dummy->last_child = NULL;
    data->root_dummy->child_count = 0;
    data->selected_node = NULL;

    tree->needs_layout = true;
    tree->dirty = true;
}

TreeNode* tree_view_get_selected(Widget* tree) {
    if (!tree || !tree->user_data) return NULL;
    TreeViewData* data = (TreeViewData*)tree->user_data;
    return data->selected_node;
}

void tree_view_set_on_select(Widget* tree, TreeViewNodeCallback callback, void* user_data) {
    if (!tree || !tree->user_data) return;
    TreeViewData* data = (TreeViewData*)tree->user_data;
    data->on_select = callback;
    data->callback_user_data = user_data;
}

// Node getters/setters

void tree_node_set_text(TreeNode* node, const char* text) {
    if (!node) return;
    if (node->text) free(node->text);
    node->text = text ? strdup(text) : NULL;
}

const char* tree_node_get_text(const TreeNode* node) {
    return node ? node->text : NULL;
}

void tree_node_set_expanded(Widget* tree, TreeNode* node, bool expanded) {
    if (!node) return;
    node->expanded = expanded;
    if (tree) {
        tree->needs_layout = true;
        tree->dirty = true;
    }
}

bool tree_node_is_expanded(const TreeNode* node) {
    return node ? node->expanded : false;
}

void tree_node_set_selected(Widget* tree, TreeNode* node, bool selected) {
    if (!node) return;

    if (selected) {
        // Deselect previous
        if (tree && tree->user_data) {
            TreeViewData* data = (TreeViewData*)tree->user_data;
            if (data->selected_node && data->selected_node != node) {
                data->selected_node->selected = false;
            }
            data->selected_node = node;
        }
        node->selected = true;
    } else {
        node->selected = false;
        if (tree && tree->user_data) {
            TreeViewData* data = (TreeViewData*)tree->user_data;
            if (data->selected_node == node) {
                data->selected_node = NULL;
            }
        }
    }

    if (tree) tree->dirty = true;
}

bool tree_node_is_selected(const TreeNode* node) {
    return node ? node->selected : false;
}

void tree_node_set_data(TreeNode* node, void* data) {
    if (node) node->user_data = data;
}

void* tree_node_get_data(const TreeNode* node) {
    return node ? node->user_data : NULL;
}

TreeNode* tree_node_get_parent(const TreeNode* node) {
    if (!node || !node->parent) return NULL;
    if (node->parent->parent == NULL && node->parent->text == NULL) return NULL; // It's dummy
    return node->parent;
}

TreeNode* tree_node_get_child(const TreeNode* node, uint32_t index) {
    if (!node) return NULL;
    TreeNode* child = node->first_child;
    for (uint32_t i = 0; i < index && child; i++) {
        child = child->next_sibling;
    }
    return child;
}

uint32_t tree_node_get_child_count(const TreeNode* node) {
    return node ? node->child_count : 0;
}

// Internal implementation

static void tree_view_destroy_node_recursive(TreeNode* node) {
    if (!node) return;

    TreeNode* child = node->first_child;
    while (child) {
        TreeNode* next = child->next_sibling;
        tree_view_destroy_node_recursive(child);
        child = next;
    }

    if (node->text) free(node->text);
    memory_free(node);
}

static float tree_view_calculate_height_recursive(TreeNode* node, float row_height) {
    float height = row_height;
    if (node->expanded) {
        TreeNode* child = node->first_child;
        while (child) {
            height += tree_view_calculate_height_recursive(child, row_height);
            child = child->next_sibling;
        }
    }
    return height;
}

static void tree_view_calculate_layout(Widget* widget) {
    TreeViewData* data = (TreeViewData*)widget->user_data;
    float total_height = 0.0f;

    TreeNode* child = data->root_dummy->first_child;
    while (child) {
        total_height += tree_view_calculate_height_recursive(child, data->row_height);
        child = child->next_sibling;
    }

    widget->preferred_size.y = total_height;
    widget->preferred_size.x = MAX(widget->preferred_size.x, 200.0f); // Default min width
}

static void tree_view_render_node(Widget* widget, TreeViewData* data, TreeNode* node, int depth, float* y_offset) {
    // Check visibility logic here (clipping)
    // For now simple log

    // Indent
    char indent_str[64] = "";
    for(int i=0; i<depth; i++) strcat(indent_str, "  ");

    char state_char = node->child_count > 0 ? (node->expanded ? '-' : '+') : ' ';
    char select_char = node->selected ? '*' : ' ';

    LOG_INFO(LOG_CAT_EDITOR, "TreeRow [%.1f]: %s%c%c %s", *y_offset, indent_str, state_char, select_char, node->text ? node->text : "");

    *y_offset += data->row_height;

    if (node->expanded) {
        TreeNode* child = node->first_child;
        while (child) {
            tree_view_render_node(widget, data, child, depth + 1, y_offset);
            child = child->next_sibling;
        }
    }
}
