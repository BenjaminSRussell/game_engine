#include "text_field.h"
#include "core/memory.h"
#include "core/logger.h"
#include "core/string_utils.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

// Key codes (matching GLFW standards as used in the engine)
#define KEY_ENTER 257
#define KEY_TAB 258
#define KEY_BACKSPACE 259
#define KEY_INSERT 260
#define KEY_DELETE 261
#define KEY_RIGHT 262
#define KEY_LEFT 263
#define KEY_DOWN 264
#define KEY_UP 265
#define KEY_HOME 268
#define KEY_END 269

// Initial buffer capacity
#define INITIAL_BUFFER_CAPACITY 64

typedef struct {
    char* text;
    uint32_t buffer_capacity;
    uint32_t length;
    uint32_t cursor_pos;
    uint32_t selection_start;
    uint32_t selection_end;
    char* placeholder;
} TextFieldData;

// Forward declarations
static void text_field_destroy(Widget* widget);
static void text_field_render(Widget* widget);
static bool text_field_handle_event(Widget* widget, UIEvent* event);

// Helper functions
static void ensure_capacity(TextFieldData* data, uint32_t required_capacity) {
    if (required_capacity > data->buffer_capacity) {
        uint32_t new_capacity = data->buffer_capacity * 2;
        if (new_capacity < required_capacity) {
            new_capacity = required_capacity;
        }

        char* new_text = MALLOC_UI(new_capacity);
        if (data->text) {
            if (new_text) {
                strcpy(new_text, data->text);
            }
            FREE(data->text);
        } else {
            if (new_text) {
                new_text[0] = '\0';
            }
        }

        data->text = new_text;
        data->buffer_capacity = new_capacity;
    }
}

static void insert_char(TextFieldData* data, char c) {
    ensure_capacity(data, data->length + 2); // +1 for char, +1 for null terminator

    // Move characters after cursor
    memmove(data->text + data->cursor_pos + 1,
            data->text + data->cursor_pos,
            data->length - data->cursor_pos + 1);

    data->text[data->cursor_pos] = c;
    data->length++;
    data->cursor_pos++;
}

static void insert_text(TextFieldData* data, const char* text) {
    if (!text) return;

    size_t len = strlen(text);
    ensure_capacity(data, data->length + len + 1);

    memmove(data->text + data->cursor_pos + len,
            data->text + data->cursor_pos,
            data->length - data->cursor_pos + 1);

    memcpy(data->text + data->cursor_pos, text, len);
    data->length += len;
    data->cursor_pos += len;
}

static void delete_char_before(TextFieldData* data) {
    if (data->cursor_pos > 0) {
        memmove(data->text + data->cursor_pos - 1,
                data->text + data->cursor_pos,
                data->length - data->cursor_pos + 1);
        data->length--;
        data->cursor_pos--;
    }
}

static void delete_char_at(TextFieldData* data) {
    if (data->cursor_pos < data->length) {
        memmove(data->text + data->cursor_pos,
                data->text + data->cursor_pos + 1,
                data->length - data->cursor_pos); // +1 included in memmove length implicitly if we want null terminator, wait.
                // data->length - data->cursor_pos is the count of chars after cursor excluding the one at cursor?
                // Example: "abc", pos 1 ('b'). len 3.
                // delete at 1. move 'c' and '\0' to 1.
                // chars to move: 'c' (index 2) and '\0' (index 3).
                // count = 2.
                // formula: len - pos. 3 - 1 = 2. Correct.
        data->length--;
    }
}

Widget* text_field_create(const char* name, const char* initial_text) {
    Widget* widget = widget_create(name);
    if (!widget) return NULL;

    TextFieldData* data = MALLOC_UI(sizeof(TextFieldData));
    if (!data) {
        widget_destroy(widget);
        return NULL;
    }

    // Initialize data
    data->buffer_capacity = INITIAL_BUFFER_CAPACITY;
    data->text = MALLOC_UI(data->buffer_capacity);
    if (!data->text) {
        FREE(data);
        widget_destroy(widget);
        return NULL;
    }

    data->text[0] = '\0';
    data->length = 0;
    data->cursor_pos = 0;
    data->selection_start = 0;
    data->selection_end = 0;
    data->placeholder = NULL;

    if (initial_text) {
        insert_text(data, initial_text);
        data->cursor_pos = data->length; // Set cursor to end by default
    } else {
        data->cursor_pos = 0;
    }

    widget->user_data = data;

    // Override methods
    widget->destroy = text_field_destroy;
    widget->render = text_field_render;
    widget->handle_event = text_field_handle_event;

    // Set default properties
    widget->focusable = true;
    widget->hoverable = true;
    widget_set_size(widget, (Vec2){200.0f, 30.0f}); // Default size
    widget_set_background_color(widget, (Vec4){0.2f, 0.2f, 0.2f, 1.0f});
    widget_set_border_color(widget, (Vec4){0.5f, 0.5f, 0.5f, 1.0f});
    widget_set_border_width(widget, 1.0f);
    widget_set_padding(widget, (BoxEdges){5.0f, 5.0f, 5.0f, 5.0f});

    return widget;
}

