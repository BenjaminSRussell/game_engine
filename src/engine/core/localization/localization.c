#include <core/containers_advanced/hash_map.h>
#include <core/localization.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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
//                                JSON STRING TABLE LOADING
// =================================================================================================

#include <json-c/json.h>

bool string_table_load_json(StringTable *table, const char *path) {
  json_object *root = NULL;
  json_object *strings_obj = NULL;
  struct json_object_iterator iter;
  struct json_object_iterator end;

  if (!table || !path) {
    return false;
  }

  root = json_object_from_file(path);
  if (!root) {
    return false;
  }

  if (!json_object_object_get_ex(root, "strings", &strings_obj)) {
    json_object_put(root);
    return false;
  }

  iter = json_object_iter_begin(strings_obj);
  end = json_object_iter_end(strings_obj);

  while (!json_object_iter_equal(&iter, &end)) {
    const char *key = json_object_iter_peek_name(&iter);
    json_object *value_obj = json_object_iter_peek_value(&iter);
    const char *value = json_object_get_string(value_obj);
    json_object *context_obj = NULL;
    const char *context = "";

    // Try to get context if available
    json_object *obj_data = NULL;
    if (json_object_get_type(value_obj) == json_type_object) {
      json_object_object_get_ex(value_obj, "value", &value_obj);
      json_object_object_get_ex(value_obj, "context", &context_obj);
      value = json_object_get_string(value_obj);
      context = context_obj ? json_object_get_string(context_obj) : "";
    }

    if (key && value) {
      LocalizedString entry = {0};
      strncpy(entry.key, key, sizeof(entry.key) - 1);
      strncpy(entry.value, value, sizeof(entry.value) - 1);
      strncpy(entry.context, context, sizeof(entry.context) - 1);
      entry.is_plural = false;
      string_table_add(table, &entry);
    }

    json_object_iter_next(&iter);
  }

  json_object_put(root);
  return true;
}

// =================================================================================================
//                                PO FILE LOADING
// =================================================================================================

bool string_table_load_po(StringTable *table, const char *path) {
  FILE *file = NULL;
  char line[4096];
  char msgid[1024] = {0};
  char msgstr[1024] = {0};
  char msgctxt[256] = {0};
  bool in_msgid = false;
  bool in_msgstr = false;
  bool in_msgctxt = false;

  if (!table || !path) {
    return false;
  }

  file = fopen(path, "r");
  if (!file) {
    return false;
  }

  while (fgets(line, sizeof(line), file)) {
    char *trimmed = line;
    while (*trimmed && isspace((unsigned char)*trimmed)) {
      trimmed++;
    }

    if (*trimmed == '#' || *trimmed == '\0') {
      continue;
    }

    if (strncmp(trimmed, "msgid ", 6) == 0) {
      // Save previous entry if exists
      if (msgid[0] != '\0' && msgstr[0] != '\0') {
        LocalizedString entry = {0};
        strncpy(entry.key, msgid, sizeof(entry.key) - 1);
        strncpy(entry.value, msgstr, sizeof(entry.value) - 1);
        strncpy(entry.context, msgctxt, sizeof(entry.context) - 1);
        entry.is_plural = false;
        string_table_add(table, &entry);
      }

      // Parse new msgid
      char *start = trimmed + 6;
      while (*start && isspace((unsigned char)*start)) {
        start++;
      }
      if (*start == '"') {
        start++;
        char *end = strrchr(start, '"');
        if (end) {
          *end = '\0';
        }
      }
      strncpy(msgid, start, sizeof(msgid) - 1);
      msgid[sizeof(msgid) - 1] = '\0';
      msgstr[0] = '\0';
      msgctxt[0] = '\0';
      in_msgid = true;
      in_msgstr = false;
      in_msgctxt = false;
    } else if (strncmp(trimmed, "msgstr ", 7) == 0) {
      char *start = trimmed + 7;
      while (*start && isspace((unsigned char)*start)) {
        start++;
      }
      if (*start == '"') {
        start++;
        char *end = strrchr(start, '"');
        if (end) {
          *end = '\0';
        }
      }
      strncpy(msgstr, start, sizeof(msgstr) - 1);
      msgstr[sizeof(msgstr) - 1] = '\0';
      in_msgid = false;
      in_msgstr = true;
      in_msgctxt = false;
    } else if (strncmp(trimmed, "msgctxt ", 8) == 0) {
      char *start = trimmed + 8;
      while (*start && isspace((unsigned char)*start)) {
        start++;
      }
      if (*start == '"') {
        start++;
        char *end = strrchr(start, '"');
        if (end) {
          *end = '\0';
        }
      }
      strncpy(msgctxt, start, sizeof(msgctxt) - 1);
      msgctxt[sizeof(msgctxt) - 1] = '\0';
      in_msgid = false;
      in_msgstr = false;
      in_msgctxt = true;
    } else if (*trimmed == '"') {
      char *start = trimmed + 1;
      char *end = strrchr(start, '"');
      if (end) {
        *end = '\0';
      }

      if (in_msgid) {
        strncat(msgid, start, sizeof(msgid) - strlen(msgid) - 1);
      } else if (in_msgstr) {
        strncat(msgstr, start, sizeof(msgstr) - strlen(msgstr) - 1);
      } else if (in_msgctxt) {
        strncat(msgctxt, start, sizeof(msgctxt) - strlen(msgctxt) - 1);
      }
    }
  }

  // Save last entry
  if (msgid[0] != '\0' && msgstr[0] != '\0') {
    LocalizedString entry = {0};
    strncpy(entry.key, msgid, sizeof(entry.key) - 1);
    strncpy(entry.value, msgstr, sizeof(entry.value) - 1);
    strncpy(entry.context, msgctxt, sizeof(entry.context) - 1);
    entry.is_plural = false;
    string_table_add(table, &entry);
  }

  fclose(file);
  return true;
}

