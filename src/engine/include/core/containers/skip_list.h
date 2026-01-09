#ifndef CORE_CONTAINERS_SKIP_LIST_H
#define CORE_CONTAINERS_SKIP_LIST_H

#include "common.h"
#include <stdbool.h>

typedef struct SkipList SkipList;

SkipList* skip_list_create(i32 max_level, f32 probability);
void skip_list_destroy(SkipList* list);

bool skip_list_insert(SkipList* list, u64 key, void* value);
void* skip_list_search(SkipList* list, u64 key);
bool skip_list_delete(SkipList* list, u64 key);

size_t skip_list_size(SkipList* list);
bool skip_list_is_empty(SkipList* list);

#endif // CORE_CONTAINERS_SKIP_LIST_H
