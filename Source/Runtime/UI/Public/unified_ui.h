#pragma once

#include <stdbool.h>
#include <stdint.h>

// Forward declarations
typedef struct UIContext UIContext;
typedef struct RenderContext RenderContext;

// UI types
typedef struct {
  float x, y;
} UIVec2;

typedef struct {
  float r, g, b, a;
} UIColor;

// UI configuration
typedef struct {
  uint32_t width;
  uint32_t height;
  const char *font_path;
  float font_size;
} UIConfig;

// UI system
UIContext *ui_create(RenderContext *renderer, const UIConfig *config);
void ui_destroy(UIContext *ui);

void ui_begin_frame(UIContext *ui);
void ui_end_frame(UIContext *ui);

// Widgets
bool ui_button(const char *label);
void ui_text(const char *text);
bool ui_checkbox(const char *label, bool *value);
bool ui_input_text(const char *label, char *buffer, size_t buffer_size);
bool ui_slider_float(const char *label, float *value, float min, float max);

// Layout
void ui_begin_window(const char *title, float x, float y, float width,
                     float height);
void ui_end_window(void);

void ui_same_line(void);
void ui_separator(void);
void ui_spacing(void);

// Utilities
void ui_set_color(UIColor color);
void ui_image(void *texture, float width, float height);
