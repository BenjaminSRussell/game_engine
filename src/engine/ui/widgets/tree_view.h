#ifndef UI_TREE_VIEW_H
#define UI_TREE_VIEW_H

#include "widget.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TreeNode TreeNode;

// Callback for node selection
typedef void (*TreeViewSelectCallback)(Widget* tree_view, TreeNode* node, void* user_data);

/**
 * Creates a new TreeView widget.
 *
 * @param name The name of the widget.
 * @return A pointer to the created widget.
 */
Widget* tree_view_create(const char* name);

/**
 * Adds a new node to the tree.
 *
 * @param tree_view The TreeView widget.
 * @param parent The parent node (NULL for root level).
 * @param text The text label for the node.
 * @return A pointer to the created TreeNode.
 */
TreeNode* tree_view_add_node(Widget* tree_view, TreeNode* parent, const char* text);

/**
 * Removes a node and its children from the tree.
 *
 * @param tree_view The TreeView widget.
 * @param node The node to remove.
 */
void tree_view_remove_node(Widget* tree_view, TreeNode* node);

/**
 * Clears all nodes from the tree.
 *
 * @param tree_view The TreeView widget.
 */
void tree_view_clear(Widget* tree_view);

/**
 * Sets the expanded state of a node.
 *
 * @param node The node.
 * @param expanded True to expand, false to collapse.
 */
void tree_node_set_expanded(TreeNode* node, bool expanded);

/**
 * Checks if a node is expanded.
 *
 * @param node The node.
 * @return True if expanded.
 */
bool tree_node_is_expanded(const TreeNode* node);

/**
 * Sets the selected state of a node.
 *
 * @param node The node.
 * @param selected True to select, false to deselect.
 */
void tree_node_set_selected(TreeNode* node, bool selected);

/**
 * Checks if a node is selected.
 *
 * @param node The node.
 * @return True if selected.
 */
bool tree_node_is_selected(const TreeNode* node);

/**
 * Gets the content widget (label) of the node.
 *
 * @param node The node.
 * @return The content widget.
 */
Widget* tree_node_get_content_widget(TreeNode* node);

/**
 * Sets user data attached to a node.
 *
 * @param node The node.
 * @param data User pointer.
 */
void tree_node_set_data(TreeNode* node, void* data);

/**
 * Gets user data attached to a node.
 *
 * @param node The node.
 * @return User pointer.
 */
void* tree_node_get_data(const TreeNode* node);

/**
 * Sets the callback for node selection.
 *
 * @param tree_view The TreeView widget.
 * @param callback The callback function.
 * @param user_data User data for the callback.
 */
void tree_view_set_on_select(Widget* tree_view, TreeViewSelectCallback callback, void* user_data);

#ifdef __cplusplus
}
#endif

#endif // UI_TREE_VIEW_H
