/*
 * ui_layout_engine.h
 * Core UI Layout Engine - Two-Pass Measure/Arrange System
 * Implements measure pass (desired size) and arrange pass (final positioning)
 *
 * Part of the UI subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef UI_LAYOUT_ENGINE_H
#define UI_LAYOUT_ENGINE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================
 */

typedef struct UILayoutElement UILayoutElement;
typedef struct UILayoutEngine UILayoutEngine;
typedef struct UILayoutMeasureResult UILayoutMeasureResult;
typedef struct UILayoutArrangeResult UILayoutArrangeResult;

/* ============================================================================
 * ENUMERATIONS
 * ============================================================================
 */

/* Box sizing mode (CSS box-sizing) */
typedef enum {
  BOX_SIZING_CONTENT_BOX, /* Width/height include only content */
  BOX_SIZING_BORDER_BOX   /* Width/height include padding and border */
} BoxSizing;

/* Anchor preset types (Unity/Unreal-style positioning) */
typedef enum {
  ANCHOR_TOP_LEFT,           /* Anchored to top-left corner */
  ANCHOR_TOP_CENTER,         /* Anchored to top-center */
  ANCHOR_TOP_RIGHT,          /* Anchored to top-right corner */
  ANCHOR_MIDDLE_LEFT,        /* Anchored to middle-left */
  ANCHOR_MIDDLE_CENTER,      /* Anchored to center */
  ANCHOR_MIDDLE_RIGHT,       /* Anchored to middle-right */
  ANCHOR_BOTTOM_LEFT,        /* Anchored to bottom-left corner */
  ANCHOR_BOTTOM_CENTER,      /* Anchored to bottom-center */
  ANCHOR_BOTTOM_RIGHT,       /* Anchored to bottom-right corner */
  ANCHOR_STRETCH_HORIZONTAL, /* Stretch horizontally, fixed vertical */
  ANCHOR_STRETCH_VERTICAL,   /* Stretch vertically, fixed horizontal */
  ANCHOR_STRETCH_BOTH,       /* Stretch both dimensions */
  ANCHOR_CUSTOM              /* Custom anchor configuration */
} AnchorPreset;

/* Dock mode for layout elements */
typedef enum {
  DOCK_NONE,   /* No docking */
  DOCK_TOP,    /* Dock to top edge */
  DOCK_BOTTOM, /* Dock to bottom edge */
  DOCK_LEFT,   /* Dock to left edge */
  DOCK_RIGHT,  /* Dock to right edge */
  DOCK_FILL    /* Fill entire parent */
} DockMode;

/* Responsive breakpoint type */
typedef enum {
  BREAKPOINT_MOBILE,  /* < 600px */
  BREAKPOINT_TABLET,  /* 600px - 1024px */
  BREAKPOINT_DESKTOP, /* 1024px - 1920px */
  BREAKPOINT_WIDE,    /* > 1920px */
  BREAKPOINT_CUSTOM   /* Custom breakpoint */
} BreakpointType;

/* ============================================================================
 * CORE STRUCTURES
 * ============================================================================
 */

/* Rectangle structure for layout bounds */
typedef struct {
  float x;
  float y;
  float width;
  float height;
} UIRect;

/* Size structure */
typedef struct {
  float width;
  float height;
} UISize;

/* Box edges (margin, padding, border) */
typedef struct {
  float top;
  float right;
  float bottom;
  float left;
} UIBoxEdges;

/* Anchor configuration */
typedef struct {
  float min_x;    /* Normalized anchor min X (0-1) */
  float max_x;    /* Normalized anchor max X (0-1) */
  float min_y;    /* Normalized anchor min Y (0-1) */
  float max_y;    /* Normalized anchor max Y (0-1) */
  float offset_x; /* Pixel offset from anchor */
  float offset_y; /* Pixel offset from anchor */
} UIAnchor;

/* Responsive breakpoint definition */
typedef struct {
  BreakpointType type;
  float min_width;
  float max_width;
  float scale_factor;
  char name[32];
} UIBreakpoint;

