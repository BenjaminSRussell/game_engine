#include "ui/ui_types.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <include/math/math.h>
#include <stdio.h>

/* =================================================================================================
 *                                UI LAYOUT SOLVER (FLEXBOX-LIKE)
 * =================================================================================================
 * 
 * Two-pass layout algorithm:
 * 1. Measure Pass: Calculate min/max sizes from leaves to root
 * 2. Arrange Pass: Calculate final positions and sizes from root to leaves
 * 
 * Features:
 * - Row/Column direction with reverse support
 * - Flex grow/shrink for dynamic sizing
 * - Alignment (start, center, end, stretch, baseline)
 * - Justify content (start, center, end, space-between, space-around, space-evenly)
 * - Multi-line wrapping
 * - Absolute and relative positioning
 * - Z-index ordering
 * - Dirty flag propagation for optimization
 * =================================================================================================
 */

// Helper: Get total margin for axis
static float get_margin_axis(const LayoutNode *node, bool is_horizontal) {
    if (is_horizontal) {
        return node->margin[1] + node->margin[3]; // right + left
    } else {
        return node->margin[0] + node->margin[2]; // top + bottom
    }
}

// Helper: Get total padding for axis
static float get_padding_axis(const LayoutNode *node, bool is_horizontal) {
    if (is_horizontal) {
        return node->padding[1] + node->padding[3]; // right + left
    } else {
        return node->padding[0] + node->padding[2]; // top + bottom
    }
}

// Helper: Check if direction is horizontal
static bool is_direction_horizontal(LayoutDirection dir) {
    return dir == LAYOUT_DIRECTION_ROW || dir == LAYOUT_DIRECTION_ROW_REVERSE;
}

// Helper: Check if direction is reverse
static bool is_direction_reverse(LayoutDirection dir) {
    return dir == LAYOUT_DIRECTION_ROW_REVERSE || dir == LAYOUT_DIRECTION_COLUMN_REVERSE;
}

// Forward declaration
void layout_node_mark_dirty(LayoutNode *node);

// Create a new layout node
LayoutNode *layout_node_create(uint32_t id, const char *name) {
    LayoutNode *node = (LayoutNode *)calloc(1, sizeof(LayoutNode));
    if (!node) return NULL;
    
    node->id = id;
    node->name = name ? strdup(name) : NULL;
    node->direction = LAYOUT_DIRECTION_ROW;
    node->align_items = ALIGN_STRETCH;
    node->align_self = ALIGN_START;
    node->align_content = ALIGN_START;
    node->justify_content = JUSTIFY_START;
    node->wrap = WRAP_NO_WRAP;
    node->position = POSITION_RELATIVE;
    node->flex_grow = 0.0f;
    node->flex_shrink = 1.0f;
    node->flex_basis = -1.0f; // Auto
    node->is_dirty = true;
    node->needs_layout = true;
    node->is_visible = true;
    node->z_index = 0;
    
    // Initialize constraints
    node->constraints.min_width = 0.0f;
    node->constraints.min_height = 0.0f;
    node->constraints.max_width = INFINITY;
    node->constraints.max_height = INFINITY;
    node->constraints.preferred_width = -1.0f; // Auto
    node->constraints.preferred_height = -1.0f; // Auto
    
    return node;
}

// Destroy layout node and children
void layout_node_destroy(LayoutNode *node) {
    if (!node) return;
    
    // Destroy all children
    for (uint32_t i = 0; i < node->child_count; i++) {
        layout_node_destroy(node->children[i]);
    }
    
    free(node->children);
    if (node->name) free((void*)node->name);
    free(node);
}

// Add child to layout node
void layout_node_add_child(LayoutNode *parent, LayoutNode *child) {
    if (!parent || !child) return;
    
    // Allocate/resize children array
    if (parent->child_count >= parent->child_capacity) {
        uint32_t new_capacity = parent->child_capacity == 0 ? 4 : parent->child_capacity * 2;
        LayoutNode **new_children = (LayoutNode **)realloc(parent->children, 
                                                           new_capacity * sizeof(LayoutNode *));
        if (!new_children) return;
        parent->children = new_children;
        parent->child_capacity = new_capacity;
    }
    
    parent->children[parent->child_count++] = child;
    child->parent = parent;
    
    // Mark parent as dirty
    layout_node_mark_dirty(parent);
}

// Mark node and ancestors as dirty
void layout_node_mark_dirty(LayoutNode *node) {
    while (node) {
        if (node->is_dirty) break; // Already marked, ancestors are too
        node->is_dirty = true;
        node->needs_layout = true;
        node = node->parent;
    }
}

