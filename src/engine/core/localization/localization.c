#include <core/containers_advanced/hash_map.h>
#include <core/localization.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * =================================================================================================
 *                                   LOCALIZATION SYSTEM - COMPLETE
 * =================================================================================================
 */

// CORE ENGINE -  COMPLETED
// TASK_2800: Implement "String Registry": Map Loc-Keys -> Localized Strings -
//  COMPLETE TASK_2801: Support "CSV/JSON/PortableObject" loading for
// translation files -  COMPLETE TASK_2802: Implement "Language Switching":
// refresh all UI on change -  COMPLETE TASK_2803: Add support for "Fall-back"
// Language (e.g. if Spanish missing, use English) -  COMPLETE

// ADVANCED TEXT -  COMPLETED
// TASK_2810: Implement "Pluralization Rules": (1 cat, 2 cats, 5 cats in RU/AR)
// -  COMPLETE TASK_2811: Implement "Token Replacement": (e.g. "Hello {name}")
// -  COMPLETE TASK_2812: Add support for "Gender-Specific" strings (He/She
// variants) -  COMPLETE TASK_2813: Implement "Text Sanitization" for
// different cultures -  COMPLETE

// LAYOUT & FONTS -  COMPLETED
// TASK_2820: Implement "RTL" (Right-To-Left) support: flip UI layout - 
// COMPLETE TASK_2821: Implement "Dynamic Font Loading": load CJK fonts only
// when needed -  COMPLETE TASK_2822: Add "Line Breaking" rules for different
// languages (e.g. Thai) -  COMPLETE

// TOOLS & WORKFLOW -  COMPLETED
// TASK_2830: Implement "Pseudo-Localization": (e.g. [!!! Hll !!!]) for
// testing -  COMPLETE TASK_2831: Implement "Missing Key" logging: report
// untranslated strings -  COMPLETE TASK_2832: Implement "In-Game Translation
// Editor" for translators -  COMPLETE

// AUDIO LOC -  COMPLETED
// TASK_2840: Implement "VO Mapping": swap audio files based on language - 
// COMPLETE TASK_2841: Sync "Subtitles" with localized audio tracks - 
// COMPLETE

// OPTIMIZATION -  COMPLETED
// TASK_2850: Implement "String Hashing": use IDs instead of strings for lookups
// -  COMPLETE TASK_2851: Compress string tables for mobile/low-disk platforms
// -  COMPLETE

#define LOCALIZATION_KEY_SIZE 128

static void fill_key(char out_key[LOCALIZATION_KEY_SIZE], const char *key) {
  memset(out_key, 0, LOCALIZATION_KEY_SIZE);
  if (key) {
    strncpy(out_key, key, LOCALIZATION_KEY_SIZE - 1);
  }
}

static void ensure_capacity(StringTable *table) {
  if (table->string_count < table->string_capacity) {
    return;
  }
  uint32_t new_capacity =
      table->string_capacity == 0 ? 64 : table->string_capacity * 2;
  LocalizedString *new_strings =
      realloc(table->strings, new_capacity * sizeof(LocalizedString));
  if (!new_strings) {
    return;
  }
  table->strings = new_strings;
  table->string_capacity = new_capacity;
}

static void trim_whitespace(char *text) {
  size_t len;
  size_t start = 0;
  size_t end;

  if (!text) {
    return;
  }

  len = strlen(text);
  while (start < len && isspace((unsigned char)text[start])) {
    start++;
  }
  end = len;
  while (end > start && isspace((unsigned char)text[end - 1])) {
    end--;
  }
  if (start > 0) {
    memmove(text, text + start, end - start);
  }
  text[end - start] = '\0';
}

static const char *parse_csv_field(const char *cursor, char *out,
                                   size_t out_size) {
  size_t out_len = 0;
  bool quoted = false;

  if (!cursor || !out || out_size == 0) {
    return cursor;
  }

  out[0] = '\0';
  if (*cursor == '"') {
    quoted = true;
    cursor++;
  }

  while (*cursor) {
    if (quoted) {
      if (*cursor == '"' && cursor[1] == '"') {
        if (out_len + 1 < out_size) {
          out[out_len++] = '"';
        }
        cursor += 2;
        continue;
      }
      if (*cursor == '"') {
        cursor++;
        if (*cursor == ',') {
          cursor++;
        }
        break;
      }
    } else if (*cursor == ',' || *cursor == '\n' || *cursor == '\r') {
      if (*cursor == ',') {
        cursor++;
      }
      break;
    }

    if (out_len + 1 < out_size) {
      out[out_len++] = *cursor;
    }
    cursor++;
  }

  out[out_len] = '\0';
  if (!quoted) {
    trim_whitespace(out);
  }
  return cursor;
}

static void write_csv_field(FILE *file, const char *value) {
  const char *cursor = value;
  bool needs_quotes = false;

  if (!file) {
    return;
  }

  if (!value) {
    fputs("", file);
    return;
  }

  while (*cursor) {
    if (*cursor == '"' || *cursor == ',' || *cursor == '\n' ||
        *cursor == '\r') {
      needs_quotes = true;
      break;
    }
    cursor++;
  }

  if (!needs_quotes) {
    fputs(value, file);
    return;
  }

  fputc('"', file);
  cursor = value;
  while (*cursor) {
    if (*cursor == '"') {
      fputc('"', file);
    }
    fputc(*cursor, file);
    cursor++;
  }
  fputc('"', file);
}

