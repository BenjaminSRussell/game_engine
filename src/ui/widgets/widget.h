/*
 * widget.h
 * Base Widget Class with Event Handling
 * Foundation for all UI widgets with comprehensive event system
 *
 * Part of the UI subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef UI_WIDGET_H
#define UI_WIDGET_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "include/math/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct Widget Widget;
typedef struct UIEvent UIEvent;
typedef struct UIEventHandler UIEventHandler;

// Box edges for layout
typedef struct {
    float top;
    float right;
    float bottom;
    float left;
} BoxEdges;

// Event types
typedef enum {
    UI_EVENT_NONE,
    UI_EVENT_MOUSE_ENTER,
    UI_EVENT_MOUSE_LEAVE,
    UI_EVENT_MOUSE_DOWN,
    UI_EVENT_MOUSE_UP,
    UI_EVENT_MOUSE_MOVE,
    UI_EVENT_MOUSE_CLICK,
    UI_EVENT_MOUSE_DOUBLE_CLICK,
    UI_EVENT_MOUSE_WHEEL,
    UI_EVENT_KEY_DOWN,
    UI_EVENT_KEY_UP,
    UI_EVENT_KEY_PRESS,
    UI_EVENT_FOCUS_GAIN,
    UI_EVENT_FOCUS_LOST,
    UI_EVENT_TOUCH_DOWN,
    UI_EVENT_TOUCH_UP,
    UI_EVENT_TOUCH_MOVE,
    UI_EVENT_RESIZE,
    UI_EVENT_SCROLL,
    UI_EVENT_HOVER,
    UI_EVENT_DRAG_START,
    UI_EVENT_DRAG_MOVE,
    UI_EVENT_DRAG_END,
    UI_EVENT_DROP,
    UI_EVENT_CUSTOM
} UIEventType;

// Event modifiers
typedef enum {
    UI_MODIFIER_NONE = 0,
    UI_MODIFIER_SHIFT = 1 << 0,
    UI_MODIFIER_CTRL = 1 << 1,
    UI_MODIFIER_ALT = 1 << 2,
    UI_MODIFIER_META = 1 << 3,
    UI_MODIFIER_CAPS_LOCK = 1 << 4,
    UI_MODIFIER_NUM_LOCK = 1 << 5
} UIModifiers;

// Mouse buttons
typedef enum {
    UI_MOUSE_BUTTON_NONE = 0,
    UI_MOUSE_BUTTON_LEFT = 1,
    UI_MOUSE_BUTTON_RIGHT = 2,
    UI_MOUSE_BUTTON_MIDDLE = 3,
    UI_MOUSE_BUTTON_X1 = 4,
    UI_MOUSE_BUTTON_X2 = 5
} UIMouseButton;

// Widget states
typedef enum {
    WIDGET_STATE_NORMAL,
    WIDGET_STATE_HOVER,
    WIDGET_STATE_PRESSED,
    WIDGET_STATE_FOCUSED,
    WIDGET_STATE_DISABLED,
    WIDGET_STATE_HIDDEN,
    WIDGET_STATE_SELECTED,
    WIDGET_STATE_DRAGGING
} WidgetState;

// Focus navigation
typedef enum {
    UI_FOCUS_NAV_NONE,
    UI_FOCUS_NAV_TAB,
    UI_FOCUS_NAV_ARROWS,
    UI_FOCUS_NAV_CYCLE
} UIFocusNavigation;

// Event handler function type
typedef bool (*UIEventCallback)(Widget* widget, UIEvent* event, void* user_data);

// Event data
struct UIEvent {
    UIEventType type;
    uint64_t timestamp;
    UIModifiers modifiers;
    
    union {
        struct {
            Vec2 position;
            Vec2 global_position;
            UIMouseButton button;
            float wheel_delta;
            int32_t click_count;
        } mouse;
        
        struct {
            uint32_t key_code;
            uint32_t scan_code;
            bool repeat;
            char character;
        } keyboard;
        
        struct {
            Vec2 position;
            Vec2 global_position;
            uint32_t touch_id;
            float pressure;
        } touch;
        
        struct {
            Vec2 old_size;
            Vec2 new_size;
        } resize;
        
        struct {
            Vec2 delta;
            Vec2 position;
        } scroll;
        
        struct {
            Vec2 position;
            Vec2 offset;
            Widget* source;
        } drag;
        
        struct {
            void* data;
            uint32_t data_size;
            uint32_t custom_type;
        } custom;
    };
    
    bool handled;
    bool prevent_default;
    bool stop_propagation;
};

// Event handler
struct UIEventHandler {
    UIEventType event_type;
    UIEventCallback callback;
    void* user_data;
    bool active;
    UIEventHandler* next;
};

// Widget base class
struct Widget {
    uint32_t id;
    char* name;
    
    // Hierarchy
    Widget* parent;
    Widget** children;
    uint32_t child_count;
    uint32_t child_capacity;
    
    // Layout
    Vec2 position;
    Vec2 size;
    Vec2 min_size;
    Vec2 max_size;
    Vec2 preferred_size;
    
    // Visual properties
    Vec4 background_color;
    Vec4 border_color;
    Vec4 text_color;
    float border_width;
    float corner_radius;
    float opacity;
    
    // State
    WidgetState state;
    bool visible;
    bool enabled;
    bool focused;
    bool hoverable;
    bool focusable;
    bool draggable;
    
    // Layout properties
    BoxEdges margin;
    BoxEdges padding;
    BoxEdges border;
    
    // Event handling
    UIEventHandler* event_handlers;
    uint32_t handler_count;
    
    // Focus navigation
    UIFocusNavigation focus_navigation;
    Widget* focus_prev;
    Widget* focus_next;
    
    // User data
    void* user_data;
    
    // Internal state
    bool dirty;
    bool needs_layout;
    bool needs_redraw;
    uint32_t z_index;
    
    // Virtual function pointers
    void (*destroy)(Widget* widget);
    void (*render)(Widget* widget);
    void (*layout)(Widget* widget, float available_width, float available_height);
    Size (*measure)(Widget* widget, float available_width, float available_height);
    bool (*handle_event)(Widget* widget, UIEvent* event);
};

/* ============================================================================
 * API
 * ============================================================================ */

