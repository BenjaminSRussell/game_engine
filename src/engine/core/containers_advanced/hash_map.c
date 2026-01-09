#include <core/containers_advanced/hash_map.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * =================================================================================================
 *                          OPTIMIZED HASH MAP IMPLEMENTATION
 * =================================================================================================
 */

// Basic FNV-1a hash for generic data
static uint64_t hash_func(const void* key, size_t size) {
    uint64_t hash = 14695981039346656037ULL;
    const uint8_t* val = (const uint8_t*)key;
    for (size_t i = 0; i < size; ++i) {
        hash ^= val[i];
        hash *= 1099511628211ULL;
    }
    return hash;
}

typedef struct {
    void* key;
    void* value;
    uint64_t hash;
    uint32_t probe_dist; // Robin Hood: distance from ideal bucket
    bool used;
} HashEntry;

struct HashMap {
    HashEntry* entries;
    size_t capacity;
    size_t count;
    size_t key_size;
    size_t value_size;
};

// Create a new hash map
HashMap* map_create(size_t key_size, size_t value_size, size_t initial_capacity) {
    HashMap* map = (HashMap*)malloc(sizeof(HashMap));
    if (!map) return NULL;

    if (initial_capacity < 16) initial_capacity = 16;
    
    // Power of 2 capacity
    size_t cap = 1;
    while (cap < initial_capacity) cap <<= 1;
    
    map->capacity = cap;
    map->count = 0;
    map->key_size = key_size;
    map->value_size = value_size;
    
    map->entries = (HashEntry*)calloc(map->capacity, sizeof(HashEntry));
    if (!map->entries) {
        free(map);
        return NULL;
    }

    return map;
}

void map_destroy(HashMap* map) {
    if (map) {
        // Free keys/values if they were copied deeply? 
        // For this generic impl, we assume trivial pod types or user managed pointers.
        // But we DO need to free the keys/values allocated in the entries if we stored them separately.
        // In this impl, let's store pointers in the entry struct for simplicity of the resizing logic,
        // or embed data. Let's assume we store copies of data.
        
        for(size_t i=0; i<map->capacity; i++) {
            if(map->entries[i].used) {
                free(map->entries[i].key);
                free(map->entries[i].value);
            }
        }
        
        free(map->entries);
        free(map);
    }
}

// Internal insert helper
static void map_insert_internal(HashEntry* entries, size_t capacity, 
                               void* key, void* value, size_t key_size, size_t value_size) {
    
    uint64_t hash = hash_func(key, key_size);
    size_t idx = hash & (capacity - 1);
    
    HashEntry entry_to_insert = {0};
    entry_to_insert.key = key;
    entry_to_insert.value = value;
    entry_to_insert.hash = hash;
    entry_to_insert.used = true;
    entry_to_insert.probe_dist = 0;

    // Robin Hood probing
    while (true) {
        if (!entries[idx].used) {
            entries[idx] = entry_to_insert;
            return;
        }

        // Evaluate probe distance
        if (entry_to_insert.probe_dist > entries[idx].probe_dist) {
            // Swap: Rich take from poor
            HashEntry temp = entries[idx];
            entries[idx] = entry_to_insert;
            entry_to_insert = temp;
        }

        entry_to_insert.probe_dist++;
        idx = (idx + 1) & (capacity - 1);
    }
}

static void map_resize(HashMap* map) {
    size_t new_cap = map->capacity * 2;
    HashEntry* new_entries = (HashEntry*)calloc(new_cap, sizeof(HashEntry));
    
    for (size_t i = 0; i < map->capacity; i++) {
        if (map->entries[i].used) {
            // Re-insert (reuse existing key/value pointers)
            map_insert_internal(new_entries, new_cap, 
                                map->entries[i].key, map->entries[i].value, 
                                map->key_size, map->value_size);
        }
    }
    
    free(map->entries);
    map->entries = new_entries;
    map->capacity = new_cap;
}

void map_insert(HashMap* map, const void* key, const void* value) {
    if (!map || !key || !value) return;

    // Load factor 0.75
    if (map->count * 4 >= map->capacity * 3) {
        map_resize(map);
    }

    // Allocate copies
    void* key_copy = malloc(map->key_size);
    void* val_copy = malloc(map->value_size);
    memcpy(key_copy, key, map->key_size);
    memcpy(val_copy, value, map->value_size);

    map_insert_internal(map->entries, map->capacity, key_copy, val_copy, map->key_size, map->value_size);
    map->count++;
}

bool map_get(HashMap* map, const void* key, void* out_value) {
    if (!map || !key) return false;

    uint64_t hash = hash_func(key, map->key_size);
    size_t idx = hash & (map->capacity - 1);
    uint32_t dist = 0;

    while (true) {
        if (!map->entries[idx].used) return false;
        
        if (dist > map->entries[idx].probe_dist) return false; // Not found early exit
        
        if (map->entries[idx].hash == hash) {
            // Check full key equality
            if (memcmp(map->entries[idx].key, key, map->key_size) == 0) {
                if (out_value) {
                    memcpy(out_value, map->entries[idx].value, map->value_size);
                }
                return true;
            }
        }

        dist++;
        idx = (idx + 1) & (map->capacity - 1);
    }
}

bool map_remove(HashMap* map, const void* key) {
    if (!map || !key) return false;

    uint64_t hash = hash_func(key, map->key_size);
    size_t idx = hash & (map->capacity - 1);
    uint32_t dist = 0;

    while (true) {
        if (!map->entries[idx].used) return false;
        if (dist > map->entries[idx].probe_dist) return false;

        if (map->entries[idx].hash == hash && 
            memcmp(map->entries[idx].key, key, map->key_size) == 0) {
            
            // Found it. Free memory
            free(map->entries[idx].key);
            free(map->entries[idx].value);
            map->entries[idx].used = false;
            map->count--;
            
            // Backshift to fill gap (Robin Hood optimization)
            size_t next_idx = (idx + 1) & (map->capacity - 1);
            while (map->entries[next_idx].used && map->entries[next_idx].probe_dist > 0) {
                map->entries[idx] = map->entries[next_idx];
                map->entries[idx].probe_dist--;
                idx = next_idx;
                next_idx = (idx + 1) & (map->capacity - 1);
            }
            map->entries[idx].used = false;
            return true;
        }

        dist++;
        idx = (idx + 1) & (map->capacity - 1);
    }
}
