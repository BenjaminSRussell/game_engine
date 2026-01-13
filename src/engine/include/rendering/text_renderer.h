// include/render/text_renderer.h
//
// Purpose: Text rendering system with font atlas, kerning, and layout support.
// Provides high-performance glyph rendering for UI, HUD, and debug text.
//
#ifndef TEXT_RENDERER_H
#define TEXT_RENDERER_H

#include "engine/include/common.h"
#include <math/vec2.h>
#include <math/vec3.h>
#include <math/vec4.h>

// Font styles
typedef enum {
  FONT_STYLE_REGULAR,
  FONT_STYLE_BOLD,
  FONT_STYLE_ITALIC,
  FONT_STYLE_BOLD_ITALIC,
} FontStyle;

// Text alignment
typedef enum {
  TEXT_ALIGN_LEFT,
  TEXT_ALIGN_CENTER,
  TEXT_ALIGN_RIGHT,
} TextAlignment;

// Vertical alignment
typedef enum {
  TEXT_VALIGN_TOP,
  TEXT_VALIGN_CENTER,
  TEXT_VALIGN_BOTTOM,
} TextVerticalAlignment;

// Glyph information
typedef struct {
  u32 codepoint;
  u32 atlas_x, atlas_y;     // Position in atlas
  u32 width, height;        // Glyph size
  i32 bearing_x, bearing_y; // Offset from baseline
  u32 advance;              // Advance width for next glyph
  f32 uv_x, uv_y;           // UV coordinates in atlas
  f32 uv_width, uv_height;
} Glyph;

// Font information
typedef struct {
  u32 font_id;
  char name[64];
  u32 size; // Font size in pixels
  FontStyle style;
  u32 line_height;
  u32 baseline;
  f32 scale_x, scale_y; // Scaling factors

  // Glyph cache (covers ASCII + common Unicode)
  Glyph glyphs[256];
  u32 glyph_count;

  // Kerning pairs
  struct {
    u32 left_glyph, right_glyph;
    i32 kerning_amount;
  } kerning_pairs[1024];
  u32 kerning_count;

  // Font atlas
  u32 atlas_texture_id;
  u32 atlas_width, atlas_height;

  bool initialized;
} Font;

// Formatted text with layout
typedef struct {
  char text[1024];
  Font *font;
  Vec4 color;
  Vec2 position;
  TextAlignment alignment;
  TextVerticalAlignment valignment;
  f32 line_spacing;
  f32 letter_spacing;
  u32 max_width; // Wrap text if needed
  bool shadow;
  Vec4 shadow_color;
  Vec2 shadow_offset;
} FormattedText;

// Text mesh (precomputed vertices for rendering)
typedef struct {
  Vec3 *vertices;
  Vec2 *uvs;
  Vec4 *colors;
  u32 *indices;
  u32 vertex_count;
  u32 index_count;
  u32 vertex_buffer; // GPU vertex buffer handle
} TextMesh;

// Font library
#define MAX_FONTS 32
#define MAX_TEXT_MESHES 256

typedef struct {
  Font fonts[MAX_FONTS];
  u32 font_count;

  TextMesh meshes[MAX_TEXT_MESHES];
  u32 mesh_count;

  u32 default_font_id;
  bool initialized;
} FontLibrary;

// ==============================================================================
// Font Library Lifecycle
// ==============================================================================

// Initialize font library
void font_library_init(FontLibrary *library);

// Shutdown font library
void font_library_shutdown(FontLibrary *library);

// ==============================================================================
// Font Loading
// ==============================================================================

// Load font from TTF file
u32 font_load_from_file(FontLibrary *library, const char *filepath, u32 size,
                        FontStyle style);

// Load font asynchronously
u32 font_load_async(FontLibrary *library, const char *filepath, u32 size,
                    FontStyle style);

// Check if font is loaded
bool font_is_loaded(FontLibrary *library, u32 font_id);

// Get font by ID
Font *font_get(FontLibrary *library, u32 font_id);

// Get default font
Font *font_get_default(FontLibrary *library);

// Set default font
void font_set_default(FontLibrary *library, u32 font_id);

// ==============================================================================
// Glyph Management
// ==============================================================================

// Get glyph for character
Glyph *font_get_glyph(Font *font, u32 codepoint);

