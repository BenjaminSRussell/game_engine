#include "../ui_types.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

/* =================================================================================================
 *                                RICH TEXT PARSER
 * =================================================================================================
 * 
 * Parses markup tags in text and generates display list of styled text runs.
 * 
 * Supported tags:
 * - <b>bold</b>
 * - <i>italic</i>
 * - <u>underline</u>
 * - <color=red>colored text</color> or <color=#FF0000>hex color</color>
 * - <size=20>sized text</size>
 * - <icon=smile> inline icons
 * - <wave>animated wave</wave>
 * - <shake>animated shake</shake>
 * - <link=url>hyperlink</link>
 * 
 * Features:
 * - Nested tag support
 * - Word wrapping with run boundaries
 * - Inline image/icon replacement
 * - Animated text effects
 * - Hyperlink hit-testing
 * - Screen reader accessibility (strips tags)
 * =================================================================================================
 */

#define MAX_TAG_DEPTH 16
#define MAX_TEXT_RUNS 256
#define MAX_TAG_NAME_LEN 64
#define MAX_TAG_VALUE_LEN 128

// Text style flags
typedef enum {
    STYLE_NONE = 0,
    STYLE_BOLD = 1 << 0,
    STYLE_ITALIC = 1 << 1,
    STYLE_UNDERLINE = 1 << 2,
    STYLE_WAVE = 1 << 3,
    STYLE_SHAKE = 1 << 4,
} TextStyleFlags;

// Color structure
typedef struct {
    float r, g, b, a;
} Color;

// Text run - segment of text with specific style
typedef struct {
    const char *text;       // Pointer to text (not owned, points into original string)
    uint32_t length;        // Length of text segment
    TextStyleFlags style;    // Style flags
    Color color;            // Text color
    float size;             // Font size
    const char *link_url;   // URL if this is a link
    const char *icon_name;  // Icon name if this is an icon
    float x, y;             // Computed position (set during layout)
    float width, height;    // Computed size (set during layout)
} TextRun;

// Rich text document
typedef struct {
    char *source;           // Original text (owned)
    TextRun runs[MAX_TEXT_RUNS];
    uint32_t run_count;
    float total_width;
    float total_height;
} RichTextDocument;

// Tag stack for nested tags
typedef struct {
    char name[MAX_TAG_NAME_LEN];
    char value[MAX_TAG_VALUE_LEN];
    TextStyleFlags style;
    Color color;
    float size;
} TagStackEntry;

// Helper: Parse color from string (name or hex)
static Color parse_color(const char *str) {
    Color c = {1.0f, 1.0f, 1.0f, 1.0f};
    
    if (str[0] == '#') {
        // Hex color
        unsigned int hex;
        if (sscanf(str + 1, "%x", &hex) == 1) {
            c.r = ((hex >> 16) & 0xFF) / 255.0f;
            c.g = ((hex >> 8) & 0xFF) / 255.0f;
            c.b = (hex & 0xFF) / 255.0f;
        }
    } else {
        // Named colors
        if (strcmp(str, "red") == 0) { c.r = 1.0f; c.g = 0.0f; c.b = 0.0f; }
        else if (strcmp(str, "green") == 0) { c.r = 0.0f; c.g = 1.0f; c.b = 0.0f; }
        else if (strcmp(str, "blue") == 0) { c.r = 0.0f; c.g = 0.0f; c.b = 1.0f; }
        else if (strcmp(str, "yellow") == 0) { c.r = 1.0f; c.g = 1.0f; c.b = 0.0f; }
        else if (strcmp(str, "white") == 0) { c.r = 1.0f; c.g = 1.0f; c.b = 1.0f; }
        else if (strcmp(str, "black") == 0) { c.r = 0.0f; c.g = 0.0f; c.b = 0.0f; }
    }
    
    return c;
}

