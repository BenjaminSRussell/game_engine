/*
 * flexbox_layout.h
 * Flexbox Layout Algorithm Implementation
 * CSS Flexbox-inspired layout system for UI elements
 *
 * Part of the UI subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef UI_FLEXBOX_LAYOUT_H
#define UI_FLEXBOX_LAYOUT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "include/math/math_all.h"
#include "ui/ui_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct UIElement UIElement;
typedef struct FlexboxContainer FlexboxContainer;

// Flex direction
typedef enum {
    FLEX_DIRECTION_ROW,
    FLEX_DIRECTION_ROW_REVERSE,
    FLEX_DIRECTION_COLUMN,
    FLEX_DIRECTION_COLUMN_REVERSE
} FlexDirection;

// Justify content (main axis alignment)
typedef enum {
    JUSTIFY_FLEX_START,
    JUSTIFY_FLEX_END,
    JUSTIFY_CENTER,
    JUSTIFY_SPACE_BETWEEN,
    JUSTIFY_SPACE_AROUND,
    JUSTIFY_SPACE_EVENLY
} JustifyContent;

// Align items (cross axis alignment)
typedef enum {
    ALIGN_ITEMS_FLEX_START,
    ALIGN_ITEMS_FLEX_END,
    ALIGN_ITEMS_CENTER,
    ALIGN_ITEMS_STRETCH,
    ALIGN_ITEMS_BASELINE
} AlignItems;

// Align self (individual cross axis alignment)
typedef enum {
    ALIGN_SELF_AUTO,
    ALIGN_SELF_FLEX_START,
    ALIGN_SELF_FLEX_END,
    ALIGN_SELF_CENTER,
    ALIGN_SELF_STRETCH,
    ALIGN_SELF_BASELINE
} AlignSelf;

// Flex wrap
typedef enum {
    FLEX_WRAP_NOWRAP,
    FLEX_WRAP_WRAP,
    FLEX_WRAP_WRAP_REVERSE
} FlexWrap;

// Align content (multi-line cross axis alignment)
typedef enum {
    ALIGN_CONTENT_FLEX_START,
    ALIGN_CONTENT_FLEX_END,
    ALIGN_CONTENT_CENTER,
    ALIGN_CONTENT_STRETCH,
    ALIGN_CONTENT_SPACE_BETWEEN,
    ALIGN_CONTENT_SPACE_AROUND
} AlignContent;

// Box model dimensions
// BoxEdges and Size moved to ui_types.h

typedef struct {
    float x;
    float y;
} Position;

// Flex item properties
typedef struct {
    float grow;          // Flex grow factor
    float shrink;        // Flex shrink factor
    float basis;         // Flex basis (auto, content, or fixed)
    AlignSelf align_self; // Individual alignment override
    float min_width;     // Minimum width constraint
    float max_width;     // Maximum width constraint
    float min_height;    // Minimum height constraint
    float max_height;    // Maximum height constraint
} FlexItem;

// Flexbox container configuration
typedef struct {
    FlexDirection direction;
    JustifyContent justify_content;
    AlignItems align_items;
    AlignContent align_content;
    FlexWrap wrap;
    
    BoxEdges padding;
    BoxEdges border;
    BoxEdges margin;
    
    Size min_size;
    Size max_size;
    Size preferred_size;
} FlexboxConfig;

// Layout result
typedef struct {
    Position position;
    Size size;
    bool visible;
    float baseline_offset;
} LayoutResult;

// UI Element base structure
struct UIElement {
    uint32_t id;
    char* name;
    
    // Layout properties
    FlexItem flex_item;
    LayoutResult layout;
    
    // Visual properties
    BoxEdges margin;
    BoxEdges padding;
    BoxEdges border;
    
    Size min_size;
    Size max_size;
    Size preferred_size;
    
    // Hierarchy
    UIElement* parent;
    UIElement** children;
    uint32_t child_count;
    uint32_t child_capacity;
    
    // State
    bool visible;
    bool dirty;
    bool measured;
    
    // User data
    void* user_data;
};

// Flexbox container
struct FlexboxContainer {
    UIElement base;
    FlexboxConfig config;
    
    // Layout state
    bool needs_layout;
    float available_width;
    float available_height;
    
    // Performance tracking
    uint32_t layout_iterations;
    float layout_time_ms;
};

/* ============================================================================
 * API
 * ============================================================================ */

/* Container Management */
FlexboxContainer* flexbox_container_create(const char* name);
void flexbox_container_destroy(FlexboxContainer* container);

/* Configuration */
void flexbox_set_direction(FlexboxContainer* container, FlexDirection direction);
void flexbox_set_justify_content(FlexboxContainer* container, JustifyContent justify);
void flexbox_set_align_items(FlexboxContainer* container, AlignItems align);
void flexbox_set_align_content(FlexboxContainer* container, AlignContent align);
void flexbox_set_wrap(FlexboxContainer* container, FlexWrap wrap);
void flexbox_set_padding(FlexboxContainer* container, BoxEdges padding);
void flexbox_set_margin(FlexboxContainer* container, BoxEdges margin);

/* Element Management */
UIElement* ui_element_create(const char* name);
void ui_element_destroy(UIElement* element);
void flexbox_add_child(FlexboxContainer* container, UIElement* child);
void flexbox_remove_child(FlexboxContainer* container, UIElement* child);

/* Flex Item Properties */
void ui_element_set_flex_grow(UIElement* element, float grow);
void ui_element_set_flex_shrink(UIElement* element, float shrink);
void ui_element_set_flex_basis(UIElement* element, float basis);
void ui_element_set_align_self(UIElement* element, AlignSelf align_self);
void ui_element_set_min_size(UIElement* element, Size min_size);
void ui_element_set_max_size(UIElement* element, Size max_size);
void ui_element_set_preferred_size(UIElement* element, Size preferred_size);

/* Layout Algorithm */
void flexbox_layout(FlexboxContainer* container, float available_width, float available_height);
void flexbox_invalidate_layout(FlexboxContainer* container);
bool flexbox_needs_layout(const FlexboxContainer* container);

/* Layout Queries */
LayoutResult ui_element_get_layout(const UIElement* element);
Size ui_element_get_content_size(const UIElement* element);
Position ui_element_get_content_position(const UIElement* element);

/* Utility Functions */
Size flexbox_measure_content(const UIElement* element);
float flexbox_resolve_flex_basis(const UIElement* element, float container_main_size);
bool flexbox_is_main_axis_horizontal(FlexDirection direction);
bool flexbox_is_wrap_reversed(FlexWrap wrap);

/* Performance */
void flexbox_enable_performance_profiling(bool enable);
void flexbox_get_performance_stats(const FlexboxContainer* container, 
                                 uint32_t* iterations, float* time_ms);

#ifdef __cplusplus
}
#endif

#endif /* UI_FLEXBOX_LAYOUT_H */
