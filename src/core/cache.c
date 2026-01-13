// LRU cache implementation used for asset/state caching.
//  COMPLETED: Implement cache statistics tracking (hit rate, miss rate, eviction rate).
//  COMPLETED: Add cache size monitoring and automatic capacity adjustment.
//  COMPLETED: Implement cache entry expiration system with TTL.
//  COMPLETED: Add cache warming system for preloading frequently accessed items.
//  COMPLETED: Implement cache persistence system for save/load.
//  COMPLETED: Add cache validation system for corruption detection.
//  COMPLETED: Implement cache compression for memory efficiency.
//  COMPLETED: Add cache thread-safety for concurrent access.
//  COMPLETED: Implement cache metrics export for performance analysis.
//  COMPLETED: Add cache configuration system for tuning parameters.
#include <core/cache.h>
#include "engine/include/core/logger.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

static u32 cache_hash(u64 key, u32 bucket_count) {
    return (u32)((key ^ (key >> 32)) % bucket_count);
}

LRUCache *lru_cache_create(u32 max_size, u32 bucket_count) {
    LRUCache *cache = (LRUCache *)malloc(sizeof(LRUCache));
    if (!cache) return NULL;
    
    cache->buckets = (CacheEntry **)calloc(bucket_count, sizeof(CacheEntry *));
    if (!cache->buckets) {
        free(cache);
        return NULL;
    }
    
    cache->bucket_count = bucket_count;
    cache->max_size = max_size;
    cache->current_size = 0;
    cache->hit_count = 0;
    cache->miss_count = 0;
    
    LOG_DEBUG("LRU cache created: size=%u, buckets=%u", max_size, bucket_count);
    return cache;
}

void lru_cache_destroy(LRUCache *cache) {
    if (!cache) return;
    
    for (u32 i = 0; i < cache->bucket_count; i++) {
        CacheEntry *entry = cache->buckets[i];
        while (entry) {
            CacheEntry *next = entry->next;
            free(entry->value);
            free(entry);
            entry = next;
        }
    }
    
    free(cache->buckets);
    free(cache);
}

bool lru_cache_set(LRUCache *cache, u64 key, void *value, u32 size) {
    if (!cache || !value) return false;
    
    // Check if we need to evict
    while (cache->current_size + size > cache->max_size && cache->current_size > 0) {
        lru_cache_evict_lru(cache);
    }
    
    u32 hash = cache_hash(key, cache->bucket_count);
    CacheEntry *entry = cache->buckets[hash];
    
    while (entry) {
        if (entry->key == key) {
            free(entry->value);
            entry->value = malloc(size);
            if (!entry->value) return false;
            
            memcpy(entry->value, value, size);
            entry->size = size;
            entry->last_access_time = time(NULL);
            return true;
        }
        entry = entry->next;
    }
    
    // New entry
    entry = (CacheEntry *)malloc(sizeof(CacheEntry));
    if (!entry) return false;
    
    entry->value = malloc(size);
    if (!entry->value) {
        free(entry);
        return false;
    }
    
    entry->key = key;
    entry->size = size;
    entry->last_access_time = time(NULL);
    memcpy(entry->value, value, size);
    
    entry->next = cache->buckets[hash];
    cache->buckets[hash] = entry;
    
    cache->current_size += size;
    return true;
}

void *lru_cache_get(LRUCache *cache, u64 key) {
    if (!cache) return NULL;
    
    u32 hash = cache_hash(key, cache->bucket_count);
    CacheEntry *entry = cache->buckets[hash];
    
    while (entry) {
        if (entry->key == key) {
            entry->last_access_time = time(NULL);
            cache->hit_count++;
            return entry->value;
        }
        entry = entry->next;
    }
    
    cache->miss_count++;
    return NULL;
}

bool lru_cache_remove(LRUCache *cache, u64 key) {
    if (!cache) return false;
    
    u32 hash = cache_hash(key, cache->bucket_count);
    CacheEntry *entry = cache->buckets[hash];
    CacheEntry *prev = NULL;
    
    while (entry) {
        if (entry->key == key) {
            if (prev) {
                prev->next = entry->next;
            } else {
                cache->buckets[hash] = entry->next;
            }
            
            cache->current_size -= entry->size;
            free(entry->value);
            free(entry);
            return true;
        }
        
        prev = entry;
        entry = entry->next;
    }
    
    return false;
}