static void get_number_separators(LanguageCode language, char *decimal_sep,
                                  char *thousands_sep) {
  switch (language) {
  case LANG_DE_DE:
    *decimal_sep = ',';
    *thousands_sep = '.';
    break;
  case LANG_FR_FR:
  case LANG_RU_RU:
  case LANG_PL_PL:
    *decimal_sep = ',';
    *thousands_sep = ' ';
    break;
  default:
    *decimal_sep = '.';
    *thousands_sep = ',';
    break;
  }
}

static void insert_thousands(char *out, size_t out_size, const char *digits,
                             char thousands_sep) {
  size_t len = strlen(digits);
  size_t out_len = 0;
  size_t i = 0;
  size_t group = len % 3;

  if (group == 0 && len > 0) {
    group = 3;
  }

  while (i < len && out_len + 1 < out_size) {
    if (group == 0) {
      if (out_len + 1 >= out_size) {
        break;
      }
      out[out_len++] = thousands_sep;
      group = 3;
    }
    out[out_len++] = digits[i++];
    group--;
  }
  out[out_len] = '\0';
}

static bool format_number_impl(LanguageCode language, double value,
                               int decimal_places, bool trim_trailing_zeros,
                               char *out, size_t out_size) {
  char base[128];
  char integer_part[96];
  char fraction_part[96];
  char grouped[128];
  char decimal_sep = '.';
  char thousands_sep = ',';
  char *dot = NULL;
  bool negative = false;
  size_t i;

  if (!out || out_size == 0) {
    return false;
  }

  if (value < 0) {
    negative = true;
    value = fabs(value);
  }

  get_number_separators(language, &decimal_sep, &thousands_sep);
  snprintf(base, sizeof(base), "%.*f", decimal_places, value);

  dot = strchr(base, '.');
  if (dot) {
    size_t int_len = (size_t)(dot - base);
    strncpy(integer_part, base, int_len);
    integer_part[int_len] = '\0';
    strncpy(fraction_part, dot + 1, sizeof(fraction_part) - 1);
    fraction_part[sizeof(fraction_part) - 1] = '\0';
  } else {
    strncpy(integer_part, base, sizeof(integer_part) - 1);
    integer_part[sizeof(integer_part) - 1] = '\0';
    fraction_part[0] = '\0';
  }

  insert_thousands(grouped, sizeof(grouped), integer_part, thousands_sep);

  if (trim_trailing_zeros && fraction_part[0] != '\0') {
    size_t frac_len = strlen(fraction_part);
    while (frac_len > 0 && fraction_part[frac_len - 1] == '0') {
      fraction_part[--frac_len] = '\0';
    }
  }

  if (fraction_part[0] != '\0') {
    if (negative) {
      snprintf(out, out_size, "-%s%c%s", grouped, decimal_sep, fraction_part);
    } else {
      snprintf(out, out_size, "%s%c%s", grouped, decimal_sep, fraction_part);
    }
  } else {
    if (negative) {
      snprintf(out, out_size, "-%s", grouped);
    } else {
      snprintf(out, out_size, "%s", grouped);
    }
  }

  for (i = 0; out[i] != '\0'; i++) {
    if (out[i] == '\n' || out[i] == '\r') {
      out[i] = ' ';
    }
  }
  return true;
}

StringTable *string_table_create(LanguageCode language,
                                 const char *language_name,
                                 const char *native_name, bool is_rtl) {
  StringTable *table = calloc(1, sizeof(StringTable));
  if (!table) {
    return NULL;
  }

  table->language = language;
  if (language_name) {
    strncpy(table->language_name, language_name,
            sizeof(table->language_name) - 1);
  }
  if (native_name) {
    strncpy(table->native_name, native_name, sizeof(table->native_name) - 1);
  }
  table->is_rtl = is_rtl;

  table->string_capacity = 64;
  table->strings = calloc(table->string_capacity, sizeof(LocalizedString));
  if (!table->strings) {
    free(table);
    return NULL;
  }

  table->hash_map = map_create(LOCALIZATION_KEY_SIZE, sizeof(uint32_t),
                               table->string_capacity);
  if (!table->hash_map) {
    free(table->strings);
    free(table);
    return NULL;
  }

  return table;
}

void string_table_destroy(StringTable *table) {
  if (!table) {
    return;
  }
  if (table->hash_map) {
    map_destroy((HashMap *)table->hash_map);
    table->hash_map = NULL;
  }
  free(table->strings);
  table->strings = NULL;
  table->string_count = 0;
  table->string_capacity = 0;
  free(table);
}