// MEASURE PASS: Calculate min/max sizes bottom-up
static void layout_measure_node(LayoutNode *node) {
    if (!node || !node->is_visible) return;
    
    // Measure children first (bottom-up)
    for (uint32_t i = 0; i < node->child_count; i++) {
        layout_measure_node(node->children[i]);
    }
    
    // Calculate intrinsic size based on children
    bool is_horizontal = is_direction_horizontal(node->direction);
    float main_size = 0.0f;
    float cross_size = 0.0f;
    
    for (uint32_t i = 0; i < node->child_count; i++) {
        LayoutNode *child = node->children[i];
        if (!child->is_visible || child->position == POSITION_ABSOLUTE) continue;
        
        float child_main = is_horizontal ? child->computed_width : child->computed_height;
        float child_cross = is_horizontal ? child->computed_height : child->computed_width;
        
        child_main += get_margin_axis(child, is_horizontal);
        child_cross += get_margin_axis(child, !is_horizontal);
        
        if (node->wrap == WRAP_NO_WRAP) {
            main_size += child_main;
            cross_size = fmaxf(cross_size, child_cross);
        } else {
            // Wrapping - more complex calculation needed
            main_size += child_main;
            cross_size = fmaxf(cross_size, child_cross);
        }
    }
    
    // Add padding
    main_size += get_padding_axis(node, is_horizontal);
    cross_size += get_padding_axis(node, !is_horizontal);
    
    // Set computed size (or use explicit width/height if set)
    if (is_horizontal) {
        node->computed_width = node->width > 0 ? node->width : main_size;
        node->computed_height = node->height > 0 ? node->height : cross_size;
    } else {
        node->computed_width = node->width > 0 ? node->width : cross_size;
        node->computed_height = node->height > 0 ? node->height : main_size;
    }
    
    // Apply constraints
    node->computed_width = fmaxf(node->constraints.min_width, 
                                fminf(node->computed_width, node->constraints.max_width));
    node->computed_height = fmaxf(node->constraints.min_height,
                                 fminf(node->computed_height, node->constraints.max_height));
}

