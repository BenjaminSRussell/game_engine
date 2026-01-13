/*
 * ui_layout_engine.c
 * Core UI Layout Engine - Two-Pass Measure/Arrange System Implementation
 *
 * Implements Slate/WPF-style two-pass layout:
 * 1. Measure Pass (bottom-up): Calculate desired sizes
 * 2. Arrange Pass (top-down): Position elements in allocated space
 *
 * Part of the UI subsystem
 * Advanced 3D Rendering Engine
 */

#include "ui_layout_engine.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ============================================================================
 * INTERNAL UTILITIES
 * ============================================================================
 */

static uint32_t g_next_engine_id = 1;
static uint32_t g_next_element_id = 1;

static double get_time_ms(void) {
  return ((double)clock() / CLOCKS_PER_SEC) * 1000.0;
}

static float clampf(float value, float min_val, float max_val) {
  if (value < min_val)
    return min_val;
  if (value > max_val)
    return max_val;
  return value;
}

/* ============================================================================
 * LAYOUT ENGINE MANAGEMENT
 * ============================================================================
 */

UILayoutEngine *ui_layout_engine_create(float viewport_width,
                                        float viewport_height) {
  UILayoutEngine *engine = (UILayoutEngine *)malloc(sizeof(UILayoutEngine));
  if (!engine)
    return NULL;

  memset(engine, 0, sizeof(UILayoutEngine));

  engine->id = g_next_engine_id++;
  engine->initialized = true;
  engine->viewport_width = viewport_width;
  engine->viewport_height = viewport_height;
  engine->viewport_dpi_scale = 1.0f;

  /* Initialize breakpoint storage */
  engine->breakpoint_capacity = 8;
  engine->breakpoints = (UIBreakpoint *)malloc(sizeof(UIBreakpoint) *
                                               engine->breakpoint_capacity);
  engine->breakpoint_count = 0;

  /* Configuration defaults */
  engine->enable_caching = true;
  engine->enable_profiling = false;
  engine->enable_debug = false;

  return engine;
}

void ui_layout_engine_destroy(UILayoutEngine *engine) {
  if (!engine)
    return;

  if (engine->root) {
    ui_layout_element_destroy(engine->root);
  }

  if (engine->breakpoints) {
    free(engine->breakpoints);
  }

  free(engine);
}

void ui_layout_engine_set_viewport(UILayoutEngine *engine, float width,
                                   float height) {
  if (!engine)
    return;

  if (engine->viewport_width != width || engine->viewport_height != height) {
    engine->viewport_width = width;
    engine->viewport_height = height;

    /* Update active breakpoint */
    ui_layout_engine_update_breakpoint(engine);

    /* Invalidate root layout */
    if (engine->root) {
      ui_layout_invalidate(engine->root);
    }

    if (engine->enable_debug) {
      printf("[UILayoutEngine] Viewport changed to %.0fx%.0f\n", width, height);
    }
  }
}

void ui_layout_engine_set_dpi_scale(UILayoutEngine *engine, float scale) {
  if (!engine)
    return;
  engine->viewport_dpi_scale = scale;

  if (engine->root) {
    ui_layout_invalidate(engine->root);
  }
}

/* ============================================================================
 * ELEMENT MANAGEMENT
 * ============================================================================
 */

UILayoutElement *ui_layout_element_create(const char *name) {
  UILayoutElement *element = (UILayoutElement *)malloc(sizeof(UILayoutElement));
  if (!element)
    return NULL;

  memset(element, 0, sizeof(UILayoutElement));

  element->id = g_next_element_id++;
  element->name = name ? strdup(name) : strdup("UIElement");

  /* Initialize box model */
  element->box_sizing = BOX_SIZING_CONTENT_BOX;

  /* Initialize size constraints */
  element->min_width = 0.0f;
  element->max_width = INFINITY;
  element->min_height = 0.0f;
  element->max_height = INFINITY;
  element->preferred_width = -1.0f;  /* Auto */
  element->preferred_height = -1.0f; /* Auto */

  /* Initialize anchor/dock */
  element->anchor_preset = ANCHOR_TOP_LEFT;
  element->anchor = ui_get_anchor_preset_config(ANCHOR_TOP_LEFT);
  element->dock = DOCK_NONE;

  /* Mark as needing layout */
  element->is_dirty = true;
  element->needs_measure = true;
  element->needs_arrange = true;

  /* Initialize children array */
  element->child_capacity = 4;
  element->children = (UILayoutElement **)malloc(sizeof(UILayoutElement *) *
                                                 element->child_capacity);
  element->child_count = 0;

  return element;
}

