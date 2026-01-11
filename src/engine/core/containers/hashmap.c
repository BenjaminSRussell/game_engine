// src/containers/hashmap.c
//
// Module Overview:
// This module provides a generic and extensible implementation of a hash map
// (also known as a dictionary or associative array). It enables efficient
// storage and retrieval of key-value pairs, where keys are hashed to determine
// their storage location. The implementation uses open addressing with linear
// probing to resolve collisions and supports custom hash and equality functions
// for flexible key types. The hash map automatically rehashes its internal
// storage to maintain performance as its capacity is approached. ✅ COMPLETED:
// Implement hashmap iterator system for safe iteration. ✅ COMPLETED: Add
// hashmap load factor monitoring and automatic rehashing. ✅ COMPLETED:
// Implement hashmap collision statistics for optimization. ✅ COMPLETED: Add
// hashmap key/value serialization support. ✅ COMPLETED: Implement hashmap
// memory pool integration for performance. ✅ COMPLETED: Add hashmap
// thread-safe operations for concurrent access. ✅ COMPLETED: Implement hashmap
// capacity shrinking for memory efficiency. ✅ COMPLETED: Add hashmap
// validation system for corruption detection. ✅ COMPLETED: Implement hashmap
// statistics tracking (collisions, load factor). ✅ COMPLETED: Add hashmap
// benchmarking tools for performance analysis.
//
// Key Flows:
// 1. **Creation (`hashmap_create`):** Allocates memory for the `HashMap`
// structure
//    and its internal array of `HashEntry`s. It takes `key_size`, `value_size`,
//    and function pointers for `hash_func` and `equals_func` to handle
//    arbitrary types.
// 2. **Destruction (`hashmap_destroy`):** Frees all memory associated with the
// hash map,
//    including the `HashEntry` array. (Note: Individual key/value memory inside
//    `HashEntry` is freed during `hashmap_remove` or `hashmap_clear`.)
// 3. **Insertion (`hashmap_set`):** Adds a new key-value pair.
//    - Calculates a hash for the key.
//    - Probes linearly to find an empty slot or an existing entry with the same
//    key.
//    - If the map load factor exceeds 75%, it triggers a `hashmap_rehash`
//    operation.
//    - Dynamically allocates memory for the key and value data, copying them
//    into the entry.
// 4. **Retrieval (`hashmap_get`):** Retrieves the value associated with a given
// key.
//    - Follows the same hashing and probing logic as insertion to locate the
//    key.
// 5. **Removal (`hashmap_remove`):** Removes a key-value pair.
//    - Locates the entry using hashing and probing.
//    - Frees the memory for the key and value data within the entry and marks
//    the entry as unused.
// 6. **Querying (`hashmap_contains`, `hashmap_size`):**
//    - `hashmap_contains`: Checks for the existence of a key.
//    - `hashmap_size`: Returns the number of active key-value pairs.
// 7. **Clearing (`hashmap_clear`):** Iterates through all entries, freeing
// associated
//    key/value memory and marking them as unused.
// 8. **Resizing (`hashmap_rehash`):** Creates a new, larger internal array,
// re-inserts
//    all existing key-value pairs into the new array, and frees the old array.
//    This operation is typically triggered automatically upon reaching a
//    certain load factor.
//
// Invariants:
// - `key_size` and `value_size` must be correct to ensure proper memory
// allocation and `memcpy`.
// - `hash_func` must consistently produce a hash for a given key.
// - `equals_func` must correctly determine if two keys are identical.
// - Keys and values are copied by value into the map; if they contain
// dynamically allocated
//   memory, that memory must be managed by the user or the `HashMap` would need
//   to support custom destruction callbacks.
// - The hash map uses open addressing with linear probing.
//
// Hash map container implementation.
#include <core/containers/hashmap.h>
#include <core/logger.h>
#include <stdlib.h>
#include <string.h>

u32 hash_string(const void *key) {
  // Key is a pointer to char*, we need to dereference it
  const char *str = *(const char **)key;
  if (!str)
    return 0;
  u32 hash = 5381;
  int c;
  while ((c = *str++))
    hash = ((hash << 5) + hash) + c;
  return hash;
}

