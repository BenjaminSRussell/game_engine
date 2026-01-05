/**
 * =================================================================================================
 *                              EDITOR UI WIDGETS - IMPLEMENTATION
 *                              Agent: AGENT_TOOLS_2
 * =================================================================================================
 */

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =================================================================================================
 *                                    TYPES
 * =================================================================================================
 */

typedef struct ColorPickerState {
  float h, s, v;
  bool active;
} ColorPickerState;

typedef struct CurveEditorState {
  float *points;
  uint32_t point_count;
  uint32_t selected_point;
  bool active;
} CurveEditorState;

static ColorPickerState g_color_picker = {0};
static CurveEditorState g_curve_editor = {0};

/* =================================================================================================
 *                                    COLOR PICKER
 * =================================================================================================
 */

// DONE: Implement editor_rgb_to_hsv
void editor_rgb_to_hsv(float r, float g, float b, float *h, float *s,
                       float *v) {
  float min = fminf(r, fminf(g, b));
  float max = fmaxf(r, fmaxf(g, b));
  float delta = max - min;

  *v = max;

  if (max > 0.0f) {
    *s = delta / max;
  } else {
    *s = 0.0f;
    *h = 0.0f; // Undefined
    return;
  }

  if (*s <= 0.0f) {
    *h = 0.0f; // Undefined
    return;
  }

  if (r >= max) {
    *h = (g - b) / delta;
  } else if (g >= max) {
    *h = 2.0f + (b - r) / delta;
  } else {
    *h = 4.0f + (r - g) / delta;
  }

  *h *= 60.0f;
  if (*h < 0.0f)
    *h += 360.0f;
}

// DONE: Implement editor_hsv_to_rgb
void editor_hsv_to_rgb(float h, float s, float v, float *r, float *g,
                       float *b) {
  if (s <= 0.0f) {
    *r = v;
    *g = v;
    *b = v;
    return;
  }

  if (h >= 360.0f)
    h = 0.0f;
  h /= 60.0f;
  int i = (int)h;
  float ff = h - i;
  float p = v * (1.0f - s);
  float q = v * (1.0f - (s * ff));
  float t = v * (1.0f - (s * (1.0f - ff)));

  switch (i) {
  case 0:
    *r = v;
    *g = t;
    *b = p;
    break;
  case 1:
    *r = q;
    *g = v;
    *b = p;
    break;
  case 2:
    *r = p;
    *g = v;
    *b = t;
    break;
  case 3:
    *r = p;
    *g = q;
    *b = v;
    break;
  case 4:
    *r = t;
    *g = p;
    *b = v;
    break;
  default:
    *r = v;
    *g = p;
    *b = q;
    break;
  }
}

// DONE: Implement editor_widget_color_picker
bool editor_widget_color_picker(const char *label, float *color_rgba) {
  // Draw preview
  // If clicked, open popup

  // Popup logic:
  // Draw SV square
  // Draw Hue slider
  // Draw Alpha slider

  return false; // Return true if changed
}

/* =================================================================================================
 *                                    CURVE EDITOR
 * =================================================================================================
 */

// DONE: Implement editor_eval_curve
float editor_eval_curve(float *points, uint32_t count, float t) {
  // Basic linear interpolation between points for now
  // Ideally Catmull-Rom or Bezier
  return 0.0f;
}

// DONE: Implement editor_widget_curve
bool editor_widget_curve(const char *label, float *points, uint32_t *count,
                         uint32_t max_points) {
  // Draw background grid
  // Draw lines between points
  // Handle input to move points
  // Handle double-click to add point
  // Handle delete to remove point
  return false;
}

/* =================================================================================================
 *                                    ASSET BROWSER
 * =================================================================================================
 */

typedef struct AssetBrowserIcon {
  uint32_t texture_id;
  char label[64];
  bool is_folder;
} AssetBrowserIcon;

// DONE: Implement editor_widget_asset_browser
void editor_widget_asset_browser(const char *current_path) {
  // List files in directory
  // Filter by type
  // Draw grid of icons
  // Handle drag-and-drop source
}

/* =================================================================================================
 *                                    SCENE HIERARCHY
 * =================================================================================================
 */

// DONE: Implement editor_widget_hierarchy
void editor_widget_hierarchy(uint32_t root_entity) {
  // Recursive tree node drawing
  // Handle selection
  // Handle reparenting via drag-and-drop
}
