/**
 * =================================================================================================
 *                              LOCALIZATION & TEXT SYSTEM
 *                                  Agent: AGENT_DOCS_1
 * =================================================================================================
 *
 * Complete localization with pluralization, formatting, and font rendering.
 *
 * =================================================================================================
 */

#ifndef LOCALIZATION_H
#define LOCALIZATION_H

#include <stdbool.h>
#include <stdint.h>

/* =================================================================================================
 *                                    LANGUAGE CODES
 * =================================================================================================
 */

typedef enum LanguageCode {
  LANG_EN_US,
  LANG_EN_GB,
  LANG_ES_ES,
  LANG_ES_MX,
  LANG_FR_FR,
  LANG_DE_DE,
  LANG_IT_IT,
  LANG_PT_BR,
  LANG_PT_PT,
  LANG_RU_RU,
  LANG_JA_JP,
  LANG_KO_KR,
  LANG_ZH_CN,
  LANG_ZH_TW,
  LANG_AR_SA,
  LANG_PL_PL,
  LANG_TR_TR,
  LANG_TH_TH,
  LANG_VI_VN,
  LANG_COUNT,
} LanguageCode;

/* =================================================================================================
 *                                    STRING TABLE
 * =================================================================================================
 */

typedef struct LocalizedString {
  char key[128];
  char value[1024];
  char context[256]; // Translator notes
  bool is_plural;
  char plural_zero[256];
  char plural_one[256];
  char plural_two[256];
  char plural_few[256];
  char plural_many[256];
  char plural_other[256];
} LocalizedString;

typedef struct StringTable {
  LanguageCode language;
  char language_name[32];
  char native_name[64];
  bool is_rtl;

  LocalizedString *strings;
  uint32_t string_count;
  uint32_t string_capacity;

  // Hash map for fast lookup
  void *hash_map;
} StringTable;

// Implemented: string_table_create
// Implemented: string_table_destroy
// TODO(AGENT_DOCS_1): Implement string_table_add [Difficulty: 4]
// Implemented: string_table_remove
// TODO(AGENT_DOCS_1): Implement string_table_get [Difficulty: 4]
// TODO(AGENT_DOCS_1): Implement string_table_load_csv [Difficulty: 5]
// TODO(AGENT_DOCS_1): Implement string_table_load_json [Difficulty: 5]
// TODO(AGENT_DOCS_1): Implement string_table_load_po [Difficulty: 6]
// TODO(AGENT_DOCS_1): Implement string_table_save [Difficulty: 5]
// TODO(AGENT_DOCS_1): Implement string_table_merge [Difficulty: 5]
// TODO(AGENT_DOCS_1): Implement string_table_find_missing [Difficulty: 5]

/* =================================================================================================
 *                                    PLURALIZATION
 * =================================================================================================
 */

typedef enum PluralCategory {
  PLURAL_ZERO,
  PLURAL_ONE,
  PLURAL_TWO,
  PLURAL_FEW,
  PLURAL_MANY,
  PLURAL_OTHER,
} PluralCategory;

// TODO(AGENT_DOCS_1): Implement plural_get_category [Difficulty: 6]
// Implemented: plural_rule_english
// TODO(AGENT_DOCS_1): Implement plural_rule_french [Difficulty: 4]
// TODO(AGENT_DOCS_1): Implement plural_rule_russian [Difficulty: 5]
// TODO(AGENT_DOCS_1): Implement plural_rule_polish [Difficulty: 5]
// TODO(AGENT_DOCS_1): Implement plural_rule_arabic [Difficulty: 6]
// TODO(AGENT_DOCS_1): Implement plural_rule_japanese [Difficulty: 3]
// TODO(AGENT_DOCS_1): Implement plural_format [Difficulty: 5]

/* =================================================================================================
 *                                    TEXT FORMATTING
 * =================================================================================================
 */

typedef struct FormatArg {
  char name[32];
  union {
    int64_t int_value;
    double float_value;
    const char *string_value;
  } value;
  uint32_t type; // int, float, string
} FormatArg;

// TODO(AGENT_DOCS_1): Implement format_string [Difficulty: 6]
// TODO(AGENT_DOCS_1): Implement format_number [Difficulty: 5]
// TODO(AGENT_DOCS_1): Implement format_currency [Difficulty: 5]
// TODO(AGENT_DOCS_1): Implement format_date [Difficulty: 6]
// TODO(AGENT_DOCS_1): Implement format_time [Difficulty: 5]
// TODO(AGENT_DOCS_1): Implement format_duration [Difficulty: 5]
// TODO(AGENT_DOCS_1): Implement format_percentage [Difficulty: 4]
// TODO(AGENT_DOCS_1): Implement format_ordinal [Difficulty: 5]
// TODO(AGENT_DOCS_1): Implement format_unit [Difficulty: 5]

/* =================================================================================================
 *                                    FONT SYSTEM
 * =================================================================================================
 */

typedef struct Glyph {
  uint32_t codepoint;
  float advance;
  float bearing_x;
  float bearing_y;
  float width;
  float height;
  float uv_min[2];
  float uv_max[2];
} Glyph;