void ui_layout_element_destroy(UILayoutElement *element) {
  if (!element)
    return;

  /* Destroy children */
  for (uint32_t i = 0; i < element->child_count; i++) {
    ui_layout_element_destroy(element->children[i]);
  }

  if (element->children) {
    free(element->children);
  }

  if (element->name) {
    free(element->name);
  }

  free(element);
}

void ui_layout_element_add_child(UILayoutElement *parent,
                                 UILayoutElement *child) {
  if (!parent || !child)
    return;

  /* Ensure capacity */
  if (parent->child_count >= parent->child_capacity) {
    parent->child_capacity *= 2;
    parent->children = (UILayoutElement **)realloc(
        parent->children, sizeof(UILayoutElement *) * parent->child_capacity);
  }

  /* Add child */
  child->parent = parent;
  parent->children[parent->child_count++] = child;

  /* Invalidate parent layout */
  ui_layout_invalidate(parent);
}

void ui_layout_element_remove_child(UILayoutElement *parent,
                                    UILayoutElement *child) {
  if (!parent || !child)
    return;

  /* Find and remove child */
  for (uint32_t i = 0; i < parent->child_count; i++) {
    if (parent->children[i] == child) {
      child->parent = NULL;

      /* Shift remaining children */
      for (uint32_t j = i; j < parent->child_count - 1; j++) {
        parent->children[j] = parent->children[j + 1];
      }
      parent->child_count--;

      /* Invalidate parent layout */
      ui_layout_invalidate(parent);
      break;
    }
  }
}

/* ============================================================================
 * TWO-PASS LAYOUT ALGORITHM - MEASURE PASS (TODO-0179)
 * ============================================================================
 */

void ui_layout_measure(UILayoutEngine *engine, UILayoutElement *element,
                       float available_width, float available_height) {
  if (!engine || !element)
    return;

  double start_time = engine->enable_profiling ? get_time_ms() : 0.0;

  /* Check measure cache */
  if (engine->enable_caching && !element->needs_measure &&
      element->measure_result.is_cached &&
      element->measure_result.cache_frame == engine->current_frame) {
    engine->measure_cache_hits++;
    return;
  }

  engine->measure_cache_misses++;

  /* Apply DPI scaling to available size */
  available_width *= engine->viewport_dpi_scale;
  available_height *= engine->viewport_dpi_scale;

  /* Subtract element spacing from available size */
  float available_content_width =
      available_width - ui_get_horizontal_spacing(element);
  float available_content_height =
      available_height - ui_get_vertical_spacing(element);
  available_content_width = fmaxf(0.0f, available_content_width);
  available_content_height = fmaxf(0.0f, available_content_height);

  /* BOTTOM-UP: Measure all children first */
  for (uint32_t i = 0; i < element->child_count; i++) {
    ui_layout_measure(engine, element->children[i], available_content_width,
                      available_content_height);
  }

  /* Compute desired size for this element */
  element->measure_result = ui_element_compute_desired_size(
      element, available_content_width, available_content_height);

  /* Cache the result */
  element->measure_result.is_cached = true;
  element->measure_result.cache_frame = engine->current_frame;
  element->needs_measure = false;

  /* Update statistics */
  if (engine->enable_profiling) {
    double end_time = get_time_ms();
    engine->total_measure_time_ms += (float)(end_time - start_time);
  }
  engine->total_measure_passes++;

  if (engine->enable_debug) {
    printf("[Measure] %s: %.2fx%.2f (available: %.2fx%.2f)\n", element->name,
           element->measure_result.desired_width,
           element->measure_result.desired_height, available_width,
           available_height);
  }
}

