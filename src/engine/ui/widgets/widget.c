/*
 * widget.c
 * Base Widget Class with Event Handling Implementation
 * Foundation for all UI widgets with comprehensive event system
 *
 * Part of the UI subsystem
 * Advanced 3D Rendering Engine
 */

#include "widget.h"
#include "core/logger.h"
#include "core/memory.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

// Global widget ID counter
static uint32_t g_widget_id_counter = 1;

/* ============================================================================
 * INTERNAL FUNCTIONS
 * ============================================================================ */

static void widget_add_handler_internal(Widget* widget, UIEventHandler* handler) {
    if (!widget || !handler) return;
    
    handler->next = widget->event_handlers;
    widget->event_handlers = handler;
    widget->handler_count++;
}

static void widget_remove_handler_internal(Widget* widget, UIEventHandler* handler) {
    if (!widget || !handler) return;
    
    if (widget->event_handlers == handler) {
        widget->event_handlers = handler->next;
    } else {
        UIEventHandler* current = widget->event_handlers;
        while (current && current->next != handler) {
            current = current->next;
        }
        if (current) {
            current->next = handler->next;
        }
    }
    
    widget->handler_count--;
}

static bool widget_propagate_event(Widget* widget, UIEvent* event) {
    if (!widget || !event || event->stop_propagation) return false;
    
    // Handle event at current widget
    bool handled = false;
    if (widget->handle_event) {
        handled = widget->handle_event(widget, event);
    }
    
    // Call event handlers
    if (!handled || !event->stop_propagation) {
        UIEventHandler* handler = widget->event_handlers;
        while (handler && !event->stop_propagation) {
            if (handler->active && handler->callback) {
                bool handler_result = handler->callback(widget, event, handler->user_data);
                if (handler_result) {
                    handled = true;
                    event->handled = true;
                }
            }
            handler = handler->next;
        }
    }
    
    // Propagate to children (for certain event types)
    if (!event->stop_propagation && !handled) {
        switch (event->type) {
            case UI_EVENT_MOUSE_DOWN:
            case UI_EVENT_MOUSE_UP:
            case UI_EVENT_MOUSE_MOVE:
            case UI_EVENT_MOUSE_CLICK:
            case UI_EVENT_MOUSE_WHEEL:
            case UI_EVENT_KEY_DOWN:
            case UI_EVENT_KEY_UP:
            case UI_EVENT_KEY_PRESS:
                // These events don't automatically propagate to children
                break;
                
            default:
                // Other events can propagate to children
                for (uint32_t i = 0; i < widget->child_count; i++) {
                    Widget* child = widget->children[i];
                    if (child->visible && widget_contains_point(child, event->mouse.position)) {
                        if (widget_propagate_event(child, event)) {
                            handled = true;
                            break;
                        }
                    }
                }
                break;
        }
    }
    
    return handled;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

Widget* widget_create(const char* name) {
    Widget* widget = memory_alloc(sizeof(Widget));
    if (!widget) {
        LOG_ERROR(LOG_CAT_GENERAL, "Failed to allocate widget");
        return NULL;
    }
    
    memset(widget, 0, sizeof(Widget));
    
    widget->id = g_widget_id_counter++;
    widget->name = name ? strdup(name) : strdup("Widget");
    widget->text = NULL;
    widget->visible = true;
    widget->enabled = true;
    widget->hoverable = true;
    widget->focusable = false;
    widget->draggable = false;
    
    // Default visual properties
    widget->background_color = (Vec4){0.0f, 0.0f, 0.0f, 0.0f};
    widget->border_color = (Vec4){0.0f, 0.0f, 0.0f, 1.0f};
    widget->text_color = (Vec4){1.0f, 1.0f, 1.0f, 1.0f};
    widget->opacity = 1.0f;
    
    // Default state
    widget->state = WIDGET_STATE_NORMAL;
    widget->focus_navigation = UI_FOCUS_NAV_TAB;
    
    widget->dirty = true;
    widget->needs_layout = true;
    widget->needs_redraw = true;
    
    LOG_INFO(LOG_CAT_GENERAL, "Created widget: %s (ID: %u)", name ? name : "unnamed", widget->id);
    return widget;
}

void widget_destroy(Widget* widget) {
    if (!widget) return;
    
    // Call custom destroy function if set
    if (widget->destroy) {
        widget->destroy(widget);
    }
    
    // Destroy all children
    for (uint32_t i = 0; i < widget->child_count; i++) {
        widget_destroy(widget->children[i]);
    }
    
    // Clean up event handlers
    widget_remove_all_event_handlers(widget, UI_EVENT_NONE);
    
    // Clean up resources
    if (widget->children) {
        memory_free(widget->children);
    }
    
    if (widget->name) {
        free(widget->name);
    }
    if (widget->text) {
        free(widget->text);
    }
    
    memory_free(widget);
}

void widget_add_child(Widget* parent, Widget* child) {
    if (!parent || !child) return;
    
    // Remove child from existing parent if any
    widget_remove_from_parent(child);
    
    // Ensure capacity
    if (parent->child_count >= parent->child_capacity) {
        uint32_t new_capacity = parent->child_capacity == 0 ? 8 : 
                               parent->child_capacity * 2;
        Widget** new_children = memory_realloc(parent->children, 
                                             new_capacity * sizeof(Widget*));
        if (!new_children) {
            LOG_ERROR(LOG_CAT_GENERAL, "Failed to resize children array");
            return;
        }
        
        parent->children = new_children;
        parent->child_capacity = new_capacity;
    }
    
    // Add child
    child->parent = parent;
    parent->children[parent->child_count] = child;
    parent->child_count++;
    
    // Invalidate layout
    widget_invalidate_layout(parent);
    child->dirty = true;
    
    LOG_INFO(LOG_CAT_GENERAL, "Added child %s to parent %s", child->name, parent->name);
}

void widget_remove_child(Widget* parent, Widget* child) {
    if (!parent || !child) return;
    
    // Find child index
    uint32_t child_index = UINT32_MAX;
    for (uint32_t i = 0; i < parent->child_count; i++) {
        if (parent->children[i] == child) {
            child_index = i;
            break;
        }
    }
    
    if (child_index == UINT32_MAX) return;
    
    // Remove child
    child->parent = NULL;
    
    // Shift remaining children
    for (uint32_t i = child_index; i < parent->child_count - 1; i++) {
        parent->children[i] = parent->children[i + 1];
    }
    
    parent->child_count--;
    
    // Invalidate layout
    widget_invalidate_layout(parent);
    
    LOG_INFO(LOG_CAT_GENERAL, "Removed child %s from parent %s", child->name, parent->name);
}

void widget_remove_from_parent(Widget* widget) {
    if (!widget || !widget->parent) return;
    
    widget_remove_child(widget->parent, widget);
}

void widget_set_position(Widget* widget, Vec2 position) {
    if (!widget) return;
    
    if (widget->position.x != position.x || widget->position.y != position.y) {
        widget->position = position;
        widget->dirty = true;
        widget_invalidate_layout(widget);
    }
}

void widget_set_size(Widget* widget, Vec2 size) {
    if (!widget) return;
    
    // Apply constraints
    size.x = fmaxf(widget->min_size.x, fminf(widget->max_size.x, size.x));
    size.y = fmaxf(widget->min_size.y, fminf(widget->max_size.y, size.y));
    
    if (widget->size.x != size.x || widget->size.y != size.y) {
        widget->size = size;
        widget->dirty = true;
        widget_invalidate_layout(widget);
    }
}

void widget_set_text(Widget* widget, const char* text) {
    if (!widget) return;

    if (widget->text) {
        free(widget->text);
        widget->text = NULL;
    }

    if (text) {
        widget->text = strdup(text);
    }

    widget->dirty = true;
    widget->needs_redraw = true;
}

const char* widget_get_text(const Widget* widget) {
    if (!widget) return NULL;
    return widget->text;
}

void widget_set_visible(Widget* widget, bool visible) {
    if (!widget) return;
    
    if (widget->visible != visible) {
        widget->visible = visible;
        
        if (!visible) {
            // Lose focus when hidden
            if (widget->focused) {
                widget_release_focus(widget);
            }
        }
        
        widget->dirty = true;
        widget_invalidate_layout(widget);
    }
}

void widget_set_enabled(Widget* widget, bool enabled) {
    if (!widget) return;
    
    if (widget->enabled != enabled) {
        widget->enabled = enabled;
        
        if (!enabled) {
            // Lose focus when disabled
            if (widget->focused) {
                widget_release_focus(widget);
            }
            widget->state = WIDGET_STATE_DISABLED;
        } else {
            widget->state = WIDGET_STATE_NORMAL;
        }
        
        widget->dirty = true;
    }
}

void widget_set_focused(Widget* widget, bool focused) {
    if (!widget || !widget->focusable) return;
    
    if (widget->focused != focused) {
        widget->focused = focused;
        
        if (focused) {
            widget->state = WIDGET_STATE_FOCUSED;
            
            // Emit focus gain event
            UIEvent* event = ui_event_create(UI_EVENT_FOCUS_GAIN);
            if (event) {
                widget_emit_event(widget, event);
                ui_event_destroy(event);
            }
        } else {
            widget->state = WIDGET_STATE_NORMAL;
            
            // Emit focus loss event
            UIEvent* event = ui_event_create(UI_EVENT_FOCUS_LOST);
            if (event) {
                widget_emit_event(widget, event);
                ui_event_destroy(event);
            }
        }
        
        widget->dirty = true;
    }
}

void widget_add_event_handler(Widget* widget, UIEventType event_type, UIEventCallback callback, void* user_data) {
    if (!widget || !callback) return;
    
    UIEventHandler* handler = memory_alloc(sizeof(UIEventHandler));
    if (!handler) {
        LOG_ERROR(LOG_CAT_GENERAL, "Failed to allocate event handler");
        return;
    }
    
    handler->event_type = event_type;
    handler->callback = callback;
    handler->user_data = user_data;
    handler->active = true;
    handler->next = NULL;
    
    widget_add_handler_internal(widget, handler);
    
    LOG_DEBUG(LOG_CAT_GENERAL, "Added event handler for type %d to widget %s", event_type, widget->name);
}

void widget_remove_all_event_handlers(Widget* widget, UIEventType event_type) {
    if (!widget || !widget->event_handlers) return;

    UIEventHandler* current = widget->event_handlers;
    UIEventHandler* prev = NULL;

    while (current) {
        bool remove = (event_type == UI_EVENT_NONE || current->event_type == event_type);

        if (remove) {
            UIEventHandler* next = current->next;

            if (prev) {
                prev->next = next;
            } else {
                widget->event_handlers = next;
            }

            memory_free(current);
            widget->handler_count--;

            current = next;
        } else {
            prev = current;
            current = current->next;
        }
    }
}

bool widget_handle_event(Widget* widget, UIEvent* event) {
    if (!widget || !event || !widget->enabled || !widget->visible) return false;
    
    // Convert to widget-local coordinates
    Vec2 local_point = widget_global_to_local(widget, event->mouse.position);
    event->mouse.position = local_point;
    
    return widget_propagate_event(widget, event);
}

bool widget_emit_event(Widget* widget, UIEvent* event) {
    if (!widget || !event) return false;
    
    event->timestamp = (uint64_t)time(NULL) * 1000;
    
    return widget_propagate_event(widget, event);
}

bool widget_contains_point(const Widget* widget, Vec2 point) {
    if (!widget || !widget->visible) return false;
    
    return point.x >= 0.0f && point.x <= widget->size.x &&
           point.y >= 0.0f && point.y <= widget->size.y;
}

Widget* widget_hit_test(const Widget* widget, Vec2 point) {
    if (!widget || !widget_contains_point(widget, point)) return NULL;
    
    // Check children first (reverse order for proper z-order)
    for (int32_t i = (int32_t)widget->child_count - 1; i >= 0; i--) {
        Widget* child = widget->children[i];
        if (child->visible) {
            Vec2 child_point = widget_global_to_local(child, point);
            Widget* hit_child = widget_hit_test(child, child_point);
            if (hit_child) return hit_child;
        }
    }
    
    return (Widget*)widget;
}

Vec2 widget_local_to_global(const Widget* widget, Vec2 local_point) {
    if (!widget) return local_point;
    
    Vec2 global_point = local_point;
    global_point.x += widget->position.x;
    global_point.y += widget->position.y;
    
    // Add parent offsets recursively
    Widget* parent = widget->parent;
    while (parent) {
        global_point.x += parent->position.x;
        global_point.y += parent->position.y;
        parent = parent->parent;
    }
    
    return global_point;
}

Vec2 widget_global_to_local(const Widget* widget, Vec2 global_point) {
    if (!widget) return global_point;
    
    Vec2 local_point = global_point;
    
    // Subtract parent offsets recursively
    Widget* parent = widget->parent;
    while (parent) {
        local_point.x -= parent->position.x;
        local_point.y -= parent->position.y;
        parent = parent->parent;
    }
    
    // Subtract widget position
    local_point.x -= widget->position.x;
    local_point.y -= widget->position.y;
    
    return local_point;
}

void widget_invalidate_layout(Widget* widget) {
    if (!widget) return;
    
    widget->needs_layout = true;
    widget->dirty = true;
    
    // Propagate to children
    for (uint32_t i = 0; i < widget->child_count; i++) {
        widget_invalidate_layout(widget->children[i]);
    }
}

void widget_invalidate_redraw(Widget* widget) {
    if (!widget) return;
    
    widget->needs_redraw = true;
    widget->dirty = true;
    
    // Propagate to children
    for (uint32_t i = 0; i < widget->child_count; i++) {
        widget_invalidate_redraw(widget->children[i]);
    }
}

bool widget_can_focus(const Widget* widget) {
    return widget && widget->visible && widget->enabled && widget->focusable;
}

bool widget_request_focus(Widget* widget) {
    if (!widget_can_focus(widget)) return false;
    
    // Find root widget
    Widget* root = widget;
    while (root->parent) {
        root = root->parent;
    }
    
    // Release current focus
    if (root) {
        // TODO: Find currently focused widget and release focus
    }
    
    // Set focus to this widget
    widget_set_focused(widget, true);
    
    return true;
}

void widget_release_focus(Widget* widget) {
    if (!widget || !widget->focused) return;
    
    widget_set_focused(widget, false);
}

/* ============================================================================
 * EVENT UTILITIES
 * ============================================================================ */

UIEvent* ui_event_create(UIEventType type) {
    UIEvent* event = memory_alloc(sizeof(UIEvent));
    if (!event) {
        LOG_ERROR(LOG_CAT_GENERAL, "Failed to allocate UI event");
        return NULL;
    }
    
    memset(event, 0, sizeof(UIEvent));
    event->type = type;
    event->timestamp = (uint64_t)time(NULL) * 1000;
    
    return event;
}

void ui_event_destroy(UIEvent* event) {
    if (event) {
        if (event->type == UI_EVENT_CUSTOM && event->custom.data) {
            memory_free(event->custom.data);
        }
        memory_free(event);
    }
}

UIEvent* ui_event_create_mouse(UIEventType type, Vec2 position, UIMouseButton button) {
    UIEvent* event = ui_event_create(type);
    if (!event) return NULL;
    
    event->mouse.position = position;
    event->mouse.button = button;
    
    return event;
}

UIEvent* ui_event_create_keyboard(UIEventType type, uint32_t key_code, uint32_t scan_code) {
    UIEvent* event = ui_event_create(type);
    if (!event) return NULL;
    
    event->keyboard.key_code = key_code;
    event->keyboard.scan_code = scan_code;
    
    return event;
}

bool ui_event_is_mouse_event(const UIEvent* event) {
    if (!event) return false;
    
    return event->type >= UI_EVENT_MOUSE_ENTER && event->type <= UI_EVENT_MOUSE_DOUBLE_CLICK;
}

bool ui_event_is_keyboard_event(const UIEvent* event) {
    if (!event) return false;
    
    return event->type >= UI_EVENT_KEY_DOWN && event->type <= UI_EVENT_KEY_PRESS;
}

Vec2 ui_event_get_position(const UIEvent* event) {
    if (!event) return (Vec2){0, 0};
    
    if (ui_event_is_mouse_event(event)) {
        return event->mouse.position;
    } else if (event->type == UI_EVENT_TOUCH_DOWN || 
               event->type == UI_EVENT_TOUCH_UP || 
               event->type == UI_EVENT_TOUCH_MOVE) {
        return event->touch.position;
    }
    
    return (Vec2){0, 0};
}

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

uint32_t widget_generate_id(void) {
    return g_widget_id_counter++;
}

void widget_print_hierarchy(const Widget* widget, int depth) {
    if (!widget) return;
    
    // Print indentation
    for (int i = 0; i < depth; i++) {
        printf("  ");
    }
    
    // Print widget info
    printf("%s (%s) - Pos:(%.1f,%.1f) Size:(%.1fx%.1f) %s\n",
           widget->name,
           widget->focused ? "FOCUSED" : "normal",
           widget->position.x, widget->position.y,
           widget->size.x, widget->size.y,
           widget->visible ? "visible" : "hidden");
    
    // Print children
    for (uint32_t i = 0; i < widget->child_count; i++) {
        widget_print_hierarchy(widget->children[i], depth + 1);
    }
}