typedef struct FontAtlas {
  uint32_t texture_id;
  uint32_t width;
  uint32_t height;
  Glyph *glyphs;
  uint32_t glyph_count;
  void *glyph_map; // codepoint -> glyph index
  float line_height;
  float ascender;
  float descender;
} FontAtlas;

typedef struct Font {
  char name[64];
  char path[256];
  FontAtlas *atlases;
  uint32_t atlas_count; // Different sizes
  float *supported_sizes;
  uint32_t size_count;
  bool is_sdf;
  float sdf_spread;
} Font;

// TODO(AGENT_DOCS_1): Implement font_load [Difficulty: 6]
// TODO(AGENT_DOCS_1): Implement font_unload [Difficulty: 4]
// TODO(AGENT_DOCS_1): Implement font_generate_atlas [Difficulty: 7]
// TODO(AGENT_DOCS_1): Implement font_get_glyph [Difficulty: 4]
// TODO(AGENT_DOCS_1): Implement font_get_kerning [Difficulty: 5]
// TODO(AGENT_DOCS_1): Implement font_measure_text [Difficulty: 5]
// TODO(AGENT_DOCS_1): Implement font_word_wrap [Difficulty: 6]
// TODO(AGENT_DOCS_1): Implement font_sdf_generate [Difficulty: 7]
// TODO(AGENT_DOCS_1): Implement font_fallback_chain [Difficulty: 5]
// TODO(AGENT_DOCS_1): Implement font_emoji_support [Difficulty: 6]

/* =================================================================================================
 *                                    TEXT RENDERING
 * =================================================================================================
 */

typedef struct TextStyle {
  uint32_t font_id;
  float font_size;
  float color[4];
  float outline_color[4];
  float outline_width;
  float shadow_color[4];
  float shadow_offset[2];
  float letter_spacing;
  float line_height_multiplier;
  bool bold;
  bool italic;
  bool underline;
  bool strikethrough;
} TextStyle;

typedef struct TextBlock {
  char *text;
  uint32_t text_length;
  TextStyle style;
  float position[2];
  float max_width;
  float max_height;
  uint32_t alignment; // Left, center, right, justify
  uint32_t vertical_align;
  bool wrap_words;
  bool truncate;
  char truncation_suffix[8];
} TextBlock;

// TODO(AGENT_DOCS_1): Implement text_render [Difficulty: 6]
// TODO(AGENT_DOCS_1): Implement text_render_sdf [Difficulty: 7]
// TODO(AGENT_DOCS_1): Implement text_layout [Difficulty: 6]
// TODO(AGENT_DOCS_1): Implement text_layout_bidi [Difficulty: 8]
// TODO(AGENT_DOCS_1): Implement text_shaping [Difficulty: 8]
// TODO(AGENT_DOCS_1): Implement text_hit_test [Difficulty: 5]
// TODO(AGENT_DOCS_1): Implement text_selection_rects [Difficulty: 5]
// TODO(AGENT_DOCS_1): Implement text_caret_position [Difficulty: 5]
// TODO(AGENT_DOCS_1): Implement text_line_breaks [Difficulty: 5]
// TODO(AGENT_DOCS_1): Implement text_rich_parse [Difficulty: 6]

/* =================================================================================================
 *                                    LOCALIZATION MANAGER
 * =================================================================================================
 */

typedef struct LocalizationManager {
  StringTable *string_tables;
  uint32_t table_count;

  LanguageCode current_language;
  LanguageCode fallback_language;

  Font *fonts;
  uint32_t font_count;

  // Currency/number format settings per language
  struct {
    char decimal_separator;
    char thousands_separator;
    char currency_symbol[8];
    bool currency_before;
  } format_settings[LANG_COUNT];

  // Callbacks
  void (*on_language_changed)(LanguageCode language);
} LocalizationManager;

// TODO(AGENT_DOCS_1): Implement loc_manager_init [Difficulty: 5]
// TODO(AGENT_DOCS_1): Implement loc_manager_shutdown [Difficulty: 4]
// TODO(AGENT_DOCS_1): Implement loc_manager_set_language [Difficulty: 5]
// TODO(AGENT_DOCS_1): Implement loc_manager_get_string [Difficulty: 4]
// TODO(AGENT_DOCS_1): Implement loc_manager_get_plural [Difficulty: 5]
// TODO(AGENT_DOCS_1): Implement loc_manager_format [Difficulty: 6]
// TODO(AGENT_DOCS_1): Implement loc_manager_load_all [Difficulty: 5]
// TODO(AGENT_DOCS_1): Implement loc_manager_detect_system_language [Difficulty:
// 5]
// TODO(AGENT_DOCS_1): Implement loc_manager_validate [Difficulty: 6]
// TODO(AGENT_DOCS_1): Implement loc_manager_export_template [Difficulty: 5]

/* =================================================================================================
 *                                    CONVENIENT MACROS
 * =================================================================================================
 */

// L("key") -> localized string
// LP("key", count) -> pluralized string
// LF("key", ...) -> formatted localized string

// TODO(AGENT_DOCS_1): Implement L macro [Difficulty: 3]
// TODO(AGENT_DOCS_1): Implement LP macro [Difficulty: 4]
// TODO(AGENT_DOCS_1): Implement LF macro [Difficulty: 5]

#endif // LOCALIZATION_H
