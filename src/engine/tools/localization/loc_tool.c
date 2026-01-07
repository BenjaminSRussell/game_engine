/**
 * =================================================================================================
 *                          LOCALIZATION TOOL
 * =================================================================================================
 */

#include "tools/localization/loc_tool.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void loc_tool_init(LocalizationTool *tool) {
    tool->language_names = NULL;
    tool->language_count = 0;
    tool->current_language = 0;
    tool->entries = NULL;
    tool->entry_count = 0;
    tool->capacity = 0;
}

void loc_tool_destroy(LocalizationTool *tool) {
    free(tool->language_names);
    for (int i = 0; i < tool->entry_count; i++) {
        free(tool->entries[i].translations);
    }
    free(tool->entries);
}

void loc_add_language(LocalizationTool *tool, const char *lang_name) {
    tool->language_count++;
    tool->language_names = realloc(tool->language_names, sizeof(char*) * tool->language_count);
    tool->language_names[tool->language_count - 1] = lang_name;
    
    // Expand all entries to include new language
    for (int i = 0; i < tool->entry_count; i++) {
        tool->entries[i].translations = realloc(tool->entries[i].translations, 
                                                 sizeof(char*) * tool->language_count);
        tool->entries[i].translations[tool->language_count - 1] = NULL;
    }
}

void loc_set_current_language(LocalizationTool *tool, int lang_index) {
    if (lang_index >= 0 && lang_index < tool->language_count) {
        tool->current_language = lang_index;
    }
}

void loc_add_key(LocalizationTool *tool, const char *key) {
    if (tool->entry_count >= tool->capacity) {
        tool->capacity = tool->capacity == 0 ? 64 : tool->capacity * 2;
        tool->entries = realloc(tool->entries, sizeof(LocalizationEntry) * tool->capacity);
    }
    
    LocalizationEntry *entry = &tool->entries[tool->entry_count++];
    entry->key = key;
    entry->translations = calloc(tool->language_count, sizeof(char*));
    entry->language_count = tool->language_count;
}

void loc_set_translation(LocalizationTool *tool, const char *key, int lang_index, const char *translation) {
    for (int i = 0; i < tool->entry_count; i++) {
        if (strcmp(tool->entries[i].key, key) == 0) {
            if (lang_index >= 0 && lang_index < tool->language_count) {
                tool->entries[i].translations[lang_index] = translation;
            }
            return;
        }
    }
}

const char* loc_get_translation(LocalizationTool *tool, const char *key) {
    for (int i = 0; i < tool->entry_count; i++) {
        if (strcmp(tool->entries[i].key, key) == 0) {
            const char *translation = tool->entries[i].translations[tool->current_language];
            return translation ? translation : key; // Fallback to key
        }
    }
    return key;
}

bool loc_export_csv(LocalizationTool *tool, const char *filepath) {
    FILE *f = fopen(filepath, "w");
    if (!f) return false;
    
    // Header
    fprintf(f, "Key");
    for (int lang = 0; lang < tool->language_count; lang++) {
        fprintf(f, ",%s", tool->language_names[lang]);
    }
    fprintf(f, "\n");
    
    // Entries
    for (int i = 0; i < tool->entry_count; i++) {
        fprintf(f, "%s", tool->entries[i].key);
        for (int lang = 0; lang < tool->language_count; lang++) {
            const char *trans = tool->entries[i].translations[lang];
            fprintf(f, ",%s", trans ? trans : "");
        }
        fprintf(f, "\n");
    }
    
    fclose(f);
    return true;
}

bool loc_import_csv(LocalizationTool *tool, const char *filepath) {
    // Stub: would parse CSV and populate entries
    return false;
}

int loc_check_missing_translations(LocalizationTool *tool, int lang_index) {
    int missing = 0;
    for (int i = 0; i < tool->entry_count; i++) {
        if (!tool->entries[i].translations[lang_index]) {
            missing++;
        }
    }
    return missing;
}