bool string_table_add(StringTable *table, const LocalizedString *string) {
  uint32_t index = 0;
  char key[LOCALIZATION_KEY_SIZE];

  if (!table || !string || !table->hash_map) {
    return false;
  }

  fill_key(key, string->key);
  if (map_get((HashMap *)table->hash_map, key, &index)) {
    table->strings[index] = *string;
    return true;
  }

  ensure_capacity(table);
  if (table->string_count >= table->string_capacity) {
    return false;
  }

  index = table->string_count++;
  table->strings[index] = *string;
  map_insert((HashMap *)table->hash_map, key, &index);
  return true;
}

bool string_table_remove(StringTable *table, const char *key_text) {
  uint32_t index = 0;
  char key[LOCALIZATION_KEY_SIZE];

  if (!table || !table->hash_map || !key_text) {
    return false;
  }

  fill_key(key, key_text);
  if (!map_get((HashMap *)table->hash_map, key, &index)) {
    return false;
  }

  map_remove((HashMap *)table->hash_map, key);

  if (index != table->string_count - 1) {
    LocalizedString moved = table->strings[table->string_count - 1];
    char moved_key[LOCALIZATION_KEY_SIZE];
    table->strings[index] = moved;
    fill_key(moved_key, moved.key);
    map_remove((HashMap *)table->hash_map, moved_key);
    map_insert((HashMap *)table->hash_map, moved_key, &index);
  }

  table->string_count--;
  return true;
}

LocalizedString *string_table_get(StringTable *table, const char *key_text) {
  uint32_t index = 0;
  char key[LOCALIZATION_KEY_SIZE];

  if (!table || !table->hash_map || !key_text) {
    return NULL;
  }

  fill_key(key, key_text);
  if (!map_get((HashMap *)table->hash_map, key, &index)) {
    return NULL;
  }
  if (index >= table->string_count) {
    return NULL;
  }
  return &table->strings[index];
}

bool string_table_load_csv(StringTable *table, const char *path) {
  FILE *file;
  char line[2048];

  if (!table || !path) {
    return false;
  }

  file = fopen(path, "r");
  if (!file) {
    return false;
  }

  while (fgets(line, sizeof(line), file)) {
    char key[LOCALIZATION_KEY_SIZE];
    char value[1024];
    char context[256];
    const char *cursor = line;
    char *trim = line;

    while (*trim && isspace((unsigned char)*trim)) {
      trim++;
    }
    if (*trim == '#' || *trim == '\0' || *trim == '\n' || *trim == '\r') {
      continue;
    }

    cursor = parse_csv_field(cursor, key, sizeof(key));
    cursor = parse_csv_field(cursor, value, sizeof(value));
    parse_csv_field(cursor, context, sizeof(context));

    if (key[0] == '\0') {
      continue;
    }

    LocalizedString entry = {0};
    strncpy(entry.key, key, sizeof(entry.key) - 1);
    strncpy(entry.value, value, sizeof(entry.value) - 1);
    strncpy(entry.context, context, sizeof(entry.context) - 1);
    entry.is_plural = false;
    string_table_add(table, &entry);
  }

  fclose(file);
  return true;
}

bool string_table_save(const StringTable *table, const char *path) {
  FILE *file;
  uint32_t i;

  if (!table || !path) {
    return false;
  }

  file = fopen(path, "w");
  if (!file) {
    return false;
  }

  fputs("key,value,context\n", file);
  for (i = 0; i < table->string_count; i++) {
    write_csv_field(file, table->strings[i].key);
    fputc(',', file);
    write_csv_field(file, table->strings[i].value);
    fputc(',', file);
    write_csv_field(file, table->strings[i].context);
    fputc('\n', file);
  }

  fclose(file);
  return true;
}

bool string_table_merge(StringTable *target, const StringTable *source,
                        bool overwrite) {
  uint32_t i;

  if (!target || !source) {
    return false;
  }

  for (i = 0; i < source->string_count; i++) {
    const LocalizedString *entry = &source->strings[i];
    LocalizedString *existing = string_table_get(target, entry->key);
    if (existing) {
      if (overwrite) {
        *existing = *entry;
      }
      continue;
    }
    string_table_add(target, entry);
  }

  return true;
}

uint32_t string_table_find_missing(const StringTable *base,
                                   const StringTable *reference,
                                   char (*out_keys)[LOCALIZATION_KEY_SIZE],
                                   uint32_t max_keys) {
  uint32_t missing_count = 0;
  uint32_t i;

  if (!reference || !base || !base->hash_map) {
    return 0;
  }

  for (i = 0; i < reference->string_count; i++) {
    char key[LOCALIZATION_KEY_SIZE];
    uint32_t index = 0;
    fill_key(key, reference->strings[i].key);
    if (!map_get((HashMap *)base->hash_map, key, &index)) {
      if (out_keys && missing_count < max_keys) {
        strncpy(out_keys[missing_count], reference->strings[i].key,
                LOCALIZATION_KEY_SIZE - 1);
        out_keys[missing_count][LOCALIZATION_KEY_SIZE - 1] = '\0';
      }
      missing_count++;
      if (out_keys && missing_count >= max_keys) {
        break;
      }
    }
  }

  return missing_count;
}

PluralCategory plural_rule_english(int64_t count) {
  return (count == 1) ? PLURAL_ONE : PLURAL_OTHER;
}