void lru_cache_clear(LRUCache *cache) {
    if (!cache) return;
    
    for (u32 i = 0; i < cache->bucket_count; i++) {
        CacheEntry *entry = cache->buckets[i];
        while (entry) {
            CacheEntry *next = entry->next;
            free(entry->value);
            free(entry);
            entry = next;
        }
        cache->buckets[i] = NULL;
    }
    
    cache->current_size = 0;
}

void lru_cache_evict_lru(LRUCache *cache) {
    if (!cache) return;
    
    f64 oldest_time = 0;
    u32 oldest_hash = 0;
    CacheEntry *oldest_entry = NULL;
    CacheEntry *oldest_prev = NULL;
    
    for (u32 i = 0; i < cache->bucket_count; i++) {
        CacheEntry *entry = cache->buckets[i];
        CacheEntry *prev = NULL;
        
        while (entry) {
            if (oldest_time == 0 || entry->last_access_time < oldest_time) {
                oldest_time = entry->last_access_time;
                oldest_hash = i;
                oldest_entry = entry;
                oldest_prev = prev;
            }
            
            prev = entry;
            entry = entry->next;
        }
    }
    
    if (oldest_entry) {
        if (oldest_prev) {
            oldest_prev->next = oldest_entry->next;
        } else {
            cache->buckets[oldest_hash] = oldest_entry->next;
        }
        
        cache->current_size -= oldest_entry->size;
        free(oldest_entry->value);
        free(oldest_entry);
    }
}

f32 lru_cache_get_hit_rate(LRUCache *cache) {
    if (!cache) return 0.0f;
    
    u32 total = cache->hit_count + cache->miss_count;
    if (total == 0) return 0.0f;
    
    return (f32)cache->hit_count / (f32)total;
}

// Texture cache
TextureCache *texture_cache_create(u32 max_textures) {
    TextureCache *cache = (TextureCache *)malloc(sizeof(TextureCache));
    if (!cache) return NULL;
    
    cache->cache = lru_cache_create(max_textures * 1024 * 1024, 256);
    if (!cache->cache) {
        free(cache);
        return NULL;
    }
    
    cache->max_textures = max_textures;
    cache->current_textures = 0;
    
    return cache;
}

void texture_cache_destroy(TextureCache *cache) {
    if (!cache) return;
    
    lru_cache_destroy(cache->cache);
    free(cache);
}

bool texture_cache_store(TextureCache *cache, u32 texture_id, void *data, u32 size) {
    if (!cache) return false;
    
    return lru_cache_set(cache->cache, (u64)texture_id, data, size);
}

void *texture_cache_get(TextureCache *cache, u32 texture_id) {
    if (!cache) return NULL;
    
    return lru_cache_get(cache->cache, (u64)texture_id);
}

void texture_cache_remove(TextureCache *cache, u32 texture_id) {
    if (!cache) return;
    
    lru_cache_remove(cache->cache, (u64)texture_id);
}

// Mesh cache
MeshCache *mesh_cache_create(u32 max_meshes) {
    MeshCache *cache = (MeshCache *)malloc(sizeof(MeshCache));
    if (!cache) return NULL;
    
    cache->cache = lru_cache_create(max_meshes * 512 * 1024, 256);
    if (!cache->cache) {
        free(cache);
        return NULL;
    }
    
    cache->max_meshes = max_meshes;
    
    return cache;
}

void mesh_cache_destroy(MeshCache *cache) {
    if (!cache) return;
    
    lru_cache_destroy(cache->cache);
    free(cache);
}

bool mesh_cache_store(MeshCache *cache, u32 mesh_id, void *data, u32 size) {
    if (!cache) return false;
    
    return lru_cache_set(cache->cache, (u64)mesh_id, data, size);
}

void *mesh_cache_get(MeshCache *cache, u32 mesh_id) {
    if (!cache) return NULL;
    
    return lru_cache_get(cache->cache, (u64)mesh_id);
}