UILayoutMeasureResult ui_element_compute_desired_size(UILayoutElement *element,
                                                      float available_width,
                                                      float available_height) {
  UILayoutMeasureResult result = {0};

  if (!element)
    return result;

  /* Start with preferred size if specified */
  float content_width =
      element->preferred_width >= 0.0f ? element->preferred_width : 0.0f;
  float content_height =
      element->preferred_height >= 0.0f ? element->preferred_height : 0.0f;

  /* If no preferred size, calculate from children */
  if (element->preferred_width < 0.0f || element->preferred_height < 0.0f) {
    float max_child_width = 0.0f;
    float total_child_height = 0.0f;

    for (uint32_t i = 0; i < element->child_count; i++) {
      UILayoutElement *child = element->children[i];
      float child_width = child->measure_result.desired_width;
      float child_height = child->measure_result.desired_height;

      /* Add child margins */
      child_width += child->margin.left + child->margin.right;
      child_height += child->margin.top + child->margin.bottom;

      if (child_width > max_child_width) {
        max_child_width = child_width;
      }
      total_child_height += child_height;
    }

    if (element->preferred_width < 0.0f) {
      content_width = max_child_width;
    }
    if (element->preferred_height < 0.0f) {
      content_height = total_child_height;
    }
  }

  /* Apply box sizing */
  UISize size = ui_apply_box_sizing(element, content_width, content_height);

  /* Add padding and border if content-box */
  if (element->box_sizing == BOX_SIZING_CONTENT_BOX) {
    size.width += element->padding.left + element->padding.right;
    size.width += element->border.left + element->border.right;
    size.height += element->padding.top + element->padding.bottom;
    size.height += element->border.top + element->border.bottom;
  }

  /* Apply min/max constraints */
  size = ui_clamp_size(element, size);

  result.desired_width = size.width;
  result.desired_height = size.height;
  result.baseline_offset = 0.0f; /* TODO: Calculate actual baseline */

  return result;
}

/* ============================================================================
 * TWO-PASS LAYOUT ALGORITHM - ARRANGE PASS (TODO-0179)
 * ============================================================================
 */

void ui_layout_arrange(UILayoutEngine *engine, UILayoutElement *element,
                       UIRect final_rect) {
  if (!engine || !element)
    return;

  double start_time = engine->enable_profiling ? get_time_ms() : 0.0;

  /* Store final rect */
  element->arrange_result.final_rect = final_rect;

  /* Calculate content rect (subtract margin, border, padding) */
  UIRect content_rect = final_rect;

  /* Subtract margin */
  content_rect.x += element->margin.left;
  content_rect.y += element->margin.top;
  content_rect.width -= element->margin.left + element->margin.right;
  content_rect.height -= element->margin.top + element->margin.bottom;

  /* Subtract border */
  content_rect.x += element->border.left;
  content_rect.y += element->border.top;
  content_rect.width -= element->border.left + element->border.right;
  content_rect.height -= element->border.top + element->border.bottom;

  /* Subtract padding */
  content_rect.x += element->padding.left;
  content_rect.y += element->padding.top;
  content_rect.width -= element->padding.left + element->padding.right;
  content_rect.height -= element->padding.top + element->padding.bottom;

  /* Ensure non-negative dimensions */
  content_rect.width = fmaxf(0.0f, content_rect.width);
  content_rect.height = fmaxf(0.0f, content_rect.height);

  element->arrange_result.content_rect = content_rect;
  element->arrange_result.is_visible =
      (content_rect.width > 0.0f && content_rect.height > 0.0f);
  element->needs_arrange = false;

  /* TOP-DOWN: Arrange children */
  ui_element_arrange_children(element, content_rect);

  /* Update statistics */
  if (engine->enable_profiling) {
    double end_time = get_time_ms();
    engine->total_arrange_time_ms += (float)(end_time - start_time);
  }
  engine->total_arrange_passes++;

  if (engine->enable_debug) {
    printf("[Arrange] %s: (%.2f, %.2f) %.2fx%.2f\n", element->name,
           final_rect.x, final_rect.y, final_rect.width, final_rect.height);
  }
}

