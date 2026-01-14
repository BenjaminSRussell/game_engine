/*
 * flexbox_layout.c
 * Flexbox Layout Algorithm Implementation
 * CSS Flexbox-inspired layout system for UI elements
 *
 * Part of the UI subsystem
 * Advanced 3D Rendering Engine
 */

#include "flexbox_layout.h"
#include "core/logger.h"
#include "core/memory.h"
#include "core/time_system.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

/* Use standard allocation if core headers don't provide it */
#ifndef core_alloc
#define core_alloc(size) malloc(size)
#endif
#ifndef core_free
#define core_free(ptr) free(ptr)
#endif
#ifndef core_realloc
#define core_realloc(ptr, size) realloc(ptr, size)
#endif

// ... rest of file (same as before) ...
// I will not copy-paste the whole file to save tokens, assuming write_file overwrites.
// Wait, write_file overwrites. I must provide full content.
// I will reuse the previous content but add the include back.

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
    UIElement** items;
    uint32_t count;
    float main_size;
    float cross_size;
    float main_offset;
    float cross_offset;
    float total_flex_grow;
    float total_flex_shrink;
    FlexboxContainer* container;
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
    
    // Handle orientation for measurement if needed, but preferred_size is WxH
    if (!is_main_axis_horizontal(direction)) {
        // Swap for calculation if we want main/cross logic, but here we just want to clamp
        float temp = preferred_main;
        preferred_main = preferred_cross;
        preferred_cross = temp;
    }

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