// Helper: Parse tag name and value from string like \"color=red\" or \"size=20\"
static void parse_tag(const char *tag, char *name, char *value) {
    const char *equals = strchr(tag, '=');
    if (equals) {
        size_t name_len = equals - tag;
        if (name_len >= MAX_TAG_NAME_LEN) name_len = MAX_TAG_NAME_LEN - 1;
        strncpy(name, tag, name_len);
        name[name_len] = '\0';
        
        size_t value_len = strlen(equals + 1);
        if (value_len >= MAX_TAG_VALUE_LEN) value_len = MAX_TAG_VALUE_LEN - 1;
        strncpy(value, equals + 1, value_len);
        value[value_len] = '\0';
    } else {
        strncpy(name, tag, MAX_TAG_NAME_LEN - 1);
        name[MAX_TAG_NAME_LEN -1] = '\0';
        value[0] = '\0';
    }
}

// Parse rich text and create document
RichTextDocument *rich_text_parse(const char *text) {
    if (!text) return NULL;
    
    RichTextDocument *doc = (RichTextDocument *)calloc(1, sizeof(RichTextDocument));
    if (!doc) return NULL;
    
    doc->source = strdup(text);
    if (!doc->source) {
        free(doc);
        return NULL;
    }
    
    // Tag stack for nested tags
    TagStackEntry tag_stack[MAX_TAG_DEPTH];
    int tag_depth = 0;
    
    // Current style
    TextStyleFlags current_style = STYLE_NONE;
    Color current_color = {1.0f, 1.0f, 1.0f, 1.0f};
    float current_size = 16.0f;
    const char *current_link = NULL;
    
    // Parse text
    const char *ptr = text;
    const char *run_start = text;
    
    while (*ptr) {
        if (*ptr == '<') {
            // Found a tag
            const char *tag_start = ptr + 1;
            const char *tag_end = strchr(tag_start, '>');
            
            if (!tag_end) {
                // Malformed tag, skip
                ptr++;
                continue;
            }
            
            // Save current text run if any
            if (run_start < ptr && doc->run_count < MAX_TEXT_RUNS) {
                TextRun *run = &doc->runs[doc->run_count++];
                run->text = run_start;
                run->length = ptr - run_start;
                run->style = current_style;
                run->color = current_color;
                run->size = current_size;
                run->link_url = current_link;
                run->icon_name = NULL;
            }
            
            // Parse tag
            size_t tag_len = tag_end - tag_start;
            char tag_str[MAX_TAG_NAME_LEN];
            if (tag_len >= MAX_TAG_NAME_LEN) tag_len = MAX_TAG_NAME_LEN - 1;
            strncpy(tag_str, tag_start, tag_len);
            tag_str[tag_len] = '\0';
            
            // Check if closing tag
            if (tag_str[0] == '/') {
                // Closing tag - pop from stack
                if (tag_depth > 0) {
                    tag_depth--;
                    if (tag_depth > 0) {
                        current_style = tag_stack[tag_depth - 1].style;
                        current_color = tag_stack[tag_depth - 1].color;
                        current_size = tag_stack[tag_depth - 1].size;
                    } else {
                        current_style = STYLE_NONE;
                        current_color = (Color){1.0f, 1.0f, 1.0f, 1.0f};
                        current_size = 16.0f;
                    }
                }
            } else {
                // Opening tag
                char tag_name[MAX_TAG_NAME_LEN];
                char tag_value[MAX_TAG_VALUE_LEN];
                parse_tag(tag_str, tag_name, tag_value);
                
                // Push current state to stack
                if (tag_depth < MAX_TAG_DEPTH) {
                    strncpy(tag_stack[tag_depth].name, tag_name, MAX_TAG_NAME_LEN - 1);
                    strncpy(tag_stack[tag_depth].value, tag_value, MAX_TAG_VALUE_LEN - 1);
                    tag_stack[tag_depth].style = current_style;
                    tag_stack[tag_depth].color = current_color;
                    tag_stack[tag_depth].size = current_size;
                    tag_depth++;
                }
                
                // Apply tag
                if (strcmp(tag_name, "b") == 0) {
                    current_style |= STYLE_BOLD;
                } else if (strcmp(tag_name, "i") == 0) {
                    current_style |= STYLE_ITALIC;
                } else if (strcmp(tag_name, "u") == 0) {
                    current_style |= STYLE_UNDERLINE;
                } else if (strcmp(tag_name, "wave") == 0) {
                    current_style |= STYLE_WAVE;
                } else if (strcmp(tag_name, "shake") == 0) {
                    current_style |= STYLE_SHAKE;
                } else if (strcmp(tag_name, "color") == 0) {
                    current_color = parse_color(tag_value);
                } else if (strcmp(tag_name, "size") == 0) {
                    current_size = atof(tag_value);
                } else if (strcmp(tag_name, "link") == 0) {
                    current_link = tag_value;
                } else if (strcmp(tag_name, "icon") == 0) {
                    // Icon placeholder - add as special run
                    if (doc->run_count < MAX_TEXT_RUNS) {
                        TextRun *run = &doc->runs[doc->run_count++];
                        run->text = NULL;
                        run->length = 0;
                        run->style = current_style;
                        run->color = current_color;
                        run->size = current_size;
                        run->icon_name = tag_value;
                    }
                }
            }
            
            ptr = tag_end + 1;
            run_start = ptr;
        } else {
            ptr++;
        }
    }
    
    // Save final text run
    if (run_start < ptr && doc->run_count < MAX_TEXT_RUNS) {
        TextRun *run = &doc->runs[doc->run_count++];
        run->text = run_start;
        run->length = ptr - run_start;
        run->style = current_style;
        run->color = current_color;
        run->size = current_size;
        run->link_url = current_link;
        run->icon_name = NULL;
    }
    
    return doc;
}

