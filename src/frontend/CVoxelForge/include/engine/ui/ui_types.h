#ifndef UI_TYPES_H
#define UI_TYPES_H

#include <stdbool.h>
#include <stdint.h>

// Forward declarations
typedef struct LayoutNode LayoutNode;

// Layout direction
typedef enum {
    LAYOUT_DIRECTION_ROW,
    LAYOUT_DIRECTION_COLUMN,
    LAYOUT_DIRECTION_ROW_REVERSE,
    LAYOUT_DIRECTION_COLUMN_REVERSE
} LayoutDirection;

// Alignment types
typedef enum {
    ALIGN_START,
    ALIGN_CENTER,
    ALIGN_END,
    ALIGN_STRETCH,
    ALIGN_BASELINE
} LayoutAlign;

// Justify content types
typedef enum {
    JUSTIFY_START,
    JUSTIFY_CENTER,
    JUSTIFY_END,
    JUSTIFY_SPACE_BETWEEN,
    JUSTIFY_SPACE_AROUND,
    JUSTIFY_SPACE_EVENLY
} LayoutJustify;

// Wrap types
typedef enum {
    WRAP_NO_WRAP,
    WRAP_WRAP,
    WRAP_WRAP_REVERSE
} LayoutWrap;

// Position types
typedef enum {
    POSITION_RELATIVE,
    POSITION_ABSOLUTE,
    POSITION_FIXED
} LayoutPosition;

// Size constraints
typedef struct {
    float min_width;
    float min_height;
    float max_width;
    float max_height;
    float preferred_width;
    float preferred_height;
} SizeConstraints;

// Layout node structure
struct LayoutNode {
    // Identifiers
    uint32_t id;
    const char *name;
    
    // Tree structure
    LayoutNode *parent;
    LayoutNode **children;
    uint32_t child_count;
    uint32_t child_capacity;
    
    // Layout properties
    LayoutDirection direction;
    LayoutAlign align_items;
    LayoutAlign align_self;
    LayoutAlign align_content;
    LayoutJustify justify_content;
    LayoutWrap wrap;
    LayoutPosition position;
    
    // Flexbox properties
    float flex_grow;
    float flex_shrink;
    float flex_basis;
    
    // Spacing
    float margin[4];  // top, right, bottom, left
    float padding[4]; // top, right, bottom, left
    float border[4];  // top, right, bottom, left
    
    // Size
    float width;
    float height;
    SizeConstraints constraints;
    
    // Calculated layout
    float computed_x;
    float computed_y;
    float computed_width;
    float computed_height;
    
    // Z-ordering
    int32_t z_index;
    
    // State
    bool is_dirty;
    bool needs_layout;
    bool is_visible;
};

#endif // UI_TYPES_H
