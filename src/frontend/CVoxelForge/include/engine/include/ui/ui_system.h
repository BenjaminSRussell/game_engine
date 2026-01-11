#ifndef ENGINE_UI_SYSTEM_H
#define ENGINE_UI_SYSTEM_H

#include "common.h"
#include "core/string_utils.h"
#include "math/rect.h"
#include "math/vec2.h"
#include <stdbool.h>

// UI Element Types
typedef enum UIElementType {
  UI_ELEMENT_CONTAINER,
  UI_ELEMENT_BUTTON,
  UI_ELEMENT_LABEL,
  UI_ELEMENT_IMAGE,
  UI_ELEMENT_SLIDER,
  UI_ELEMENT_TEXT_INPUT,
  UI_ELEMENT_CUSTOM
} UIElementType;

// Layout constraints
typedef enum UILayoutType {
  UI_LAYOUT_ABSOLUTE,
  UI_LAYOUT_RELATIVE,
  UI_LAYOUT_FLEX_ROW,
  UI_LAYOUT_FLEX_COLUMN,
  UI_LAYOUT_GRID
} UILayoutType;

typedef struct UILayoutStyle {
  UILayoutType type;
  f32 width;  // 0 for auto
  f32 height; // 0 for auto
  f32 min_width;
  f32 min_height;
  f32 max_width;
  f32 max_height;
  f32 padding[4]; // top, right, bottom, left
  f32 margin[4];
  f32 flex_grow;
  f32 gap;
} UILayoutStyle;

// Forward declaration
typedef struct UIElement UIElement;

// Event callbacks
typedef void (*UIEventCallback)(UIElement *element, void *event_data,
                                void *user_data);

struct UIElement {
  const char *id;
  UIElementType type;
  UIElement *parent;
  UIElement **children; // Dynamic array
  u32 child_count;

  // Geometry (computed)
  Rect computed_rect;

  // Style & Layout
  UILayoutStyle style;
  bool is_visible;
  bool is_hovered;
  bool is_pressed;

  // Callbacks
  UIEventCallback on_click;
  UIEventCallback on_hover;

  // Data
  void *widget_data; // Type-specific data
  void *user_data;
};

// System API
void ui_system_init(void);
void ui_system_shutdown(void);
void ui_system_update(f32 dt);
void ui_system_render(void);

// Element management
UIElement *ui_create_element(UIElementType type, const char *id);
void ui_destroy_element(UIElement *element);
void ui_add_child(UIElement *parent, UIElement *child);

// Layout
void ui_compute_layout(UIElement *root, Rect viewport);

#endif // ENGINE_UI_SYSTEM_H
