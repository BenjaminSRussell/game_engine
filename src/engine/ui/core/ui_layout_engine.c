/**
 * @file ui_layout_engine.c
 * @brief Flexbox-style UI Layout Engine.
 *
 * recursively calculates sizes and positions of UI widgets based on
 * constraints (Min/Max Width, Padding, Margin, Flex Grow).
 *
 * @copyright (c) 2024 Minecraft v2 Engine
 */

#include <ui/core/ui_layout_engine.h>

// =================================================================================================
//                                      STRUCTS
// =================================================================================================

typedef struct LayoutStyle {
  float width, height; // -1 for Auto
  float min_width, min_height;
  float max_width, max_height;

  float padding[4]; // T, R, B, L
  float margin[4];

  float flex_grow;
  int flex_direction;  // ROW, COL
  int justify_content; // START, CENTER, SPACE_BETWEEN
  int align_items;     // STRETCH, CENTER
} LayoutStyle;

typedef struct UIWidget {
  LayoutStyle style;
  struct UIWidget *parent;
  struct UIWidget *children[32];
  int child_count;

  // Computed Values
  float final_x, final_y;
  float final_w, final_h;
} UIWidget;

// =================================================================================================
//                                      IMPLEMENTATION
// =================================================================================================

/**
 * @brief First Pass: Measure preferred sizes (recursive bottom-up).
 */
void ui_layout_measure(UIWidget *w) {
  // 1. Measure children
  for (int i = 0; i < w->child_count; i++) {
    ui_layout_measure(w->children[i]);
  }

  // 2. Compute own size based on children + padding
  if (w->style.width == -1) {
    // ... Sum child widths ...
  }
}

/**
 * @brief Second Pass: Layout (recursive top-down).
 */
void ui_layout_arrange(UIWidget *w, float x, float y, float width,
                       float height) {
  w->final_x = x + w->style.margin[3];
  w->final_y = y + w->style.margin[0];
  w->final_w = width; // Simplified
  w->final_h = height;

  // Calculate remaining space for children
  float child_space = width - (w->style.padding[1] + w->style.padding[3]);
  float total_flex = 0;

  for (int i = 0; i < w->child_count; i++)
    total_flex += w->children[i]->style.flex_grow;

  // Distribute space
  float cur_x = w->final_x + w->style.padding[3];
  float cur_y = w->final_y + w->style.padding[0];

  for (int i = 0; i < w->child_count; i++) {
    UIWidget *c = w->children[i];
    float child_w = c->final_w; // From measure

    if (total_flex > 0) {
      float share = c->style.flex_grow / total_flex;
      child_w = child_space * share;
    }

    ui_layout_arrange(c, cur_x, cur_y, child_w, height); // Assumes ROW layout

    cur_x += child_w;
  }
}