// ARRANGE PASS: Calculate final positions top-down
static void layout_arrange_node(LayoutNode *node, float x, float y, float width, float height) {
    if (!node || !node->is_visible) return;
    
    // Set node position and size
    node->computed_x = x;
    node->computed_y = y;
    node->computed_width = width;
    node->computed_height = height;
    
    // Calculate content area (minus padding)
    float content_x = x + node->padding[3]; // left padding
    float content_y = y + node->padding[0]; // top padding
    float content_width = width - node->padding[1] - node->padding[3]; // minus right and left
    float content_height = height - node->padding[0] - node->padding[2]; // minus top and bottom
    
    // Arrange children
    bool is_horizontal = is_direction_horizontal(node->direction);
    bool is_reverse = is_direction_reverse(node->direction);
    
    // Calculate total flex grow/shrink
    float total_flex_grow = 0.0f;
    float total_flex_shrink = 0.0f;
    float total_fixed_size = 0.0f;
    uint32_t visible_children = 0;
    
    for (uint32_t i = 0; i < node->child_count; i++) {
        LayoutNode *child = node->children[i];
        if (!child->is_visible || child->position == POSITION_ABSOLUTE) continue;
        
        visible_children++;
        total_flex_grow += child->flex_grow;
        total_flex_shrink += child->flex_shrink;
        
        float child_size = is_horizontal ? child->computed_width : child->computed_height;
        total_fixed_size += child_size + get_margin_axis(child, is_horizontal);
    }
    
    // Calculate available space
    float available_space = is_horizontal ? content_width : content_height;
    float remaining_space = available_space - total_fixed_size;
    
    // Distribute flex space
    float flex_unit = 0.0f;
    if (remaining_space > 0 && total_flex_grow > 0) {
        flex_unit = remaining_space / total_flex_grow;
    } else if (remaining_space < 0 && total_flex_shrink > 0) {
        flex_unit = remaining_space / total_flex_shrink;
    }
    
    // Calculate spacing for justify-content
    float spacing = 0.0f;
    float offset = 0.0f;
    
    switch (node->justify_content) {
        case JUSTIFY_START:
            offset = 0.0f;
            break;
        case JUSTIFY_CENTER:
            offset = remaining_space / 2.0f;
            break;
        case JUSTIFY_END:
            offset = remaining_space;
            break;
        case JUSTIFY_SPACE_BETWEEN:
            spacing = visible_children > 1 ? remaining_space / (visible_children - 1) : 0.0f;
            break;
        case JUSTIFY_SPACE_AROUND:
            spacing = visible_children > 0 ? remaining_space / visible_children : 0.0f;
            offset = spacing / 2.0f;
            break;
        case JUSTIFY_SPACE_EVENLY:
            spacing = visible_children > 0 ? remaining_space / (visible_children + 1) : 0.0f;
            offset = spacing;
            break;
    }
    
    // Position children
    float current_pos = is_horizontal ? content_x : content_y;
    current_pos += offset;
    
    for (uint32_t i = 0; i < node->child_count; i++) {
        uint32_t child_idx = is_reverse ? (node->child_count - 1 - i) : i;
        LayoutNode *child = node->children[child_idx];
        
        if (!child->is_visible) continue;
        
        // Handle absolute positioning
        if (child->position == POSITION_ABSOLUTE) {
            float abs_x = x + child->margin[3];
            float abs_y = y + child->margin[0];
            layout_arrange_node(child, abs_x, abs_y, child->computed_width, child->computed_height);
            continue;
        }
        
        // Calculate child size with flex
        float child_main_size = is_horizontal ? child->computed_width : child->computed_height;
        float child_cross_size = is_horizontal ? child->computed_height : child->computed_width;
        
        if (flex_unit != 0.0f) {
            if (remaining_space > 0) {
                child_main_size += child->flex_grow * flex_unit;
            } else {
                child_main_size += child->flex_shrink * flex_unit;
            }
        }
        
        // Handle cross-axis alignment
        float cross_offset = 0.0f;
        LayoutAlign align = child->align_self != ALIGN_START ? child->align_self : node->align_items;
        
        switch (align) {
            case ALIGN_START:
                cross_offset = 0.0f;
                break;
            case ALIGN_CENTER:
                cross_offset = ((is_horizontal ? content_height : content_width) - child_cross_size) / 2.0f;
                break;
            case ALIGN_END:
                cross_offset = (is_horizontal ? content_height : content_width) - child_cross_size;
                break;
            case ALIGN_STRETCH:
                child_cross_size = is_horizontal ? content_height : content_width;
                break;
            case ALIGN_BASELINE:
                // Simplified - treat as start for now
                cross_offset = 0.0f;
                break;
        }
        
        // Calculate child position
        float child_x, child_y, child_w, child_h;
        if (is_horizontal) {
            child_x = current_pos + child->margin[3];
            child_y = content_y + cross_offset + child->margin[0];
            child_w = child_main_size;
            child_h = child_cross_size;
        } else {
            child_x = content_x + cross_offset + child->margin[3];
            child_y = current_pos + child->margin[0];
            child_w = child_cross_size;
            child_h = child_main_size;
        }
        
        // Recursively arrange child
        layout_arrange_node(child, child_x, child_y, child_w, child_h);
        
        // Advance position
        current_pos += child_main_size + get_margin_axis(child, is_horizontal) + spacing;
    }
    
    node->is_dirty = false;
    node->needs_layout = false;
}

// Main layout function - performs two-pass layout
void layout_solve(LayoutNode *root, float width, float height) {
    if (!root) return;
    
    // Only layout if dirty
    if (!root->needs_layout && !root->is_dirty) return;
    
    // PASS 1: Measure (bottom-up)
    layout_measure_node(root);
    
    // PASS 2: Arrange (top-down)
    layout_arrange_node(root, 0.0f, 0.0f, width > 0 ? width : root->computed_width, 
                       height > 0 ? height : root->computed_height);
}

// Get layout bounds for a node
void layout_get_bounds(const LayoutNode *node, float *x, float *y, float *width, float *height) {
    if (!node) return;
    if (x) *x = node->computed_x;
    if (y) *y = node->computed_y;
    if (width) *width = node->computed_width;
    if (height) *height = node->computed_height;
}

// Debug: Print layout tree
void layout_debug_print(const LayoutNode *node, int depth) {
    if (!node) return;
    
    for (int i = 0; i < depth; i++) printf("  ");
    printf("%s [%u]: pos=(%.1f, %.1f) size=(%.1f x %.1f) z=%d%s\n",
           node->name ? node->name : "node",
           node->id,
           node->computed_x, node->computed_y,
           node->computed_width, node->computed_height,
           node->z_index,
           node->is_dirty ? " [DIRTY]" : "");
    
    for (uint32_t i = 0; i < node->child_count; i++) {
        layout_debug_print(node->children[i], depth + 1);
    }
}

