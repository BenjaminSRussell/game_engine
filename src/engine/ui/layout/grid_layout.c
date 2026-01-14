/*
 * grid_layout.c
 * Grid Layout System Implementation
 * CSS Grid-inspired layout system for UI elements
 *
 * Part of the UI subsystem
 * Advanced 3D Rendering Engine
 */

#include "grid_layout.h"
#include "flexbox_layout.h" // Required for ui_element_destroy and UIElement utilities
#include "core/logger.h"
#include "core/time_system.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

/* Use standard allocation to avoid conflicts with core headers during partial compilation */
#ifndef core_alloc
#define core_alloc(size) malloc(size)
#endif
#ifndef core_free
#define core_free(ptr) free(ptr)
#endif
#ifndef core_realloc
#define core_realloc(ptr, size) realloc(ptr, size)
#endif

#define MAX_GRID_SIZE 100
#define LAYOUT_TOLERANCE 0.1f

// Global performance profiling state
static bool g_performance_profiling_enabled = false;

/* ============================================================================
 * INTERNAL TYPES
 * ============================================================================
 */

typedef struct {
    float used_space;
    float free_space;
    float total_fraction;
    uint32_t auto_track_count;
} TrackMeasurement;

typedef struct {
    float* positions;
    float* sizes;
    uint32_t count;
    float total_size;
} TrackLayout;

/* ============================================================================
 * INTERNAL FUNCTIONS
 * ============================================================================
 */

static float resolve_track_size(const GridTrack* track, float container_size, float min_size, float max_size) {
    switch (track->type) {
        case GRID_TRACK_AUTO:
        case GRID_TRACK_MIN_CONTENT:
        case GRID_TRACK_MAX_CONTENT:
            return min_size; // Simplified - use min size for auto tracks
            
        case GRID_TRACK_FIXED:
            return track->value;
            
        case GRID_TRACK_PERCENT:
            return (track->value / 100.0f) * container_size;
            
        case GRID_TRACK_FRACTION:
            return track->value; // Will be resolved in fraction distribution
            
        case GRID_TRACK_MIN_MAX:
            return fmaxf(track->min_value, fminf(track->max_value, track->value));
            
        default:
            return 0.0f;
    }
}

static TrackMeasurement measure_tracks(const GridTrack* tracks, uint32_t track_count, 
                                     float container_size, const float* min_sizes, 
                                     const float* max_sizes) {
    TrackMeasurement measurement = {0};
    
    for (uint32_t i = 0; i < track_count; i++) {
        const GridTrack* track = &tracks[i];
        float min_size = min_sizes ? min_sizes[i] : 0.0f;
        float max_size = max_sizes ? max_sizes[i] : FLT_MAX;
        
        if (track->type == GRID_TRACK_FRACTION) {
            measurement.total_fraction += track->value;
        } else if (track->type == GRID_TRACK_AUTO || 
                  track->type == GRID_TRACK_MIN_CONTENT ||
                  track->type == GRID_TRACK_MAX_CONTENT) {
            measurement.auto_track_count++;
        } else {
            float size = resolve_track_size(track, container_size, min_size, max_size);
            measurement.used_space += size;
        }
    }
    
    measurement.free_space = fmaxf(0.0f, container_size - measurement.used_space);
    return measurement;
}

static void distribute_fraction_space(TrackLayout* layout, const GridTrack* tracks, 
                                     uint32_t track_count, float available_space) {
    float total_fraction = 0.0f;
    
    // Calculate total fraction
    for (uint32_t i = 0; i < track_count; i++) {
        if (tracks[i].type == GRID_TRACK_FRACTION) {
            total_fraction += tracks[i].value;
        }
    }
    
    if (total_fraction <= 0.0f) return;
    
    // Distribute space to fraction tracks
    for (uint32_t i = 0; i < track_count; i++) {
        if (tracks[i].type == GRID_TRACK_FRACTION) {
            float fraction = tracks[i].value / total_fraction;
            layout->sizes[i] = available_space * fraction;
        }
    }
}

