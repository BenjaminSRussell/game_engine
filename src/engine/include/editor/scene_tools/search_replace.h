#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    SEARCH_MATCH_EXACT,
    SEARCH_MATCH_PARTIAL,
    SEARCH_MATCH_REGEX
} SearchMatchMode;

typedef struct {
    SearchMatchMode mode;
    char query[256];
    
    // Results
    uint64_t *found_entities;
    int result_count;
} SceneSearch;

typedef struct {
    char find[256];
    char replace[256];
    bool case_sensitive;
    bool whole_word;
} BatchReplace;

void scene_search_init(SceneSearch *search);
void scene_search_destroy(SceneSearch *search);

// Search
void scene_search_execute(SceneSearch *search, const char *query, SearchMatchMode mode);
void scene_search_clear(SceneSearch *search);

// Batch replace
void scene_batch_replace(BatchReplace *replace, void *scene);

// Rename tool
void scene_rename_entities(uint64_t *entities, int count, const char *pattern);