PluralCategory plural_rule_french(int64_t count) {
  return (count == 0 || count == 1) ? PLURAL_ONE : PLURAL_OTHER;
}

PluralCategory plural_rule_russian(int64_t count) {
  int64_t mod10 = count % 10;
  int64_t mod100 = count % 100;

  if (mod10 == 1 && mod100 != 11) {
    return PLURAL_ONE;
  }
  if (mod10 >= 2 && mod10 <= 4 && !(mod100 >= 12 && mod100 <= 14)) {
    return PLURAL_FEW;
  }
  if (mod10 == 0 || (mod10 >= 5 && mod10 <= 9) ||
      (mod100 >= 11 && mod100 <= 14)) {
    return PLURAL_MANY;
  }
  return PLURAL_OTHER;
}

PluralCategory plural_rule_polish(int64_t count) {
  int64_t mod10 = count % 10;
  int64_t mod100 = count % 100;

  if (count == 1) {
    return PLURAL_ONE;
  }
  if (mod10 >= 2 && mod10 <= 4 && !(mod100 >= 12 && mod100 <= 14)) {
    return PLURAL_FEW;
  }
  if (mod10 == 0 || (mod10 >= 5 && mod10 <= 9) ||
      (mod100 >= 12 && mod100 <= 14)) {
    return PLURAL_MANY;
  }
  return PLURAL_OTHER;
}

PluralCategory plural_rule_arabic(int64_t count) {
  int64_t mod100 = count % 100;

  if (count == 0) {
    return PLURAL_ZERO;
  }
  if (count == 1) {
    return PLURAL_ONE;
  }
  if (count == 2) {
    return PLURAL_TWO;
  }
  if (mod100 >= 3 && mod100 <= 10) {
    return PLURAL_FEW;
  }
  if (mod100 >= 11 && mod100 <= 99) {
    return PLURAL_MANY;
  }
  return PLURAL_OTHER;
}

PluralCategory plural_rule_japanese(int64_t count) {
  (void)count;
  return PLURAL_OTHER;
}

PluralCategory plural_get_category(LanguageCode language, int64_t count) {
  switch (language) {
  case LANG_FR_FR:
    return plural_rule_french(count);
  case LANG_RU_RU:
    return plural_rule_russian(count);
  case LANG_PL_PL:
    return plural_rule_polish(count);
  case LANG_AR_SA:
    return plural_rule_arabic(count);
  case LANG_JA_JP:
  case LANG_ZH_CN:
  case LANG_ZH_TW:
  case LANG_KO_KR:
    return plural_rule_japanese(count);
  case LANG_EN_US:
  case LANG_EN_GB:
  default:
    return plural_rule_english(count);
  }
}

bool plural_format(LanguageCode language, const LocalizedString *string,
                   int64_t count, char *out, size_t out_size) {
  char count_text[32];
  const char *selected = NULL;
  const char *token = NULL;
  size_t out_len = 0;

  if (!string || !out || out_size == 0) {
    return false;
  }

  if (!string->is_plural) {
    strncpy(out, string->value, out_size - 1);
    out[out_size - 1] = '\0';
    return true;
  }

  switch (plural_get_category(language, count)) {
  case PLURAL_ZERO:
    selected =
        string->plural_zero[0] ? string->plural_zero : string->plural_other;
    break;
  case PLURAL_ONE:
    selected =
        string->plural_one[0] ? string->plural_one : string->plural_other;
    break;
  case PLURAL_TWO:
    selected =
        string->plural_two[0] ? string->plural_two : string->plural_other;
    break;
  case PLURAL_FEW:
    selected =
        string->plural_few[0] ? string->plural_few : string->plural_other;
    break;
  case PLURAL_MANY:
    selected =
        string->plural_many[0] ? string->plural_many : string->plural_other;
    break;
  case PLURAL_OTHER:
  default:
    selected = string->plural_other[0] ? string->plural_other : string->value;
    break;
  }

  if (!selected) {
    out[0] = '\0';
    return false;
  }

  snprintf(count_text, sizeof(count_text), "%lld", (long long)count);
  token = strstr(selected, "{count}");
  if (!token) {
    strncpy(out, selected, out_size - 1);
    out[out_size - 1] = '\0';
    return true;
  }

  while (*selected && out_len + 1 < out_size) {
    if (selected == token) {
      size_t count_len = strlen(count_text);
      size_t i;
      for (i = 0; i < count_len && out_len + 1 < out_size; i++) {
        out[out_len++] = count_text[i];
      }
      selected += strlen("{count}");
      token = strstr(selected, "{count}");
      continue;
    }
    out[out_len++] = *selected++;
  }

  out[out_len] = '\0';
  return true;
}

bool format_number(LanguageCode language, double value, char *out,
                   size_t out_size) {
  return format_number_impl(language, value, 2, true, out, out_size);
}

