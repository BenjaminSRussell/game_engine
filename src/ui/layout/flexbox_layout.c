/*
 * flexbox_layout.c
 * Flexbox Layout Algorithm Implementation
 * CSS Flexbox-inspired layout system for UI elements
 *
 * Part of the UI subsystem
 * Advanced 3D Rendering Engine
 */

#include "flexbox_layout.h"
#include "engine/include/core/logger.h"
#include "engine/include/core/memory.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_LAYOUT_ITERATIONS 10
#define LAYOUT_TOLERANCE 0.1f

// Global performance profiling state
static bool g_performance_profiling_enabled = false;

/* ============================================================================
 * INTERNAL TYPES
 * ============================================================================ */

typedef struct {
    float main_size;
    float cross_size;
    float main_baseline;
} FlexSize;

typedef struct {
    float main_pos;
    float cross_pos;
} FlexPosition;

typedef struct {
    float total_flex_grow;
    float total_flex_shrink;
    float total_auto_margin;
    uint32_t line_count;
} FlexLineMetrics;

typedef struct {
    UIElement** items;
    uint32_t count;
    float main_size;
    float cross_size;
    float main_offset;
    float cross_offset;
} FlexLine;

/* ============================================================================
 * INTERNAL FUNCTIONS
 * ============================================================================ */

static float resolve_length(float value, float parent_size, float auto_value) {
    if (value < 0.0f) return auto_value; // Auto value
    return value;
}

static bool is_main_axis_horizontal(FlexDirection direction) {
    return direction == FLEX_DIRECTION_ROW || direction == FLEX_DIRECTION_ROW_REVERSE;
}

static float get_main_margin(const BoxEdges* margins, FlexDirection direction) {
    if (is_main_axis_horizontal(direction)) {
        return margins->left + margins->right;
    } else {
        return margins->top + margins->bottom;
    }
}

static float get_cross_margin(const BoxEdges* margins, FlexDirection direction) {
    if (is_main_axis_horizontal(direction)) {
        return margins->top + margins->bottom;
    } else {
        return margins->left + margins->right;
    }
}

static FlexSize measure_element(UIElement* element, float available_main, float available_cross, 
                               FlexDirection direction) {
    FlexSize size = {0};
    
    // Resolve preferred size
    float preferred_main = resolve_length(element->preferred_size.width, available_main, -1.0f);
    float preferred_cross = resolve_length(element->preferred_size.height, available_cross, -1.0f);
    
    // Apply min/max constraints
    if (is_main_axis_horizontal(direction)) {
        preferred_main = fmaxf(element->min_size.width, fminf(element->max_size.width, preferred_main));
        preferred_cross = fmaxf(element->min_size.height, fminf(element->max_size.height, preferred_cross));
    } else {
        preferred_main = fmaxf(element->min_size.height, fminf(element->max_size.height, preferred_main));
        preferred_cross = fmaxf(element->min_size.width, fminf(element->max_size.width, preferred_cross));
    }
    
    // Resolve flex basis
    float flex_basis = element->flex_item.basis;
    if (flex_basis < 0.0f) { // Auto
        flex_basis = preferred_main;
    }
    
    size.main_size = fmaxf(0.0f, flex_basis);
    size.cross_size = fmaxf(0.0f, preferred_cross);
    
    return size;
}

static void resolve_flexible_lengths(FlexLine* line, float remaining_free_space, bool shrinking) {
    if (line->count == 0) return;
    
    float total_factor = 0.0f;
    
    // Calculate total flex factor
    for (uint32_t i = 0; i < line->count; i++) {
        UIElement* item = line->items[i];
        if (shrinking) {
            total_factor += item->flex_item.shrink;
        } else {
            total_factor += item->flex_item.grow;
        }
    }
    
    if (total_factor <= 0.0f) return;
    
    // Distribute space proportionally
    for (uint32_t i = 0; i < line->count; i++) {
        UIElement* item = line->items[i];
        float factor = shrinking ? item->flex_item.shrink : item->flex_item.grow;
        
        if (factor > 0.0f) {
            float share = (factor / total_factor) * remaining_free_space;
            
            if (shrinking) {
                // Shrink the item
                float current_size = is_main_axis_horizontal(line->container->config.direction) ?
                                  item->layout.size.width : item->layout.size.height;
                float new_size = fmaxf(0.0f, current_size - share);
                float delta = current_size - new_size;
                
                if (is_main_axis_horizontal(line->container->config.direction)) {
                    item->layout.size.width = new_size;
                } else {
                    item->layout.size.height = new_size;
                }
                
                remaining_free_space -= delta;
            } else {
                // Grow the item
                float current_size = is_main_axis_horizontal(line->container->config.direction) ?
                                  item->layout.size.width : item->layout.size.height;
                float new_size = current_size + share;
                float delta = new_size - current_size;
                
                if (is_main_axis_horizontal(line->container->config.direction)) {
                    item->layout.size.width = new_size;
                } else {
                    item->layout.size.height = new_size;
                }
                
                remaining_free_space -= delta;
            }
        }
    }
}

