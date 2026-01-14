/**
 * @file localization.c
 * @brief System Implementation
 * @description System implementation
 * @date 2026-01-13
 */

#include "engine/documentation/localization/localization.h"
#include <stdlib.h>
#include <string.h>

// STRING TABLE MANAGEMENT

StringTable *string_table_create(LanguageCode language) {
  StringTable *table = (StringTable *)malloc(sizeof(StringTable));
  if (!table) {
    return NULL;
  }

  table->language = language;
  table->string_count = 0;
  table->string_capacity = 32; // Initial capacity
  table->strings = (LocalizedString *)calloc(table->string_capacity, sizeof(LocalizedString));
  table->hash_map = NULL; // TODO: Implement hash map

  // Set default properties based on language
  // This is a basic stub
  table->is_rtl = (language == LANG_AR_SA);

  return table;
}

void string_table_destroy(StringTable *table) {
  if (!table) {
    return;
  }

  if (table->strings) {
    free(table->strings);
  }

  // TODO: Destroy hash map if implemented

  free(table);
}

void string_table_remove(StringTable *table, const char *key) {
  if (!table || !key) {
    return;
  }

  // Linear search for now (O(n)) - specific TODO implementation
  for (uint32_t i = 0; i < table->string_count; i++) {
    if (strcmp(table->strings[i].key, key) == 0) {
      // Found it, remove by swapping with last element
      if (i < table->string_count - 1) {
        table->strings[i] = table->strings[table->string_count - 1];
      }
      // Clear the last element (optional, but good for debug)
      memset(&table->strings[table->string_count - 1], 0, sizeof(LocalizedString));
      table->string_count--;
      return;
    }
  }
}

// PLURALIZATION

PluralCategory plural_rule_english(int count) {
  // English rules:
  // count = 1 -> ONE
  // count != 1 -> OTHER (Zero is treated as "other" in English: "0 cats", "2 cats")
  
  if (count == 1) {
    return PLURAL_ONE;
  }
  return PLURAL_OTHER;
}