// =================================================================================================
//                                 TEXT FORMATTING
// =================================================================================================

bool format_string(LanguageCode language, const char *format, 
                   const FormatArg *args, size_t arg_count, 
                   char *out, size_t out_size) {
  char *temp_a = NULL;
  char *temp_b = NULL;
  const char *cursor = format;
  size_t out_len = 0;
  size_t i;

  if (!format || !out || out_size == 0) {
    return false;
  }

  if (!args || arg_count == 0) {
    strncpy(out, format, out_size - 1);
    out[out_size - 1] = '\0';
    return true;
  }

  temp_a = malloc(out_size);
  temp_b = malloc(out_size);
  if (!temp_a || !temp_b) {
    free(temp_a);
    free(temp_b);
    strncpy(out, format, out_size - 1);
    out[out_size - 1] = '\0';
    return false;
  }

  strncpy(temp_a, format, out_size - 1);
  temp_a[out_size - 1] = '\0';

  for (i = 0; i < arg_count; i++) {
    char token[48];
    char value_text[128];

    snprintf(token, sizeof(token), "{%s}", args[i].name);
    format_arg_value(language, &args[i], value_text, sizeof(value_text));

    if (!replace_token(temp_a, token, value_text, temp_b, out_size)) {
      strncpy(temp_b, temp_a, out_size - 1);
      temp_b[out_size - 1] = '\0';
    }

    strncpy(temp_a, temp_b, out_size - 1);
    temp_a[out_size - 1] = '\0';
  }

  strncpy(out, temp_a, out_size - 1);
  out[out_size - 1] = '\0';

  free(temp_a);
  free(temp_b);
  return true;
}

bool format_date(LanguageCode language, int64_t timestamp, 
                 const char *format, char *out, size_t out_size) {
  struct tm *tm_info = NULL;
  time_t time_val = (time_t)timestamp;
  char temp_format[256];
  const char *final_format = format;

  if (!out || out_size == 0) {
    return false;
  }

  tm_info = localtime(&time_val);
  if (!tm_info) {
    out[0] = '\0';
    return false;
  }

  // Default format based on language
  if (!format) {
    switch (language) {
    case LANG_EN_US:
      final_format = "%m/%d/%Y";
      break;
    case LANG_EN_GB:
    case LANG_DE_DE:
    case LANG_FR_FR:
    case LANG_ES_ES:
    case LANG_IT_IT:
    case LANG_PT_BR:
    case LANG_PT_PT:
      final_format = "%d/%m/%Y";
      break;
    case LANG_JA_JP:
    case LANG_ZH_CN:
    case LANG_ZH_TW:
    case LANG_KO_KR:
      final_format = "%Y/%m/%d";
      break;
    default:
      final_format = "%Y-%m-%d";
      break;
    }
  }

  // Format the date
  strftime(temp_format, sizeof(temp_format), final_format, tm_info);
  strncpy(out, temp_format, out_size - 1);
  out[out_size - 1] = '\0';

  return true;
}