static void justify_content_line(FlexLine* line, JustifyContent justify, float container_main_size) {
    if (line->count == 0) return;
    
    float total_item_size = 0.0f;
    for (uint32_t i = 0; i < line->count; i++) {
        UIElement* item = line->items[i];
        float item_main_size = is_main_axis_horizontal(line->container->config.direction) ?
                              item->layout.size.width : item->layout.size.height;
        total_item_size += item_main_size + get_main_margin(&item->margin, line->container->config.direction);
    }
    
    float free_space = container_main_size - total_item_size;
    float current_pos = line->main_offset;
    
    switch (justify) {
        case JUSTIFY_FLEX_START:
            // Items start at the beginning
            break;
            
        case JUSTIFY_FLEX_END:
            // Items end at the end
            current_pos += free_space;
            break;
            
        case JUSTIFY_CENTER:
            // Items centered
            current_pos += free_space * 0.5f;
            break;
            
        case JUSTIFY_SPACE_BETWEEN:
            // Equal space between items
            if (line->count > 1) {
                float space = free_space / (line->count - 1);
                for (uint32_t i = 0; i < line->count; i++) {
                    UIElement* item = line->items[i];
                    if (is_main_axis_horizontal(line->container->config.direction)) {
                        item->layout.position.x = current_pos + item->margin.left;
                    } else {
                        item->layout.position.y = current_pos + item->margin.top;
                    }
                    
                    float item_main_size = is_main_axis_horizontal(line->container->config.direction) ?
                                          item->layout.size.width : item->layout.size.height;
                    current_pos += item_main_size + get_main_margin(&item->margin, line->container->config.direction) + space;
                }
                return;
            }
            break;
            
        case JUSTIFY_SPACE_AROUND:
            // Equal space around items
            if (line->count > 0) {
                float space = free_space / line->count;
                current_pos += space * 0.5f;
                for (uint32_t i = 0; i < line->count; i++) {
                    UIElement* item = line->items[i];
                    if (is_main_axis_horizontal(line->container->config.direction)) {
                        item->layout.position.x = current_pos + item->margin.left;
                    } else {
                        item->layout.position.y = current_pos + item->margin.top;
                    }
                    
                    float item_main_size = is_main_axis_horizontal(line->container->config.direction) ?
                                          item->layout.size.width : item->layout.size.height;
                    current_pos += item_main_size + get_main_margin(&item->margin, line->container->config.direction) + space;
                }
                return;
            }
            break;
            
        case JUSTIFY_SPACE_EVENLY:
            // Equal space between and around items
            if (line->count > 0) {
                float space = free_space / (line->count + 1);
                current_pos += space;
                for (uint32_t i = 0; i < line->count; i++) {
                    UIElement* item = line->items[i];
                    if (is_main_axis_horizontal(line->container->config.direction)) {
                        item->layout.position.x = current_pos + item->margin.left;
                    } else {
                        item->layout.position.y = current_pos + item->margin.top;
                    }
                    
                    float item_main_size = is_main_axis_horizontal(line->container->config.direction) ?
                                          item->layout.size.width : item->layout.size.height;
                    current_pos += item_main_size + get_main_margin(&item->margin, line->container->config.direction) + space;
                }
                return;
            }
            break;
    }
    
    // Default case: place items sequentially
    for (uint32_t i = 0; i < line->count; i++) {
        UIElement* item = line->items[i];
        if (is_main_axis_horizontal(line->container->config.direction)) {
            item->layout.position.x = current_pos + item->margin.left;
        } else {
            item->layout.position.y = current_pos + item->margin.top;
        }
        
        float item_main_size = is_main_axis_horizontal(line->container->config.direction) ?
                              item->layout.size.width : item->layout.size.height;
        current_pos += item_main_size + get_main_margin(&item->margin, line->container->config.direction);
    }
}

