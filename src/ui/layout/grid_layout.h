/*
 * grid_layout.h
 * Grid Layout System Implementation
 * CSS Grid-inspired layout system for UI elements
 *
 * Part of the UI subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef UI_GRID_LAYOUT_H
#define UI_GRID_LAYOUT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "include/math/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct UIElement UIElement;
typedef struct GridContainer GridContainer;

// Grid track sizing
typedef enum {
    GRID_TRACK_AUTO,
    GRID_TRACK_FIXED,
    GRID_TRACK_PERCENT,
    GRID_TRACK_FRACTION,
    GRID_TRACK_MIN_CONTENT,
    GRID_TRACK_MAX_CONTENT,
    GRID_TRACK_MIN_MAX
} GridTrackType;

// Grid positioning
typedef enum {
    GRID_POSITION_AUTO,
    GRID_POSITION_LINE,
    GRID_POSITION_SPAN,
    GRID_POSITION_AREA
} GridPositionType;

// Grid alignment
typedef enum {
    GRID_ALIGN_START,
    GRID_ALIGN_END,
    GRID_ALIGN_CENTER,
    GRID_ALIGN_STRETCH
} GridAlign;

// Grid track definition
typedef struct {
    GridTrackType type;
    float value;           // Fixed size, percentage, or fraction value
    float min_value;      // For min-max sizing
    float max_value;      // For min-max sizing
} GridTrack;

// Grid area definition
typedef struct {
    int32_t column_start;
    int32_t column_end;
    int32_t row_start;
    int32_t row_end;
    char name[64];        // Named grid area
} GridArea;

// Grid item placement
typedef struct {
    GridPositionType column_position_type;
    GridPositionType row_position_type;
    
    union {
        int32_t line;     // Line number
        int32_t span;     // Span count
        GridArea area;    // Named area
    } column_position, row_position;
    
    GridAlign justify_self;  // Horizontal alignment within grid cell
    GridAlign align_self;    // Vertical alignment within grid cell
} GridPlacement;

// Grid container configuration
typedef struct {
    // Grid tracks
    GridTrack* columns;
    uint32_t column_count;
    GridTrack* rows;
    uint32_t row_count;
    
    // Grid gaps
    float column_gap;
    float row_gap;
    
    // Container alignment
    GridAlign justify_items;  // Default horizontal alignment
    GridAlign align_items;    // Default vertical alignment
    GridAlign justify_content; // Alignment of grid within container
    GridAlign align_content;   // Alignment of grid within container
    
    // Auto sizing
    bool auto_columns;
    bool auto_rows;
    uint32_t auto_flow;       // Auto placement direction
    
    // Container dimensions
    BoxEdges padding;
    BoxEdges margin;
    
    Size min_size;
    Size max_size;
} GridConfig;

// Grid cell measurement
typedef struct {
    float x;
    float y;
    float width;
    float height;
} GridCell;

// Grid layout result
typedef struct {
    GridCell* cells;
    uint32_t column_count;
    uint32_t row_count;
    float total_width;
    float total_height;
} GridLayout;

// Grid container
struct GridContainer {
    UIElement base;
    GridConfig config;
    GridLayout layout;
    
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
GridContainer* grid_container_create(const char* name);
void grid_container_destroy(GridContainer* container);

/* Grid Track Configuration */
void grid_set_columns(GridContainer* container, const GridTrack* tracks, uint32_t count);
void grid_set_rows(GridContainer* container, const GridTrack* tracks, uint32_t count);
void grid_add_column(GridContainer* container, GridTrack track);
void grid_add_row(GridContainer* container, GridTrack track);
void grid_set_column_gap(GridContainer* container, float gap);
void grid_set_row_gap(GridContainer* container, float gap);

/* Grid Alignment */
void grid_set_justify_items(GridContainer* container, GridAlign align);
void grid_set_align_items(GridContainer* container, GridAlign align);
void grid_set_justify_content(GridContainer* container, GridAlign align);
void grid_set_align_content(GridContainer* container, GridAlign align);

/* Element Management */
void grid_add_child(GridContainer* container, UIElement* child);
void grid_remove_child(GridContainer* container, UIElement* child);

/* Grid Item Placement */
void grid_item_set_column(GridContainer* container, UIElement* child, int32_t column);
void grid_item_set_column_span(GridContainer* container, UIElement* child, int32_t span);
void grid_item_set_row(GridContainer* container, UIElement* child, int32_t row);
void grid_item_set_row_span(GridContainer* container, UIElement* child, int32_t span);
void grid_item_set_area(GridContainer* container, UIElement* child, const char* area_name);
void grid_item_set_justify_self(GridContainer* container, UIElement* child, GridAlign align);
void grid_item_set_align_self(GridContainer* container, UIElement* child, GridAlign align);

/* Layout Algorithm */
void grid_layout(GridContainer* container, float available_width, float available_height);
void grid_invalidate_layout(GridContainer* container);
bool grid_needs_layout(const GridContainer* container);

/* Layout Queries */
GridCell grid_item_get_cell(const UIElement* element);
Size grid_get_content_size(const GridContainer* container);
Position grid_get_content_position(const GridContainer* container);

/* Utility Functions */
GridTrack grid_track_fixed(float size);
GridTrack grid_track_percent(float percentage);
GridTrack grid_track_fraction(float fraction);
GridTrack grid_track_auto(void);
GridTrack grid_track_min_content(void);
GridTrack grid_track_max_content(void);
GridTrack grid_track_min_max(float min_size, float max_size);

/* Performance */
void grid_enable_performance_profiling(bool enable);
void grid_get_performance_stats(const GridContainer* container, 
                               uint32_t* iterations, float* time_ms);

/* Grid Template Areas */
void grid_define_template_areas(GridContainer* container, const char* areas_string);
void grid_set_template_columns(GridContainer* container, const char* columns_string);
void grid_set_template_rows(GridContainer* container, const char* rows_string);

#ifdef __cplusplus
}
#endif

#endif /* UI_GRID_LAYOUT_H */
