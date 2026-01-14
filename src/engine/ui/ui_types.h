#ifndef UI_TYPES_H
#define UI_TYPES_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
// #include "include/math/math.h" // Removed to avoid vec3_add conflict

// Forward declarations
typedef struct UIElement UIElement;
typedef struct UIElement LayoutNode; // Backwards compatibility

// Layout Types
typedef enum {
    LAYOUT_TYPE_NONE,
    LAYOUT_TYPE_FLEX,
    LAYOUT_TYPE_GRID,
    LAYOUT_TYPE_CONSTRAINT
} LayoutType;

// Basic geometric types
typedef struct {
    float top;
    float right;
    float bottom;
    float left;
} BoxEdges;

typedef struct {
    float width;
    float height;
} Size;

typedef struct {
    float x;
    float y;
} Position;

typedef struct {
    float x;
    float y;
    float width;
    float height;
} Rect;

// ----------------------------------------------------------------------------
// Flexbox Types
// ----------------------------------------------------------------------------

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

// ----------------------------------------------------------------------------
// Grid Types
// ----------------------------------------------------------------------------

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

// ----------------------------------------------------------------------------
// UI Element
// ----------------------------------------------------------------------------

// Layout result
typedef struct {
    Position position;
    Size size;
    bool visible;
    float baseline_offset;
    
    // Compatibility fields for LayoutNode usage
    float computed_x;
    float computed_y;
    float computed_width;
    float computed_height;
} LayoutResult;

struct UIElement {
    uint32_t id;
    char* name;
    LayoutType layout_type;
    
    // Layout properties
    FlexItem flex_item;
    GridPlacement grid_item;
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
    int32_t z_index;

    // User data
    void* user_data;
};

#endif // UI_TYPES_H