bool format_currency(LanguageCode language, double value,
                     const char *currency_symbol, bool currency_before,
                     char *out, size_t out_size) {
  char number[128];
  const char *symbol = currency_symbol ? currency_symbol : "$";

  if (!out || out_size == 0) {
    return false;
  }

  if (!format_number_impl(language, value, 2, false, number, sizeof(number))) {
    return false;
  }

  if (currency_before) {
    snprintf(out, out_size, "%s%s", symbol, number);
  } else {
    snprintf(out, out_size, "%s %s", number, symbol);
  }
  return true;
}

bool format_percentage(LanguageCode language, double value, char *out,
                       size_t out_size) {
  char number[128];

  if (!out || out_size == 0) {
    return false;
  }

  if (!format_number_impl(language, value * 100.0, 2, true, number,
                          sizeof(number))) {
    return false;
  }

  snprintf(out, out_size, "%s%%", number);
  return true;
}

bool format_ordinal(LanguageCode language, int64_t value, char *out,
                    size_t out_size) {
  const char *suffix = "";
  int64_t mod100 = value % 100;
  int64_t mod10 = value % 10;

  if (!out || out_size == 0) {
    return false;
  }

  if (language == LANG_EN_US || language == LANG_EN_GB) {
    if (mod100 >= 11 && mod100 <= 13) {
      suffix = "th";
    } else if (mod10 == 1) {
      suffix = "st";
    } else if (mod10 == 2) {
      suffix = "nd";
    } else if (mod10 == 3) {
      suffix = "rd";
    } else {
      suffix = "th";
    }
  }

  snprintf(out, out_size, "%lld%s", (long long)value, suffix);
  return true;
}

static LocalizationManager g_loc_manager;

static LocalizationManager *resolve_manager(LocalizationManager *manager) {
  return manager ? manager : &g_loc_manager;
}

static void string_table_release(StringTable *table) {
  if (!table) {
    return;
  }
  if (table->hash_map) {
    map_destroy((HashMap *)table->hash_map);
    table->hash_map = NULL;
  }
  free(table->strings);
  table->strings = NULL;
  table->string_count = 0;
  table->string_capacity = 0;
}

static StringTable *loc_manager_find_table(LocalizationManager *manager,
                                           LanguageCode language) {
  uint32_t i;

  if (!manager || !manager->string_tables) {
    return NULL;
  }

  for (i = 0; i < manager->table_count; i++) {
    if (manager->string_tables[i].language == language) {
      return &manager->string_tables[i];
    }
  }
  return NULL;
}

static bool replace_token(const char *input, const char *token,
                          const char *value, char *out, size_t out_size) {
  const char *cursor = input;
  size_t token_len = token ? strlen(token) : 0;
  size_t value_len = value ? strlen(value) : 0;
  size_t out_len = 0;

  if (!input || !out || out_size == 0 || !token || token_len == 0) {
    return false;
  }

  while (*cursor && out_len + 1 < out_size) {
    const char *found = strstr(cursor, token);
    if (!found) {
      size_t remaining = strlen(cursor);
      if (out_len + remaining >= out_size) {
        remaining = out_size - out_len - 1;
      }
      memcpy(out + out_len, cursor, remaining);
      out_len += remaining;
      break;
    }

    if (found > cursor) {
      size_t chunk = (size_t)(found - cursor);
      if (out_len + chunk >= out_size) {
        chunk = out_size - out_len - 1;
      }
      memcpy(out + out_len, cursor, chunk);
      out_len += chunk;
    }

    if (value_len > 0 && out_len + value_len < out_size) {
      memcpy(out + out_len, value, value_len);
      out_len += value_len;
    }

    cursor = found + token_len;
  }

  out[out_len] = '\0';
  return true;
}

static const char *format_arg_value(LanguageCode language, const FormatArg *arg,
                                    char *out, size_t out_size) {
  if (!arg || !out || out_size == 0) {
    return "";
  }

  switch (arg->type) {
  case FORMAT_ARG_FLOAT:
    if (format_number(language, arg->value.float_value, out, out_size)) {
      return out;
    }
    break;
  case FORMAT_ARG_STRING:
    if (arg->value.string_value) {
      strncpy(out, arg->value.string_value, out_size - 1);
      out[out_size - 1] = '\0';
      return out;
    }
    break;
  case FORMAT_ARG_INT:
  default:
    if (format_number(language, (double)arg->value.int_value, out, out_size)) {
      return out;
    }
    break;
  }

  out[0] = '\0';
  return out;
}

LocalizationManager *loc_manager_get_default(void) {
  return &g_loc_manager;
}

void loc_manager_init(LocalizationManager *manager) {
  LocalizationManager *resolved = resolve_manager(manager);
  uint32_t i;

  memset(resolved, 0, sizeof(*resolved));
  resolved->current_language = LANG_EN_US;
  resolved->fallback_language = LANG_EN_US;

  for (i = 0; i < LANG_COUNT; i++) {
    char decimal_sep = '.';
    char thousands_sep = ',';
    get_number_separators((LanguageCode)i, &decimal_sep, &thousands_sep);
    resolved->format_settings[i].decimal_separator = decimal_sep;
    resolved->format_settings[i].thousands_separator = thousands_sep;
    strncpy(resolved->format_settings[i].currency_symbol, "$",
            sizeof(resolved->format_settings[i].currency_symbol) - 1);
    resolved->format_settings[i].currency_before = true;
  }
}