void ui_element_arrange_children(UILayoutElement *element,
                                 UIRect content_rect) {
  if (!element || element->child_count == 0)
    return;

  /* Simple vertical stack layout for default behavior */
  float current_y = content_rect.y;

  for (uint32_t i = 0; i < element->child_count; i++) {
    UILayoutElement *child = element->children[i];

    /* Calculate child rect based on anchor/dock */
    UIRect child_rect;

    if (child->dock != DOCK_NONE) {
      /* Apply dock layout */
      child_rect = ui_element_apply_dock_layout(child, content_rect);
    } else {
      /* Apply anchor layout */
      child_rect = ui_element_apply_anchor_layout(child, content_rect);

      /* Default positioning if using top-left anchor */
      if (child->anchor_preset == ANCHOR_TOP_LEFT) {
        child_rect.x = content_rect.x;
        child_rect.y = current_y;
        child_rect.width = child->measure_result.desired_width;
        child_rect.height = child->measure_result.desired_height;
      }
    }

    /* Recursively arrange this child */
    if (element->parent && element->parent->parent) {
      /* Get engine from root traversal - simplified for now */
      UILayoutEngine dummy_engine = {0};
      dummy_engine.enable_profiling = false;
      dummy_engine.enable_debug = false;
      ui_layout_arrange(&dummy_engine, child, child_rect);
    }

    /* Advance y position for next child */
    current_y += child_rect.height + child->margin.top + child->margin.bottom;
  }
}

void ui_layout_invalidate(UILayoutElement *element) {
  if (!element)
    return;

  element->is_dirty = true;
  element->needs_measure = true;
  element->needs_arrange = true;
  element->measure_result.is_cached = false;

  /* Invalidate all children */
  for (uint32_t i = 0; i < element->child_count; i++) {
    ui_layout_invalidate(element->children[i]);
  }
}

/* ============================================================================
 * BOX MODEL CALCULATIONS (TODO-0180)
 * ============================================================================
 */

UISize ui_calculate_content_size(const UILayoutElement *element,
                                 UISize outer_size) {
  if (!element)
    return (UISize){0, 0};

  UISize content = outer_size;

  /* Subtract padding and border */
  content.width -= element->padding.left + element->padding.right;
  content.width -= element->border.left + element->border.right;
  content.height -= element->padding.top + element->padding.bottom;
  content.height -= element->border.top + element->border.bottom;

  /* Ensure non-negative */
  content.width = fmaxf(0.0f, content.width);
  content.height = fmaxf(0.0f, content.height);

  return content;
}

UISize ui_calculate_border_box_size(const UILayoutElement *element,
                                    UISize content_size) {
  if (!element)
    return content_size;

  UISize border_box = content_size;

  /* Add padding and border */
  border_box.width += element->padding.left + element->padding.right;
  border_box.width += element->border.left + element->border.right;
  border_box.height += element->padding.top + element->padding.bottom;
  border_box.height += element->border.top + element->border.bottom;

  return border_box;
}

float ui_get_horizontal_spacing(const UILayoutElement *element) {
  if (!element)
    return 0.0f;

  return element->margin.left + element->margin.right + element->padding.left +
         element->padding.right + element->border.left + element->border.right;
}

float ui_get_vertical_spacing(const UILayoutElement *element) {
  if (!element)
    return 0.0f;

  return element->margin.top + element->margin.bottom + element->padding.top +
         element->padding.bottom + element->border.top + element->border.bottom;
}

UISize ui_apply_box_sizing(const UILayoutElement *element, float width,
                           float height) {
  if (!element)
    return (UISize){width, height};

  UISize size = {width, height};

  if (element->box_sizing == BOX_SIZING_BORDER_BOX) {
    /* Width/height already include padding and border */
    /* No adjustment needed */
  } else {
    /* Content-box: width/height are just content dimensions */
    /* No adjustment needed here - padding/border added separately */
  }

  return size;
}

/* ============================================================================
 * ANCHORING AND DOCKING (TODO-0181)
 * ============================================================================
 */

void ui_element_set_anchor_preset(UILayoutElement *element,
                                  AnchorPreset preset) {
  if (!element)
    return;

  element->anchor_preset = preset;
  element->anchor = ui_get_anchor_preset_config(preset);
  ui_layout_invalidate(element);
}

void ui_element_set_anchor(UILayoutElement *element, UIAnchor anchor) {
  if (!element)
    return;

  element->anchor = anchor;
  element->anchor_preset = ANCHOR_CUSTOM;
  ui_layout_invalidate(element);
}

