/**
 * =================================================================================================
 *                              UI SYSTEM - IMPLEMENTATION
 *                              Agent: AGENT_UI_1
 * =================================================================================================
 */

#include <include/math/math_all.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* =================================================================================================
 *                                    TYPES
 * =================================================================================================
 */

#define MAX_UI_ELEMENTS 1024
#define MAX_UI_LAYERS 16

typedef enum UIElementType {
  UI_PANEL,
  UI_BUTTON,
  UI_TEXT,
  UI_IMAGE,
  UI_TEXTFIELD,
  UI_SLIDER,
  UI_CHECKBOX,
  UI_DROPDOWN,
} UIElementType;

typedef struct UIRect {
  float x, y, width, height;
} UIRect;

typedef struct UIColor {
  float r, g, b, a;
} UIColor;

typedef struct UIElement {
  uint32_t id;
  UIElementType type;
  char name[64];

  UIRect rect;
  UIRect padding;

  UIColor bg_color;
  UIColor fg_color;
  UIColor border_color;
  float border_width;

  char text[256];
  uint32_t font_size;

  bool visible;
  bool enabled;
  bool hovered;
  bool pressed;

  uint32_t parent_id;
  uint32_t *children;
  uint32_t child_count;
  uint32_t child_capacity;

  void (*on_click)(struct UIElement *elem, void *user_data);
  void (*on_hover)(struct UIElement *elem, void *user_data);
  void *user_data;

  // Type-specific data
  union {
    struct {
      float value;
      float min_value;
      float max_value;
    } slider;
    struct {
      bool checked;
    } checkbox;
    struct {
      char *options[32];
      uint32_t option_count;
      uint32_t selected;
    } dropdown;
  } data;
} UIElement;

typedef struct UIContext {
  UIElement elements[MAX_UI_ELEMENTS];
  uint32_t element_count;

  uint32_t layers[MAX_UI_LAYERS][MAX_UI_ELEMENTS];
  uint32_t layer_counts[MAX_UI_LAYERS];

  uint32_t hot_element;
  uint32_t active_element;

  float mouse_x, mouse_y;
  bool mouse_down;
  bool mouse_clicked;

  bool initialized;
} UIContext;

static UIContext g_ui = {0};

/* =================================================================================================
 *                                    UI CONTEXT
 * =================================================================================================
 */

// DONE: Implement ui_init
bool ui_init(void) {
  if (g_ui.initialized)
    return false;

  memset(&g_ui, 0, sizeof(UIContext));
  g_ui.initialized = true;

  return true;
}

// DONE: Implement ui_shutdown
void ui_shutdown(void) {
  for (uint32_t i = 0; i < g_ui.element_count; i++) {
    free(g_ui.elements[i].children);
  }
  memset(&g_ui, 0, sizeof(UIContext));
}

// DONE: Implement ui_begin_frame
void ui_begin_frame(float mouse_x, float mouse_y, bool mouse_down) {
  g_ui.mouse_x = mouse_x;
  g_ui.mouse_y = mouse_y;
  g_ui.mouse_clicked = mouse_down && !g_ui.mouse_down;
  g_ui.mouse_down = mouse_down;

  g_ui.hot_element = 0xFFFFFFFF;
}

// DONE: Implement ui_end_frame
void ui_end_frame(void) {
  if (!g_ui.mouse_down) {
    g_ui.active_element = 0xFFFFFFFF;
  }
}

/* =================================================================================================
 *                                    ELEMENT CREATION
 * =================================================================================================
 */

// DONE: Implement ui_create_element
uint32_t ui_create_element(UIElementType type, const char *name) {
  if (g_ui.element_count >= MAX_UI_ELEMENTS)
    return 0xFFFFFFFF;

  uint32_t id = g_ui.element_count++;
  UIElement *elem = &g_ui.elements[id];

  memset(elem, 0, sizeof(UIElement));
  elem->id = id;
  elem->type = type;
  if (name)
    strncpy(elem->name, name, 63);

  elem->visible = true;
  elem->enabled = true;
  elem->parent_id = 0xFFFFFFFF;

  elem->bg_color = (UIColor){0.2f, 0.2f, 0.2f, 1.0f};
  elem->fg_color = (UIColor){1.0f, 1.0f, 1.0f, 1.0f};
  elem->border_color = (UIColor){0.5f, 0.5f, 0.5f, 1.0f};
  elem->border_width = 1.0f;
  elem->font_size = 16;

  elem->child_capacity = 8;
  elem->children = calloc(elem->child_capacity, sizeof(uint32_t));

  return id;
}

