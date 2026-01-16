/**
 * =================================================================================================
 *                              ADVANCED TEXT RENDERING SYSTEM
 *                            Professional Unicode Text Rendering
 * =================================================================================================
 *
 * Enterprise-grade text rendering with full Unicode support, SDF rendering,
 * complex text layout, bidirectional text, and professional text shaping.
 *
 * =================================================================================================
 */

#ifndef TEXT_RENDERING_ADVANCED_H
#define TEXT_RENDERING_ADVANCED_H

#include "core/localization.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "math/vec4.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =================================================================================================
 *                                    UNICODE SUPPORT
 * =================================================================================================
 */

#define UNICODE_MAX_CODEPOINT 0x10FFFF
#define UNICODE_SURROGATE_START 0xD800
#define UNICODE_SURROGATE_END 0xDFFF
#define UNICODE_BMP_MAX 0xFFFF

typedef struct UnicodeProperties {
  uint32_t codepoint;
  uint8_t general_category;
  uint8_t script;
  uint8_t bidirectional_class;
  uint8_t combining_class;
  bool is_mirrored;
  bool is_combining;
  bool is_control;
  bool is_whitespace;
  bool is_bidi_control;
} UnicodeProperties;

typedef struct UnicodeString {
  uint32_t *codepoints;
  size_t length;
  size_t capacity;
  bool owns_memory;
} UnicodeString;

UnicodeString *unicode_string_create(const char *utf8_text);
UnicodeString *unicode_string_create_empty(size_t initial_capacity);
void unicode_string_destroy(UnicodeString *str);
size_t unicode_string_append_utf8(UnicodeString *str, const char *utf8_text);
size_t unicode_string_append_codepoint(UnicodeString *str, uint32_t codepoint);
bool unicode_string_normalize_nfc(UnicodeString *str);
bool unicode_string_normalize_nfd(UnicodeString *str);
bool unicode_string_to_uppercase(UnicodeString *str, LanguageCode language);
bool unicode_string_to_lowercase(UnicodeString *str, LanguageCode language);
bool unicode_string_to_titlecase(UnicodeString *str, LanguageCode language);

/* =================================================================================================
 *                                    FONT SYSTEM
 * =================================================================================================
 */

typedef enum FontHinting {
  FONT_HINTING_NONE,
  FONT_HINTING_LIGHT,
  FONT_HINTING_NORMAL,
  FONT_HINTING_AUTO
} FontHinting;

typedef enum FontRenderMode {
  FONT_RENDER_BITMAP,
  FONT_RENDER_SDF,
  FONT_RENDER_MSDF, // Multi-channel SDF
  FONT_RENDER_VECTOR
} FontRenderMode;

typedef struct AdvancedGlyph {
  uint32_t codepoint;
  float advance_x, advance_y;
  float bearing_x, bearing_y;
  float width, height;
  float uv_x, uv_y, uv_width, uv_height;
  uint32_t atlas_page;
  bool is_colored; // For emoji
  uint32_t ligature_caret_count;
  float *ligature_caret_positions;
} AdvancedGlyph;

typedef struct KerningPair {
  uint32_t left_glyph;
  uint32_t right_glyph;
  float kerning;
} KerningPair;

typedef struct FontMetrics {
  float units_per_em;
  float ascender;
  float descender;
  float line_gap;
  float cap_height;
  float x_height;
  float underline_position;
  float underline_thickness;
  float strikethrough_position;
  float strikethrough_thickness;
} FontMetrics;

typedef struct FontAtlasRegion {
  uint32_t x, y, width, height;
  bool occupied;
} FontAtlasRegion;

typedef struct AdvancedFontAtlas {
  uint32_t texture_id;
  uint32_t width, height;
  uint32_t format; // RGBA8, RGB8, etc.
  uint8_t *pixels;
  bool owns_pixels;

  FontAtlasRegion *regions;
  uint32_t region_count;
  uint32_t region_capacity;
} AdvancedFontAtlas;