UIAnchor ui_get_anchor_preset_config(AnchorPreset preset) {
  UIAnchor anchor = {0};

  switch (preset) {
  case ANCHOR_TOP_LEFT:
    anchor.min_x = 0.0f;
    anchor.max_x = 0.0f;
    anchor.min_y = 0.0f;
    anchor.max_y = 0.0f;
    break;
  case ANCHOR_TOP_CENTER:
    anchor.min_x = 0.5f;
    anchor.max_x = 0.5f;
    anchor.min_y = 0.0f;
    anchor.max_y = 0.0f;
    break;
  case ANCHOR_TOP_RIGHT:
    anchor.min_x = 1.0f;
    anchor.max_x = 1.0f;
    anchor.min_y = 0.0f;
    anchor.max_y = 0.0f;
    break;
  case ANCHOR_MIDDLE_LEFT:
    anchor.min_x = 0.0f;
    anchor.max_x = 0.0f;
    anchor.min_y = 0.5f;
    anchor.max_y = 0.5f;
    break;
  case ANCHOR_MIDDLE_CENTER:
    anchor.min_x = 0.5f;
    anchor.max_x = 0.5f;
    anchor.min_y = 0.5f;
    anchor.max_y = 0.5f;
    break;
  case ANCHOR_MIDDLE_RIGHT:
    anchor.min_x = 1.0f;
    anchor.max_x = 1.0f;
    anchor.min_y = 0.5f;
    anchor.max_y = 0.5f;
    break;
  case ANCHOR_BOTTOM_LEFT:
    anchor.min_x = 0.0f;
    anchor.max_x = 0.0f;
    anchor.min_y = 1.0f;
    anchor.max_y = 1.0f;
    break;
  case ANCHOR_BOTTOM_CENTER:
    anchor.min_x = 0.5f;
    anchor.max_x = 0.5f;
    anchor.min_y = 1.0f;
    anchor.max_y = 1.0f;
    break;
  case ANCHOR_BOTTOM_RIGHT:
    anchor.min_x = 1.0f;
    anchor.max_x = 1.0f;
    anchor.min_y = 1.0f;
    anchor.max_y = 1.0f;
    break;
  case ANCHOR_STRETCH_HORIZONTAL:
    anchor.min_x = 0.0f;
    anchor.max_x = 1.0f;
    anchor.min_y = 0.5f;
    anchor.max_y = 0.5f;
    break;
  case ANCHOR_STRETCH_VERTICAL:
    anchor.min_x = 0.5f;
    anchor.max_x = 0.5f;
    anchor.min_y = 0.0f;
    anchor.max_y = 1.0f;
    break;
  case ANCHOR_STRETCH_BOTH:
    anchor.min_x = 0.0f;
    anchor.max_x = 1.0f;
    anchor.min_y = 0.0f;
    anchor.max_y = 1.0f;
    break;
  case ANCHOR_CUSTOM:
    /* Use existing anchor values */
    break;
  }

  return anchor;
}

UIRect ui_element_apply_anchor_layout(const UILayoutElement *element,
                                      UIRect parent_rect) {
  if (!element)
    return (UIRect){0, 0, 0, 0};

  UIRect rect = {0};
  UIAnchor anchor = element->anchor;

  /* Calculate anchor points in parent space */
  float anchor_min_x = parent_rect.x + parent_rect.width * anchor.min_x;
  float anchor_max_x = parent_rect.x + parent_rect.width * anchor.max_x;
  float anchor_min_y = parent_rect.y + parent_rect.height * anchor.min_y;
  float anchor_max_y = parent_rect.y + parent_rect.height * anchor.max_y;

  /* Calculate element rect */
  if (anchor.min_x == anchor.max_x) {
    /* Fixed width, positioned at anchor */
    rect.width = element->measure_result.desired_width;
    rect.x = anchor_min_x + anchor.offset_x - rect.width * anchor.min_x;
  } else {
    /* Stretched horizontally */
    rect.x = anchor_min_x + anchor.offset_x;
    rect.width = (anchor_max_x - anchor_min_x) - anchor.offset_x * 2.0f;
  }

  if (anchor.min_y == anchor.max_y) {
    /* Fixed height, positioned at anchor */
    rect.height = element->measure_result.desired_height;
    rect.y = anchor_min_y + anchor.offset_y - rect.height * anchor.min_y;
  } else {
    /* Stretched vertically */
    rect.y = anchor_min_y + anchor.offset_y;
    rect.height = (anchor_max_y - anchor_min_y) - anchor.offset_y * 2.0f;
  }

  return rect;
}

