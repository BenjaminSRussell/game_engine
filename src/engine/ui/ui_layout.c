#include <common.h>
#include <core/logger.h>
#include <core/memory.h>
#include <ui/ui_system.h>
#include <include/math/math.h>

void ui_system_init(void) { LOG_INFO("UI System Initialized"); }

void ui_system_shutdown(void) { LOG_INFO("UI System Shutdown"); }

static void compute_flex_layout(UIElement *parent, Rect available_space) {
  if (!parent || parent->child_count == 0)
    return;

  bool is_row = parent->style.type == UI_LAYOUT_FLEX_ROW;
  f32 main_axis_pos = is_row ? available_space.x : available_space.y;
  f32 cross_axis_pos = is_row ? available_space.y : available_space.x;

  // Pass 1: Measure fixed children and total flex grow
  f32 total_flex = 0;
  f32 used_space = 0;

  for (u32 i = 0; i < parent->child_count; i++) {
    UIElement *child = parent->children[i];
    if (!child->is_visible)
      continue;

    // Apply margins
    used_space += is_row ? (child->style.margin[1] + child->style.margin[3])
                         : (child->style.margin[0] + child->style.margin[2]);

    if (child->style.flex_grow > 0) {
      total_flex += child->style.flex_grow;
    } else {
      f32 size = is_row ? child->style.width : child->style.height;
      used_space += size;
    }
  }

  // Add gaps
  if (parent->child_count > 1) {
    used_space += parent->style.gap * (parent->child_count - 1);
  }

  f32 remaining_space =
      (is_row ? available_space.width : available_space.height) - used_space;
  if (remaining_space < 0)
    remaining_space = 0;

  // Pass 2: Position children
  f32 current_pos = main_axis_pos;

  for (u32 i = 0; i < parent->child_count; i++) {
    UIElement *child = parent->children[i];
    if (!child->is_visible)
      continue;

    f32 child_size = 0;
    if (child->style.flex_grow > 0) {
      child_size = (child->style.flex_grow / total_flex) * remaining_space;
    } else {
      child_size = is_row ? child->style.width : child->style.height;
    }

    f32 cross_size = is_row ? child->style.height : child->style.width;
    if (cross_size == 0)
      cross_size = is_row ? available_space.height
                          : available_space.width; // Stretch by default

    Rect child_rect;
    if (is_row) {
      current_pos += child->style.margin[3]; // Left margin
      child_rect.x = current_pos;
      child_rect.y = cross_axis_pos + child->style.margin[0]; // Top margin
      child_rect.width = child_size;
      child_rect.height = cross_size;
      current_pos += child_size + child->style.margin[1] +
                     parent->style.gap; // Right margin + gap
    } else {
      current_pos += child->style.margin[0];                  // Top margin
      child_rect.x = cross_axis_pos + child->style.margin[3]; // Left margin
      child_rect.y = current_pos;
      child_rect.width = cross_size;
      child_rect.height = child_size;
      current_pos += child_size + child->style.margin[2] +
                     parent->style.gap; // Bottom margin + gap
    }

    // Apply padding for content
    // In a real system, we'd subtract padding from content rect, but for now we
    // set the element rect
    child->computed_rect = child_rect;

    // Recursive layout
    ui_compute_layout(child, child_rect);
  }
}

void ui_compute_layout(UIElement *root, Rect viewport) {
  if (!root)
    return;

  root->computed_rect = viewport;

  switch (root->style.type) {
  case UI_LAYOUT_ABSOLUTE:
    // Just recurse
    for (u32 i = 0; i < root->child_count; i++) {
      ui_compute_layout(root->children[i], viewport);
    }
    break;

  case UI_LAYOUT_FLEX_ROW:
  case UI_LAYOUT_FLEX_COLUMN:
    compute_flex_layout(root, viewport);
    break;

  default:
    break;
  }
}