typedef struct AdvancedFont {
  char name[128];
  char family[128];
  char style[64];
  char file_path[512];

  FontMetrics metrics;
  FontRenderMode render_mode;
  FontHinting hinting;

  float sdf_spread;
  float sdf_gamma;
  uint32_t msdf_range;

  AdvancedGlyph *glyphs;
  uint32_t glyph_count;
  uint32_t glyph_capacity;

  KerningPair *kerning_pairs;
  uint32_t kerning_count;
  uint32_t kerning_capacity;

  AdvancedFontAtlas *atlases;
  uint32_t atlas_count;
  uint32_t max_atlases;

  // Unicode coverage
  uint32_t *supported_codepoints;
  uint32_t supported_count;
  uint32_t supported_capacity;

  // Fallback fonts
  uint32_t *fallback_font_ids;
  uint32_t fallback_count;
  uint32_t fallback_capacity;

  // Font features
  uint32_t features;
  bool is_variable_font;
  bool has_color_glyphs;
  bool has_svg_glyphs;

  void *font_data; // Platform-specific font data
  size_t font_data_size;
  bool owns_font_data;

  // Thread safety
  void *mutex;

  bool initialized;
} AdvancedFont;

typedef struct FontLibrary {
  AdvancedFont *fonts;
  uint32_t font_count;
  uint32_t font_capacity;

  uint32_t default_font_id;
  uint32_t default_sdf_font_id;

  // Font loading cache
  struct FontCacheEntry {
    char path[512];
    uint32_t font_id;
    uint64_t last_access;
  } *font_cache;
  uint32_t cache_count;
  uint32_t cache_capacity;

  // Memory management
  void *allocator;
  size_t total_memory_used;

  bool initialized;
} FontLibrary;

/* Font library management */
FontLibrary *font_library_create(void);
void font_library_destroy(FontLibrary *library);
uint32_t font_library_load_font(FontLibrary *library, const char *file_path,
                                float size, FontRenderMode render_mode);
uint32_t font_library_load_font_from_memory(FontLibrary *library,
                                            const uint8_t *data,
                                            size_t data_size, float size,
                                            FontRenderMode render_mode);
AdvancedFont *font_library_get_font(FontLibrary *library, uint32_t font_id);
bool font_library_unload_font(FontLibrary *library, uint32_t font_id);
bool font_library_set_fallback_chain(FontLibrary *library, uint32_t font_id,
                                     const uint32_t *fallback_ids,
                                     uint32_t count);

/* Font metrics and glyph access */
AdvancedGlyph *advanced_font_get_glyph(AdvancedFont *font, uint32_t codepoint);
float advanced_font_get_kerning(AdvancedFont *font, uint32_t left_codepoint,
                                uint32_t right_codepoint);
bool advanced_font_get_glyph_bounds(AdvancedFont *font, uint32_t codepoint,
                                    float *out_x, float *out_y,
                                    float *out_width, float *out_height);
float advanced_font_get_text_width(AdvancedFont *font, const char *text,
                                   size_t text_length);
float advanced_font_get_text_height(AdvancedFont *font, const char *text,
                                    size_t text_length);

/* =================================================================================================
 *                                    SDF TEXT RENDERING
 * =================================================================================================
 */

typedef struct SDFTextShader {
  uint32_t program_id;

  // Uniform locations
  int32_t u_mvp_matrix;
  int32_t u_text_color;
  int32_t u_outline_color;
  int32_t u_outline_width;
  int32_t u_shadow_color;
  int32_t u_shadow_offset;
  int32_t u_sdf_params; // (spread, gamma, threshold, padding)
  int32_t u_texture;
  int32_t u_msdf_mode;
} SDFTextShader;

typedef struct SDFTextBatch {
  float *positions; // x, y, z per vertex
  float *texcoords; // u, v per vertex
  float *colors;    // r, g, b, a per vertex
  uint32_t *indices;

  uint32_t vertex_count;
  uint32_t index_count;
  uint32_t max_vertices;
  uint32_t max_indices;

  uint32_t vao;
  uint32_t position_buffer;
  uint32_t texcoord_buffer;
  uint32_t color_buffer;
  uint32_t index_buffer;

  bool dirty;
} SDFTextBatch;

typedef struct SDFRenderer {
  SDFTextShader shader;
  SDFTextBatch batch;

  // Rendering state
  uint32_t current_atlas;
  bool blending_enabled;
  uint32_t blend_src, blend_dst;

  // Performance stats
  uint64_t triangles_rendered;
  uint64_t draw_calls;
  uint64_t texture_switches;

  bool initialized;
} SDFRenderer;