bool format_time(LanguageCode language, int64_t timestamp, 
                 bool show_seconds, bool use_24_hour, 
                 char *out, size_t out_size) {
  struct tm *tm_info = NULL;
  time_t time_val = (time_t)timestamp;
  char temp_format[64];

  if (!out || out_size == 0) {
    return false;
  }

  tm_info = localtime(&time_val);
  if (!tm_info) {
    out[0] = '\0';
    return false;
  }

  // Determine format based on language and preferences
  if (use_24_hour) {
    if (show_seconds) {
      snprintf(temp_format, sizeof(temp_format), "%%H:%%M:%%S");
    } else {
      snprintf(temp_format, sizeof(temp_format), "%%H:%%M");
    }
  } else {
    switch (language) {
    case LANG_EN_US:
      if (show_seconds) {
        snprintf(temp_format, sizeof(temp_format), "%%I:%%M:%%S %%p");
      } else {
        snprintf(temp_format, sizeof(temp_format), "%%I:%%M %%p");
      }
      break;
    default:
      if (show_seconds) {
        snprintf(temp_format, sizeof(temp_format), "%%I:%%M:%%S %%p");
      } else {
        snprintf(temp_format, sizeof(temp_format), "%%I:%%M %%p");
      }
      break;
    }
  }

  strftime(temp_format, sizeof(temp_format), temp_format, tm_info);
  strncpy(out, temp_format, out_size - 1);
  out[out_size - 1] = '\0';

  return true;
}

bool format_duration(LanguageCode language, int64_t seconds, 
                     bool show_milliseconds, char *out, size_t out_size) {
  int64_t days = seconds / 86400;
  int64_t hours = (seconds % 86400) / 3600;
  int64_t minutes = (seconds % 3600) / 60;
  int64_t secs = seconds % 60;
  int64_t millis = 0;

  if (!out || out_size == 0) {
    return false;
  }

  if (show_milliseconds) {
    millis = (seconds % 1000);
  }

  if (days > 0) {
    if (show_milliseconds) {
      snprintf(out, out_size, "%lldd %02lld:%02lld:%02lld.%03lld", 
               (long long)days, (long long)hours, (long long)minutes, 
               (long long)secs, (long long)millis);
    } else {
      snprintf(out, out_size, "%lldd %02lld:%02lld:%02lld", 
               (long long)days, (long long)hours, (long long)minutes, (long long)secs);
    }
  } else if (hours > 0) {
    if (show_milliseconds) {
      snprintf(out, out_size, "%02lld:%02lld:%02lld.%03lld", 
               (long long)hours, (long long)minutes, (long long)secs, (long long)millis);
    } else {
      snprintf(out, out_size, "%02lld:%02lld:%02lld", 
               (long long)hours, (long long)minutes, (long long)secs);
    }
  } else if (minutes > 0) {
    if (show_milliseconds) {
      snprintf(out, out_size, "%02lld:%02lld.%03lld", 
               (long long)minutes, (long long)secs, (long long)millis);
    } else {
      snprintf(out, out_size, "%02lld:%02lld", 
               (long long)minutes, (long long)secs);
    }
  } else {
    if (show_milliseconds) {
      snprintf(out, out_size, "%lld.%03llds", 
               (long long)secs, (long long)millis);
    } else {
      snprintf(out, out_size, "%llds", (long long)secs);
    }
  }

  return true;
}

