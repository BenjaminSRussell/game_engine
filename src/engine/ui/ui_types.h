#ifndef UI_TYPES_H
#define UI_TYPES_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef struct UIElement UIElement;
typedef struct LayoutNode LayoutNode;

/* ============================================================================
 * Shared geometry
 * ============================================================================ */

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

typedef struct {
    float min_width;
    float min_height;
    float max_width;
    float max_height;
    float preferred_width;
    float preferred_height;
} SizeConstraints;

/* ============================================================================
 * Legacy layout node (renderer example). Enumerator names must not collide
 * with JustifyContent below (JUSTIFY_CENTER, JUSTIFY_SPACE_*).
 * ============================================================================ */

typedef enum {
    LAYOUT_DIRECTION_ROW,
    LAYOUT_DIRECTION_COLUMN,
    LAYOUT_DIRECTION_ROW_REVERSE,
    LAYOUT_DIRECTION_COLUMN_REVERSE
} LayoutDirection;

typedef enum {
    ALIGN_START,
    ALIGN_CENTER,
    ALIGN_END,
    ALIGN_STRETCH,
    ALIGN_BASELINE
} LayoutAlign;

typedef enum {
    JUSTIFY_START,
    JUSTIFY_LAYOUT_CENTER,
    JUSTIFY_END,
    JUSTIFY_LAYOUT_SPACE_BETWEEN,
    JUSTIFY_LAYOUT_SPACE_AROUND,
    JUSTIFY_LAYOUT_SPACE_EVENLY
} LayoutJustify;

typedef enum {
    WRAP_NO_WRAP,
    WRAP_WRAP,
    WRAP_WRAP_REVERSE
} LayoutWrap;

typedef enum {
    POSITION_RELATIVE,
    POSITION_ABSOLUTE,
    POSITION_FIXED
} LayoutPosition;

struct LayoutNode {
    uint32_t id;
    const char *name;

    LayoutNode *parent;
    LayoutNode **children;
    uint32_t child_count;
    uint32_t child_capacity;

    LayoutDirection direction;
    LayoutAlign align_items;
    LayoutAlign align_self;
    LayoutAlign align_content;
    LayoutJustify justify_content;
    LayoutWrap wrap;
    LayoutPosition position;

    float flex_grow;
    float flex_shrink;
    float flex_basis;

    float margin[4];
    float padding[4];
    float border[4];

    float width;
    float height;
    SizeConstraints constraints;

    float computed_x;
    float computed_y;
    float computed_width;
    float computed_height;

    int32_t z_index;

    bool is_dirty;
    bool needs_layout;
    bool is_visible;
};

/* ============================================================================
 * Flexbox / grid element used by the layout solver
 * ============================================================================ */

typedef enum {
    LAYOUT_TYPE_NONE,
    LAYOUT_TYPE_FLEX,
    LAYOUT_TYPE_GRID,
    LAYOUT_TYPE_CONSTRAINT
} LayoutType;

typedef enum {
    FLEX_DIRECTION_ROW,
    FLEX_DIRECTION_ROW_REVERSE,
    FLEX_DIRECTION_COLUMN,
    FLEX_DIRECTION_COLUMN_REVERSE
} FlexDirection;

typedef enum {
    JUSTIFY_FLEX_START,
    JUSTIFY_FLEX_END,
    JUSTIFY_CENTER,
    JUSTIFY_SPACE_BETWEEN,
    JUSTIFY_SPACE_AROUND,
    JUSTIFY_SPACE_EVENLY
} JustifyContent;

typedef enum {
    ALIGN_ITEMS_FLEX_START,
    ALIGN_ITEMS_FLEX_END,
    ALIGN_ITEMS_CENTER,
    ALIGN_ITEMS_STRETCH,
    ALIGN_ITEMS_BASELINE
} AlignItems;

typedef enum {
    ALIGN_SELF_AUTO,
    ALIGN_SELF_FLEX_START,
    ALIGN_SELF_FLEX_END,
    ALIGN_SELF_CENTER,
    ALIGN_SELF_STRETCH,
    ALIGN_SELF_BASELINE
} AlignSelf;

typedef enum {
    FLEX_WRAP_NOWRAP,
    FLEX_WRAP_WRAP,
    FLEX_WRAP_WRAP_REVERSE
} FlexWrap;

typedef enum {
    ALIGN_CONTENT_FLEX_START,
    ALIGN_CONTENT_FLEX_END,
    ALIGN_CONTENT_CENTER,
    ALIGN_CONTENT_STRETCH,
    ALIGN_CONTENT_SPACE_BETWEEN,
    ALIGN_CONTENT_SPACE_AROUND
} AlignContent;

typedef struct {
    float grow;
    float shrink;
    float basis;
    AlignSelf align_self;
    float min_width;
    float max_width;
    float min_height;
    float max_height;
} FlexItem;

typedef enum {
    GRID_TRACK_AUTO,
    GRID_TRACK_FIXED,
    GRID_TRACK_PERCENT,
    GRID_TRACK_FRACTION,
    GRID_TRACK_MIN_CONTENT,
    GRID_TRACK_MAX_CONTENT,
    GRID_TRACK_MIN_MAX
} GridTrackType;

typedef enum {
    GRID_POSITION_AUTO,
    GRID_POSITION_LINE,
    GRID_POSITION_SPAN,
    GRID_POSITION_AREA
} GridPositionType;

typedef enum {
    GRID_ALIGN_START,
    GRID_ALIGN_END,
    GRID_ALIGN_CENTER,
    GRID_ALIGN_STRETCH
} GridAlign;

typedef struct {
    GridTrackType type;
    float value;
    float min_value;
    float max_value;
} GridTrack;

typedef struct {
    int32_t column_start;
    int32_t column_end;
    int32_t row_start;
    int32_t row_end;
    char name[64];
} GridArea;

typedef struct {
    GridPositionType column_position_type;
    GridPositionType row_position_type;

    union {
        int32_t line;
        int32_t span;
        GridArea area;
    } column_position, row_position;

    GridAlign justify_self;
    GridAlign align_self;
} GridPlacement;

typedef struct {
    Position position;
    Size size;
    bool visible;
    float baseline_offset;

    float computed_x;
    float computed_y;
    float computed_width;
    float computed_height;
} LayoutResult;

struct UIElement {
    uint32_t id;
    char* name;
    LayoutType layout_type;

    FlexItem flex_item;
    GridPlacement grid_item;
    LayoutResult layout;

    BoxEdges margin;
    BoxEdges padding;
    BoxEdges border;

    Size min_size;
    Size max_size;
    Size preferred_size;

    UIElement* parent;
    UIElement** children;
    uint32_t child_count;
    uint32_t child_capacity;

    bool visible;
    bool dirty;
    bool measured;
    int32_t z_index;

    void* user_data;
};

#endif // UI_TYPES_H