/* SDF rendering functions */
SDFRenderer *sdf_renderer_create(void);
void sdf_renderer_destroy(SDFRenderer *renderer);
bool sdf_renderer_init(SDFRenderer *renderer);
void sdf_renderer_shutdown(SDFRenderer *renderer);

bool sdf_renderer_begin_frame(SDFRenderer *renderer);
bool sdf_renderer_end_frame(SDFRenderer *renderer);

bool sdf_renderer_add_text(SDFRenderer *renderer, AdvancedFont *font,
                           const char *text, float x, float y, float scale,
                           const float color[4]);
bool sdf_renderer_add_text_with_outline(SDFRenderer *renderer,
                                        AdvancedFont *font, const char *text,
                                        float x, float y, float scale,
                                        const float color[4],
                                        const float outline_color[4],
                                        float outline_width);
bool sdf_renderer_add_text_with_shadow(
    SDFRenderer *renderer, AdvancedFont *font, const char *text, float x,
    float y, float scale, const float color[4], const float shadow_color[4],
    float shadow_offset_x, float shadow_offset_y);

void sdf_renderer_flush(SDFRenderer *renderer);

/* =================================================================================================
 *                                    TEXT LAYOUT ENGINE
 * =================================================================================================
 */

typedef enum TextAlignment {
  TEXT_ALIGN_LEFT,
  TEXT_ALIGN_CENTER,
  TEXT_ALIGN_RIGHT,
  TEXT_ALIGN_JUSTIFY
} TextAlignment;

typedef enum TextVerticalAlignment {
  TEXT_VALIGN_TOP,
  TEXT_VALIGN_CENTER,
  TEXT_VALIGN_BOTTOM,
  TEXT_VALIGN_BASELINE
} TextVerticalAlignment;

typedef enum TextDirection {
  TEXT_DIRECTION_LTR, // Left-to-Right
  TEXT_DIRECTION_RTL, // Right-to-Left
  TEXT_DIRECTION_AUTO // Auto-detect
} TextDirection;

typedef enum LineBreakMode {
  LINE_BREAK_WORD,      // Break at word boundaries
  LINE_BREAK_CHAR,      // Break at character boundaries
  LINE_BREAK_HYPHENATE, // Hyphenate words
  LINE_BREAK_TRUNCATE   // Truncate with ellipsis
} LineBreakMode;

typedef struct TextMetrics {
  float width;
  float height;
  float baseline;
  uint32_t line_count;
  float *line_widths;
  float *line_heights;
  float *line_baselines;
} TextMetrics;

typedef struct TextLayout {
  // Input parameters
  AdvancedFont *font;
  const char *text;
  size_t text_length;

  // Layout constraints
  float max_width;
  float max_height;
  TextAlignment alignment;
  TextVerticalAlignment vertical_alignment;
  TextDirection direction;
  LineBreakMode line_break_mode;

  // Spacing
  float line_spacing;
  float paragraph_spacing;
  float letter_spacing;
  float word_spacing;

  // Effects
  bool enable_outline;
  float outline_width;
  bool enable_shadow;
  float shadow_offset_x, shadow_offset_y;

  // Results
  TextMetrics metrics;
  struct TextLine {
    uint32_t start_offset;
    uint32_t end_offset;
    float x, y, width, height;
    float baseline;
    TextDirection direction;
  } *lines;
  uint32_t line_count;

  // Memory management
  void *allocator;
  bool owns_memory;
} TextLayout;

/* Text layout functions */
TextLayout *text_layout_create(AdvancedFont *font, const char *text);
void text_layout_destroy(TextLayout *layout);
bool text_layout_set_constraints(TextLayout *layout, float max_width,
                                 float max_height);
bool text_layout_set_alignment(TextLayout *layout, TextAlignment alignment,
                               TextVerticalAlignment vertical_alignment);
bool text_layout_set_direction(TextLayout *layout, TextDirection direction);
bool text_layout_set_spacing(TextLayout *layout, float line_spacing,
                             float letter_spacing, float word_spacing);
bool text_layout_calculate(TextLayout *layout);
TextMetrics text_layout_get_metrics(const TextLayout *layout);
bool text_layout_get_line_info(const TextLayout *layout, uint32_t line_index,
                               float *out_x, float *out_y, float *out_width,
                               float *out_height);

/* =================================================================================================
 *                                    BIDIRECTIONAL TEXT
 * =================================================================================================
 */