static void align_items_line(FlexLine* line, AlignItems align, float container_cross_size) {
    if (line->count == 0) return;
    
    for (uint32_t i = 0; i < line->count; i++) {
        UIElement* item = line->items[i];
        AlignSelf align_self = item->flex_item.align_self;
        
        // Use align-self if not auto, otherwise use container align-items
        AlignItems item_align = (align_self == ALIGN_SELF_AUTO) ? align : 
                              (AlignItems)align_self;
        
        float item_cross_size = is_main_axis_horizontal(line->container->config.direction) ?
                               item->layout.size.height : item->layout.size.width;
        
        float cross_pos = line->cross_offset;
        
        switch (item_align) {
            case ALIGN_ITEMS_FLEX_START:
                // Already at start position
                break;
                
            case ALIGN_ITEMS_FLEX_END:
                cross_pos = container_cross_size - item_cross_size - 
                           get_cross_margin(&item->margin, line->container->config.direction);
                break;
                
            case ALIGN_ITEMS_CENTER:
                cross_pos = (container_cross_size - item_cross_size) * 0.5f;
                break;
                
            case ALIGN_ITEMS_STRETCH:
                // Stretch to fill container
                if (is_main_axis_horizontal(line->container->config.direction)) {
                    item->layout.size.height = fmaxf(0.0f, container_cross_size - 
                                                   get_cross_margin(&item->margin, line->container->config.direction));
                } else {
                    item->layout.size.width = fmaxf(0.0f, container_cross_size - 
                                                 get_cross_margin(&item->margin, line->container->config.direction));
                }
                cross_pos = get_cross_margin(&item->margin, line->container->config.direction) * 0.5f;
                break;
                
            case ALIGN_ITEMS_BASELINE:
                // Simplified baseline alignment (use center for now)
                cross_pos = (container_cross_size - item_cross_size) * 0.5f;
                break;
        }
        
        // Apply cross position
        if (is_main_axis_horizontal(line->container->config.direction)) {
            item->layout.position.y = cross_pos + item->margin.top;
        } else {
            item->layout.position.x = cross_pos + item->margin.left;
        }
    }
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

FlexboxContainer* flexbox_container_create(const char* name) {
    FlexboxContainer* container = memory_alloc(sizeof(FlexboxContainer));
    if (!container) {
        LOG_ERROR("Failed to allocate flexbox container");
        return NULL;
    }
    
    memset(container, 0, sizeof(FlexboxContainer));
    
    // Initialize base element
    container->base.id = (uint32_t)(uintptr_t)container;
    container->base.name = name ? strdup(name) : strdup("FlexboxContainer");
    container->base.visible = true;
    container->base.dirty = true;
    
    // Initialize default configuration
    container->config.direction = FLEX_DIRECTION_ROW;
    container->config.justify_content = JUSTIFY_FLEX_START;
    container->config.align_items = ALIGN_ITEMS_STRETCH;
    container->config.align_content = ALIGN_CONTENT_STRETCH;
    container->config.wrap = FLEX_WRAP_NOWRAP;
    
    container->needs_layout = true;
    
    LOG_INFO("Created flexbox container: %s", name ? name : "unnamed");
    return container;
}

void flexbox_container_destroy(FlexboxContainer* container) {
    if (!container) return;
    
    // Destroy all children
    for (uint32_t i = 0; i < container->base.child_count; i++) {
        ui_element_destroy(container->base.children[i]);
    }
    
    if (container->base.children) {
        memory_free(container->base.children);
    }
    
    if (container->base.name) {
        free(container->base.name);
    }
    
    memory_free(container);
}

void flexbox_set_direction(FlexboxContainer* container, FlexDirection direction) {
    if (!container) return;
    
    container->config.direction = direction;
    container->needs_layout = true;
}

void flexbox_set_justify_content(FlexboxContainer* container, JustifyContent justify) {
    if (!container) return;
    
    container->config.justify_content = justify;
    container->needs_layout = true;
}

void flexbox_set_align_items(FlexboxContainer* container, AlignItems align) {
    if (!container) return;
    
    container->config.align_items = align;
    container->needs_layout = true;
}

void flexbox_set_wrap(FlexboxContainer* container, FlexWrap wrap) {
    if (!container) return;
    
    container->config.wrap = wrap;
    container->needs_layout = true;
}

UIElement* ui_element_create(const char* name) {
    UIElement* element = memory_alloc(sizeof(UIElement));
    if (!element) {
        LOG_ERROR("Failed to allocate UI element");
        return NULL;
    }
    
    memset(element, 0, sizeof(UIElement));
    
    element->id = (uint32_t)(uintptr_t)element;
    element->name = name ? strdup(name) : strdup("UIElement");
    element->visible = true;
    element->dirty = true;
    
    // Default flex properties
    element->flex_item.grow = 0.0f;
    element->flex_item.shrink = 1.0f;
    element->flex_item.basis = -1.0f; // Auto
    element->flex_item.align_self = ALIGN_SELF_AUTO;
    
    LOG_INFO("Created UI element: %s", name ? name : "unnamed");
    return element;
}

void ui_element_destroy(UIElement* element) {
    if (!element) return;
    
    // Destroy children recursively
    for (uint32_t i = 0; i < element->child_count; i++) {
        ui_element_destroy(element->children[i]);
    }
    
    if (element->children) {
        memory_free(element->children);
    }
    
    if (element->name) {
        free(element->name);
    }
    
    memory_free(element);
}

void flexbox_add_child(FlexboxContainer* container, UIElement* child) {
    if (!container || !child) return;
    
    // Ensure capacity
    if (container->base.child_count >= container->base.child_capacity) {
        uint32_t new_capacity = container->base.child_capacity == 0 ? 8 : 
                               container->base.child_capacity * 2;
        UIElement** new_children = memory_realloc(container->base.children, 
                                                 new_capacity * sizeof(UIElement*));
        if (!new_children) {
            LOG_ERROR("Failed to resize children array");
            return;
        }
        
        container->base.children = new_children;
        container->base.child_capacity = new_capacity;
    }
    
    // Add child
    child->parent = &container->base;
    container->base.children[container->base.child_count] = child;
    container->base.child_count++;
    
    container->needs_layout = true;
    child->dirty = true;
    
    LOG_INFO("Added child %s to container %s", child->name, container->base.name);
}

void ui_element_set_flex_grow(UIElement* element, float grow) {
    if (!element) return;
    
    element->flex_item.grow = fmaxf(0.0f, grow);
    element->dirty = true;
    
    if (element->parent) {
        FlexboxContainer* container = (FlexboxContainer*)element->parent;
        container->needs_layout = true;
    }
}

void ui_element_set_flex_shrink(UIElement* element, float shrink) {
    if (!element) return;
    
    element->flex_item.shrink = fmaxf(0.0f, shrink);
    element->dirty = true;
    
    if (element->parent) {
        FlexboxContainer* container = (FlexboxContainer*)element->parent;
        container->needs_layout = true;
    }
}

void ui_element_set_preferred_size(UIElement* element, Size preferred_size) {
    if (!element) return;
    
    element->preferred_size = preferred_size;
    element->dirty = true;
    
    if (element->parent) {
        FlexboxContainer* container = (FlexboxContainer*)element->parent;
        container->needs_layout = true;
    }
}

void flexbox_layout(FlexboxContainer* container, float available_width, float available_height) {
    if (!container || !container->needs_layout) return;
    
    clock_t start_time = g_performance_profiling_enabled ? clock() : 0;
    
    container->available_width = available_width;
    container->available_height = available_height;
    
    // Subtract container padding from available space
    float inner_width = available_width - container->config.padding.left - container->config.padding.right;
    float inner_height = available_height - container->config.padding.top - container->config.padding.bottom;
    
    inner_width = fmaxf(0.0f, inner_width);
    inner_height = fmaxf(0.0f, inner_height);
    
    // Determine main and cross axis sizes
    float main_size = is_main_axis_horizontal(container->config.direction) ? inner_width : inner_height;
    float cross_size = is_main_axis_horizontal(container->config.direction) ? inner_height : inner_width;
    
    // Measure all children
    for (uint32_t i = 0; i < container->base.child_count; i++) {
        UIElement* child = container->base.children[i];
        if (!child->visible) continue;
        
        FlexSize measured = measure_element(child, main_size, cross_size, container->config.direction);
        
        if (is_main_axis_horizontal(container->config.direction)) {
            child->layout.size.width = measured.main_size;
            child->layout.size.height = measured.cross_size;
        } else {
            child->layout.size.width = measured.cross_size;
            child->layout.size.height = measured.main_size;
        }
        
        child->measured = true;
    }
    
    // Simple single-line layout for now (TODO: implement wrapping)
    float total_main_size = 0.0f;
    uint32_t visible_children = 0;
    
    for (uint32_t i = 0; i < container->base.child_count; i++) {
        UIElement* child = container->base.children[i];
        if (!child->visible) continue;
        
        float child_main_size = is_main_axis_horizontal(container->config.direction) ?
                              child->layout.size.width : child->layout.size.height;
        total_main_size += child_main_size + get_main_margin(&child->margin, container->config.direction);
        visible_children++;
    }
    
    // Distribute remaining space
    float remaining_space = main_size - total_main_size;
    if (fabsf(remaining_space) > LAYOUT_TOLERANCE) {
        // Resolve flexible lengths
        if (remaining_space > 0.0f) {
            // Grow items
            for (uint32_t i = 0; i < container->base.child_count; i++) {
                UIElement* child = container->base.children[i];
                if (!child->visible || child->flex_item.grow <= 0.0f) continue;
                
                float child_main_size = is_main_axis_horizontal(container->config.direction) ?
                                      child->layout.size.width : child->layout.size.height;
                float new_size = child_main_size + (remaining_space * child->flex_item.grow / visible_children);
                
                if (is_main_axis_horizontal(container->config.direction)) {
                    child->layout.size.width = fmaxf(0.0f, new_size);
                } else {
                    child->layout.size.height = fmaxf(0.0f, new_size);
                }
            }
        } else {
            // Shrink items
            for (uint32_t i = 0; i < container->base.child_count; i++) {
                UIElement* child = container->base.children[i];
                if (!child->visible || child->flex_item.shrink <= 0.0f) continue;
                
                float child_main_size = is_main_axis_horizontal(container->config.direction) ?
                                      child->layout.size.width : child->layout.size.height;
                float new_size = child_main_size + (remaining_space * child->flex_item.shrink / visible_children);
                
                if (is_main_axis_horizontal(container->config.direction)) {
                    child->layout.size.width = fmaxf(0.0f, new_size);
                } else {
                    child->layout.size.height = fmaxf(0.0f, new_size);
                }
            }
        }
    }
    
    // Position items
    float current_main = container->config.padding.left;
    if (!is_main_axis_horizontal(container->config.direction)) {
        current_main = container->config.padding.top;
    }
    
    for (uint32_t i = 0; i < container->base.child_count; i++) {
        UIElement* child = container->base.children[i];
        if (!child->visible) continue;
        
        // Set main axis position
        if (is_main_axis_horizontal(container->config.direction)) {
            child->layout.position.x = current_main + child->margin.left;
        } else {
            child->layout.position.y = current_main + child->margin.top;
        }
        
        // Set cross axis position (simplified - use stretch)
        if (is_main_axis_horizontal(container->config.direction)) {
            child->layout.position.y = container->config.padding.top + child->margin.top;
            child->layout.size.height = fmaxf(0.0f, cross_size - get_cross_margin(&child->margin, container->config.direction));
        } else {
            child->layout.position.x = container->config.padding.left + child->margin.left;
            child->layout.size.width = fmaxf(0.0f, cross_size - get_cross_margin(&child->margin, container->config.direction));
        }
        
        // Advance main position
        float child_main_size = is_main_axis_horizontal(container->config.direction) ?
                              child->layout.size.width : child->layout.size.height;
        current_main += child_main_size + get_main_margin(&child->margin, container->config.direction);
    }
    
    // Update container size
    if (is_main_axis_horizontal(container->config.direction)) {
        container->base.layout.size.width = total_main_size + container->config.padding.left + container->config.padding.right;
        container->base.layout.size.height = cross_size + container->config.padding.top + container->config.padding.bottom;
    } else {
        container->base.layout.size.width = cross_size + container->config.padding.left + container->config.padding.right;
        container->base.layout.size.height = total_main_size + container->config.padding.top + container->config.padding.bottom;
    }
    
    container->needs_layout = false;
    container->base.dirty = false;
    
    // Update performance stats
    if (g_performance_profiling_enabled) {
        clock_t end_time = clock();
        container->layout_time_ms = ((double)(end_time - start_time)) / CLOCKS_PER_SEC * 1000.0;
        container->layout_iterations = 1;
    }
    
    LOG_DEBUG("Layout completed for container %s: %.2fx%.2f", 
              container->base.name, container->base.layout.size.width, container->base.layout.size.height);
}

void flexbox_enable_performance_profiling(bool enable) {
    g_performance_profiling_enabled = enable;
}

void flexbox_get_performance_stats(const FlexboxContainer* container, 
                                 uint32_t* iterations, float* time_ms) {
    if (!container) return;
    
    if (iterations) *iterations = container->layout_iterations;
    if (time_ms) *time_ms = container->layout_time_ms;
}