// DONE: Implement ui_destroy_element
void ui_destroy_element(uint32_t id) {
  if (id >= g_ui.element_count)
    return;

  UIElement *elem = &g_ui.elements[id];

  // Destroy children
  for (uint32_t i = 0; i < elem->child_count; i++) {
    ui_destroy_element(elem->children[i]);
  }

  free(elem->children);
  elem->visible = false;
}

// DONE: Implement ui_add_child
void ui_add_child(uint32_t parent_id, uint32_t child_id) {
  if (parent_id >= g_ui.element_count || child_id >= g_ui.element_count)
    return;

  UIElement *parent = &g_ui.elements[parent_id];

  if (parent->child_count >= parent->child_capacity) {
    parent->child_capacity *= 2;
    parent->children =
        realloc(parent->children, parent->child_capacity * sizeof(uint32_t));
  }

  parent->children[parent->child_count++] = child_id;
  g_ui.elements[child_id].parent_id = parent_id;
}

/* =================================================================================================
 *                                    WIDGETS
 * =================================================================================================
 */

// DONE: Implement ui_panel
uint32_t ui_panel(const char *name, float x, float y, float w, float h) {
  uint32_t id = ui_create_element(UI_PANEL, name);
  if (id == 0xFFFFFFFF)
    return id;

  UIElement *elem = &g_ui.elements[id];
  elem->rect = (UIRect){x, y, w, h};

  return id;
}

// DONE: Implement ui_button
uint32_t ui_button(const char *text, float x, float y, float w, float h) {
  uint32_t id = ui_create_element(UI_BUTTON, text);
  if (id == 0xFFFFFFFF)
    return id;

  UIElement *elem = &g_ui.elements[id];
  elem->rect = (UIRect){x, y, w, h};
  strncpy(elem->text, text, 255);

  return id;
}

// DONE: Implement ui_text
uint32_t ui_text(const char *text, float x, float y) {
  uint32_t id = ui_create_element(UI_TEXT, "text");
  if (id == 0xFFFFFFFF)
    return id;

  UIElement *elem = &g_ui.elements[id];
  elem->rect = (UIRect){x, y, 100, 20};
  strncpy(elem->text, text, 255);
  elem->bg_color.a = 0; // Transparent background

  return id;
}

// DONE: Implement ui_slider
uint32_t ui_slider(const char *name, float x, float y, float w, float min_val,
                   float max_val, float initial) {
  uint32_t id = ui_create_element(UI_SLIDER, name);
  if (id == 0xFFFFFFFF)
    return id;

  UIElement *elem = &g_ui.elements[id];
  elem->rect = (UIRect){x, y, w, 20};
  elem->data.slider.min_value = min_val;
  elem->data.slider.max_value = max_val;
  elem->data.slider.value = initial;

  return id;
}

// DONE: Implement ui_checkbox
uint32_t ui_checkbox(const char *label, float x, float y, bool checked) {
  uint32_t id = ui_create_element(UI_CHECKBOX, label);
  if (id == 0xFFFFFFFF)
    return id;

  UIElement *elem = &g_ui.elements[id];
  elem->rect = (UIRect){x, y, 20, 20};
  strncpy(elem->text, label, 255);
  elem->data.checkbox.checked = checked;

  return id;
}

/* =================================================================================================
 *                                    INPUT HANDLING
 * =================================================================================================
 */

static bool ui_point_in_rect(float px, float py, UIRect rect) {
  return px >= rect.x && px < rect.x + rect.width && py >= rect.y &&
         py < rect.y + rect.height;
}