typedef enum BidiClass {
  BIDI_L,   // Left-to-Right
  BIDI_R,   // Right-to-Left
  BIDI_AL,  // Right-to-Left Arabic
  BIDI_EN,  // European Number
  BIDI_ES,  // European Number Separator
  BIDI_ET,  // European Number Terminator
  BIDI_AN,  // Arabic Number
  BIDI_CS,  // Common Number Separator
  BIDI_NSM, // Non-Spacing Mark
  BIDI_BN,  // Boundary Neutral
  BIDI_B,   // Paragraph Separator
  BIDI_S,   // Segment Separator
  BIDI_WS,  // Whitespace
  BIDI_ON,  // Other Neutrals
  BIDI_LRE, // Left-to-Right Embedding
  BIDI_LRO, // Left-to-Right Override
  BIDI_RLE, // Right-to-Left Embedding
  BIDI_RLO, // Right-to-Left Override
  BIDI_PDF, // Pop Directional Format
  BIDI_LRI, // Left-to-Right Isolate
  BIDI_RLI, // Right-to-Left Isolate
  BIDI_FSI, // First Strong Isolate
  BIDI_PDI  // Pop Directional Isolate
} BidiClass;

typedef struct BidiLevel {
  uint8_t level;       // Embedding level
  BidiClass direction; // Resolved direction
  bool is_rtl;         // true if RTL
} BidiLevel;

typedef struct BidiRun {
  uint32_t start_offset;
  uint32_t end_offset;
  BidiLevel level;
  bool is_rtl;
} BidiRun;

typedef struct BidiText {
  UnicodeString *text;
  BidiLevel *levels;
  BidiRun *runs;
  uint32_t run_count;
  uint32_t run_capacity;

  // Memory management
  void *allocator;
  bool owns_memory;
} BidiText;

/* Bidirectional text functions */
BidiText *bidi_text_create(const UnicodeString *text);
void bidi_text_destroy(BidiText *bidi_text);
bool bidi_text_process(BidiText *bidi_text, TextDirection base_direction);
bool bidi_text_resolve_paragraph(BidiText *bidi_text, uint32_t start_offset,
                                 uint32_t end_offset,
                                 TextDirection base_direction);
BidiRun *bidi_text_get_runs(const BidiText *bidi_text, uint32_t *out_run_count);
bool bidi_text_reorder_visual(BidiText *bidi_text, uint32_t **out_visual_order,
                              uint32_t *out_count);
bool bidi_text_get_logical_from_visual(const BidiText *bidi_text,
                                       uint32_t visual_index,
                                       uint32_t *out_logical);
bool bidi_text_get_visual_from_logical(const BidiText *bidi_text,
                                       uint32_t logical_index,
                                       uint32_t *out_visual);

/* =================================================================================================
 *                                    TEXT SHAPING
 * =================================================================================================
 */

typedef enum ScriptType {
  SCRIPT_LATIN,
  SCRIPT_ARABIC,
  SCRIPT_DEVANAGARI,
  SCRIPT_THAI,
  SCRIPT_CHINESE,
  SCRIPT_JAPANESE,
  SCRIPT_KOREAN,
  SCRIPT_HEBREW,
  SCRIPT_CYRILLIC,
  SCRIPT_GREEK,
  SCRIPT_GEORGIAN,
  SCRIPT_ARMENIAN,
  SCRIPT_BENGALI,
  SCRIPT_GUJARATI,
  SCRIPT_GURMUKHI,
  SCRIPT_KANNADA,
  SCRIPT_MALAYALAM,
  SCRIPT_ORIYA,
  SCRIPT_SINHALA,
  SCRIPT_TAMIL,
  SCRIPT_TELUGU,
  SCRIPT_TIBETAN,
  SCRIPT_ETHIOPIC,
  SCRIPT_KHMER,
  SCRIPT_LAO,
  SCRIPT_MYANMAR,

  SCRIPT_UNKNOWN
} ScriptType;

typedef struct ShapingFeature {
  uint32_t tag; // 4-byte tag (e.g., 'liga', 'kern', 'rlig')
  bool enabled;
  uint32_t value;
} ShapingFeature;

typedef struct ShapingOptions {
  ScriptType script;
  LanguageCode language;
  TextDirection direction;

  // Font features
  ShapingFeature *features;
  uint32_t feature_count;

  // Shaping behavior
  bool enable_ligatures;
  bool enable_kerning;
  bool enable_mark_positioning;
  bool enable_cursive_positioning;

  // Advanced options
  uint32_t variation_settings_count;
  struct {
    uint32_t tag;
    float value;
  } *variation_settings;
} ShapingOptions;

