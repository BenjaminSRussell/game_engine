#ifndef UI_TREE_VIEW_H
#define UI_TREE_VIEW_H

#include "widget.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct TreeNode TreeNode;

// Callback for node selection
typedef void (*TreeViewNodeCallback)(Widget* tree, TreeNode* node, void* user_data);

/**
 * Creates a new TreeView widget.
 *
 * @param name The name of the widget.
 * @return A pointer to the created widget, or NULL on failure.
 */
Widget* tree_view_create(const char* name);

/**
 * Adds a new node to the tree view.
 *
 * @param tree The TreeView widget.
 * @param parent The parent node, or NULL to add to the root.
 * @param text The text to display for the node.
 * @return A pointer to the created node.
 */
TreeNode* tree_view_add_node(Widget* tree, TreeNode* parent, const char* text);

/**
 * Removes a node and all its children from the tree view.
 *
 * @param tree The TreeView widget.
 * @param node The node to remove.
 */
void tree_view_remove_node(Widget* tree, TreeNode* node);

/**
 * Removes all nodes from the tree view.
 *
 * @param tree The TreeView widget.
 */
void tree_view_clear(Widget* tree);

/**
 * Gets the selected node.
 *
 * @param tree The TreeView widget.
 * @return The selected node, or NULL if none selected.
 */
TreeNode* tree_view_get_selected(Widget* tree);

/**
 * Sets the callback function for node selection.
 *
 * @param tree The TreeView widget.
 * @param callback The callback function.
 * @param user_data User data to pass to the callback.
 */
void tree_view_set_on_select(Widget* tree, TreeViewNodeCallback callback, void* user_data);

// Node operations
void tree_node_set_text(TreeNode* node, const char* text);
const char* tree_node_get_text(const TreeNode* node);

void tree_node_set_expanded(Widget* tree, TreeNode* node, bool expanded);
bool tree_node_is_expanded(const TreeNode* node);

void tree_node_set_selected(Widget* tree, TreeNode* node, bool selected);
bool tree_node_is_selected(const TreeNode* node);

void tree_node_set_data(TreeNode* node, void* data);
void* tree_node_get_data(const TreeNode* node);

TreeNode* tree_node_get_parent(const TreeNode* node);
TreeNode* tree_node_get_child(const TreeNode* node, uint32_t index);
uint32_t tree_node_get_child_count(const TreeNode* node);

#ifdef __cplusplus
}
#endif

#endif // UI_TREE_VIEW_H