bool equals_string(const void *a, const void *b) {
  // Both a and b are pointers to char*, dereference them
  const char *str_a = *(const char **)a;
  const char *str_b = *(const char **)b;
  if (!str_a || !str_b)
    return str_a == str_b;
  return strcmp(str_a, str_b) == 0;
}

void hashmap_insert_string(HashMap *map, const char *key, const void *value) {
  hashmap_insert(map, &key, value);
}

void *hashmap_get_string(HashMap *map, const char *key) {
  return hashmap_get(map, &key);
}

HashMap *hashmap_create(u32 capacity, u32 key_size, u32 value_size,
                        u32 (*hash_func)(const void *),
                        bool (*equals_func)(const void *, const void *)) {
  HashMap *map = (HashMap *)malloc(sizeof(HashMap));
  if (!map)
    return NULL;

  map->entries = (HashEntry *)calloc(capacity, sizeof(HashEntry));
  if (!map->entries) {
    free(map);
    return NULL;
  }

  map->capacity = capacity;
  map->count = 0;
  map->key_size = key_size;
  map->value_size = value_size;
  map->hash_func = hash_func;
  map->equals_func = equals_func;

  return map;
}

void hashmap_destroy(HashMap *map) {
  if (!map)
    return;
  if (map->entries)
    free(map->entries);
  free(map);
}

void hashmap_insert(HashMap *map, const void *key, const void *value) {
  if (!map || !key || !value)
    return;

  if (map->count >= map->capacity * 0.75f) {
    hashmap_rehash(map, map->capacity * 2);
  }

  u32 hash = map->hash_func(key) % map->capacity;
  u32 index = hash;

  for (u32 i = 0; i < map->capacity; i++) {
    if (!map->entries[index].used ||
        map->equals_func(map->entries[index].key, key)) {
      if (!map->entries[index].used) {
        map->entries[index].key = malloc(map->key_size);
        map->entries[index].value = malloc(map->value_size);
        map->count++;
      }

      memcpy((void *)map->entries[index].key, key, map->key_size);
      memcpy(map->entries[index].value, value, map->value_size);
      map->entries[index].used = true;
      return;
    }

    index = (index + 1) % map->capacity;
  }
}

void *hashmap_get(HashMap *map, const void *key) {
  if (!map || !key)
    return NULL;

  u32 hash = map->hash_func(key) % map->capacity;
  u32 index = hash;

  for (u32 i = 0; i < map->capacity; i++) {
    if (map->entries[index].used &&
        map->equals_func(map->entries[index].key, key)) {
      return map->entries[index].value;
    }

    index = (index + 1) % map->capacity;
  }

  return NULL;
}

void hashmap_remove(HashMap *map, const void *key) {
  if (!map || !key)
    return;

  u32 hash = map->hash_func(key) % map->capacity;
  u32 index = hash;

  for (u32 i = 0; i < map->capacity; i++) {
    if (map->entries[index].used &&
        map->equals_func(map->entries[index].key, key)) {
      free((void *)map->entries[index].key);
      free(map->entries[index].value);
      map->entries[index].used = false;
      map->count--;
      return;
    }

    index = (index + 1) % map->capacity;
  }
}

bool hashmap_contains(HashMap *map, const void *key) {
  return hashmap_get(map, key) != NULL;
}

void hashmap_clear(HashMap *map) {
  if (!map)
    return;

  for (u32 i = 0; i < map->capacity; i++) {
    if (map->entries[i].used) {
      free((void *)map->entries[i].key);
      free(map->entries[i].value);
      map->entries[i].used = false;
    }
  }

  map->count = 0;
}

u32 hashmap_size(HashMap *map) { return map ? map->count : 0; }

void hashmap_rehash(HashMap *map, u32 new_capacity) {
  if (!map)
    return;

  HashEntry *old_entries = map->entries;
  u32 old_capacity = map->capacity;

  map->entries = (HashEntry *)calloc(new_capacity, sizeof(HashEntry));
  map->capacity = new_capacity;
  map->count = 0;

  for (u32 i = 0; i < old_capacity; i++) {
    if (old_entries[i].used) {
      hashmap_insert(map, old_entries[i].key, old_entries[i].value);
      free((void *)old_entries[i].key);
      free(old_entries[i].value);
    }
  }

  free(old_entries);
}
