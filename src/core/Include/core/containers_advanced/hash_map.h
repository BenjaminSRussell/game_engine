#ifndef CORE_CONTAINERS_ADVANCED_HASH_MAP_H
#define CORE_CONTAINERS_ADVANCED_HASH_MAP_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct HashMap HashMap;

HashMap* map_create(size_t key_size, size_t value_size, size_t initial_capacity);
void map_destroy(HashMap* map);
void map_insert(HashMap* map, const void* key, const void* value);
bool map_get(HashMap* map, const void* key, void* out_value);
bool map_remove(HashMap* map, const void* key);

#ifdef __cplusplus
}
#endif

#endif // CORE_CONTAINERS_ADVANCED_HASH_MAP_H