typedef struct ShapedGlyph {
  uint32_t codepoint;
  uint32_t cluster; // Index back to original text
  float advance_x, advance_y;
  float offset_x, offset_y;

  // For complex scripts
  uint32_t mark_attachment;
  bool is_mark;
  bool is_ligature;
  uint32_t ligature_component;

  // Visual properties
  float scale_x, scale_y;
  float rotation;
} ShapedGlyph;

typedef struct ShapedText {
  ShapedGlyph *glyphs;
  uint32_t glyph_count;
  uint32_t glyph_capacity;

  // Original text reference
  const char *original_text;
  size_t original_length;

  // Shaping information
  ShapingOptions options;

  // Result metrics
  float total_advance;
  float ascent;
  float descent;
  float line_gap;

  // Memory management
  void *allocator;
  bool owns_memory;
} ShapedText;

/* Text shaping functions */
ShapedText *text_shaper_create_text(const char *text, AdvancedFont *font,
                                    const ShapingOptions *options);
void text_shaper_destroy_text(ShapedText *shaped_text);
bool text_shaper_shape_text(ShapedText *shaped_text, const char *text,
                            size_t length, AdvancedFont *font,
                            const ShapingOptions *options);
bool text_shaper_reshape_text(ShapedText *shaped_text,
                              const ShapingOptions *new_options);
ScriptType text_shaper_detect_script(const char *text, size_t length);
bool text_shaper_get_glyph_at_position(const ShapedText *shaped_text, float x,
                                       float y, uint32_t *out_glyph_index,
                                       float *out_offset);
bool text_shaper_get_cursor_position(const ShapedText *shaped_text,
                                     uint32_t text_index, float *out_x,
                                     float *out_y, bool *out_trailing);

/* =================================================================================================
 *                                    RICH TEXT SUPPORT
 * =================================================================================================
 */

typedef enum RichTextTagType {
  RICH_TEXT_BOLD,
  RICH_TEXT_ITALIC,
  RICH_TEXT_UNDERLINE,
  RICH_TEXT_STRIKETHROUGH,
  RICH_TEXT_COLOR,
  RICH_TEXT_FONT_SIZE,
  RICH_TEXT_FONT_FAMILY,
  RICH_TEXT_LINK,
  RICH_TEXT_SUPERSCRIPT,
  RICH_TEXT_SUBSCRIPT,
  RICH_TEXT_HIGHLIGHT,
  RICH_TEXT_CUSTOM
} RichTextTagType;

typedef struct RichTextSpan {
  uint32_t start_offset;
  uint32_t end_offset;
  RichTextTagType tag_type;
  union {
    struct {
      float r, g, b, a;
    } color;
    struct {
      float size;
    } font_size;
    struct {
      char family[64];
    } font_family;
    struct {
      char url[256];
    } link;
    struct {
      uint32_t custom_id;
    } custom;
  } data;
  struct RichTextSpan *next;
} RichTextSpan;

typedef struct RichText {
  char *text;
  size_t text_length;
  RichTextSpan *spans;
  uint32_t span_count;

  // Memory management
  void *allocator;
  bool owns_memory;
} RichText;

/* Rich text functions */
RichText *rich_text_create(const char *text);
void rich_text_destroy(RichText *rich_text);
bool rich_text_parse_html(RichText *rich_text, const char *html_text);
bool rich_text_parse_markdown(RichText *rich_text, const char *markdown_text);
bool rich_text_add_span(RichText *rich_text, uint32_t start, uint32_t end,
                        RichTextTagType type, const void *data);
bool rich_text_clear_spans(RichText *rich_text);
RichTextSpan *rich_text_get_spans_at_offset(const RichText *rich_text,
                                            uint32_t offset,
                                            uint32_t *out_count);

/* =================================================================================================
 *                                    HIGH-LEVEL API
 * =================================================================================================
 */

typedef struct TextRenderer {
  FontLibrary *font_library;
  SDFRenderer *sdf_renderer;
  void *allocator;

  // Default settings
  uint32_t default_font_id;
  TextDirection default_direction;
  LanguageCode default_language;

  // Performance monitoring
  uint64_t total_glyphs_rendered;
  uint64_t total_text_shaped;
  uint64_t cache_hits;
  uint64_t cache_misses;

  bool initialized;
} TextRenderer;