static void resolve_flexible_lengths(FlexLine* line, float container_main_size) {
    if (line->count == 0) return;
    
    // Calculate used main size
    float used_main_size = 0.0f;
    for (uint32_t i = 0; i < line->count; i++) {
        UIElement* item = line->items[i];
        float item_main_size = is_main_axis_horizontal(line->container->config.direction) ?
                              item->layout.size.width : item->layout.size.height;
        used_main_size += item_main_size + get_main_margin(&item->margin, line->container->config.direction);
    }
    
    float remaining_free_space = container_main_size - used_main_size;
    bool shrinking = remaining_free_space < 0.0f;
    float total_factor = shrinking ? line->total_flex_shrink : line->total_flex_grow;

    if (total_factor <= 0.0f) return;
    
    // Distribute space proportionally
    for (uint32_t i = 0; i < line->count; i++) {
        UIElement* item = line->items[i];
        float factor = shrinking ? item->flex_item.shrink : item->flex_item.grow;
        
        if (factor > 0.0f) {
            float share = (factor / total_factor) * remaining_free_space;
            
            float current_size = is_main_axis_horizontal(line->container->config.direction) ?
                                item->layout.size.width : item->layout.size.height;

            float new_size = fmaxf(0.0f, current_size + share);

            if (is_main_axis_horizontal(line->container->config.direction)) {
                item->layout.size.width = new_size;
            } else {
                item->layout.size.height = new_size;
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

static void align_items_line(FlexLine* line, AlignItems align, float line_cross_size, float cross_start) {
    if (line->count == 0) return;
    
    for (uint32_t i = 0; i < line->count; i++) {
        UIElement* item = line->items[i];
        AlignSelf align_self = item->flex_item.align_self;
        
        AlignItems item_align = (align_self == ALIGN_SELF_AUTO) ? align : (AlignItems)align_self;
        
        float item_cross_size = is_main_axis_horizontal(line->container->config.direction) ?
                               item->layout.size.height : item->layout.size.width;
        
        float cross_pos = cross_start;
        
        switch (item_align) {
            case ALIGN_ITEMS_FLEX_START:
                cross_pos = cross_start;
                break;
                
            case ALIGN_ITEMS_FLEX_END:
                cross_pos = cross_start + line_cross_size - item_cross_size -
                           get_cross_margin(&item->margin, line->container->config.direction);
                break;
                
            case ALIGN_ITEMS_CENTER:
                cross_pos = cross_start + (line_cross_size - item_cross_size) * 0.5f;
                break;
                
            case ALIGN_ITEMS_STRETCH:
                if (is_main_axis_horizontal(line->container->config.direction)) {
                    item->layout.size.height = fmaxf(0.0f, line_cross_size -
                                                   get_cross_margin(&item->margin, line->container->config.direction));
                } else {
                    item->layout.size.width = fmaxf(0.0f, line_cross_size -
                                                 get_cross_margin(&item->margin, line->container->config.direction));
                }
                cross_pos = cross_start + get_cross_margin(&item->margin, line->container->config.direction) * 0.5f; // Roughly center/start
                break;
                
            case ALIGN_ITEMS_BASELINE:
                // Simplified baseline
                cross_pos = cross_start + (line_cross_size - item_cross_size) * 0.5f;
                break;
        }
        
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
    FlexboxContainer* container = core_alloc(sizeof(FlexboxContainer));
    if (!container) {
        LOGE("Failed to allocate flexbox container");
        return NULL;
    }
    
    memset(container, 0, sizeof(FlexboxContainer));
    
    // Initialize base element
    container->base.id = (uint32_t)(uintptr_t)container;
    container->base.name = name ? strdup(name) : strdup("FlexboxContainer");
    container->base.visible = true;
    container->base.dirty = true;
    container->base.layout_type = LAYOUT_TYPE_FLEX;

    // Init max size to FLT_MAX
    container->base.max_size.width = FLT_MAX;
    container->base.max_size.height = FLT_MAX;
    
    // Initialize default configuration
    container->config.direction = FLEX_DIRECTION_ROW;
    container->config.justify_content = JUSTIFY_FLEX_START;
    container->config.align_items = ALIGN_ITEMS_STRETCH;
    container->config.align_content = ALIGN_CONTENT_STRETCH;
    container->config.wrap = FLEX_WRAP_NOWRAP;
    
    container->needs_layout = true;

    layout_profiling_reset(&container->stats);
    
    LOGI("Created flexbox container: %s", name ? name : "unnamed");
    return container;
}

void flexbox_container_destroy(FlexboxContainer* container) {
    if (!container) return;
    
    // Destroy all children
    for (uint32_t i = 0; i < container->base.child_count; i++) {
        ui_element_destroy(container->base.children[i]);
    }
    
    if (container->base.children) {
        core_free(container->base.children);
    }
    
    if (container->base.name) {
        free(container->base.name);
    }
    
    core_free(container);
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

void flexbox_set_align_content(FlexboxContainer* container, AlignContent align) {
    if (!container) return;
    container->config.align_content = align;
    container->needs_layout = true;
}

void flexbox_set_wrap(FlexboxContainer* container, FlexWrap wrap) {
    if (!container) return;
    container->config.wrap = wrap;
    container->needs_layout = true;
}

UIElement* ui_element_create(const char* name) {
    UIElement* element = core_alloc(sizeof(UIElement));
    if (!element) {
        LOGE("Failed to allocate UI element");
        return NULL;
    }
    
    memset(element, 0, sizeof(UIElement));
    
    element->id = (uint32_t)(uintptr_t)element;
    element->name = name ? strdup(name) : strdup("UIElement");
    element->visible = true;
    element->dirty = true;
    element->layout_type = LAYOUT_TYPE_NONE;

    // Init max size to FLT_MAX
    element->max_size.width = FLT_MAX;
    element->max_size.height = FLT_MAX;
    
    // Default flex properties
    element->flex_item.grow = 0.0f;
    element->flex_item.shrink = 1.0f;
    element->flex_item.basis = -1.0f; // Auto
    element->flex_item.align_self = ALIGN_SELF_AUTO;
    
    LOGI("Created UI element: %s", name ? name : "unnamed");
    return element;
}

void ui_element_destroy(UIElement* element) {
    if (!element) return;
    
    // Destroy children recursively
    for (uint32_t i = 0; i < element->child_count; i++) {
        ui_element_destroy(element->children[i]);
    }
    
    if (element->children) {
        core_free(element->children);
    }
    
    if (element->name) {
        free(element->name);
    }
    
    core_free(element);
}

void flexbox_add_child(FlexboxContainer* container, UIElement* child) {
    if (!container || !child) return;
    
    // Ensure capacity
    if (container->base.child_count >= container->base.child_capacity) {
        uint32_t new_capacity = container->base.child_capacity == 0 ? 8 : 
                               container->base.child_capacity * 2;
        UIElement** new_children = core_realloc(container->base.children,
                                                 new_capacity * sizeof(UIElement*));
        if (!new_children) {
            LOGE("Failed to resize children array");
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
    
    LOGI("Added child %s to container %s", child->name, container->base.name);
}

void ui_element_set_flex_grow(UIElement* element, float grow) {
    if (!element) return;
    element->flex_item.grow = fmaxf(0.0f, grow);
    element->dirty = true;
    if (element->parent) {
        // Assume parent is a container
        ((FlexboxContainer*)element->parent)->needs_layout = true;
    }
}

void ui_element_set_flex_shrink(UIElement* element, float shrink) {
    if (!element) return;
    element->flex_item.shrink = fmaxf(0.0f, shrink);
    element->dirty = true;
    if (element->parent) {
        ((FlexboxContainer*)element->parent)->needs_layout = true;
    }
}

void ui_element_set_preferred_size(UIElement* element, Size preferred_size) {
    if (!element) return;
    element->preferred_size = preferred_size;
    element->dirty = true;
    if (element->parent) {
        ((FlexboxContainer*)element->parent)->needs_layout = true;
    }
}

void flexbox_layout(FlexboxContainer* container, float available_width, float available_height) {
    if (!container || !container->needs_layout) return;
    
    uint64_t start_ns = 0;
    if (g_performance_profiling_enabled) {
        start_ns = get_time_nanos();
    }
    
    LOGD("Layout container %s, available: %.2f x %.2f", container->base.name, available_width, available_height);

    container->available_width = available_width;
    container->available_height = available_height;
    
    // Subtract container padding
    float inner_width = available_width - container->config.padding.left - container->config.padding.right;
    float inner_height = available_height - container->config.padding.top - container->config.padding.bottom;
    inner_width = fmaxf(0.0f, inner_width);
    inner_height = fmaxf(0.0f, inner_height);
    
    float main_size = is_main_axis_horizontal(container->config.direction) ? inner_width : inner_height;
    float cross_size = is_main_axis_horizontal(container->config.direction) ? inner_height : inner_width;
    
    // 1. Measure children
    for (uint32_t i = 0; i < container->base.child_count; i++) {
        UIElement* child = container->base.children[i];
        if (!child->visible) continue;
        
        FlexSize measured = measure_element(child, main_size, cross_size, container->config.direction);
        LOGD("Measured child %s: %.2f x %.2f (Preferred: %.2f x %.2f)",
             child->name, measured.main_size, measured.cross_size,
             child->preferred_size.width, child->preferred_size.height);

        if (is_main_axis_horizontal(container->config.direction)) {
            child->layout.size.width = measured.main_size;
            child->layout.size.height = measured.cross_size;
        } else {
            child->layout.size.width = measured.cross_size;
            child->layout.size.height = measured.main_size;
        }
        child->measured = true;
    }

    uint64_t measure_end_ns = 0;
    if (g_performance_profiling_enabled) {
        measure_end_ns = get_time_nanos();
    }

    // 2. Collect lines (Wrapping logic)
    // We'll allocate a dynamic array of lines. For simplicity, start with child_count worst case.
    uint32_t max_lines = container->base.child_count + 1;
    FlexLine* lines = core_alloc(sizeof(FlexLine) * max_lines);
    // Zero init lines
    memset(lines, 0, sizeof(FlexLine) * max_lines);

    uint32_t line_count = 0;
    
    // Initialize first line
    lines[0].container = container;
    lines[0].items = core_alloc(sizeof(UIElement*) * container->base.child_count);
    lines[0].count = 0;
    lines[0].main_size = 0.0f;
    lines[0].cross_size = 0.0f;
    lines[0].total_flex_grow = 0.0f;
    lines[0].total_flex_shrink = 0.0f;
    line_count = 1;

    for (uint32_t i = 0; i < container->base.child_count; i++) {
        UIElement* child = container->base.children[i];
        if (!child->visible) continue;
        
        float child_main = is_main_axis_horizontal(container->config.direction) ? child->layout.size.width : child->layout.size.height;
        float child_main_total = child_main + get_main_margin(&child->margin, container->config.direction);

        LOGD("  Item %s main: %.2f (total %.2f)", child->name, child_main, child_main_total);

        if (container->config.wrap != FLEX_WRAP_NOWRAP &&
            lines[line_count-1].count > 0 &&
            lines[line_count-1].main_size + child_main_total > main_size + LAYOUT_TOLERANCE) {

            // New line
            line_count++;
            lines[line_count-1].container = container;
            lines[line_count-1].items = core_alloc(sizeof(UIElement*) * container->base.child_count);
            lines[line_count-1].count = 0;
            lines[line_count-1].main_size = 0.0f;
            lines[line_count-1].cross_size = 0.0f;
            lines[line_count-1].total_flex_grow = 0.0f;
            lines[line_count-1].total_flex_shrink = 0.0f;
            LOGD("  New line created (index %d)", line_count-1);
        }

        FlexLine* current_line = &lines[line_count-1];
        current_line->items[current_line->count++] = child;
        current_line->main_size += child_main_total;
        current_line->total_flex_grow += child->flex_item.grow;
        current_line->total_flex_shrink += child->flex_item.shrink;

        float child_cross = is_main_axis_horizontal(container->config.direction) ? child->layout.size.height : child->layout.size.width;
        float child_cross_total = child_cross + get_cross_margin(&child->margin, container->config.direction);
        current_line->cross_size = fmaxf(current_line->cross_size, child_cross_total);
    }
    
    // 3. Process each line
    float total_cross_size = 0.0f;
    for (uint32_t i = 0; i < line_count; i++) {
        FlexLine* line = &lines[i];

        // Resolve flexible lengths
        resolve_flexible_lengths(line, main_size);

        // Recalculate line size after flex resolution
        line->main_size = 0.0f;
        line->cross_size = 0.0f;
        for (uint32_t j = 0; j < line->count; j++) {
            UIElement* item = line->items[j];
            float item_main = is_main_axis_horizontal(container->config.direction) ? item->layout.size.width : item->layout.size.height;
            line->main_size += item_main + get_main_margin(&item->margin, container->config.direction);

            float item_cross = is_main_axis_horizontal(container->config.direction) ? item->layout.size.height : item->layout.size.width;
            line->cross_size = fmaxf(line->cross_size, item_cross + get_cross_margin(&item->margin, container->config.direction));
        }

        total_cross_size += line->cross_size;
    }
    
    // 4. Align content (distribute lines along cross axis)
    float free_cross_space = cross_size - total_cross_size;
    float current_cross_pos = is_main_axis_horizontal(container->config.direction) ? container->config.padding.top : container->config.padding.left;

    float spacing = 0.0f;
    if (line_count > 1 && free_cross_space > 0) {
        switch (container->config.align_content) {
            case ALIGN_CONTENT_FLEX_START:
                break;
            case ALIGN_CONTENT_FLEX_END:
                current_cross_pos += free_cross_space;
                break;
            case ALIGN_CONTENT_CENTER:
                current_cross_pos += free_cross_space * 0.5f;
                break;
            case ALIGN_CONTENT_SPACE_BETWEEN:
                spacing = free_cross_space / (line_count - 1);
                break;
            case ALIGN_CONTENT_SPACE_AROUND:
                spacing = free_cross_space / line_count;
                current_cross_pos += spacing * 0.5f;
                break;
            case ALIGN_CONTENT_STRETCH:
                // Distribute free space to lines
                {
                    float extra = free_cross_space / line_count;
                    for (uint32_t i = 0; i < line_count; i++) {
                        lines[i].cross_size += extra;
                    }
                }
                break;
        }
    } else if (line_count == 1 && container->config.align_content == ALIGN_CONTENT_STRETCH) {
         if (container->config.wrap == FLEX_WRAP_NOWRAP) {
             // For single line NOWRAP, the line stretches to fill cross size
             lines[0].cross_size = cross_size;
         }
    }
    
    // 5. Layout each line
    for (uint32_t i = 0; i < line_count; i++) {
        // Handle Wrap Reverse
        FlexLine* line = (container->config.wrap == FLEX_WRAP_WRAP_REVERSE) ? &lines[line_count - 1 - i] : &lines[i];
        
        line->main_offset = is_main_axis_horizontal(container->config.direction) ? container->config.padding.left : container->config.padding.top;
        
        // Justify content (main axis)
        justify_content_line(line, container->config.justify_content, main_size);

        // Align items (cross axis)
        align_items_line(line, container->config.align_items, line->cross_size, current_cross_pos);
        
        current_cross_pos += line->cross_size + spacing;
    }

    // Update container total size
    float final_main_size = 0.0f;
    for (uint32_t i = 0; i < line_count; i++) {
        final_main_size = fmaxf(final_main_size, lines[i].main_size);
    }
    
    if (is_main_axis_horizontal(container->config.direction)) {
        container->base.layout.size.width = final_main_size + container->config.padding.left + container->config.padding.right;
        container->base.layout.size.height = total_cross_size + container->config.padding.top + container->config.padding.bottom;
    } else {
        container->base.layout.size.width = total_cross_size + container->config.padding.left + container->config.padding.right;
        container->base.layout.size.height = final_main_size + container->config.padding.top + container->config.padding.bottom;
    }

    // Cleanup
    for (uint32_t i = 0; i < line_count; i++) {
        core_free(lines[i].items);
    }
    core_free(lines);
    
    container->needs_layout = false;
    container->base.dirty = false;
    
    // Update performance stats
    if (g_performance_profiling_enabled) {
        uint64_t end_ns = get_time_nanos();
        uint64_t total_ns = end_ns - start_ns;
        uint64_t measure_ns = measure_end_ns - start_ns;
        uint64_t arrange_ns = end_ns - measure_end_ns;

        layout_profiling_update(&container->stats, total_ns, measure_ns, arrange_ns);

        container->layout_time_ms = (float)total_ns / 1000000.0f;
        container->layout_iterations = (uint32_t)container->stats.total_layout_count;
    }
    
    LOGD("Layout completed for container %s: %.2fx%.2f",
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

void flexbox_reset_performance_stats(FlexboxContainer* container) {
    if (!container) return;
    layout_profiling_reset(&container->stats);
    container->layout_iterations = 0;
    container->layout_time_ms = 0.0f;
}