static void distribute_auto_space(TrackLayout* layout, uint32_t track_count, 
                                 float available_space, const float* min_sizes) {
    uint32_t auto_count = 0;
    
    // Count auto tracks
    for (uint32_t i = 0; i < track_count; i++) {
        if (layout->sizes[i] < 0.0f) { // Negative indicates auto
            auto_count++;
        }
    }
    
    if (auto_count == 0) return;
    
    float auto_space = available_space / auto_count;
    
    // Distribute space to auto tracks
    for (uint32_t i = 0; i < track_count; i++) {
        if (layout->sizes[i] < 0.0f) {
            float min_size = min_sizes ? min_sizes[i] : 0.0f;
            layout->sizes[i] = fmaxf(min_size, auto_space);
        }
    }
}

static void calculate_track_positions(TrackLayout* layout) {
    layout->positions[0] = 0.0f;
    
    for (uint32_t i = 1; i < layout->count; i++) {
        layout->positions[i] = layout->positions[i - 1] + layout->sizes[i - 1];
    }
    
    layout->total_size = layout->positions[layout->count - 1] + layout->sizes[layout->count - 1];
}

static TrackLayout create_track_layout(const GridTrack* tracks, uint32_t track_count, 
                                      float container_size, const float* min_sizes, 
                                      const float* max_sizes) {
    TrackLayout layout = {0};
    layout.count = track_count;
    layout.positions = core_alloc(track_count * sizeof(float));
    layout.sizes = core_alloc(track_count * sizeof(float));
    
    if (!layout.positions || !layout.sizes) {
        LOGE("Failed to allocate track layout arrays");
        if (layout.positions) core_free(layout.positions);
        if (layout.sizes) core_free(layout.sizes);
        return layout;
    }
    
    // Initialize sizes with fixed values
    for (uint32_t i = 0; i < track_count; i++) {
        const GridTrack* track = &tracks[i];
        float min_size = min_sizes ? min_sizes[i] : 0.0f;
        float max_size = max_sizes ? max_sizes[i] : FLT_MAX;
        
        if (track->type == GRID_TRACK_AUTO || 
            track->type == GRID_TRACK_MIN_CONTENT ||
            track->type == GRID_TRACK_MAX_CONTENT) {
            layout.sizes[i] = -1.0f; // Mark as auto
        } else if (track->type != GRID_TRACK_FRACTION) {
            layout.sizes[i] = resolve_track_size(track, container_size, min_size, max_size);
        } else {
            layout.sizes[i] = 0.0f; // Will be resolved later
        }
    }
    
    // Measure used space and fractions
    TrackMeasurement measurement = measure_tracks(tracks, track_count, container_size, min_sizes, max_sizes);
    
    // Distribute fraction space
    if (measurement.total_fraction > 0.0f) {
        distribute_fraction_space(&layout, tracks, track_count, measurement.free_space);
    }
    
    // Calculate remaining space for auto tracks
    float used_space = 0.0f;
    for (uint32_t i = 0; i < track_count; i++) {
        if (layout.sizes[i] >= 0.0f) {
            used_space += layout.sizes[i];
        }
    }
    
    float remaining_space = fmaxf(0.0f, container_size - used_space);
    
    // Distribute auto space
    distribute_auto_space(&layout, track_count, remaining_space, min_sizes);
    
    // Calculate positions
    calculate_track_positions(&layout);
    
    return layout;
}

static void destroy_track_layout(TrackLayout* layout) {
    if (layout) {
        if (layout->positions) core_free(layout->positions);
        if (layout->sizes) core_free(layout->sizes);
        memset(layout, 0, sizeof(TrackLayout));
    }
}

static float get_main_margin(const BoxEdges* margins, bool horizontal) {
    if (horizontal) {
        return margins->left + margins->right;
    } else {
        return margins->top + margins->bottom;
    }
}

static void mark_cells_occupied(bool* occupied, uint32_t cols, uint32_t rows,
                              int32_t col_start, int32_t row_start,
                              int32_t col_span, int32_t row_span) {
    if (!occupied) return;
    for (int32_t r = row_start; r < row_start + row_span; r++) {
        for (int32_t c = col_start; c < col_start + col_span; c++) {
            if (r >= 0 && r < (int32_t)rows && c >= 0 && c < (int32_t)cols) {
                occupied[r * cols + c] = true;
            }
        }
    }
}

