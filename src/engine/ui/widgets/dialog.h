/*
 * dialog.h
 * Dialog/Modal Widget Implementation
 * Customizable dialog boxes with various configurations and callbacks
 *
 * Part of the UI subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef UI_DIALOG_H
#define UI_DIALOG_H

#include "widget.h"
#include "include/math/math_all.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef enum DialogType {
    DIALOG_TYPE_ALERT,      // Single OK button
    DIALOG_TYPE_CONFIRM,    // OK/Cancel buttons
    DIALOG_TYPE_PROMPT,     // Text input + OK/Cancel
    DIALOG_TYPE_WARNING,    // Warning with OK button
    DIALOG_TYPE_ERROR,      // Error with OK button
    DIALOG_TYPE_CUSTOM      // User-defined content and buttons
} DialogType;

typedef enum DialogButton {
    DIALOG_BUTTON_OK     = 1 << 0,
    DIALOG_BUTTON_CANCEL = 1 << 1,
    DIALOG_BUTTON_YES    = 1 << 2,
    DIALOG_BUTTON_NO     = 1 << 3,
    DIALOG_BUTTON_APPLY  = 1 << 4,
    DIALOG_BUTTON_RESET  = 1 << 5,
    DIALOG_BUTTON_CUSTOM = 1 << 6
} DialogButton;

typedef enum DialogResult {
    DIALOG_RESULT_NONE,
    DIALOG_RESULT_OK,
    DIALOG_RESULT_CANCEL,
    DIALOG_RESULT_YES,
    DIALOG_RESULT_NO,
    DIALOG_RESULT_APPLY,
    DIALOG_RESULT_RESET,
    DIALOG_RESULT_CUSTOM
} DialogResult;

// Forward declaration
typedef struct UIDialog UIDialog;

// Result callback: called when dialog is closed with result
typedef void (*DialogResultCallback)(UIDialog* dialog, DialogResult result, void* user_data);

// Close callback: called before dialog is destroyed
typedef void (*DialogCloseCallback)(UIDialog* dialog, void* user_data);

// Dialog widget structure
typedef struct UIDialog {
    Widget base;                    // Inherit from Widget

    // Configuration
    DialogType type;
    char* title;
    char* message;
    uint32_t button_flags;          // Bitfield of DialogButton

    // Modal behavior
    bool is_modal;                  // Block interaction behind dialog
    bool close_on_backdrop_click;   // Click outside to close
    bool close_on_escape;           // ESC key to close

    // Window behavior
    bool draggable;
    bool resizable;
    Vec2 drag_offset;
    Vec2 min_size;
    Vec2 max_size;
    bool resizing;

    // Layout containers
    Widget* title_bar;
    Widget* close_button;
    Widget* content_container;
    Widget* button_container;

    // Standard buttons (created automatically)
    Widget* ok_button;
    Widget* cancel_button;
    Widget* yes_button;
    Widget* no_button;
    Widget* apply_button;
    Widget* reset_button;

    // For prompt dialogs
    Widget* text_input;
    char* prompt_result;

    // Animation
    float fade_progress;            // 0.0 to 1.0
    float target_fade;
    bool is_animating;
    bool is_closing;

    // Callbacks
    DialogResultCallback on_result;
    DialogCloseCallback on_close;
    void* user_data;

    // Result
    DialogResult result;

    // Visual properties
    Vec4 title_bg_color;
    Vec4 content_bg_color;
    float corner_radius;
    float shadow_offset;
    Vec4 shadow_color;
} UIDialog;

/* ============================================================================
 * API - Creation and Destruction
 * ============================================================================ */

/**
 * Create a new dialog widget
 *
 * @param name     Name identifier for the dialog
 * @param title    Dialog title text
 * @param message  Dialog message/content text
 * @param type     Type of dialog (alert, confirm, etc.)
 * @return         Pointer to created dialog, or NULL on failure
 */
UIDialog* dialog_create(const char* name, const char* title, const char* message, DialogType type);

/**
 * Destroy a dialog widget and free all resources
 *
 * @param dialog   Dialog to destroy
 */
void dialog_destroy(UIDialog* dialog);

/* ============================================================================
 * API - Configuration
 * ============================================================================ */

/**
 * Set whether dialog is modal (blocks interaction behind it)
 *
 * @param dialog   Target dialog
 * @param modal    Whether dialog is modal
 */
void dialog_set_modal(UIDialog* dialog, bool modal);

