#ifndef CORE_HASHMAP_H
#define CORE_HASHMAP_H

#include "include/common.h"

typedef struct {
  const void *key;
  void *value;
  bool used;
} HashEntry;

typedef struct {
  HashEntry *entries;
  u32 capacity;
  u32 count;
  u32 key_size;
  u32 value_size;
  u32 (*hash_func)(const void *);
  bool (*equals_func)(const void *, const void *);
} HashMap;

HashMap *hashmap_create(u32 capacity, u32 key_size, u32 value_size,
                        u32 (*hash_func)(const void *),
                        bool (*equals_func)(const void *, const void *));
void hashmap_destroy(HashMap *map);
void hashmap_set(HashMap *map, const void *key, const void *value);
void *hashmap_get(HashMap *map, const void *key);
void hashmap_remove(HashMap *map, const void *key);
bool hashmap_contains(HashMap *map, const void *key);
void hashmap_clear(HashMap *map);
u32 hashmap_size(HashMap *map);
void hashmap_rehash(HashMap *map, u32 new_capacity);

// String helpers
u32 hash_string(const void *key);
bool equals_string(const void *a, const void *b);

void hashmap_set_string(HashMap *map, const char *key, const void *value);
void *hashmap_get_string(HashMap *map, const char *key);

#endif