/* Main text renderer API */
TextRenderer *text_renderer_create(void);
void text_renderer_destroy(TextRenderer *renderer);
bool text_renderer_init(TextRenderer *renderer);
void text_renderer_shutdown(TextRenderer *renderer);

bool text_renderer_load_font(TextRenderer *renderer, const char *file_path,
                             float size, FontRenderMode mode);
bool text_renderer_load_font_from_memory(TextRenderer *renderer,
                                         const uint8_t *data, size_t size,
                                         float font_size, FontRenderMode mode);

bool text_renderer_render_text(TextRenderer *renderer, const char *text,
                               float x, float y, uint32_t font_id,
                               float font_size, const float color[4]);
bool text_renderer_render_text_with_layout(TextRenderer *renderer,
                                           const char *text,
                                           const TextLayout *layout,
                                           const ShapingOptions *shaping);
bool text_renderer_render_rich_text(TextRenderer *renderer,
                                    const RichText *rich_text, float x, float y,
                                    uint32_t font_id, float font_size);

TextMetrics text_renderer_measure_text(TextRenderer *renderer, const char *text,
                                       uint32_t font_id, float font_size,
                                       float max_width);
bool text_renderer_get_cursor_position(TextRenderer *renderer, const char *text,
                                       uint32_t font_id, float font_size,
                                       uint32_t character_index, float *out_x,
                                       float *out_y);

/* =================================================================================================
 *                                    ERROR HANDLING
 * =================================================================================================
 */

typedef enum TextError {
  TEXT_ERROR_NONE = 0,
  TEXT_ERROR_INVALID_PARAMETER,
  TEXT_ERROR_OUT_OF_MEMORY,
  TEXT_ERROR_FONT_NOT_FOUND,
  TEXT_ERROR_FONT_LOAD_FAILED,
  TEXT_ERROR_GLYPH_NOT_FOUND,
  TEXT_ERROR_UNSUPPORTED_SCRIPT,
  TEXT_ERROR_SHAPING_FAILED,
  TEXT_ERROR_LAYOUT_FAILED,
  TEXT_ERROR_RENDERING_FAILED,
  TEXT_ERROR_UNICODE_ERROR,
  TEXT_ERROR_FILE_NOT_FOUND,
  TEXT_ERROR_PARSE_ERROR
} TextError;

typedef struct TextErrorInfo {
  TextError error;
  const char *message;
  const char *function;
  const char *file;
  uint32_t line;
  void *context;
} TextErrorInfo;

/* Error handling functions */
const char *text_error_to_string(TextError error);
void text_set_error_handler(void (*handler)(const TextErrorInfo *info));
TextError text_get_last_error(void);
const char *text_get_last_error_message(void);

/* =================================================================================================
 *                                    DEBUG AND PROFILING
 * =================================================================================================
 */

typedef struct TextRendererStats {
  uint64_t total_text_renders;
  uint64_t total_text_shaped;
  uint64_t total_glyphs_shaped;
  uint64_t total_glyphs_rendered;
  uint64_t cache_hits;
  uint64_t cache_misses;
  uint64_t texture_uploads;
  uint64_t draw_calls;

  // Timing
  double avg_shaping_time_ms;
  double avg_layout_time_ms;
  double avg_render_time_ms;

  // Memory
  size_t total_memory_used;
  size_t font_memory_used;
  size_t atlas_memory_used;
  size_t cache_memory_used;

  // Error counts
  uint64_t shaping_errors;
  uint64_t layout_errors;
  uint64_t rendering_errors;
} TextRendererStats;

/* Debug and profiling functions */
void text_renderer_get_stats(TextRenderer *renderer, TextRendererStats *stats);
void text_renderer_reset_stats(TextRenderer *renderer);
void text_renderer_log_stats(TextRenderer *renderer);
bool text_renderer_dump_font_info(TextRenderer *renderer, uint32_t font_id,
                                  const char *output_path);
bool text_renderer_validate_fonts(TextRenderer *renderer,
                                  char (*out_messages)[256],
                                  uint32_t max_messages);

#ifdef __cplusplus
}
#endif

#endif // TEXT_RENDERING_ADVANCED_H