#include "core/logger.h"
#include "core/memory.h"
#include <ui/ui_system.h>
#include <stdlib.h>
#include <string.h>

// Widget data structures
typedef struct {
  char *text;
  f32 font_size;
  // Color color;
} UILabelData;

typedef struct {
  bool is_toggled;
} UIButtonData;

UIElement *ui_create_element(UIElementType type, const char *id) {
  UIElement *el = calloc(1, sizeof(UIElement));
  el->id = strdup(id);
  el->type = type;
  el->is_visible = true;
  el->children = NULL;
  el->child_count = 0;

  // Default style
  el->style.type = UI_LAYOUT_FLEX_COLUMN;
  el->style.flex_grow = 0;

  // Init widget specific data
  switch (type) {
  case UI_ELEMENT_LABEL:
    el->widget_data = calloc(1, sizeof(UILabelData));
    break;
  case UI_ELEMENT_BUTTON:
    el->widget_data = calloc(1, sizeof(UIButtonData));
    break;
  default:
    break;
  }

  return el;
}

void ui_destroy_element(UIElement *element) {
  if (!element)
    return;

  // Destroy children first
  for (u32 i = 0; i < element->child_count; i++) {
    ui_destroy_element(element->children[i]);
  }

  if (element->children)
    free(element->children);
  if (element->widget_data)
    free(element->widget_data);
  // free((void*)element->id); // const cast

  free(element);
}

void ui_add_child(UIElement *parent, UIElement *child) {
  if (!parent || !child)
    return;

  parent->child_count++;
  parent->children =
      realloc(parent->children, sizeof(UIElement *) * parent->child_count);
  parent->children[parent->child_count - 1] = child;
  child->parent = parent;
}