/**
 * Set whether dialog is draggable by title bar
 *
 * @param dialog    Target dialog
 * @param draggable Whether draggable
 */
void dialog_set_draggable(UIDialog* dialog, bool draggable);

/**
 * Set whether dialog is resizable from edges
 *
 * @param dialog    Target dialog
 * @param resizable Whether resizable
 */
void dialog_set_resizable(UIDialog* dialog, bool resizable);

/**
 * Set minimum and maximum size constraints
 *
 * @param dialog   Target dialog
 * @param min_size Minimum size (0,0 = no minimum)
 * @param max_size Maximum size (0,0 = no maximum)
 */
void dialog_set_size_constraints(UIDialog* dialog, Vec2 min_size, Vec2 max_size);

/**
 * Configure which buttons to show
 *
 * @param dialog   Target dialog
 * @param flags    Bitfield of DialogButton flags
 */
void dialog_set_buttons(UIDialog* dialog, uint32_t flags);

/**
 * Configure dialog close behavior
 *
 * @param dialog              Target dialog
 * @param close_on_backdrop   Whether clicking backdrop closes dialog
 * @param close_on_escape     Whether ESC key closes dialog
 */
void dialog_set_close_behavior(UIDialog* dialog, bool close_on_backdrop, bool close_on_escape);

/**
 * Add a custom widget to the content area
 *
 * @param dialog   Target dialog
 * @param widget   Widget to add as content
 */
void dialog_add_custom_widget(UIDialog* dialog, Widget* widget);

/**
 * Set dialog messages
 *
 * @param dialog   Target dialog
 * @param title    New title (or NULL to keep current)
 * @param message  New message (or NULL to keep current)
 */
void dialog_set_messages(UIDialog* dialog, const char* title, const char* message);

/* ============================================================================
 * API - Callbacks
 * ============================================================================ */

/**
 * Set result callback - called when dialog closes with a result
 *
 * @param dialog    Target dialog
 * @param callback  Callback function (or NULL to unset)
 * @param user_data Data to pass to callback
 */
void dialog_set_result_callback(UIDialog* dialog, DialogResultCallback callback, void* user_data);

/**
 * Set close callback - called before dialog is destroyed
 *
 * @param dialog    Target dialog
 * @param callback  Callback function (or NULL to unset)
 * @param user_data Data to pass to callback
 */
void dialog_set_close_callback(UIDialog* dialog, DialogCloseCallback callback, void* user_data);

/* ============================================================================
 * API - Display Control
 * ============================================================================ */

/**
 * Show dialog with fade-in animation
 *
 * @param dialog   Target dialog
 */
void dialog_show(UIDialog* dialog);

/**
 * Hide dialog with fade-out animation
 * Dialog will be removed after animation completes
 *
 * @param dialog   Target dialog
 */
void dialog_hide(UIDialog* dialog);

/**
 * Close dialog with a specific result
 * Triggers callbacks and starts fade-out animation
 *
 * @param dialog   Target dialog
 * @param result   Result to return via callback
 */
void dialog_close(UIDialog* dialog, DialogResult result);

/* ============================================================================
 * API - State Query
 * ============================================================================ */

/**
 * Check if dialog is currently visible
 *
 * @param dialog   Target dialog
 * @return         True if visible
 */
bool dialog_is_visible(const UIDialog* dialog);

/**
 * Check if dialog is modal
 *
 * @param dialog   Target dialog
 * @return         True if modal
 */
bool dialog_is_modal(const UIDialog* dialog);

/**
 * Get the dialog's result
 *
 * @param dialog   Target dialog
 * @return         Dialog result enum value
 */
DialogResult dialog_get_result(const UIDialog* dialog);

/**
 * Get text entered in prompt dialog
 *
 * @param dialog   Target dialog
 * @return         Pointer to prompt text, or empty string if none
 */
const char* dialog_get_prompt_text(const UIDialog* dialog);

/**
 * Get the base widget from a dialog
 *
 * @param dialog   Target dialog
 * @return         Pointer to base Widget structure
 */
Widget* dialog_get_widget(UIDialog* dialog);

/* ============================================================================
 * API - Visual Customization
 * ============================================================================ */

/**
 * Set title bar background color
 *
 * @param dialog   Target dialog
 * @param color    Title bar color (RGBA)
 */
void dialog_set_title_bg_color(UIDialog* dialog, Vec4 color);

/**
 * Set content area background color
 *
 * @param dialog   Target dialog
 * @param color    Content background color (RGBA)
 */