// DONE: Implement ui_update
void ui_update(void) {
  // Update hover states
  for (uint32_t i = 0; i < g_ui.element_count; i++) {
    UIElement *elem = &g_ui.elements[i];
    if (!elem->visible || !elem->enabled)
      continue;

    bool was_hovered = elem->hovered;
    elem->hovered = ui_point_in_rect(g_ui.mouse_x, g_ui.mouse_y, elem->rect);

    if (elem->hovered && !was_hovered && elem->on_hover) {
      elem->on_hover(elem, elem->user_data);
    }

    if (elem->hovered) {
      g_ui.hot_element = i;
    }
  }

  // Handle clicks
  if (g_ui.mouse_clicked && g_ui.hot_element != 0xFFFFFFFF) {
    UIElement *elem = &g_ui.elements[g_ui.hot_element];

    if (elem->type == UI_BUTTON && elem->on_click) {
      elem->on_click(elem, elem->user_data);
    }

    if (elem->type == UI_CHECKBOX) {
      elem->data.checkbox.checked = !elem->data.checkbox.checked;
      if (elem->on_click) {
        elem->on_click(elem, elem->user_data);
      }
    }

    g_ui.active_element = g_ui.hot_element;
  }

  // Update sliders
  if (g_ui.active_element != 0xFFFFFFFF) {
    UIElement *elem = &g_ui.elements[g_ui.active_element];

    if (elem->type == UI_SLIDER && g_ui.mouse_down) {
      float t = (g_ui.mouse_x - elem->rect.x) / elem->rect.width;
      t = t < 0 ? 0 : (t > 1 ? 1 : t);

      elem->data.slider.value =
          elem->data.slider.min_value +
          t * (elem->data.slider.max_value - elem->data.slider.min_value);
    }
  }
}

/* =================================================================================================
 *                                    LAYOUT
 * =================================================================================================
 */

// DONE: Implement ui_layout_vertical
void ui_layout_vertical(uint32_t parent_id, float spacing) {
  if (parent_id >= g_ui.element_count)
    return;

  UIElement *parent = &g_ui.elements[parent_id];
  float y = parent->rect.y + parent->padding.y;

  for (uint32_t i = 0; i < parent->child_count; i++) {
    UIElement *child = &g_ui.elements[parent->children[i]];
    child->rect.x = parent->rect.x + parent->padding.x;
    child->rect.y = y;
    y += child->rect.height + spacing;
  }
}

// DONE: Implement ui_layout_horizontal
void ui_layout_horizontal(uint32_t parent_id, float spacing) {
  if (parent_id >= g_ui.element_count)
    return;

  UIElement *parent = &g_ui.elements[parent_id];
  float x = parent->rect.x + parent->padding.x;

  for (uint32_t i = 0; i < parent->child_count; i++) {
    UIElement *child = &g_ui.elements[parent->children[i]];
    child->rect.x = x;
    child->rect.y = parent->rect.y + parent->padding.y;
    x += child->rect.width + spacing;
  }
}

// DONE: Implement ui_layout_grid
void ui_layout_grid(uint32_t parent_id, uint32_t columns, float spacing) {
  if (parent_id >= g_ui.element_count)
    return;

  UIElement *parent = &g_ui.elements[parent_id];
  float start_x = parent->rect.x + parent->padding.x;
  float start_y = parent->rect.y + parent->padding.y;

  for (uint32_t i = 0; i < parent->child_count; i++) {
    UIElement *child = &g_ui.elements[parent->children[i]];

    uint32_t col = i % columns;
    uint32_t row = i / columns;

    child->rect.x = start_x + col * (child->rect.width + spacing);
    child->rect.y = start_y + row * (child->rect.height + spacing);
  }
}

/* =================================================================================================
 *                                    RENDERING HELPERS
 * =================================================================================================
 */