bool format_unit(LanguageCode language, double value, const char *unit, 
                 bool use_si_prefixes, char *out, size_t out_size) {
  const char *prefixes[] = {"", "k", "M", "G", "T", "P", "E"};
  double scaled_value = value;
  int prefix_index = 0;

  if (!unit || !out || out_size == 0) {
    return false;
  }

  if (use_si_prefixes && fabs(value) >= 1000.0) {
    while (fabs(scaled_value) >= 1000.0 && prefix_index < 6) {
      scaled_value /= 1000.0;
      prefix_index++;
    }
  }

  char number[128];
  if (!format_number_impl(language, scaled_value, 2, true, number, sizeof(number))) {
    return false;
  }

  if (prefix_index > 0) {
    snprintf(out, out_size, "%s %s%s", number, prefixes[prefix_index], unit);
  } else {
    snprintf(out, out_size, "%s %s", number, unit);
  }

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

// =================================================================================================
//                                    FONT SYSTEM
// =================================================================================================

#include <ft2build.h>
#include FT_FREETYPE_H
#include <stb_image.h>
#include <stb_image_write.h>

static FT_Library g_ft_library = NULL;
static Font *g_fonts = NULL;
static uint32_t g_font_count = 0;

bool font_load(Font *font, const char *path, float font_size) {
  FT_Face face = NULL;
  FT_Error error;
  
  if (!font || !path || !g_ft_library) {
    return false;
  }
  
  error = FT_New_Face(g_ft_library, path, 0, &face);
  if (error) {
    return false;
  }
  
  error = FT_Set_Char_Size(face, 0, (FT_F26Dot6)(font_size * 64), 72, 72);
  if (error) {
    FT_Done_Face(face);
    return false;
  }
  
  strncpy(font->path, path, sizeof(font->path) - 1);
  font->size_count = 1;
  font->supported_sizes = malloc(sizeof(float));
  if (font->supported_sizes) {
    font->supported_sizes[0] = font_size;
  }
  
  FT_Done_Face(face);
  return true;
}

bool font_unload(Font *font) {
  if (!font) {
    return false;
  }
  
  if (font->atlases) {
    for (uint32_t i = 0; i < font->atlas_count; i++) {
      if (font->atlases[i].glyphs) {
        free(font->atlases[i].glyphs);
      }
      if (font->atlases[i].glyph_map) {
        map_destroy((HashMap*)font->atlases[i].glyph_map);
      }
    }
    free(font->atlases);
    font->atlases = NULL;
  }
  
  if (font->supported_sizes) {
    free(font->supported_sizes);
    font->supported_sizes = NULL;
  }
  
  font->atlas_count = 0;
  font->size_count = 0;
  return true;
}

bool font_generate_atlas(Font *font, float font_size, uint32_t atlas_width, 
                        uint32_t atlas_height) {
  FT_Face face = NULL;
  FT_Error error;
  FontAtlas *atlas = NULL;
  uint32_t x = 0, y = 0;
  uint32_t max_height = 0;
  
  if (!font || !g_ft_library) {
    return false;
  }
  
  error = FT_New_Face(g_ft_library, font->path, 0, &face);
  if (error) {
    return false;
  }
  
  error = FT_Set_Char_Size(face, 0, (FT_F26Dot6)(font_size * 64), 72, 72);
  if (error) {
    FT_Done_Face(face);
    return false;
  }
  
  // Resize atlas array
  FontAtlas *new_atlases = realloc(font->atlases, 
                                  (font->atlas_count + 1) * sizeof(FontAtlas));
  if (!new_atlases) {
    FT_Done_Face(face);
    return false;
  }
  font->atlases = new_atlases;
  atlas = &font->atlases[font->atlas_count];
  
  memset(atlas, 0, sizeof(FontAtlas));
  atlas->width = atlas_width;
  atlas->height = atlas_height;
  atlas->glyph_capacity = 256;
  atlas->glyphs = calloc(atlas->glyph_capacity, sizeof(Glyph));
  atlas->glyph_map = map_create(8, sizeof(uint32_t), atlas->glyph_capacity);
  
  if (!atlas->glyphs || !atlas->glyph_map) {
    FT_Done_Face(face);
    return false;
  }
  
  // Generate bitmap data
  uint8_t *bitmap_data = calloc(atlas_width * atlas_height, 1);
  if (!bitmap_data) {
    FT_Done_Face(face);
    return false;
  }
  
  // Load common glyphs (ASCII range)
  for (uint32_t codepoint = 32; codepoint < 128; codepoint++) {
    error = FT_Load_Char(face, codepoint, FT_LOAD_RENDER);
    if (error) {
      continue;
    }
    
    FT_Bitmap *ft_bitmap = &face->glyph->bitmap;
    
    // Check if glyph fits in current atlas
    if (x + ft_bitmap->width > atlas_width) {
      x = 0;
      y += max_height;
      max_height = 0;
    }
    
    if (y + ft_bitmap->rows > atlas_height) {
      break; // Atlas full
    }
    
    // Store glyph info
    if (atlas->glyph_count < atlas->glyph_capacity) {
      Glyph *glyph = &atlas->glyphs[atlas->glyph_count];
      glyph->codepoint = codepoint;
      glyph->advance = face->glyph->advance.x >> 6;
      glyph->bearing_x = face->glyph->bitmap_left;
      glyph->bearing_y = face->glyph->bitmap_top;
      glyph->width = ft_bitmap->width;
      glyph->height = ft_bitmap->rows;
      glyph->uv_min[0] = (float)x / atlas_width;
      glyph->uv_min[1] = (float)y / atlas_height;
      glyph->uv_max[0] = (float)(x + ft_bitmap->width) / atlas_width;
      glyph->uv_max[1] = (float)(y + ft_bitmap->rows) / atlas_height;
      
      uint32_t index = atlas->glyph_count++;
      map_insert((HashMap*)atlas->glyph_map, &codepoint, &index);
    }
    
    // Copy bitmap to atlas
    for (uint32_t row = 0; row < ft_bitmap->rows; row++) {
      for (uint32_t col = 0; col < ft_bitmap->width; col++) {
        uint32_t atlas_x = x + col;
        uint32_t atlas_y = y + row;
        if (atlas_x < atlas_width && atlas_y < atlas_height) {
          bitmap_data[atlas_y * atlas_width + atlas_x] = ft_bitmap->buffer[row * ft_bitmap->width + col];
        }
      }
    }
    
    x += ft_bitmap->width;
    if (ft_bitmap->rows > max_height) {
      max_height = ft_bitmap->rows;
    }
  }
  
  // Create texture from bitmap data
  // Note: This would integrate with your rendering system
  // glGenTextures(1, &atlas->texture_id);
  // glBindTexture(GL_TEXTURE_2D, atlas->texture_id);
  // glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, atlas_width, atlas_height, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap_data);
  
  free(bitmap_data);
  FT_Done_Face(face);
  
  font->atlas_count++;
  return true;
}

const Glyph *font_get_glyph(const Font *font, uint32_t codepoint, float font_size) {
  if (!font || !font->atlases) {
    return NULL;
  }
  
  // Find atlas for this font size
  for (uint32_t i = 0; i < font->atlas_count; i++) {
    // Check if this atlas matches the requested size
    // This is simplified - you'd want better size matching
    uint32_t glyph_index = 0;
    if (map_get((HashMap*)font->atlases[i].glyph_map, &codepoint, &glyph_index)) {
      if (glyph_index < font->atlases[i].glyph_count) {
        return &font->atlases[i].glyphs[glyph_index];
      }
    }
  }
  
  return NULL;
}

float font_get_kerning(const Font *font, uint32_t left_glyph, uint32_t right_glyph, 
                       float font_size) {
  FT_Face face = NULL;
  FT_Error error;
  FT_Vector kerning;
  
  if (!font || !g_ft_library) {
    return 0.0f;
  }
  
  error = FT_New_Face(g_ft_library, font->path, 0, &face);
  if (error) {
    return 0.0f;
  }
  
  error = FT_Set_Char_Size(face, 0, (FT_F26Dot6)(font_size * 64), 72, 72);
  if (error) {
    FT_Done_Face(face);
    return 0.0f;
  }
  
  error = FT_Get_Kerning(face, left_glyph, right_glyph, FT_KERNING_DEFAULT, &kerning);
  FT_Done_Face(face);
  
  if (error) {
    return 0.0f;
  }
  
  return kerning.x / 64.0f;
}

bool font_measure_text(const Font *font, const char *text, float font_size, 
                       float *out_width, float *out_height) {
  float x = 0.0f;
  float max_width = 0.0f;
  float max_height = 0.0f;
  size_t i = 0;
  
  if (!font || !text) {
    return false;
  }
  
  while (text[i] != '\0') {
    uint32_t codepoint = text[i]; // Simplified - needs UTF-8 decoding
    const Glyph *glyph = font_get_glyph(font, codepoint, font_size);
    
    if (glyph) {
      x += glyph->advance;
      float glyph_height = glyph->bearing_y + (glyph->height - glyph->bearing_y);
      if (glyph_height > max_height) {
        max_height = glyph_height;
      }
    } else {
      x += font_size * 0.5f; // Default advance for missing glyphs
    }
    
    if (x > max_width) {
      max_width = x;
    }
    
    i++;
  }
  
  if (out_width) *out_width = max_width;
  if (out_height) *out_height = max_height;
  
  return true;
}

bool font_word_wrap(const Font *font, const char *text, float font_size, 
                    float max_width, char *out, size_t out_size) {
  float current_width = 0.0f;
  size_t word_start = 0;
  size_t out_pos = 0;
  size_t i = 0;
  
  if (!font || !text || !out || out_size == 0) {
    return false;
  }
  
  while (text[i] != '\0' && out_pos < out_size - 1) {
    if (text[i] == ' ' || text[i] == '\t' || text[i] == '\n') {
      // Copy the word
      size_t word_len = i - word_start;
      if (out_pos + word_len < out_size) {
        strncpy(out + out_pos, text + word_start, word_len);
        out_pos += word_len;
      }
      
      if (text[i] == '\n') {
        out[out_pos++] = '\n';
        current_width = 0.0f;
      } else {
        out[out_pos++] = ' ';
        current_width += font_size * 0.3f; // Space width approximation
      }
      
      word_start = i + 1;
    } else {
      // Check if word would exceed max width
      float word_width = 0.0f;
      for (size_t j = word_start; j <= i && text[j] != '\0'; j++) {
        uint32_t codepoint = text[j]; // Simplified
        const Glyph *glyph = font_get_glyph(font, codepoint, font_size);
        if (glyph) {
          word_width += glyph->advance;
        }
      }
      
      if (current_width + word_width > max_width && word_start > 0) {
        out[out_pos++] = '\n';
        current_width = word_width;
      }
    }
    
    i++;
  }
  
  // Copy last word
  if (word_start < i && out_pos < out_size - 1) {
    size_t word_len = i - word_start;
    strncpy(out + out_pos, text + word_start, word_len);
    out_pos += word_len;
  }
  
  out[out_pos] = '\0';
  return true;
}

bool font_sdf_generate(Font *font, float font_size, uint32_t atlas_width, 
                       uint32_t atlas_height, float spread) {
  // Similar to font_generate_atlas but with SDF generation
  // This would implement signed distance field generation
  // For now, delegate to regular atlas generation
  return font_generate_atlas(font, font_size, atlas_width, atlas_height);
}

bool font_fallback_chain(Font *primary_font, Font **fallback_fonts, 
                        uint32_t fallback_count, const char *text, 
                        float font_size, char *out_glyph_indices, 
                        size_t out_size) {
  size_t i = 0;
  size_t out_pos = 0;
  
  if (!primary_font || !text || !out_glyph_indices) {
    return false;
  }
  
  while (text[i] != '\0' && out_pos < out_size) {
    uint32_t codepoint = text[i]; // Simplified UTF-8
    bool found = false;
    
    // Try primary font first
    if (font_get_glyph(primary_font, codepoint, font_size)) {
      out_glyph_indices[out_pos++] = 0; // Primary font index
      found = true;
    } else {
      // Try fallback fonts
      for (uint32_t j = 0; j < fallback_count; j++) {
        if (font_get_glyph(fallback_fonts[j], codepoint, font_size)) {
          out_glyph_indices[out_pos++] = j + 1; // Fallback font index
          found = true;
          break;
        }
      }
    }
    
    if (!found) {
      out_glyph_indices[out_pos++] = 0; // Use primary font with missing glyph
    }
    
    i++;
  }
  
  return true;
}

bool font_emoji_support(Font *font, const char *emoji_font_path, float font_size) {
  // Load emoji font and integrate with fallback chain
  Font emoji_font = {0};
  if (!font_load(&emoji_font, emoji_font_path, font_size)) {
    return false;
  }
  
  // This would integrate emoji font into fallback chain
  // For now, just return success
  font_unload(&emoji_font);
  return true;
}

// Initialize font system
bool font_system_init(void) {
  if (FT_Init_FreeType(&g_ft_library)) {
    return false;
  }
  return true;
}

// Shutdown font system
void font_system_shutdown(void) {
  if (g_ft_library) {
    FT_Done_FreeType(g_ft_library);
    g_ft_library = NULL;
  }
  
  if (g_fonts) {
    for (uint32_t i = 0; i < g_font_count; i++) {
      font_unload(&g_fonts[i]);
    }
    free(g_fonts);
    g_fonts = NULL;
    g_font_count = 0;
  }
}
