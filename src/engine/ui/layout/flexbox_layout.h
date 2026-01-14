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
// #include "include/math/math.h" // Removed to avoid conflict
#include "ui/ui_types.h"
#include "layout_profiling.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct FlexboxContainer FlexboxContainer;

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
    LayoutPerformanceStats stats;
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
