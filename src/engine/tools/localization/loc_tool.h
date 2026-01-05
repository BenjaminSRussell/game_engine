#pragma once

#include <stdbool.h>

typedef struct {
    const char *key;
    const char **translations; // Array indexed by language
    int language_count;
} LocalizationEntry;

typedef struct {
    const char **language_names;
    int language_count;
    int current_language;
    
    LocalizationEntry *entries;
    int entry_count;
    int capacity;
} LocalizationTool;

void loc_tool_init(LocalizationTool *tool);
void loc_tool_destroy(LocalizationTool *tool);

// Language management
void loc_add_language(LocalizationTool *tool, const char *lang_name);
void loc_set_current_language(LocalizationTool *tool, int lang_index);

// Translation
void loc_add_key(LocalizationTool *tool, const char *key);
void loc_set_translation(LocalizationTool *tool, const char *key, int lang_index, const char *translation);
const char* loc_get_translation(LocalizationTool *tool, const char *key);

// Import/Export
bool loc_export_csv(LocalizationTool *tool, const char *filepath);
bool loc_import_csv(LocalizationTool *tool, const char *filepath);

// Validation
int loc_check_missing_translations(LocalizationTool *tool, int lang_index);