/* Measure result - desired size from measure pass */
struct UILayoutMeasureResult {
  float desired_width;
  float desired_height;
  float baseline_offset; /* Distance from top to text baseline */
  bool is_cached;
  uint32_t cache_frame;
};

/* Arrange result - final layout from arrange pass */
struct UILayoutArrangeResult {
  UIRect final_rect;
  UIRect content_rect; /* Rect excluding margin/padding/border */
  bool is_visible;
  bool is_clipped;
};

/* Layout element properties */
struct UILayoutElement {
  uint32_t id;
  char *name;

  /* Hierarchy */
  UILayoutElement *parent;
  UILayoutElement **children;
  uint32_t child_count;
  uint32_t child_capacity;

  /* Box model */
  UIBoxEdges margin;
  UIBoxEdges padding;
  UIBoxEdges border;
  BoxSizing box_sizing;

  /* Size constraints */
  float min_width;
  float max_width;
  float min_height;
  float max_height;
  float preferred_width;
  float preferred_height;

  /* Anchor and dock */
  AnchorPreset anchor_preset;
  UIAnchor anchor;
  DockMode dock;

  /* Layout state */
  UILayoutMeasureResult measure_result;
  UILayoutArrangeResult arrange_result;
  bool is_dirty;
  bool needs_measure;
  bool needs_arrange;

  /* Responsive properties */
  uint32_t active_breakpoint_index;

  /* User data */
  void *user_data;
};

/* Layout engine instance */
struct UILayoutEngine {
  uint32_t id;
  bool initialized;

  /* Root element */
  UILayoutElement *root;

  /* Viewport size */
  float viewport_width;
  float viewport_height;
  float viewport_dpi_scale;

  /* Responsive breakpoints */
  UIBreakpoint *breakpoints;
  uint32_t breakpoint_count;
  uint32_t breakpoint_capacity;
  uint32_t active_breakpoint_index;

  /* Performance tracking */
  uint32_t current_frame;
  uint32_t total_measure_passes;
  uint32_t total_arrange_passes;
  float total_measure_time_ms;
  float total_arrange_time_ms;
  uint32_t measure_cache_hits;
  uint32_t measure_cache_misses;

  /* Configuration */
  bool enable_caching;
  bool enable_profiling;
  bool enable_debug;
};

/* ============================================================================
 * API - LAYOUT ENGINE MANAGEMENT
 * ============================================================================
 */

/* Create layout engine */
UILayoutEngine *ui_layout_engine_create(float viewport_width,
                                        float viewport_height);

/* Destroy layout engine */
void ui_layout_engine_destroy(UILayoutEngine *engine);

/* Set viewport size (triggers relayout) */
void ui_layout_engine_set_viewport(UILayoutEngine *engine, float width,
                                   float height);

/* Set DPI scale factor */
void ui_layout_engine_set_dpi_scale(UILayoutEngine *engine, float scale);

/* ============================================================================
 * API - ELEMENT MANAGEMENT
 * ============================================================================
 */

/* Create layout element */
UILayoutElement *ui_layout_element_create(const char *name);

/* Destroy layout element */
void ui_layout_element_destroy(UILayoutElement *element);

/* Add child element */
void ui_layout_element_add_child(UILayoutElement *parent,
                                 UILayoutElement *child);

/* Remove child element */
void ui_layout_element_remove_child(UILayoutElement *parent,
                                    UILayoutElement *child);

/* ============================================================================
 * API - TWO-PASS LAYOUT ALGORITHM
 * ============================================================================
 */

/* MEASURE PASS: Calculate desired size bottom-up */
void ui_layout_measure(UILayoutEngine *engine, UILayoutElement *element,
                       float available_width, float available_height);

/* Compute desired size for element (override in custom elements) */
UILayoutMeasureResult ui_element_compute_desired_size(UILayoutElement *element,
                                                      float available_width,
                                                      float available_height);

/* ARRANGE PASS: Position elements top-down */
void ui_layout_arrange(UILayoutEngine *engine, UILayoutElement *element,
                       UIRect final_rect);