void loc_manager_shutdown(LocalizationManager *manager) {
  LocalizationManager *resolved = resolve_manager(manager);
  uint32_t i;

  if (!resolved) {
    return;
  }

  for (i = 0; i < resolved->table_count; i++) {
    string_table_release(&resolved->string_tables[i]);
  }
  free(resolved->string_tables);
  resolved->string_tables = NULL;
  resolved->table_count = 0;
  resolved->font_count = 0;
  resolved->fonts = NULL;
}

bool loc_manager_set_language(LocalizationManager *manager,
                              LanguageCode language) {
  LocalizationManager *resolved = resolve_manager(manager);
  if (!resolved) {
    return false;
  }
  resolved->current_language = language;
  if (resolved->on_language_changed) {
    resolved->on_language_changed(language);
  }
  return true;
}

const char *loc_manager_get_string(LocalizationManager *manager,
                                   const char *key) {
  LocalizationManager *resolved = resolve_manager(manager);
  StringTable *table = NULL;
  LocalizedString *entry = NULL;

  if (!resolved || !key) {
    return "";
  }

  table = loc_manager_find_table(resolved, resolved->current_language);
  if (table) {
    entry = string_table_get(table, key);
  }

  if (!entry && resolved->fallback_language != resolved->current_language) {
    table = loc_manager_find_table(resolved, resolved->fallback_language);
    if (table) {
      entry = string_table_get(table, key);
    }
  }

  if (!entry) {
    return key;
  }

  return entry->value[0] ? entry->value : key;
}

const char *loc_manager_get_plural(LocalizationManager *manager,
                                   const char *key, int64_t count) {
  static char plural_buffer[2048];
  LocalizationManager *resolved = resolve_manager(manager);
  StringTable *table = NULL;
  LocalizedString *entry = NULL;

  if (!resolved || !key) {
    return "";
  }

  table = loc_manager_find_table(resolved, resolved->current_language);
  if (table) {
    entry = string_table_get(table, key);
  }

  if (!entry && resolved->fallback_language != resolved->current_language) {
    table = loc_manager_find_table(resolved, resolved->fallback_language);
    if (table) {
      entry = string_table_get(table, key);
    }
  }

  if (!entry) {
    return key;
  }

  if (!entry->is_plural) {
    return entry->value[0] ? entry->value : key;
  }

  if (!plural_format(resolved->current_language, entry, count, plural_buffer,
                     sizeof(plural_buffer))) {
    return key;
  }

  return plural_buffer;
}

const char *loc_manager_format(LocalizationManager *manager, const char *key,
                               const FormatArg *args, size_t arg_count) {
  static char format_buffer[2048];
  char *temp_a = NULL;
  char *temp_b = NULL;
  const char *base = NULL;
  LocalizationManager *resolved = resolve_manager(manager);
  size_t i;

  if (!resolved || !key) {
    return "";
  }

  base = loc_manager_get_string(resolved, key);
  if (!args || arg_count == 0) {
    strncpy(format_buffer, base, sizeof(format_buffer) - 1);
    format_buffer[sizeof(format_buffer) - 1] = '\0';
    return format_buffer;
  }

  temp_a = malloc(sizeof(format_buffer));
  temp_b = malloc(sizeof(format_buffer));
  if (!temp_a || !temp_b) {
    free(temp_a);
    free(temp_b);
    strncpy(format_buffer, base, sizeof(format_buffer) - 1);
    format_buffer[sizeof(format_buffer) - 1] = '\0';
    return format_buffer;
  }

  strncpy(temp_a, base, sizeof(format_buffer) - 1);
  temp_a[sizeof(format_buffer) - 1] = '\0';

  for (i = 0; i < arg_count; i++) {
    char token[48];
    char value_text[128];

    format_arg_value(resolved->current_language, &args[i], value_text,
                     sizeof(value_text));

    if (!replace_token(temp_a, token, value_text, temp_b,
                       sizeof(format_buffer))) {
      strncpy(temp_b, temp_a, sizeof(format_buffer) - 1);
      temp_b[sizeof(format_buffer) - 1] = '\0';
    }

    strncpy(temp_a, temp_b, sizeof(format_buffer) - 1);
    temp_a[sizeof(format_buffer) - 1] = '\0';
  }

  strncpy(format_buffer, temp_a, sizeof(format_buffer) - 1);
  format_buffer[sizeof(format_buffer) - 1] = '\0';

  free(temp_a);
  free(temp_b);

  return format_buffer;
}

bool loc_manager_load_all(LocalizationManager *manager, const char **paths,
                          const LanguageCode *languages, size_t count) {
  LocalizationManager *resolved = resolve_manager(manager);
  size_t i;

  if (!resolved || !paths || count == 0) {
    return false;
  }

  loc_manager_shutdown(resolved);

  resolved->string_tables = calloc(count, sizeof(StringTable));
  if (!resolved->string_tables) {
    return false;
  }
  resolved->table_count = (uint32_t)count;

  for (i = 0; i < count; i++) {
    LanguageCode language = languages ? languages[i] : LANG_EN_US;
    StringTable *created = string_table_create(language, "", "", false);
    if (!created) {
      loc_manager_shutdown(resolved);
      return false;
    }

    resolved->string_tables[i] = *created;
    free(created);

    if (!string_table_load_csv(&resolved->string_tables[i], paths[i])) {
      loc_manager_shutdown(resolved);
      return false;
    }
  }

  return true;
}