static void text_field_destroy(Widget* widget) {
    TextFieldData* data = (TextFieldData*)widget->user_data;
    if (data) {
        if (data->text) FREE(data->text);
        if (data->placeholder) FREE(data->placeholder);
        FREE(data);
        widget->user_data = NULL;
    }
    // Base widget destruction is handled by the caller (widget_destroy in widget.c)
}

static void text_field_render(Widget* widget) {
    // Stub implementation
    // In a real implementation, this would draw the box, text, and cursor
    // For now we just log if debug needed, but usually we don't spam logs in render
}

static bool text_field_handle_event(Widget* widget, UIEvent* event) {
    TextFieldData* data = (TextFieldData*)widget->user_data;
    if (!data) return false;

    switch (event->type) {
        case UI_EVENT_MOUSE_DOWN:
            if (widget_contains_point(widget, event->mouse.position)) {
                widget_request_focus(widget);
                // TODO: Calculate cursor position from mouse click
                return true;
            }
            break;

        case UI_EVENT_KEY_DOWN:
            if (!widget->focused) break;

            switch (event->keyboard.key_code) {
                case KEY_LEFT:
                    if (data->cursor_pos > 0) {
                        data->cursor_pos--;
                        widget->dirty = true;
                        widget->needs_redraw = true;
                    }
                    return true;

                case KEY_RIGHT:
                    if (data->cursor_pos < data->length) {
                        data->cursor_pos++;
                        widget->dirty = true;
                        widget->needs_redraw = true;
                    }
                    return true;

                case KEY_HOME:
                    data->cursor_pos = 0;
                    widget->dirty = true;
                    widget->needs_redraw = true;
                    return true;

                case KEY_END:
                    data->cursor_pos = data->length;
                    widget->dirty = true;
                    widget->needs_redraw = true;
                    return true;

                case KEY_BACKSPACE:
                    delete_char_before(data);
                    widget->dirty = true;
                    widget->needs_redraw = true;
                    return true;

                case KEY_DELETE:
                    delete_char_at(data);
                    widget->dirty = true;
                    widget->needs_redraw = true;
                    return true;

                case KEY_ENTER:
                    // For single line text field, maybe lose focus or emit event?
                    // For now, consume it.
                    return true;
            }
            break;

        case UI_EVENT_KEY_PRESS:
            if (!widget->focused) break;

            char c = event->keyboard.character;
            if (isprint(c)) {
                insert_char(data, c);
                widget->dirty = true;
                widget->needs_redraw = true;
                return true;
            }
            break;

        default:
            break;
    }

    return false;
}

void text_field_set_text(Widget* widget, const char* text) {
    TextFieldData* data = (TextFieldData*)widget->user_data;
    if (!data) return;

    if (text) {
        size_t len = strlen(text);
        if (len + 1 > data->buffer_capacity) {
            ensure_capacity(data, (uint32_t)len + 1);
        }
        strcpy(data->text, text);
        data->length = (uint32_t)len;

        // Reset cursor if out of bounds
        if (data->cursor_pos > data->length) {
            data->cursor_pos = data->length;
        }
    } else {
        data->text[0] = '\0';
        data->length = 0;
        data->cursor_pos = 0;
    }

    widget->dirty = true;
    widget->needs_redraw = true;
}

const char* text_field_get_text(const Widget* widget) {
    TextFieldData* data = (TextFieldData*)widget->user_data;
    if (!data) return NULL;
    return data->text;
}

void text_field_set_placeholder(Widget* widget, const char* placeholder) {
    TextFieldData* data = (TextFieldData*)widget->user_data;
    if (!data) return;

    if (data->placeholder) {
        FREE(data->placeholder);
    }

    if (placeholder) {
        data->placeholder = string_duplicate(placeholder);
    } else {
        data->placeholder = NULL;
    }

    widget->dirty = true;
    widget->needs_redraw = true;
}

void text_field_set_cursor_position(Widget* widget, uint32_t position) {
    TextFieldData* data = (TextFieldData*)widget->user_data;
    if (!data) return;

    if (position <= data->length) {
        data->cursor_pos = position;
        widget->dirty = true;
        widget->needs_redraw = true;
    }
}

uint32_t text_field_get_cursor_position(const Widget* widget) {
    TextFieldData* data = (TextFieldData*)widget->user_data;
    if (!data) return 0;
    return data->cursor_pos;
}

void text_field_select_range(Widget* widget, uint32_t start, uint32_t end) {
    TextFieldData* data = (TextFieldData*)widget->user_data;
    if (!data) return;

    if (start <= data->length && end <= data->length) {
        data->selection_start = start;
        data->selection_end = end;
        widget->dirty = true;
        widget->needs_redraw = true;
    }
}