/* Arrange children within allocated space (override in custom elements) */
void ui_element_arrange_children(UILayoutElement *element, UIRect final_rect);

/* Invalidate layout (mark for re-measure and re-arrange) */
void ui_layout_invalidate(UILayoutElement *element);

/* ============================================================================
 * API - BOX MODEL CALCULATIONS (TODO-0180)
 * ============================================================================
 */

/* Calculate content box size (excludes padding and border) */
UISize ui_calculate_content_size(const UILayoutElement *element,
                                 UISize outer_size);

/* Calculate border box size (includes padding and border) */
UISize ui_calculate_border_box_size(const UILayoutElement *element,
                                    UISize content_size);

/* Get total horizontal spacing (margin + padding + border) */
float ui_get_horizontal_spacing(const UILayoutElement *element);

/* Get total vertical spacing (margin + padding + border) */
float ui_get_vertical_spacing(const UILayoutElement *element);

/* Apply box sizing mode to dimensions */
UISize ui_apply_box_sizing(const UILayoutElement *element, float width,
                           float height);

/* ============================================================================
 * API - ANCHORING AND DOCKING (TODO-0181)
 * ============================================================================
 */

/* Set anchor preset */
void ui_element_set_anchor_preset(UILayoutElement *element,
                                  AnchorPreset preset);

/* Set custom anchor */
void ui_element_set_anchor(UILayoutElement *element, UIAnchor anchor);

/* Get anchor configuration for preset */
UIAnchor ui_get_anchor_preset_config(AnchorPreset preset);

/* Apply anchor layout to element */
UIRect ui_element_apply_anchor_layout(const UILayoutElement *element,
                                      UIRect parent_rect);

/* Set dock mode */
void ui_element_set_dock(UILayoutElement *element, DockMode dock);

/* Apply dock layout to element */
UIRect ui_element_apply_dock_layout(const UILayoutElement *element,
                                    UIRect parent_rect);

/* ============================================================================
 * API - RESPONSIVE DESIGN (TODO-0182)
 * ============================================================================
 */

/* Add responsive breakpoint */
void ui_layout_engine_add_breakpoint(UILayoutEngine *engine,
                                     UIBreakpoint breakpoint);

/* Get active breakpoint based on viewport */
UIBreakpoint *
ui_layout_engine_get_active_breakpoint(const UILayoutEngine *engine);

/* Update active breakpoint (called when viewport changes) */
void ui_layout_engine_update_breakpoint(UILayoutEngine *engine);

/* ============================================================================
 * API - UTILITIES
 * ============================================================================
 */

/* Clamp size to min/max constraints */
UISize ui_clamp_size(const UILayoutElement *element, UISize size);

/* Check if element needs layout */
bool ui_element_needs_layout(const UILayoutElement *element);

/* Clear measure cache */
void ui_layout_clear_measure_cache(UILayoutEngine *engine);

/* ============================================================================
 * API - PERFORMANCE AND DEBUGGING
 * ============================================================================
 */

/* Enable/disable measure caching */
void ui_layout_engine_set_caching(UILayoutEngine *engine, bool enable);

/* Enable/disable profiling */
void ui_layout_engine_set_profiling(UILayoutEngine *engine, bool enable);

/* Enable/disable debug output */
void ui_layout_engine_set_debug(UILayoutEngine *engine, bool enable);

/* Get performance statistics */
void ui_layout_engine_get_stats(const UILayoutEngine *engine,
                                uint32_t *out_measure_passes,
                                uint32_t *out_arrange_passes,
                                float *out_avg_measure_time_ms,
                                float *out_avg_arrange_time_ms,
                                uint32_t *out_cache_hits,
                                uint32_t *out_cache_misses);

/* Print layout tree */
void ui_layout_print_tree(const UILayoutElement *element, int depth);

/* Print element layout info */
void ui_layout_print_element(const UILayoutElement *element);

#ifdef __cplusplus
}
#endif

#endif /* UI_LAYOUT_ENGINE_H */
