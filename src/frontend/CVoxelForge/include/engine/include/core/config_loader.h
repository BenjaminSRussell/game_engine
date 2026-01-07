// Configuration file loader - load and save system configurations
// Supports JSON and INI formats for configuration files
#ifndef CONFIG_LOADER_H
#define CONFIG_LOADER_H

#include "../common.h"

// Config value types
typedef enum {
  CONFIG_TYPE_INT = 0,
  CONFIG_TYPE_FLOAT = 1,
  CONFIG_TYPE_BOOL = 2,
  CONFIG_TYPE_STRING = 3,
  CONFIG_TYPE_ARRAY = 4,
  CONFIG_TYPE_OBJECT = 5
} ConfigValueType;

// Configuration value
typedef struct {
  ConfigValueType type;
  union {
    i32 int_value;
    f32 float_value;
    bool bool_value;
    char *string_value;
    void *array_value;
    void *object_value;
  } value;
} ConfigValue;

// Configuration section
typedef struct {
  char *name;
  ConfigValue *values;
  u32 value_count;
  u32 max_values;
} ConfigSection;

// Configuration document
typedef struct {
  ConfigSection *sections;
  u32 section_count;
  u32 max_sections;
  char *filename;
  bool modified;
} ConfigDocument;

// File format types
typedef enum {
  CONFIG_FORMAT_JSON = 0,
  CONFIG_FORMAT_INI = 1,
  CONFIG_FORMAT_TOML = 2,
  CONFIG_FORMAT_AUTO = 3  // Detect from extension
} ConfigFormat;

// Initialization
ConfigDocument *config_document_create(u32 max_sections);
void config_document_destroy(ConfigDocument *doc);

// Loading and saving
bool config_document_load(ConfigDocument *doc, const char *filename);
bool config_document_save(ConfigDocument *doc);
bool config_document_save_as(ConfigDocument *doc, const char *filename);

// Format control
void config_document_set_format(ConfigDocument *doc, ConfigFormat format);
ConfigFormat config_document_detect_format(const char *filename);

// Section management
ConfigSection *config_document_add_section(ConfigDocument *doc,
                                          const char *section_name);
ConfigSection *config_document_get_section(ConfigDocument *doc,
                                          const char *section_name);
bool config_document_remove_section(ConfigDocument *doc,
                                   const char *section_name);

// Value management
bool config_section_set_int(ConfigSection *section, const char *key,
                           i32 value);
bool config_section_set_float(ConfigSection *section, const char *key,
                             f32 value);
bool config_section_set_bool(ConfigSection *section, const char *key,
                            bool value);
bool config_section_set_string(ConfigSection *section, const char *key,
                              const char *value);

i32 config_section_get_int(ConfigSection *section, const char *key,
                          i32 default_value);
f32 config_section_get_float(ConfigSection *section, const char *key,
                            f32 default_value);
bool config_section_get_bool(ConfigSection *section, const char *key,
                            bool default_value);
const char *config_section_get_string(ConfigSection *section,
                                     const char *key,
                                     const char *default_value);

// Shortcut functions
bool config_set_int(ConfigDocument *doc, const char *section,
                   const char *key, i32 value);
bool config_set_float(ConfigDocument *doc, const char *section,
                     const char *key, f32 value);
bool config_set_bool(ConfigDocument *doc, const char *section,
                    const char *key, bool value);
bool config_set_string(ConfigDocument *doc, const char *section,
                      const char *key, const char *value);

i32 config_get_int(ConfigDocument *doc, const char *section, const char *key,
                  i32 default_value);
f32 config_get_float(ConfigDocument *doc, const char *section,
                    const char *key, f32 default_value);
bool config_get_bool(ConfigDocument *doc, const char *section,
                    const char *key, bool default_value);
const char *config_get_string(ConfigDocument *doc, const char *section,
                             const char *key, const char *default_value);

// Utilities
bool config_value_exists(ConfigSection *section, const char *key);
void config_section_clear(ConfigSection *section);
void config_document_print(ConfigDocument *doc);

// Type conversion
ConfigValue config_value_from_int(i32 value);
ConfigValue config_value_from_float(f32 value);
ConfigValue config_value_from_bool(bool value);
ConfigValue config_value_from_string(const char *value);

// Statistics
typedef struct {
  u32 sections;
  u32 total_values;
  u32 int_count;
  u32 float_count;
  u32 bool_count;
  u32 string_count;
} ConfigStats;

ConfigStats config_document_get_stats(ConfigDocument *doc);

#endif // CONFIG_LOADER_H
