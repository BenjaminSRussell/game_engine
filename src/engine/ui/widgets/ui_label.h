/*
 * ui_label.h
 * Label Widget Implementation
 * Text display widget for UI labels and messages
 *
 * Part of the UI subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef UI_LABEL_H
#define UI_LABEL_H

#include "widget.h"
#include "include/math/math.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef enum LabelAlignment {
    LABEL_ALIGN_LEFT,
    LABEL_ALIGN_CENTER,
    LABEL_ALIGN_RIGHT,
} LabelAlignment;

typedef enum LabelVerticalAlignment {
    LABEL_VALIGN_TOP,
    LABEL_VALIGN_CENTER,
    LABEL_VALIGN_BOTTOM,
} LabelVerticalAlignment;

typedef enum LabelTextFormat {
    LABEL_FORMAT_PLAIN,      // Plain text
    LABEL_FORMAT_BOLD,       // Bold text
    LABEL_FORMAT_ITALIC,     // Italic text
    LABEL_FORMAT_MONOSPACE,  // Monospace font
} LabelTextFormat;

// Label state structure
typedef struct UILabel {
    Widget base;                   // Inherit from Widget

    // Content
    char* text;
    uint32_t text_length;

    // Text properties
    float font_size;
    Vec4 text_color;
    LabelAlignment alignment;
    LabelVerticalAlignment v_alignment;
    LabelTextFormat format;

    // Layout properties
    bool word_wrap;               // Enable word wrapping
    bool ellipsis;               // Show "..." for overflow
    uint32_t max_lines;          // Max number of lines (0 = unlimited)

    // Display properties
    float shadow_offset_x;
    float shadow_offset_y;
    Vec4 shadow_color;
    bool show_shadow;

    // Internal
    uint32_t calculated_lines;
} UILabel;

/* ============================================================================
 * API - Creation and Destruction
 * ============================================================================ */

/**
 * Create a new label widget
 *
 * @param name   Name identifier for the label
 * @param text   Label text to display
 * @return       Pointer to created label, or NULL on failure
 */
UILabel* ui_label_create(const char* name, const char* text);

/**
 * Destroy a label widget and free all resources
 *
 * @param label  Label to destroy
 */
void ui_label_destroy(UILabel* label);

/* ============================================================================
 * API - Content Management
 * ============================================================================ */

/**
 * Set the label's text
 *
 * @param label  Target label
 * @param text   New text to display
 */
void ui_label_set_text(UILabel* label, const char* text);

/**
 * Get the label's text
 *
 * @param label  Target label
 * @return       Pointer to label text
 */
const char* ui_label_get_text(const UILabel* label);

/**
 * Set the label's text with formatted string (printf-style)
 *
 * @param label  Target label
 * @param fmt    Format string
 * @param ...    Format arguments
 */
void ui_label_set_text_format(UILabel* label, const char* fmt, ...);

/* ============================================================================
 * API - Text Properties
 * ============================================================================ */

/**
 * Set the font size
 *
 * @param label     Target label
 * @param font_size Font size in points
 */
void ui_label_set_font_size(UILabel* label, float font_size);

/**
 * Set the text color
 *
 * @param label  Target label
 * @param color  Text color (RGBA)
 */
void ui_label_set_text_color(UILabel* label, Vec4 color);

/**
 * Set horizontal text alignment
 *
 * @param label     Target label
 * @param alignment Alignment mode
 */
void ui_label_set_alignment(UILabel* label, LabelAlignment alignment);

/**
 * Set vertical text alignment
 *
 * @param label      Target label
 * @param v_alignment Vertical alignment mode
 */
void ui_label_set_vertical_alignment(UILabel* label, LabelVerticalAlignment v_alignment);

/**
 * Set text format/style
 *
 * @param label    Target label
 * @param format   Text format style
 */
void ui_label_set_format(UILabel* label, LabelTextFormat format);

/* ============================================================================
 * API - Layout Properties
 * ============================================================================ */

/**
 * Enable or disable word wrapping
 *
 * @param label    Target label
 * @param wrap     Whether to wrap text
 */
void ui_label_set_word_wrap(UILabel* label, bool wrap);

/**
 * Enable or disable ellipsis for overflow text
 *
 * @param label    Target label
 * @param ellipsis Whether to show ellipsis
 */
void ui_label_set_ellipsis(UILabel* label, bool ellipsis);

/**
 * Set the maximum number of lines
 *
 * @param label      Target label
 * @param max_lines  Maximum lines (0 = unlimited)
 */
void ui_label_set_max_lines(UILabel* label, uint32_t max_lines);

/**
 * Get the number of calculated/displayed lines
 *
 * @param label  Target label
 * @return       Number of lines
 */
uint32_t ui_label_get_line_count(const UILabel* label);

/* ============================================================================
 * API - Visual Effects
 * ============================================================================ */

/**
 * Set text shadow
 *
 * @param label        Target label
 * @param offset_x     Shadow X offset
 * @param offset_y     Shadow Y offset
 * @param color        Shadow color
 * @param show_shadow  Whether to show shadow
 */
void ui_label_set_shadow(UILabel* label, float offset_x, float offset_y, Vec4 color, bool show_shadow);

/**
 * Disable text shadow
 *
 * @param label  Target label
 */
void ui_label_disable_shadow(UILabel* label);

/* ============================================================================
 * API - Utility
 * ============================================================================ */

/**
 * Get the base widget from a label (for hierarchy operations)
 *
 * @param label  Target label
 * @return       Pointer to base Widget structure
 */
Widget* ui_label_get_widget(UILabel* label);

/**
 * Get the calculated size needed to display the text
 *
 * @param label    Target label
 * @param max_width Maximum width for layout (0 = unlimited)
 * @return         Calculated size needed
 */
Vec2 ui_label_get_content_size(UILabel* label, float max_width);

#ifdef __cplusplus
}
#endif

#endif /* UI_LABEL_H */
