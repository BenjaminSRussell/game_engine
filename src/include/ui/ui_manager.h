// include/engine/ui_manager.h
//
// Purpose: Complete UI framework
//
#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "engine/include/common.h"
#include <math/vec2.h>
#include <math/vec4.h>

// Forward declarations
typedef struct UIManager UIManager;
typedef struct UIElement UIElement;
typedef struct UIFont UIFont;
typedef struct UITexture UITexture;

// UI element types
typedef enum {
    UI_ELEMENT_BUTTON,
    UI_ELEMENT_LABEL,
    UI_ELEMENT_TEXT_INPUT,
    UI_ELEMENT_IMAGE,
    UI_ELEMENT_PANEL,
    UI_ELEMENT_SCROLL_VIEW,
    UI_ELEMENT_LIST_VIEW,
    UI_ELEMENT_SLIDER,
    UI_ELEMENT_PROGRESS_BAR,
    UI_ELEMENT_CHECKBOX,
    UI_ELEMENT_RADIO_BUTTON
} UIElementType;

// UI layout types
typedef enum {
    UI_LAYOUT_ABSOLUTE,
    UI_LAYOUT_RELATIVE,
    UI_LAYOUT_ANCHOR,
    UI_LAYOUT_GRID,
    UI_LAYOUT_FLEX,
    UI_LAYOUT_STACK
} UILayoutType;

// UI event types
typedef enum {
    UI_EVENT_CLICK,
    UI_EVENT_HOVER,
    UI_EVENT_FOCUS,
    UI_EVENT_BLUR,
    UI_EVENT_CHANGE,
    UI_EVENT_DRAG_START,
    UI_EVENT_DRAG_END
} UIEventType;

// UI event
typedef struct {
    UIEventType type;
    UIElement* element;
    Vec2 position;
    Vec2 delta;
    void* data;
} UIEvent;

// UI element descriptor
typedef struct {
    UIElementType type;
    const char* id;
    Vec2 position;
    Vec2 size;
    Vec4 color;
    Vec4 background_color;
    const char* text;
    UIFont* font;
    f32 font_size;
    bool visible;
    bool enabled;
    bool interactive;
    void* user_data;
} UIElementDesc;

// UI manager interface
typedef struct UIManager {
    // Lifecycle
    bool (*init)(UIManager* ui, u32 max_elements);
    void (*shutdown)(UIManager* ui);
    void (*update)(UIManager* ui, f32 delta_time);
    void (*render)(UIManager* ui);
    
    // Element management
    UIElement* (*create_element)(UIManager* ui, const UIElementDesc* desc);
    void (*destroy_element)(UIManager* ui, UIElement* element);
    UIElement* (*get_element)(UIManager* ui, const char* id);
    
    // Layout
    void (*set_layout)(UIManager* ui, UILayoutType layout);
    void (*set_root_element)(UIManager* ui, UIElement* root);
    
    // Event handling
    void (*add_event_listener)(UIManager* ui, UIElement* element, UIEventType type, void (*callback)(const UIEvent*, void*), void* user_data);
    void (*remove_event_listener)(UIManager* ui, UIElement* element, UIEventType type);
    
    // Input handling
    void (*handle_mouse_move)(UIManager* ui, Vec2 position);
    void (*handle_mouse_click)(UIManager* ui, Vec2 position, bool pressed);
    void (*handle_key)(UIManager* ui, u32 key, bool pressed);
    void (*handle_text_input)(UIManager* ui, const char* text);
    
    // Styling
    void (*set_theme)(UIManager* ui, const char* theme_name);
    void (*set_font)(UIManager* ui, const char* font_name, f32 size);
    
    // Utilities
    UIElement* (*get_element_at)(UIManager* ui, Vec2 position);
    void (*bring_to_front)(UIManager* ui, UIElement* element);
    void (*set_focus)(UIManager* ui, UIElement* element);
    UIElement* (*get_focused_element)(UIManager* ui);
    
    // Platform-specific data
    void* platform_data;
} UIManager;

// Factory functions
UIManager* create_ui_manager(void);

// Utility functions
UIElementDesc ui_create_button_desc(const char* id, Vec2 position, Vec2 size, const char* text);
UIElementDesc ui_create_label_desc(const char* id, Vec2 position, const char* text);
UIElementDesc ui_create_panel_desc(const char* id, Vec2 position, Vec2 size);

#endif // UI_MANAGER_H