// DONE: Implement ui_render_element
void ui_render_element(uint32_t id) {
  if (id >= g_ui.element_count)
    return;

  UIElement *elem = &g_ui.elements[id];
  if (!elem->visible)
    return;

  // Would call actual rendering API here
  // For now, just a placeholder showing the structure

  // Draw background
  if (elem->bg_color.a > 0) {
    // render_rect(elem->rect, elem->bg_color);
  }

  // Draw border
  if (elem->border_width > 0) {
    // render_rect_outline(elem->rect, elem->border_color, elem->border_width);
  }

  // Draw type-specific content
  switch (elem->type) {
  case UI_TEXT:
  case UI_BUTTON:
    // render_text(elem->text, elem->rect.x, elem->rect.y, elem->fg_color);
    break;

  case UI_SLIDER: {
    float t = (elem->data.slider.value - elem->data.slider.min_value) /
              (elem->data.slider.max_value - elem->data.slider.min_value);
    float handle_x = elem->rect.x + t * elem->rect.width;
    // render_rect({handle_x - 5, elem->rect.y, 10, elem->rect.height},
    // fg_color);
    break;
  }

  case UI_CHECKBOX:
    if (elem->data.checkbox.checked) {
      // render_checkmark(elem->rect, elem->fg_color);
    }
    break;

  default:
    break;
  }

  // Render children
  for (uint32_t i = 0; i < elem->child_count; i++) {
    ui_render_element(elem->children[i]);
  }
}

// DONE: Implement ui_render
void ui_render(void) {
  // Render all root elements (those without parents)
  for (uint32_t i = 0; i < g_ui.element_count; i++) {
    if (g_ui.elements[i].parent_id == 0xFFFFFFFF) {
      ui_render_element(i);
    }
  }
}

/* =================================================================================================
 *                                    PROPERTIES
 * =================================================================================================
 */

// DONE: Implement ui_set_position
void ui_set_position(uint32_t id, float x, float y) {
  if (id >= g_ui.element_count)
    return;
  g_ui.elements[id].rect.x = x;
  g_ui.elements[id].rect.y = y;
}

// DONE: Implement ui_set_size
void ui_set_size(uint32_t id, float w, float h) {
  if (id >= g_ui.element_count)
    return;
  g_ui.elements[id].rect.width = w;
  g_ui.elements[id].rect.height = h;
}

// DONE: Implement ui_set_color
void ui_set_color(uint32_t id, float r, float g, float b, float a) {
  if (id >= g_ui.element_count)
    return;
  g_ui.elements[id].bg_color = (UIColor){r, g, b, a};
}

// DONE: Implement ui_set_text
void ui_set_text(uint32_t id, const char *text) {
  if (id >= g_ui.element_count)
    return;
  strncpy(g_ui.elements[id].text, text, 255);
}

// DONE: Implement ui_set_visible
void ui_set_visible(uint32_t id, bool visible) {
  if (id >= g_ui.element_count)
    return;
  g_ui.elements[id].visible = visible;
}

// DONE: Implement ui_set_enabled
void ui_set_enabled(uint32_t id, bool enabled) {
  if (id >= g_ui.element_count)
    return;
  g_ui.elements[id].enabled = enabled;
}

// DONE: Implement ui_set_callback
void ui_set_callback(uint32_t id, void (*callback)(UIElement *, void *),
                     void *user_data) {
  if (id >= g_ui.element_count)
    return;
  g_ui.elements[id].on_click = callback;
  g_ui.elements[id].user_data = user_data;
}

// DONE: Implement ui_get_slider_value
float ui_get_slider_value(uint32_t id) {
  if (id >= g_ui.element_count)
    return 0;
  return g_ui.elements[id].data.slider.value;
}

// DONE: Implement ui_get_checkbox_checked
bool ui_get_checkbox_checked(uint32_t id) {
  if (id >= g_ui.element_count)
    return false;
  return g_ui.elements[id].data.checkbox.checked;
}

// DONE: Implement ui_is_hovered
bool ui_is_hovered(uint32_t id) {
  if (id >= g_ui.element_count)
    return false;
  return g_ui.elements[id].hovered;
}

// DONE: Implement ui_is_clicked
bool ui_is_clicked(uint32_t id) {
  if (id >= g_ui.element_count)
    return false;
  return g_ui.elements[id].hovered && g_ui.mouse_clicked;
}
