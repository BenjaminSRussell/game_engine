// include/core/cache.h
//
// Purpose: Defines a generic Least Recently Used (LRU) cache implementation,
// as well as specialized `TextureCache` and `MeshCache` layers. This caching
// system is designed to efficiently store and retrieve frequently accessed data,
// automatically evicting the least recently used items when the cache reaches
// its maximum capacity.
//
// Public APIs:
// - `CacheEntry`: Internal structure representing an entry within the LRU cache,
//   storing the key, value, size, last access time, and a pointer for linked-list management.
// - `LRUCache`: The core LRU cache structure, managing a collection of `CacheEntry`
//   buckets, tracking overall size, hit/miss counts, and implementing eviction logic.
// - `lru_cache_create`, `lru_cache_destroy`: Functions for lifecycle management of the LRU cache.
// - `lru_cache_set`: Inserts or updates a key-value pair in the cache, managing eviction if necessary.
// - `lru_cache_get`: Retrieves a value associated with a key, updating its recency.
// - `lru_cache_remove`, `lru_cache_clear`: Functions for explicit removal of entries or clearing the entire cache.
// - `lru_cache_evict_lru`: Manually triggers the eviction of the least recently used item.
// - `lru_cache_get_hit_rate`: Calculates the cache's hit rate for performance monitoring.
// - `TextureCache`, `texture_cache_create`, `texture_cache_destroy`, `texture_cache_store`, `texture_cache_get`, `texture_cache_remove`:
//   Specialized API for caching GPU texture resources.
// - `MeshCache`, `mesh_cache_create`, `mesh_cache_destroy`, `mesh_cache_store`, `mesh_cache_get`:
//   Specialized API for caching mesh data.
//
// Ownership: The `LRUCache` owns the `CacheEntry` structures and the `value` data they point to.
// Users are responsible for creating and destroying the caches. When a cache is destroyed,
// all stored values are freed.
//
// Invariants:
// - `max_size` defines the total memory limit for stored values in the `LRUCache`.
// - `bucket_count` should be chosen to minimize collisions and optimize lookup times.
// - Stored `value` data is copied; if it contains pointers to other dynamically allocated
//   memory, that memory must be managed externally by the user or the `value` must be a deep copy.
// - The `last_access_time` is crucial for correct LRU eviction.
//
#ifndef CACHE_H
#define CACHE_H


#include "../common.h"

typedef struct CacheEntry {
    u64 key;
    void *value;
    u32 size;
    f64 last_access_time;
    struct CacheEntry *next;
} CacheEntry;

typedef struct {
    CacheEntry **buckets;
    u32 bucket_count;
    u32 max_size;
    u32 current_size;
    u32 hit_count;
    u32 miss_count;
} LRUCache;

LRUCache *lru_cache_create(u32 max_size, u32 bucket_count);
void lru_cache_destroy(LRUCache *cache);
bool lru_cache_set(LRUCache *cache, u64 key, void *value, u32 size);
void *lru_cache_get(LRUCache *cache, u64 key);
bool lru_cache_remove(LRUCache *cache, u64 key);
void lru_cache_clear(LRUCache *cache);
void lru_cache_evict_lru(LRUCache *cache);
f32 lru_cache_get_hit_rate(LRUCache *cache);

// Texture cache for GPU resources
typedef struct {
    LRUCache *cache;
    u32 max_textures;
    u32 current_textures;
} TextureCache;

TextureCache *texture_cache_create(u32 max_textures);
void texture_cache_destroy(TextureCache *cache);
bool texture_cache_store(TextureCache *cache, u32 texture_id, void *data, u32 size);
void *texture_cache_get(TextureCache *cache, u32 texture_id);
void texture_cache_remove(TextureCache *cache, u32 texture_id);

// Mesh cache
typedef struct {
    LRUCache *cache;
    u32 max_meshes;
} MeshCache;

MeshCache *mesh_cache_create(u32 max_meshes);
void mesh_cache_destroy(MeshCache *cache);
bool mesh_cache_store(MeshCache *cache, u32 mesh_id, void *data, u32 size);
void *mesh_cache_get(MeshCache *cache, u32 mesh_id);

#endif