// Destroy rich text document
void rich_text_destroy(RichTextDocument *doc) {
    if (!doc) return;
    free(doc->source);
    free(doc);
}

// Layout text runs with word wrapping
void rich_text_layout(RichTextDocument *doc, float max_width) {
    if (!doc) return;
    
    float x = 0.0f;
    float y = 0.0f;
    float line_height = 20.0f; // Should be calculated from font
    
    for (uint32_t i = 0; i < doc->run_count; i++) {
        TextRun *run = &doc->runs[i];
        
        // Simplified layout - would need actual text measurement
        float run_width = run->length * run->size * 0.6f; // Rough estimate
        
        if (x + run_width > max_width && x > 0) {
            // Wrap to next line
            x = 0.0f;
            y += line_height;
        }
        
        run->x = x;
        run->y = y;
        run->width = run_width;
        run->height = line_height;
        
        x += run_width;
    }
    
    doc->total_width = max_width;
    doc->total_height = y + line_height;
}

// Hit test for hyperlinks
const char *rich_text_hit_test(const RichTextDocument *doc, float x, float y) {
    if (!doc) return NULL;
    
    for (uint32_t i = 0; i < doc->run_count; i++) {
        const TextRun *run = &doc->runs[i];
        
        if (run->link_url &&
            x >= run->x && x <= run->x + run->width &&
            y >= run->y && y <= run->y + run->height) {
            return run->link_url;
        }
    }
    
    return NULL;
}

// Strip all tags for screen reader
char *rich_text_strip_tags(const char *text) {
    if (!text) return NULL;
    
    size_t len = strlen(text);
    char *result = (char *)malloc(len + 1);
    if (!result) return NULL;
    
    const char *src = text;
    char *dst = result;
    
    while (*src) {
        if (*src == '<') {
            // Skip until '>'
            while (*src && *src != '>') src++;
            if (*src == '>') src++;
        } else {
            *dst++ = *src++;
        }
    }
    
    *dst = '\0';
    return result;
}

// Get run count
uint32_t rich_text_get_run_count(const RichTextDocument *doc) {
    return doc ? doc->run_count : 0;
}

// Get text run
const TextRun *rich_text_get_run(const RichTextDocument *doc, uint32_t index) {
    if (!doc || index >= doc->run_count) return NULL;
    return &doc->runs[index];
}

// Debug: Print document structure
void rich_text_debug_print(const RichTextDocument *doc) {
    if (!doc) return;
    
    printf("Rich Text Document (%u runs):\n", doc->run_count);
    for (uint32_t i = 0; i < doc->run_count; i++) {
        const TextRun *run = &doc->runs[i];
        printf("  Run %u: \"%.*s\" style=%d color=(%.2f,%.2f,%.2f) size=%.1f\n",
               i, run->length, run->text ? run->text : "(icon)",
               run->style, run->color.r, run->color.g, run->color.b, run->size);
    }
}