void ui_element_set_dock(UILayoutElement *element, DockMode dock) {
  if (!element)
    return;

  element->dock = dock;
  ui_layout_invalidate(element);
}

UIRect ui_element_apply_dock_layout(const UILayoutElement *element,
                                    UIRect parent_rect) {
  if (!element)
    return (UIRect){0, 0, 0, 0};

  UIRect rect = parent_rect;
  float desired_width = element->measure_result.desired_width;
  float desired_height = element->measure_result.desired_height;

  switch (element->dock) {
  case DOCK_FILL:
    /* Use entire parent rect */
    break;

  case DOCK_TOP:
    rect.height = desired_height;
    break;

  case DOCK_BOTTOM:
    rect.y = parent_rect.y + parent_rect.height - desired_height;
    rect.height = desired_height;
    break;

  case DOCK_LEFT:
    rect.width = desired_width;
    break;

  case DOCK_RIGHT:
    rect.x = parent_rect.x + parent_rect.width - desired_width;
    rect.width = desired_width;
    break;

  case DOCK_NONE:
    /* No docking */
    rect.width = desired_width;
    rect.height = desired_height;
    break;
  }

  return rect;
}

/* ============================================================================
 * RESPONSIVE DESIGN (TODO-0182)
 * ============================================================================
 */

void ui_layout_engine_add_breakpoint(UILayoutEngine *engine,
                                     UIBreakpoint breakpoint) {
  if (!engine)
    return;

  /* Ensure capacity */
  if (engine->breakpoint_count >= engine->breakpoint_capacity) {
    engine->breakpoint_capacity *= 2;
    engine->breakpoints = (UIBreakpoint *)realloc(
        engine->breakpoints,
        sizeof(UIBreakpoint) * engine->breakpoint_capacity);
  }

  engine->breakpoints[engine->breakpoint_count++] = breakpoint;
  ui_layout_engine_update_breakpoint(engine);
}

UIBreakpoint *
ui_layout_engine_get_active_breakpoint(const UILayoutEngine *engine) {
  if (!engine || engine->breakpoint_count == 0)
    return NULL;

  if (engine->active_breakpoint_index < engine->breakpoint_count) {
    return &engine->breakpoints[engine->active_breakpoint_index];
  }

  return NULL;
}

void ui_layout_engine_update_breakpoint(UILayoutEngine *engine) {
  if (!engine || engine->breakpoint_count == 0)
    return;

  float width = engine->viewport_width;
  uint32_t old_index = engine->active_breakpoint_index;

  /* Find matching breakpoint */
  for (uint32_t i = 0; i < engine->breakpoint_count; i++) {
    UIBreakpoint *bp = &engine->breakpoints[i];
    if (width >= bp->min_width && width <= bp->max_width) {
      engine->active_breakpoint_index = i;
      break;
    }
  }

  /* Invalidate layout if breakpoint changed */
  if (old_index != engine->active_breakpoint_index && engine->root) {
    ui_layout_invalidate(engine->root);

    if (engine->enable_debug) {
      UIBreakpoint *bp = &engine->breakpoints[engine->active_breakpoint_index];
      printf("[UILayoutEngine] Breakpoint changed to: %s (scale: %.2f)\n",
             bp->name, bp->scale_factor);
    }
  }
}

/* ============================================================================
 * UTILITIES
 * ============================================================================
 */

UISize ui_clamp_size(const UILayoutElement *element, UISize size) {
  if (!element)
    return size;

  UISize clamped = size;
  clamped.width = clampf(clamped.width, element->min_width, element->max_width);
  clamped.height =
      clampf(clamped.height, element->min_height, element->max_height);

  return clamped;
}

bool ui_element_needs_layout(const UILayoutElement *element) {
  return element && (element->is_dirty || element->needs_measure ||
                     element->needs_arrange);
}

static void clear_cache_recursive(UILayoutElement *element) {
  if (!element)
    return;

  element->measure_result.is_cached = false;
  element->needs_measure = true;

  for (uint32_t i = 0; i < element->child_count; i++) {
    clear_cache_recursive(element->children[i]);
  }
}