void dialog_set_content_bg_color(UIDialog* dialog, Vec4 color);

/**
 * Set corner radius for dialog background
 *
 * @param dialog   Target dialog
 * @param radius   Radius in pixels
 */
void dialog_set_corner_radius(UIDialog* dialog, float radius);

/**
 * Set drop shadow
 *
 * @param dialog      Target dialog
 * @param offset      Shadow offset distance
 * @param color       Shadow color
 */
void dialog_set_shadow(UIDialog* dialog, float offset, Vec4 color);

/* ============================================================================
 * API - Helper Functions (Convenience)
 * ============================================================================ */

/**
 * Create an alert dialog (title + message, OK button, non-modal)
 *
 * @param title    Dialog title
 * @param message  Dialog message
 * @return         Created dialog, or NULL on failure
 */
UIDialog* dialog_alert(const char* title, const char* message);

/**
 * Create a confirmation dialog (title + message, OK/Cancel buttons, modal)
 *
 * @param title    Dialog title
 * @param message  Dialog message
 * @return         Created dialog, or NULL on failure
 */
UIDialog* dialog_confirm(const char* title, const char* message);

/**
 * Create a yes/no dialog
 *
 * @param title    Dialog title
 * @param message  Dialog message
 * @return         Created dialog, or NULL on failure
 */
UIDialog* dialog_yes_no(const char* title, const char* message);

/**
 * Create a warning dialog (title + message, OK button, warning style)
 *
 * @param title    Dialog title
 * @param message  Dialog message
 * @return         Created dialog, or NULL on failure
 */
UIDialog* dialog_warning(const char* title, const char* message);

/**
 * Create an error dialog (title + message, OK button, error style)
 *
 * @param title    Dialog title
 * @param message  Dialog message
 * @return         Created dialog, or NULL on failure
 */
UIDialog* dialog_error(const char* title, const char* message);

/**
 * Create a prompt/input dialog (title + message + text input, OK/Cancel, modal)
 *
 * @param title         Dialog title
 * @param message       Dialog message
 * @param default_text  Default text in input field
 * @return              Created dialog, or NULL on failure
 */
UIDialog* dialog_prompt(const char* title, const char* message, const char* default_text);

/**
 * Create a custom dialog (title only, user adds content)
 *
 * @param title    Dialog title
 * @return         Created dialog, or NULL on failure
 */
UIDialog* dialog_custom(const char* title);

/* ============================================================================
 * API - Builder Pattern (Fluent Interface)
 * ============================================================================ */

typedef struct DialogBuilder DialogBuilder;

/**
 * Create a new dialog builder
 *
 * @return  New DialogBuilder instance
 */
DialogBuilder* dialog_builder_create(void);

/**
 * Destroy a dialog builder
 *
 * @param builder  DialogBuilder to destroy
 */
void dialog_builder_destroy(DialogBuilder* builder);

/**
 * Set the title (builder method, returns self for chaining)
 */
DialogBuilder* dialog_builder_title(DialogBuilder* builder, const char* title);

/**
 * Set the message (builder method, returns self for chaining)
 */
DialogBuilder* dialog_builder_message(DialogBuilder* builder, const char* message);

/**
 * Set the dialog type (builder method, returns self for chaining)
 */
DialogBuilder* dialog_builder_type(DialogBuilder* builder, DialogType type);

/**
 * Set button flags (builder method, returns self for chaining)
 */
DialogBuilder* dialog_builder_buttons(DialogBuilder* builder, uint32_t flags);

/**
 * Set modal behavior (builder method, returns self for chaining)
 */
DialogBuilder* dialog_builder_modal(DialogBuilder* builder, bool modal);

/**
 * Set size (builder method, returns self for chaining)
 */
DialogBuilder* dialog_builder_size(DialogBuilder* builder, Vec2 size);

/**
 * Set draggable (builder method, returns self for chaining)
 */
DialogBuilder* dialog_builder_draggable(DialogBuilder* builder, bool draggable);

/**
 * Set resizable (builder method, returns self for chaining)
 */
DialogBuilder* dialog_builder_resizable(DialogBuilder* builder, bool resizable);

/**
 * Build and return the dialog
 *
 * @param builder  DialogBuilder instance
 * @return         Created UIDialog, or NULL on failure
 */
UIDialog* dialog_builder_build(DialogBuilder* builder);

#ifdef __cplusplus
}
#endif

#endif /* UI_DIALOG_H */