// Pre-cache glyphs (load before rendering)
void font_precache_glyphs(Font *font, const char *text);

// Get glyph kerning between two characters
i32 font_get_kerning(Font *font, u32 left_codepoint, u32 right_codepoint);

// ==============================================================================
// Text Measurement
// ==============================================================================

// Measure text width in pixels
f32 text_measure_width(Font *font, const char *text);

// Measure text height in pixels
f32 text_measure_height(Font *font, const char *text);

// Measure formatted text dimensions
typedef struct {
  f32 width, height;
  u32 line_count;
} TextMeasurement;

TextMeasurement text_measure_formatted(FormattedText *formatted_text);

// Get character position within text
Vec2 text_get_char_position(Font *font, const char *text, u32 char_index);

// Get character index at position
u32 text_get_char_at_position(Font *font, const char *text, f32 x);

// ==============================================================================
// Text Rendering (Mesh Generation)
// ==============================================================================

// Generate text mesh for rendering
u32 text_mesh_create(FontLibrary *library, Font *font, const char *text,
                     Vec2 position, Vec4 color);

// Update text mesh
void text_mesh_update(FontLibrary *library, u32 mesh_id, const char *new_text);

// Delete text mesh
void text_mesh_delete(FontLibrary *library, u32 mesh_id);

// Get text mesh
TextMesh *text_mesh_get(FontLibrary *library, u32 mesh_id);

// Render text at position (high-level interface)
void text_render(Font *font, const char *text, Vec2 position, Vec4 color);

// Render formatted text
void text_render_formatted(FormattedText *formatted_text);

// ==============================================================================
// Text Formatting
// ==============================================================================

// Create formatted text structure
FormattedText text_format_create(Font *font, const char *text);

// Set text color
void text_format_set_color(FormattedText *text, Vec4 color);

// Set text position
void text_format_set_position(FormattedText *text, Vec2 position);

// Set text alignment
void text_format_set_alignment(FormattedText *text, TextAlignment align,
                               TextVerticalAlignment valign);

// Set line spacing (multiplier of line height)
void text_format_set_line_spacing(FormattedText *text, f32 spacing);

// Set letter spacing (in pixels)
void text_format_set_letter_spacing(FormattedText *text, f32 spacing);

// Enable text shadow
void text_format_set_shadow(FormattedText *text, Vec4 shadow_color,
                            Vec2 offset);

// Set text wrapping width
void text_format_set_wrap_width(FormattedText *text, u32 max_width);

// ==============================================================================
// Text Effects
// ==============================================================================

// Apply outline effect to text
void text_effect_outline(TextMesh *mesh, f32 outline_width, Vec4 outline_color);

// Apply glow effect to text
void text_effect_glow(TextMesh *mesh, f32 glow_amount, Vec4 glow_color);

// Apply strikethrough
void text_effect_strikethrough(TextMesh *mesh, Vec4 line_color);

// Apply underline
void text_effect_underline(TextMesh *mesh, Vec4 line_color);

// ==============================================================================
// Font Metrics
// ==============================================================================

// Get font ascender height
f32 font_get_ascender(Font *font);

// Get font descender height
f32 font_get_descender(Font *font);

// Get font line gap
f32 font_get_line_gap(Font *font);

// ==============================================================================
// Debug Rendering
// ==============================================================================

// Render debug font info (glyph atlas, metrics)
void text_debug_render_glyph_atlas(Font *font);

// Log font information
void text_log_font_info(Font *font);

// Log library statistics
void text_library_log_stats(FontLibrary *library);

// ==============================================================================
// Unicode Support
// ==============================================================================

// Get character width (handling wide characters)
f32 text_get_char_width(Font *font, u32 codepoint);

// Convert UTF-8 string to codepoints
u32 text_convert_utf8(const char *utf8_str, u32 *out_codepoints,
                      u32 max_codepoints);

// ==============================================================================
// Font Serialization
// ==============================================================================

// Export font metrics to binary format
bool font_serialize(Font *font, u8 *buffer, u32 buffer_size, u32 *out_size);

// Import font metrics from binary format
bool font_deserialize(u8 *buffer, u32 size, Font *out_font);

#endif // TEXT_RENDERER_H