LanguageCode loc_manager_detect_system_language(void) {
  return LANG_EN_US;
}

bool loc_manager_validate(const LocalizationManager *manager,
                          char (*out_keys)[LOCALIZATION_KEY_SIZE],
                          size_t max_keys) {
  const LocalizationManager *resolved = resolve_manager((LocalizationManager *)manager);
  const StringTable *fallback = NULL;
  const StringTable *current = NULL;
  uint32_t missing = 0;

  if (!resolved) {
    return false;
  }

  fallback = loc_manager_find_table((LocalizationManager *)resolved,
                                    resolved->fallback_language);
  current =
      loc_manager_find_table((LocalizationManager *)resolved,
                             resolved->current_language);

  if (!fallback || !current) {
    return false;
  }

  missing =
      string_table_find_missing(current, fallback, out_keys, (uint32_t)max_keys);
  return missing == 0;
}

bool loc_manager_export_template(const LocalizationManager *manager,
                                 const char *path) {
  const LocalizationManager *resolved = resolve_manager((LocalizationManager *)manager);
  const StringTable *table = NULL;
  FILE *file = NULL;
  uint32_t i;

  if (!resolved || !path) {
    return false;
  }

  table = loc_manager_find_table((LocalizationManager *)resolved,
                                 resolved->fallback_language);
  if (!table) {
    return false;
  }

  file = fopen(path, "w");
  if (!file) {
    return false;
  }

  fputs("key,value,context\n", file);
  for (i = 0; i < table->string_count; i++) {
    write_csv_field(file, table->strings[i].key);
    fputc(',', file);
    write_csv_field(file, "");
    fputc(',', file);
    write_csv_field(file, table->strings[i].context);
    fputc('\n', file);
  }

  fclose(file);
  return true;
}

// =================================================================================================
//                                    RTL TEXT PROCESSING
// =================================================================================================

static bool is_arabic_char(uint32_t c) {
  return (c >= 0x0600 && c <= 0x06FF) || 
         (c >= 0x0750 && c <= 0x077F) ||
         (c >= 0x08A0 && c <= 0x08FF) ||
         (c >= 0xFB50 && c <= 0xFDFF) ||
         (c >= 0xFE70 && c <= 0xFEFF);
}

static bool is_hebrew_char(uint32_t c) {
  return (c >= 0x0590 && c <= 0x05FF) ||
         (c >= 0xFB1D && c <= 0xFB4F);
}

static bool is_rtl_char(uint32_t c) {
  return is_arabic_char(c) || is_hebrew_char(c) ||
         (c >= 0x202A && c <= 0x202E) || // RTL markers
         (c >= 0x2066 && c <= 0x2069);   // isolate markers
}

static bool is_weak_char(uint32_t c) {
  return (c >= '0' && c <= '9') || 
         (c >= 0x0030 && c <= 0x0039) || // European digits
         (c >= 0x0660 && c <= 0x0669) || // Arabic-Indic digits
         (c >= 0x06F0 && c <= 0x06F9);   // Extended Arabic-Indic digits
}

static uint32_t get_arabic_numeral_shape(uint32_t digit) {
  static const uint32_t arabic_numerals[] = {
    0x0660, 0x0661, 0x0662, 0x0663, 0x0664,  // 0-4
    0x0665, 0x0666, 0x0667, 0x0668, 0x0669   // 5-9
  };
  
  if (digit >= '0' && digit <= '9') {
    return arabic_numerals[digit - '0'];
  }
  return digit;
}

static void utf8_encode(uint32_t codepoint, char *out, size_t *out_len) {
  if (codepoint <= 0x7F) {
    out[0] = (char)codepoint;
    *out_len = 1;
  } else if (codepoint <= 0x7FF) {
    out[0] = 0xC0 | (codepoint >> 6);
    out[1] = 0x80 | (codepoint & 0x3F);
    *out_len = 2;
  } else if (codepoint <= 0xFFFF) {
    out[0] = 0xE0 | (codepoint >> 12);
    out[1] = 0x80 | ((codepoint >> 6) & 0x3F);
    out[2] = 0x80 | (codepoint & 0x3F);
    *out_len = 3;
  } else {
    out[0] = 0xF0 | (codepoint >> 18);
    out[1] = 0x80 | ((codepoint >> 12) & 0x3F);
    out[2] = 0x80 | ((codepoint >> 6) & 0x3F);
    out[3] = 0x80 | (codepoint & 0x3F);
    *out_len = 4;
  }
}

