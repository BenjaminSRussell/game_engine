/**
 * @file ui_api.h
 * @brief UI subsystem API
 */

#ifndef VOXELFORGE_UI_API_H
#define VOXELFORGE_UI_API_H

#include "Core/Public/Math/Vector/vec2.h"
#include "Core/Public/Math/Vector/vec4.h"
#include "Core/Public/core_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Forward Declarations
// ============================================================================

typedef struct UIContext UIContext;
typedef struct UIElement UIElement;
typedef struct UIStyle UIStyle;
typedef struct UIFont UIFont;

// ============================================================================
// Element Types
// ============================================================================

typedef enum UIElementType {
  UI_ELEMENT_PANEL = 0,
  UI_ELEMENT_LABEL,
  UI_ELEMENT_BUTTON,
  UI_ELEMENT_IMAGE,
  UI_ELEMENT_TEXTFIELD,
  UI_ELEMENT_CHECKBOX,
  UI_ELEMENT_SLIDER,
  UI_ELEMENT_SCROLLVIEW,
  UI_ELEMENT_LIST,
  UI_ELEMENT_DROPDOWN,
  UI_ELEMENT_PROGRESSBAR,
} UIElementType;

// ============================================================================
// Layout
// ============================================================================

typedef enum UILayoutType {
  UI_LAYOUT_NONE = 0, // Manual positioning
  UI_LAYOUT_HORIZONTAL,
  UI_LAYOUT_VERTICAL,
  UI_LAYOUT_GRID,
} UILayoutType;

typedef enum UIAnchor {
  UI_ANCHOR_TOP_LEFT = 0,
  UI_ANCHOR_TOP_CENTER,
  UI_ANCHOR_TOP_RIGHT,
  UI_ANCHOR_CENTER_LEFT,
  UI_ANCHOR_CENTER,
  UI_ANCHOR_CENTER_RIGHT,
  UI_ANCHOR_BOTTOM_LEFT,
  UI_ANCHOR_BOTTOM_CENTER,
  UI_ANCHOR_BOTTOM_RIGHT,
  UI_ANCHOR_STRETCH,
} UIAnchor;

typedef struct UIRect {
  f32 x, y, width, height;
} UIRect;

typedef struct UIPadding {
  f32 left, right, top, bottom;
} UIPadding;

// ============================================================================
// Styling
// ============================================================================

typedef struct UIColor {
  f32 r, g, b, a;
} UIColor;

#define UI_COLOR_WHITE ((UIColor){1, 1, 1, 1})
#define UI_COLOR_BLACK ((UIColor){0, 0, 0, 1})
#define UI_COLOR_RED ((UIColor){1, 0, 0, 1})
#define UI_COLOR_GREEN ((UIColor){0, 1, 0, 1})
#define UI_COLOR_BLUE ((UIColor){0, 0, 1, 1})
#define UI_COLOR_TRANSPARENT ((UIColor){0, 0, 0, 0})

typedef struct UIStyle {
  UIColor background_color;
  UIColor border_color;
  UIColor text_color;
  f32 border_width;
  f32 border_radius;
  UIPadding padding;
  UIFont *font;
  f32 font_size;
} UIStyle;

// ============================================================================
// Context
// ============================================================================

VF_API UIContext *ui_context_create(void);
VF_API void ui_context_destroy(UIContext *ctx);
VF_API void ui_context_set_viewport(UIContext *ctx, f32 width, f32 height);
VF_API void ui_context_update(UIContext *ctx, f32 delta_time);
VF_API void ui_context_render(UIContext *ctx);
VF_API void ui_context_set_scale(UIContext *ctx, f32 scale);

// ============================================================================
// Element Creation
// ============================================================================

VF_API UIElement *ui_panel_create(UIContext *ctx, UIElement *parent);
VF_API UIElement *ui_label_create(UIContext *ctx, UIElement *parent,
                                  const char *text);
VF_API UIElement *ui_button_create(UIContext *ctx, UIElement *parent,
                                   const char *text);
VF_API UIElement *ui_image_create(UIContext *ctx, UIElement *parent,
                                  ResourceHandle texture);
VF_API UIElement *ui_textfield_create(UIContext *ctx, UIElement *parent,
                                      const char *placeholder);
VF_API UIElement *ui_checkbox_create(UIContext *ctx, UIElement *parent,
                                     const char *label);
VF_API UIElement *ui_slider_create(UIContext *ctx, UIElement *parent, f32 min,
                                   f32 max, f32 value);
VF_API UIElement *ui_scrollview_create(UIContext *ctx, UIElement *parent);
VF_API UIElement *ui_progressbar_create(UIContext *ctx, UIElement *parent,
                                        f32 value);
VF_API void ui_element_destroy(UIElement *element);

// ============================================================================
// Element Properties
// ============================================================================

VF_API void ui_element_set_rect(UIElement *element, UIRect rect);
VF_API UIRect ui_element_get_rect(UIElement *element);
VF_API void ui_element_set_anchor(UIElement *element, UIAnchor anchor);
VF_API void ui_element_set_layout(UIElement *element, UILayoutType layout);
VF_API void ui_element_set_visible(UIElement *element, b8 visible);
VF_API b8 ui_element_is_visible(UIElement *element);
VF_API void ui_element_set_enabled(UIElement *element, b8 enabled);
VF_API b8 ui_element_is_enabled(UIElement *element);
VF_API void ui_element_set_style(UIElement *element, const UIStyle *style);
VF_API void ui_element_set_user_data(UIElement *element, void *data);
VF_API void *ui_element_get_user_data(UIElement *element);

// ============================================================================
// Specific Element Controls
// ============================================================================

VF_API void ui_label_set_text(UIElement *element, const char *text);
VF_API const char *ui_label_get_text(UIElement *element);
VF_API void ui_button_set_text(UIElement *element, const char *text);
VF_API void ui_textfield_set_text(UIElement *element, const char *text);
VF_API const char *ui_textfield_get_text(UIElement *element);
VF_API void ui_checkbox_set_checked(UIElement *element, b8 checked);
VF_API b8 ui_checkbox_is_checked(UIElement *element);
VF_API void ui_slider_set_value(UIElement *element, f32 value);
VF_API f32 ui_slider_get_value(UIElement *element);
VF_API void ui_progressbar_set_value(UIElement *element, f32 value);
VF_API void ui_image_set_texture(UIElement *element, ResourceHandle texture);

// ============================================================================
// Events
// ============================================================================

typedef enum UIEventType {
  UI_EVENT_CLICK = 0,
  UI_EVENT_HOVER_ENTER,
  UI_EVENT_HOVER_EXIT,
  UI_EVENT_VALUE_CHANGED,
  UI_EVENT_SUBMIT,
  UI_EVENT_FOCUS,
  UI_EVENT_BLUR,
} UIEventType;

typedef void (*UIEventCallback)(UIElement *element, UIEventType event,
                                void *user_data);

VF_API void ui_element_on_event(UIElement *element, UIEventType event,
                                UIEventCallback callback, void *user_data);
VF_API b8 ui_element_was_clicked(UIElement *element);
VF_API b8 ui_element_is_hovered(UIElement *element);
VF_API b8 ui_element_is_focused(UIElement *element);

// ============================================================================
// Fonts
// ============================================================================

VF_API UIFont *ui_font_load(const char *path, f32 size);
VF_API void ui_font_destroy(UIFont *font);
VF_API Vec2 ui_font_measure_text(UIFont *font, const char *text);

// ============================================================================
// System
// ============================================================================

VF_API VF_Result ui_init(void);
VF_API void ui_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif // VOXELFORGE_UI_API_H