static void find_next_empty(const bool* occupied, uint32_t cols, uint32_t rows,
                          int32_t* current_col, int32_t* current_row,
                          int32_t col_span, int32_t row_span) {
    if (!occupied) return;

    // Simple row-major search
    while (*current_row < (int32_t)rows) {
        bool fits = true;
        // Check if span fits
        if (*current_col + col_span > (int32_t)cols) {
            *current_col = 0;
            (*current_row)++;
            continue;
        }

        for (int32_t r = 0; r < row_span; r++) {
            for (int32_t c = 0; c < col_span; c++) {
                int32_t idx = (*current_row + r) * cols + (*current_col + c);
                if (idx >= (int32_t)(cols * rows) || occupied[idx]) {
                    fits = false;
                    break;
                }
            }
            if (!fits) break;
        }

        if (fits) return;

        (*current_col)++;
        if (*current_col >= (int32_t)cols) {
            *current_col = 0;
            (*current_row)++;
        }
    }
}

static void position_grid_items(GridContainer* container, const TrackLayout* columns, 
                               const TrackLayout* rows) {
    float column_gap = container->config.column_gap;
    float row_gap = container->config.row_gap;
    
    uint32_t cols = columns->count;
    uint32_t row_count = rows->count;

    LOGD("Positioning grid items: cols=%u rows=%u", cols, row_count);
    for (uint32_t k = 0; k < cols; k++) {
        LOGD("  Col %u: pos=%.2f size=%.2f", k, columns->positions[k], columns->sizes[k]);
    }

    // Track occupied cells for auto placement
    // Allocation size: cols * rows.
    // WARNING: If rows are implicit/infinite, this approach needs expansion.
    // For now, limited to defined rows.
    bool* occupied = core_alloc(cols * row_count * sizeof(bool));
    if (occupied) {
        memset(occupied, 0, cols * row_count * sizeof(bool));
    }

    int32_t auto_cursor_col = 0;
    int32_t auto_cursor_row = 0;

    // First pass: Place explicit items
    for (uint32_t i = 0; i < container->base.child_count; i++) {
        UIElement* child = container->base.children[i];
        if (!child->visible) continue;

        GridPlacement* gp = &child->grid_item;

        // If explicit position
        if (gp->column_position_type == GRID_POSITION_LINE && gp->row_position_type == GRID_POSITION_LINE) {
            int32_t col_start = gp->column_position.line;
            int32_t row_start = gp->row_position.line;
            int32_t col_span = 1;
            int32_t row_span = 1;

            // TODO: Support span with explicit start

            mark_cells_occupied(occupied, cols, row_count, col_start, row_start, col_span, row_span);
        }
    }

    // Second pass: Position everyone
    for (uint32_t i = 0; i < container->base.child_count; i++) {
        UIElement* child = container->base.children[i];
        if (!child->visible) continue;
        
        GridPlacement* gp = &child->grid_item;

        int32_t col_start = 0;
        int32_t row_start = 0;
        int32_t col_span = 1;
        int32_t row_span = 1;

        // Resolve spans
        if (gp->column_position_type == GRID_POSITION_SPAN) {
            col_span = gp->column_position.span;
        }
        if (gp->row_position_type == GRID_POSITION_SPAN) {
            row_span = gp->row_position.span;
        }

        // Resolve position
        if (gp->column_position_type == GRID_POSITION_LINE) {
             col_start = gp->column_position.line;
        } else {
             // Auto column
             // We need to find position later
             col_start = -1;
        }

        if (gp->row_position_type == GRID_POSITION_LINE) {
             row_start = gp->row_position.line;
        } else {
             // Auto row
             row_start = -1;
        }

        // If fully auto or partial auto, find spot
        if (col_start == -1 || row_start == -1) {
            int32_t search_col = (col_start != -1) ? col_start : auto_cursor_col;
            int32_t search_row = (row_start != -1) ? row_start : auto_cursor_row;

            // If we have one dimension fixed, we iterate the other?
            // Simplified: Use cursor for auto placement

            find_next_empty(occupied, cols, row_count, &search_col, &search_row, col_span, row_span);
            col_start = search_col;
            row_start = search_row;

            // Update cursor
            // auto_cursor_col = col_start + col_span; // standard auto flow
        }

        // Mark occupied
        mark_cells_occupied(occupied, cols, row_count, col_start, row_start, col_span, row_span);

        int32_t col_end = col_start + col_span;
        int32_t row_end = row_start + row_span;
        
        // Clamp to grid bounds
        col_start = fmaxf(0, fminf(col_start, (int32_t)columns->count - 1));
        col_end = fmaxf(1, fminf(col_end, (int32_t)columns->count));
        row_start = fmaxf(0, fminf(row_start, (int32_t)rows->count - 1));
        row_end = fmaxf(1, fminf(row_end, (int32_t)rows->count));
        
        // Calculate cell position and size
        float cell_x = columns->positions[col_start];
        float cell_y = rows->positions[row_start];
        float cell_width = columns->positions[col_end - 1] + columns->sizes[col_end - 1] - cell_x;
        float cell_height = rows->positions[row_end - 1] + rows->sizes[row_end - 1] - cell_y;
        
        LOGD("  Item %s: col=%d-%d row=%d-%d -> rect=%.2f,%.2f %.2fx%.2f",
             child->name, col_start, col_end, row_start, row_end, cell_x, cell_y, cell_width, cell_height);

        // Add gaps
        if (col_end > col_start + 1) {
            cell_width += column_gap * (col_end - col_start - 1);
        }
        if (row_end > row_start + 1) {
            cell_height += row_gap * (row_end - row_start - 1);
        }
        
        // Apply alignment (simplified - stretch)
        // TODO: Use justify_self/align_self
        child->layout.position.x = cell_x + child->margin.left;
        child->layout.position.y = cell_y + child->margin.top;
        child->layout.size.width = fmaxf(0.0f, cell_width - get_main_margin(&child->margin, true));
        child->layout.size.height = fmaxf(0.0f, cell_height - get_main_margin(&child->margin, false));
    }

    if (occupied) core_free(occupied);
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================
 */

GridContainer* grid_container_create(const char* name) {
    GridContainer* container = core_alloc(sizeof(GridContainer));
    if (!container) {
        LOGE("Failed to allocate grid container");
        return NULL;
    }
    
    memset(container, 0, sizeof(GridContainer));
    
    // Initialize base element
    container->base.id = (uint32_t)(uintptr_t)container;
    container->base.name = name ? strdup(name) : strdup("GridContainer");
    container->base.visible = true;
    container->base.dirty = true;
    container->base.layout_type = LAYOUT_TYPE_GRID;
    
    // Initialize default configuration
    container->config.column_gap = 0.0f;
    container->config.row_gap = 0.0f;
    container->config.justify_items = GRID_ALIGN_STRETCH;
    container->config.align_items = GRID_ALIGN_STRETCH;
    container->config.justify_content = GRID_ALIGN_START;
    container->config.align_content = GRID_ALIGN_START;
    
    container->needs_layout = true;

    layout_profiling_reset(&container->stats);
    
    LOGI("Created grid container: %s", name ? name : "unnamed");
    return container;
}

void grid_container_destroy(GridContainer* container) {
    if (!container) return;
    
    // Destroy all children
    for (uint32_t i = 0; i < container->base.child_count; i++) {
        ui_element_destroy(container->base.children[i]);
    }
    
    if (container->base.children) {
        core_free(container->base.children);
    }
    
    if (container->config.columns) {
        core_free(container->config.columns);
    }
    
    if (container->config.rows) {
        core_free(container->config.rows);
    }
    
    if (container->layout.cells) {
        core_free(container->layout.cells);
    }
    
    if (container->base.name) {
        free(container->base.name);
    }
    
    core_free(container);
}

void grid_set_columns(GridContainer* container, const GridTrack* tracks, uint32_t count) {
    if (!container || !tracks || count == 0) return;
    
    if (container->config.columns) {
        core_free(container->config.columns);
    }
    
    container->config.columns = core_alloc(count * sizeof(GridTrack));
    if (!container->config.columns) {
        LOGE("Failed to allocate grid columns");
        return;
    }
    
    memcpy(container->config.columns, tracks, count * sizeof(GridTrack));
    container->config.column_count = count;
    container->needs_layout = true;
    
    LOGI("Set %u grid columns", count);
}

void grid_set_rows(GridContainer* container, const GridTrack* tracks, uint32_t count) {
    if (!container || !tracks || count == 0) return;
    
    if (container->config.rows) {
        core_free(container->config.rows);
    }
    
    container->config.rows = core_alloc(count * sizeof(GridTrack));
    if (!container->config.rows) {
        LOGE("Failed to allocate grid rows");
        return;
    }
    
    memcpy(container->config.rows, tracks, count * sizeof(GridTrack));
    container->config.row_count = count;
    container->needs_layout = true;
    
    LOGI("Set %u grid rows", count);
}

void grid_add_child(GridContainer* container, UIElement* child) {
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
    
    // Default grid placement (auto)
    memset(&child->grid_item, 0, sizeof(GridPlacement));

    LOGI("Added child %s to grid container %s", child->name, container->base.name);
}

void grid_item_set_column(GridContainer* container, UIElement* child, int32_t column) {
    if (!child) return;
    child->grid_item.column_position_type = GRID_POSITION_LINE;
    child->grid_item.column_position.line = column;
    child->dirty = true;
    if (container) container->needs_layout = true;
}

void grid_item_set_column_span(GridContainer* container, UIElement* child, int32_t span) {
    if (!child) return;
    
    child->grid_item.column_position_type = GRID_POSITION_SPAN;
    child->grid_item.column_position.span = span;
    
    child->dirty = true;
    if (container) container->needs_layout = true;
}

void grid_item_set_row(GridContainer* container, UIElement* child, int32_t row) {
    if (!child) return;
    child->grid_item.row_position_type = GRID_POSITION_LINE;
    child->grid_item.row_position.line = row;
    child->dirty = true;
    if (container) container->needs_layout = true;
}

void grid_item_set_row_span(GridContainer* container, UIElement* child, int32_t span) {
    if (!child) return;
    child->grid_item.row_position_type = GRID_POSITION_SPAN;
    child->grid_item.row_position.span = span;
    child->dirty = true;
    if (container) container->needs_layout = true;
}

void grid_item_set_area(GridContainer* container, UIElement* child, const char* area_name) {
    if (!child || !area_name) return;
    child->grid_item.column_position_type = GRID_POSITION_AREA;
    strncpy(child->grid_item.column_position.area.name, area_name, 63);
    child->dirty = true;
    if (container) container->needs_layout = true;
}

void grid_item_set_justify_self(GridContainer* container, UIElement* child, GridAlign align) {
    if (!child) return;
    child->grid_item.justify_self = align;
    child->dirty = true;
    if (container) container->needs_layout = true;
}

void grid_item_set_align_self(GridContainer* container, UIElement* child, GridAlign align) {
    if (!child) return;
    child->grid_item.align_self = align;
    child->dirty = true;
    if (container) container->needs_layout = true;
}

/* Utility Functions */
GridTrack grid_track_fixed(float size) {
    return (GridTrack){GRID_TRACK_FIXED, size, 0.0f, 0.0f};
}

GridTrack grid_track_percent(float percentage) {
    return (GridTrack){GRID_TRACK_PERCENT, percentage, 0.0f, 0.0f};
}

GridTrack grid_track_fraction(float fraction) {
    return (GridTrack){GRID_TRACK_FRACTION, fraction, 0.0f, 0.0f};
}

GridTrack grid_track_auto(void) {
    return (GridTrack){GRID_TRACK_AUTO, 0.0f, 0.0f, 0.0f};
}

GridTrack grid_track_min_content(void) {
    return (GridTrack){GRID_TRACK_MIN_CONTENT, 0.0f, 0.0f, 0.0f};
}

GridTrack grid_track_max_content(void) {
    return (GridTrack){GRID_TRACK_MAX_CONTENT, 0.0f, 0.0f, 0.0f};
}

GridTrack grid_track_min_max(float min_size, float max_size) {
    return (GridTrack){GRID_TRACK_MIN_MAX, 0.0f, min_size, max_size};
}

void grid_enable_performance_profiling(bool enable) {
    g_performance_profiling_enabled = enable;
}

void grid_get_performance_stats(const GridContainer* container, 
                               uint32_t* iterations, float* time_ms) {
    if (!container) return;
    
    if (iterations) *iterations = container->layout_iterations;
    if (time_ms) *time_ms = container->layout_time_ms;
}

void grid_reset_performance_stats(GridContainer* container) {
    if (!container) return;
    layout_profiling_reset(&container->stats);
    container->layout_iterations = 0;
    container->layout_time_ms = 0.0f;
}
