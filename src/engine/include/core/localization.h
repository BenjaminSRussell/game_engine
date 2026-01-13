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
#include <stddef.h>
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

bool string_table_load_json(StringTable *table, const char *path);
bool string_table_load_po(StringTable *table, const char *path);

StringTable *string_table_create(LanguageCode language,
                                 const char *language_name,
                                 const char *native_name, bool is_rtl);
void string_table_destroy(StringTable *table);
bool string_table_add(StringTable *table, const LocalizedString *string);
bool string_table_remove(StringTable *table, const char *key);
LocalizedString *string_table_get(StringTable *table, const char *key);
bool string_table_load_csv(StringTable *table, const char *path);
bool string_table_save(const StringTable *table, const char *path);
bool string_table_merge(StringTable *target, const StringTable *source,
                        bool overwrite);
uint32_t string_table_find_missing(const StringTable *base,
                                   const StringTable *reference,
                                   char (*out_keys)[128], uint32_t max_keys);

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

PluralCategory plural_rule_french(int64_t count);
PluralCategory plural_rule_russian(int64_t count);
PluralCategory plural_rule_polish(int64_t count);
PluralCategory plural_rule_arabic(int64_t count);
PluralCategory plural_rule_japanese(int64_t count);
bool plural_format(LanguageCode language, const LocalizedString *string,
                   int64_t count, char *out, size_t out_size);

PluralCategory plural_get_category(LanguageCode language, int64_t count);
PluralCategory plural_rule_english(int64_t count);

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

#define FORMAT_ARG_INT 0u
#define FORMAT_ARG_FLOAT 1u
#define FORMAT_ARG_STRING 2u

bool format_string(LanguageCode language, const char *format, 
                   const FormatArg *args, size_t arg_count, 
                   char *out, size_t out_size);
bool format_number(LanguageCode language, double value, char *out,
                   size_t out_size);
bool format_currency(LanguageCode language, double value,
                     const char *currency_symbol, bool currency_before,
                     char *out, size_t out_size);
bool format_date(LanguageCode language, int64_t timestamp, 
                 const char *format, char *out, size_t out_size);
bool format_time(LanguageCode language, int64_t timestamp, 
                 bool show_seconds, bool use_24_hour, 
                 char *out, size_t out_size);
bool format_duration(LanguageCode language, int64_t seconds, 
                     bool show_milliseconds, char *out, size_t out_size);
bool format_percentage(LanguageCode language, double value, char *out,
                       size_t out_size);
bool format_ordinal(LanguageCode language, int64_t value, char *out,
                    size_t out_size);
bool format_unit(LanguageCode language, double value, const char *unit, 
                 bool use_si_prefixes, char *out, size_t out_size);

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

bool font_load(Font *font, const char *path, float font_size);
bool font_unload(Font *font);
bool font_generate_atlas(Font *font, float font_size, uint32_t atlas_width, 
                        uint32_t atlas_height);
const Glyph *font_get_glyph(const Font *font, uint32_t codepoint, float font_size);
float font_get_kerning(const Font *font, uint32_t left_glyph, uint32_t right_glyph, 
                       float font_size);
bool font_measure_text(const Font *font, const char *text, float font_size, 
                       float *out_width, float *out_height);
bool font_word_wrap(const Font *font, const char *text, float font_size, 
                    float max_width, char *out, size_t out_size);
bool font_sdf_generate(Font *font, float font_size, uint32_t atlas_width, 
                       uint32_t atlas_height, float spread);
bool font_fallback_chain(Font *primary_font, Font **fallback_fonts, 
                        uint32_t fallback_count, const char *text, 
                        float font_size, char *out_glyph_indices, 
                        size_t out_size);
bool font_emoji_support(Font *font, const char *emoji_font_path, float font_size);

// Font system management
bool font_system_init(void);
void font_system_shutdown(void);

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

LocalizationManager *loc_manager_get_default(void);
void loc_manager_init(LocalizationManager *manager);
void loc_manager_shutdown(LocalizationManager *manager);
bool loc_manager_set_language(LocalizationManager *manager,
                              LanguageCode language);
const char *loc_manager_get_string(LocalizationManager *manager,
                                   const char *key);
const char *loc_manager_get_plural(LocalizationManager *manager,
                                   const char *key, int64_t count);
const char *loc_manager_format(LocalizationManager *manager, const char *key,
                               const FormatArg *args, size_t arg_count);
bool loc_manager_load_all(LocalizationManager *manager, const char **paths,
                          const LanguageCode *languages, size_t count);
LanguageCode loc_manager_detect_system_language(void);
bool loc_manager_validate(const LocalizationManager *manager,
                          char (*out_keys)[128], size_t max_keys);
bool loc_manager_export_template(const LocalizationManager *manager,
                                 const char *path);

/* =================================================================================================
 *                                    RTL TEXT PROCESSING
 * =================================================================================================
 */

bool loc_manager_is_rtl_language(LanguageCode language);
bool loc_manager_is_rtl_text(const char *text);
bool loc_manager_process_rtl_text(LanguageCode language, const char *input, 
                                  char *output, size_t output_size);
bool loc_manager_reverse_text_for_rtl(const char *input, char *output, size_t output_size);
void loc_manager_get_text_direction(LanguageCode language, bool *is_rtl, 
                                   const char **direction_marker);

/* =================================================================================================
 *                                    CONVENIENT MACROS
 * =================================================================================================
 */

// L("key") -> localized string
// LP("key", count) -> pluralized string
// LF("key", ...) -> formatted localized string

#define L(key) loc_manager_get_string(loc_manager_get_default(), (key))
#define LP(key, count) \
  loc_manager_get_plural(loc_manager_get_default(), (key), (count))
#define LF(key, args, arg_count) \
  loc_manager_format(loc_manager_get_default(), (key), (args), (arg_count))

#endif // LOCALIZATION_H