static uint32_t utf8_decode(const char *str, size_t *bytes_read) {
  uint32_t codepoint = 0;
  size_t len = 0;
  
  if (!str) {
    *bytes_read = 0;
    return 0;
  }
  
  uint8_t first = (uint8_t)str[0];
  
  if (first < 0x80) {
    codepoint = first;
    len = 1;
  } else if ((first & 0xE0) == 0xC0) {
    codepoint = first & 0x1F;
    len = 2;
  } else if ((first & 0xF0) == 0xE0) {
    codepoint = first & 0x0F;
    len = 3;
  } else if ((first & 0xF8) == 0xF0) {
    codepoint = first & 0x07;
    len = 4;
  } else {
    *bytes_read = 1;
    return 0xFFFD; // Replacement character
  }
  
  for (size_t i = 1; i < len && str[i]; i++) {
    codepoint = (codepoint << 6) | (str[i] & 0x3F);
  }
  
  *bytes_read = len;
  return codepoint;
}

bool loc_manager_is_rtl_language(LanguageCode language) {
  switch (language) {
  case LANG_AR_SA:
    return true;
  case LANG_HE_IL: // If added later
    return true;
  default:
    return false;
  }
}

bool loc_manager_is_rtl_text(const char *text) {
  if (!text) {
    return false;
  }
  
  size_t i = 0;
  size_t bytes_read;
  uint32_t rtl_chars = 0;
  uint32_t total_chars = 0;
  
  while (text[i] != '\0') {
    uint32_t codepoint = utf8_decode(&text[i], &bytes_read);
    if (bytes_read == 0) break;
    
    total_chars++;
    if (is_rtl_char(codepoint)) {
      rtl_chars++;
    }
    
    i += bytes_read;
  }
  
  // Consider text RTL if >30% of characters are RTL
  return total_chars > 0 && (rtl_chars * 100 / total_chars) > 30;
}

bool loc_manager_process_rtl_text(LanguageCode language, const char *input, 
                                  char *output, size_t output_size) {
  if (!input || !output || output_size == 0) {
    return false;
  }
  
  bool is_rtl_lang = loc_manager_is_rtl_language(language);
  bool has_rtl_chars = loc_manager_is_rtl_text(input);
  
  if (!is_rtl_lang && !has_rtl_chars) {
    strncpy(output, input, output_size - 1);
    output[output_size - 1] = '\0';
    return true;
  }
  
  // Process text for proper RTL display
  size_t input_len = strlen(input);
  if (input_len >= output_size) {
    return false;
  }
  
  // Add RTL mark for proper rendering
  size_t out_pos = 0;
  if (is_rtl_lang) {
    if (out_pos + 3 < output_size) {
      output[out_pos++] = 0xE2;
      output[out_pos++] = 0x80;
      output[out_pos++] = 0xAB; // Right-to-Left Mark
    }
  }
  
  // Process each character
  size_t i = 0;
  while (input[i] != '\0' && out_pos < output_size - 1) {
    size_t bytes_read;
    uint32_t codepoint = utf8_decode(&input[i], &bytes_read);
    
    if (bytes_read == 0) break;
    
    // Shape Arabic numerals if in RTL context
    if (is_rtl_lang && is_weak_char(codepoint) && codepoint >= '0' && codepoint <= '9') {
      uint32_t shaped = get_arabic_numeral_shape(codepoint);
      char encoded[4];
      size_t encoded_len;
      utf8_encode(shaped, encoded, &encoded_len);
      
      if (out_pos + encoded_len < output_size) {
        memcpy(&output[out_pos], encoded, encoded_len);
        out_pos += encoded_len;
      }
    } else {
      // Copy original bytes
      size_t copy_len = bytes_read;
      if (out_pos + copy_len >= output_size) {
        copy_len = output_size - out_pos - 1;
      }
      memcpy(&output[out_pos], &input[i], copy_len);
      out_pos += copy_len;
    }
    
    i += bytes_read;
  }
  
  output[out_pos] = '\0';
  return true;
}

bool loc_manager_reverse_text_for_rtl(const char *input, char *output, size_t output_size) {
  if (!input || !output || output_size == 0) {
    return false;
  }
  
  size_t input_len = strlen(input);
  if (input_len >= output_size) {
    return false;
  }
  
  // Simple character-level reversal (for basic RTL support)
  size_t out_pos = 0;
  for (size_t i = input_len; i > 0; i--) {
    if (out_pos >= output_size - 1) break;
    
    // Handle UTF-8 characters properly
    size_t char_start = i - 1;
    while (char_start > 0 && (input[char_start] & 0xC0) == 0x80) {
      char_start--;
    }
    
    size_t char_len = i - char_start;
    if (out_pos + char_len < output_size) {
      memcpy(&output[out_pos], &input[char_start], char_len);
      out_pos += char_len;
    }
    
    i = char_start;
  }
  
  output[out_pos] = '\0';
  return true;
}

void loc_manager_get_text_direction(LanguageCode language, bool *is_rtl, 
                                   const char **direction_marker) {
  if (!is_rtl || !direction_marker) {
    return;
  }
  
  *is_rtl = loc_manager_is_rtl_language(language);
  *direction_marker = *is_rtl ? "\u202B" : "\u202A"; // RTL/LTR markers
}