/* Widget Management */
Widget* widget_create(const char* name);
void widget_destroy(Widget* widget);
void widget_add_child(Widget* parent, Widget* child);
void widget_remove_child(Widget* parent, Widget* child);
void widget_remove_from_parent(Widget* widget);

/* Hierarchy */
Widget* widget_get_parent(const Widget* widget);
Widget* widget_get_child(const Widget* widget, uint32_t index);
uint32_t widget_get_child_count(const Widget* widget);
Widget* widget_find_child(const Widget* widget, const char* name);
Widget* widget_find_by_id(const Widget* widget, uint32_t id);

/* Layout */
void widget_set_position(Widget* widget, Vec2 position);
void widget_set_size(Widget* widget, Vec2 size);
void widget_set_min_size(Widget* widget, Vec2 min_size);
void widget_set_max_size(Widget* widget, Vec2 max_size);
void widget_set_preferred_size(Widget* widget, Vec2 preferred_size);
void widget_set_margins(Widget* widget, BoxEdges margins);
void widget_set_padding(Widget* widget, BoxEdges padding);
void widget_set_border(Widget* widget, BoxEdges border, float width);

Vec2 widget_get_position(const Widget* widget);
Vec2 widget_get_size(const Widget* widget);
Vec2 widget_get_content_position(const Widget* widget);
Vec2 widget_get_content_size(const Widget* widget);
Rect widget_get_bounds(const Widget* widget);
Rect widget_get_content_bounds(const Widget* widget);

/* Visual Properties */
void widget_set_background_color(Widget* widget, Vec4 color);
void widget_set_border_color(Widget* widget, Vec4 color);
void widget_set_text_color(Widget* widget, Vec4 color);
void widget_set_opacity(Widget* widget, float opacity);
void widget_set_border_width(Widget* widget, float width);
void widget_set_corner_radius(Widget* widget, float radius);

/* State Management */
void widget_set_visible(Widget* widget, bool visible);
void widget_set_enabled(Widget* widget, bool enabled);
void widget_set_focused(Widget* widget, bool focused);
void widget_set_state(Widget* widget, WidgetState state);
bool widget_is_visible(const Widget* widget);
bool widget_is_enabled(const Widget* widget);
bool widget_is_focused(const Widget* widget);
bool widget_is_hover(const Widget* widget);
WidgetState widget_get_state(const Widget* widget);

/* Event Handling */
void widget_add_event_handler(Widget* widget, UIEventType event_type, UIEventCallback callback, void* user_data);
void widget_remove_event_handler(Widget* widget, UIEventHandler* handler);
void widget_remove_all_event_handlers(Widget* widget, UIEventType event_type);
bool widget_handle_event(Widget* widget, UIEvent* event);
bool widget_emit_event(Widget* widget, UIEvent* event);

/* Focus Management */
bool widget_can_focus(const Widget* widget);
bool widget_request_focus(Widget* widget);
void widget_release_focus(Widget* widget);
void widget_set_focusable(Widget* widget, bool focusable);
void widget_set_focus_navigation(Widget* widget, UIFocusNavigation navigation);
void widget_set_focus_chain(Widget* widget, Widget* next, Widget* prev);
Widget* widget_get_next_focusable(const Widget* widget);
Widget* widget_get_prev_focusable(const Widget* widget);

/* Hit Testing */
bool widget_contains_point(const Widget* widget, Vec2 point);
Widget* widget_hit_test(const Widget* widget, Vec2 point);
Widget* widget_get_widget_at_point(const Widget* widget, Vec2 point);

/* Coordinate Conversion */
Vec2 widget_local_to_global(const Widget* widget, Vec2 local_point);
Vec2 widget_global_to_local(const Widget* widget, Vec2 global_point);

/* Layout System */
void widget_invalidate_layout(Widget* widget);
void widget_invalidate_redraw(Widget* widget);
bool widget_needs_layout(const Widget* widget);
bool widget_needs_redraw(const Widget* widget);
void widget_layout(Widget* widget, float available_width, float available_height);

/* Event Creation */
UIEvent* ui_event_create(UIEventType type);
void ui_event_destroy(UIEvent* event);
UIEvent* ui_event_create_mouse(UIEventType type, Vec2 position, UIMouseButton button);
UIEvent* ui_event_create_keyboard(UIEventType type, uint32_t key_code, uint32_t scan_code);
UIEvent* ui_event_create_touch(UIEventType type, Vec2 position, uint32_t touch_id);

/* Utility Functions */
uint32_t widget_generate_id(void);
const char* widget_type_name(const Widget* widget);
void widget_print_hierarchy(const Widget* widget, int depth);

/* Event Utilities */
bool ui_event_is_mouse_event(const UIEvent* event);
bool ui_event_is_keyboard_event(const UIEvent* event);
bool ui_event_is_touch_event(const UIEvent* event);
bool ui_event_has_modifier(const UIEvent* event, UIModifiers modifier);
Vec2 ui_event_get_position(const UIEvent* event);

#ifdef __cplusplus
}
#endif

#endif /* UI_WIDGET_H */
