/*
 * grid_layout.c
 * Grid Layout System Implementation
 * CSS Grid-inspired layout system for UI elements
 *
 * Part of the UI subsystem
 * Advanced 3D Rendering Engine
 */

#include "grid_layout.h"
#include "flexbox_layout.h" // Reuse UIElement
#include "core/logger.h"
#include "core/memory.h"
#include <stdlib.h>
#include <string.h>
#include <math/math.h>

#define MAX_GRID_SIZE 100
#define LAYOUT_TOLERANCE 0.1f

// Global performance profiling state
static bool g_performance_profiling_enabled = false;

/* ============================================================================
 * INTERNAL TYPES
 * ============================================================================ */

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
 * ============================================================================ */

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
    layout.positions = memory_alloc(track_count * sizeof(float));
    layout.sizes = memory_alloc(track_count * sizeof(float));
    
    if (!layout.positions || !layout.sizes) {
        LOG_ERROR("Failed to allocate track layout arrays");
        if (layout.positions) memory_free(layout.positions);
        if (layout.sizes) memory_free(layout.sizes);
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
        distribute_fraction_space(&layout, tracks, track_count, measurement.free_space, min_sizes, max_sizes);
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
        if (layout->positions) memory_free(layout->positions);
        if (layout->sizes) memory_free(layout->sizes);
        memset(layout, 0, sizeof(TrackLayout));
    }
}

static void position_grid_items(GridContainer* container, const TrackLayout* columns, 
                               const TrackLayout* rows) {
    float column_gap = container->config.column_gap;
    float row_gap = container->config.row_gap;
    
    for (uint32_t i = 0; i < container->base.child_count; i++) {
        UIElement* child = container->base.children[i];
        if (!child->visible) continue;
        
        // Get grid placement (simplified - assume direct line positioning)
        int32_t col_start = 0; // TODO: Get from child's grid placement
        int32_t col_end = col_start + 1;
        int32_t row_start = 0;
        int32_t row_end = row_start + 1;
        
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
        
        // Add gaps
        if (col_end > col_start + 1) {
            cell_width += column_gap * (col_end - col_start - 1);
        }
        if (row_end > row_start + 1) {
            cell_height += row_gap * (row_end - row_start - 1);
        }
        
        // Apply alignment (simplified - stretch)
        child->layout.position.x = cell_x + child->margin.left;
        child->layout.position.y = cell_y + child->margin.top;
        child->layout.size.width = fmaxf(0.0f, cell_width - get_main_margin(&child->margin, true));
        child->layout.size.height = fmaxf(0.0f, cell_height - get_main_margin(&child->margin, false));
    }
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

GridContainer* grid_container_create(const char* name) {
    GridContainer* container = memory_alloc(sizeof(GridContainer));
    if (!container) {
        LOG_ERROR("Failed to allocate grid container");
        return NULL;
    }
    
    memset(container, 0, sizeof(GridContainer));
    
    // Initialize base element
    container->base.id = (uint32_t)(uintptr_t)container;
    container->base.name = name ? strdup(name) : strdup("GridContainer");
    container->base.visible = true;
    container->base.dirty = true;
    
    // Initialize default configuration
    container->config.column_gap = 0.0f;
    container->config.row_gap = 0.0f;
    container->config.justify_items = GRID_ALIGN_STRETCH;
    container->config.align_items = GRID_ALIGN_STRETCH;
    container->config.justify_content = GRID_ALIGN_START;
    container->config.align_content = GRID_ALIGN_START;
    
    container->needs_layout = true;
    
    LOG_INFO("Created grid container: %s", name ? name : "unnamed");
    return container;
}

void grid_container_destroy(GridContainer* container) {
    if (!container) return;
    
    // Destroy all children
    for (uint32_t i = 0; i < container->base.child_count; i++) {
        ui_element_destroy(container->base.children[i]);
    }
    
    if (container->base.children) {
        memory_free(container->base.children);
    }
    
    if (container->config.columns) {
        memory_free(container->config.columns);
    }
    
    if (container->config.rows) {
        memory_free(container->config.rows);
    }
    
    if (container->layout.cells) {
        memory_free(container->layout.cells);
    }
    
    if (container->base.name) {
        free(container->base.name);
    }
    
    memory_free(container);
}

void grid_set_columns(GridContainer* container, const GridTrack* tracks, uint32_t count) {
    if (!container || !tracks || count == 0) return;
    
    if (container->config.columns) {
        memory_free(container->config.columns);
    }
    
    container->config.columns = memory_alloc(count * sizeof(GridTrack));
    if (!container->config.columns) {
        LOG_ERROR("Failed to allocate grid columns");
        return;
    }
    
    memcpy(container->config.columns, tracks, count * sizeof(GridTrack));
    container->config.column_count = count;
    container->needs_layout = true;
    
    LOG_INFO("Set %u grid columns", count);
}

void grid_set_rows(GridContainer* container, const GridTrack* tracks, uint32_t count) {
    if (!container || !tracks || count == 0) return;
    
    if (container->config.rows) {
        memory_free(container->config.rows);
    }
    
    container->config.rows = memory_alloc(count * sizeof(GridTrack));
    if (!container->config.rows) {
        LOG_ERROR("Failed to allocate grid rows");
        return;
    }
    
    memcpy(container->config.rows, tracks, count * sizeof(GridTrack));
    container->config.row_count = count;
    container->needs_layout = true;
    
    LOG_INFO("Set %u grid rows", count);
}

void grid_add_child(GridContainer* container, UIElement* child) {
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
    
    LOG_INFO("Added child %s to grid container %s", child->name, container->base.name);
}

void grid_layout(GridContainer* container, float available_width, float available_height) {
    if (!container || !container->needs_layout) return;
    
    clock_t start_time = g_performance_profiling_enabled ? clock() : 0;
    
    container->available_width = available_width;
    container->available_height = available_height;
    
    // Subtract container padding from available space
    float inner_width = available_width - container->config.padding.left - container->config.padding.right;
    float inner_height = available_height - container->config.padding.top - container->config.padding.bottom;
    
    inner_width = fmaxf(0.0f, inner_width);
    inner_height = fmaxf(0.0f, inner_height);
    
    // Layout columns
    TrackLayout column_layout = {0};
    if (container->config.column_count > 0) {
        column_layout = create_track_layout(container->config.columns, container->config.column_count,
                                         inner_width, NULL, NULL);
    }
    
    // Layout rows
    TrackLayout row_layout = {0};
    if (container->config.row_count > 0) {
        row_layout = create_track_layout(container->config.rows, container->config.row_count,
                                       inner_height, NULL, NULL);
    }
    
    // Position items in grid
    if (column_layout.count > 0 && row_layout.count > 0) {
        position_grid_items(container, &column_layout, &row_layout);
    }
    
    // Update container size
    container->base.layout.size.width = column_layout.total_size + 
                                      container->config.padding.left + container->config.padding.right;
    container->base.layout.size.height = row_layout.total_size + 
                                       container->config.padding.top + container->config.padding.bottom;
    
    container->needs_layout = false;
    container->base.dirty = false;
    
    // Update performance stats
    if (g_performance_profiling_enabled) {
        clock_t end_time = clock();
        container->layout_time_ms = ((double)(end_time - start_time)) / CLOCKS_PER_SEC * 1000.0;
        container->layout_iterations = 1;
    }
    
    // Cleanup
    destroy_track_layout(&column_layout);
    destroy_track_layout(&row_layout);
    
    LOG_DEBUG("Grid layout completed for container %s: %.2fx%.2f", 
              container->base.name, container->base.layout.size.width, container->base.layout.size.height);
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