void ui_layout_clear_measure_cache(UILayoutEngine *engine) {
  if (!engine || !engine->root)
    return;

  clear_cache_recursive(engine->root);
}

/* ============================================================================
 * PERFORMANCE AND DEBUGGING
 * ============================================================================
 */

void ui_layout_engine_set_caching(UILayoutEngine *engine, bool enable) {
  if (!engine)
    return;
  engine->enable_caching = enable;

  if (!enable) {
    ui_layout_clear_measure_cache(engine);
  }
}

void ui_layout_engine_set_profiling(UILayoutEngine *engine, bool enable) {
  if (!engine)
    return;
  engine->enable_profiling = enable;
}

void ui_layout_engine_set_debug(UILayoutEngine *engine, bool enable) {
  if (!engine)
    return;
  engine->enable_debug = enable;
}

void ui_layout_engine_get_stats(const UILayoutEngine *engine,
                                uint32_t *out_measure_passes,
                                uint32_t *out_arrange_passes,
                                float *out_avg_measure_time_ms,
                                float *out_avg_arrange_time_ms,
                                uint32_t *out_cache_hits,
                                uint32_t *out_cache_misses) {
  if (!engine)
    return;

  if (out_measure_passes)
    *out_measure_passes = engine->total_measure_passes;
  if (out_arrange_passes)
    *out_arrange_passes = engine->total_arrange_passes;

  if (out_avg_measure_time_ms) {
    *out_avg_measure_time_ms =
        engine->total_measure_passes > 0
            ? engine->total_measure_time_ms / engine->total_measure_passes
            : 0.0f;
  }

  if (out_avg_arrange_time_ms) {
    *out_avg_arrange_time_ms =
        engine->total_arrange_passes > 0
            ? engine->total_arrange_time_ms / engine->total_arrange_passes
            : 0.0f;
  }

  if (out_cache_hits)
    *out_cache_hits = engine->measure_cache_hits;
  if (out_cache_misses)
    *out_cache_misses = engine->measure_cache_misses;
}

void ui_layout_print_tree(const UILayoutElement *element, int depth) {
  if (!element)
    return;

  for (int i = 0; i < depth; i++) {
    printf("  ");
  }

  printf("%s: (%.2f, %.2f) %.2fx%.2f [desired: %.2fx%.2f]\n", element->name,
         element->arrange_result.final_rect.x,
         element->arrange_result.final_rect.y,
         element->arrange_result.final_rect.width,
         element->arrange_result.final_rect.height,
         element->measure_result.desired_width,
         element->measure_result.desired_height);

  for (uint32_t i = 0; i < element->child_count; i++) {
    ui_layout_print_tree(element->children[i], depth + 1);
  }
}

void ui_layout_print_element(const UILayoutElement *element) {
  if (!element)
    return;

  printf("=== Element: %s (ID: %u) ===\n", element->name, element->id);
  printf("Box Sizing: %s\n", element->box_sizing == BOX_SIZING_BORDER_BOX
                                 ? "border-box"
                                 : "content-box");
  printf("Anchor: %d, Dock: %d\n", element->anchor_preset, element->dock);
  printf("Margin: %.2f %.2f %.2f %.2f\n", element->margin.top,
         element->margin.right, element->margin.bottom, element->margin.left);
  printf("Padding: %.2f %.2f %.2f %.2f\n", element->padding.top,
         element->padding.right, element->padding.bottom,
         element->padding.left);
  printf("Border: %.2f %.2f %.2f %.2f\n", element->border.top,
         element->border.right, element->border.bottom, element->border.left);
  printf("Desired Size: %.2fx%.2f\n", element->measure_result.desired_width,
         element->measure_result.desired_height);
  printf("Final Rect: (%.2f, %.2f) %.2fx%.2f\n",
         element->arrange_result.final_rect.x,
         element->arrange_result.final_rect.y,
         element->arrange_result.final_rect.width,
         element->arrange_result.final_rect.height);
  printf("Children: %u\n", element->child_count);
  printf("Needs Layout: %s\n", ui_element_needs_layout(element) ? "yes" : "no");
}
