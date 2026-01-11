// include/containers/hashmap.h
//
// Purpose: Defines a generic hash map (dictionary) data structure and its associated API.
// This header provides an efficient key-value storage mechanism, allowing for fast
// insertion, retrieval, and removal of elements based on a hash of their keys.
// It supports custom hash and equality functions to handle various key types.
//
// Public APIs:
// - `HashEntry`: Internal structure representing a single entry in the hash map,
//   containing the key, value, and a `used` flag.
// - `HashMap`: Structure representing the hash map itself, including its array of entries,
//   capacity, current element count, sizes of keys and values, and function pointers
//   for hashing and equality comparison.
// - `hashmap_create`: Allocates and initializes a new `HashMap` with a specified capacity,
//   key size, value size, and custom hash/equals functions.
// - `hashmap_destroy`: Frees all memory associated with a `HashMap`.
// - `hashmap_insert`: Inserts a key-value pair into the map.
// - `hashmap_get`: Retrieves the value associated with a given key.
// - `hashmap_remove`: Removes a key-value pair from the map.
// - `hashmap_contains`: Checks if a key exists in the map.
// - `hashmap_clear`: Removes all entries from the map.
// - `hashmap_size`: Returns the number of elements currently in the map.
// - `hashmap_rehash`: Resizes the internal storage and rehashes all entries, typically
//   called internally when the map becomes too full.
//
// Ownership: A `HashMap` instance owns its internal `entries` array. Users are responsible
// for calling `hashmap_destroy` to prevent memory leaks. Keys and values are copied
// by value into the hash map; if they contain dynamically allocated memory, users are
// responsible for managing that memory outside the `HashMap`'s scope.
//
// Invariants:
// - `key_size` and `value_size` provided during creation must be accurate.
// - `hash_func` and `equals_func` must be provided and correctly implemented for the key type.
// - The `capacity` should ideally be a prime number for better hash distribution, though not strictly enforced.
// - The hash map uses open addressing with linear probing for collision resolution.
//
#ifndef HASHMAP_H
#define HASHMAP_H


#include "../game_common.h"

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
    u32 (*hash_func)(const void *key);
    bool (*equals_func)(const void *a, const void *b);
} HashMap;

HashMap *hashmap_create(u32 capacity, u32 key_size, u32 value_size,
                        u32 (*hash_func)(const void *),
                        bool (*equals_func)(const void *, const void *));
void hashmap_destroy(HashMap *map);
void hashmap_insert(HashMap *map, const void *key, const void *value);
void *hashmap_get(HashMap *map, const void *key);
void hashmap_remove(HashMap *map, const void *key);
bool hashmap_contains(HashMap *map, const void *key);
void hashmap_clear(HashMap *map);
u32 hashmap_size(HashMap *map);
void hashmap_rehash(HashMap *map, u32 new_capacity);

#endif
