// src/engine/ai/blackboard.c
//
// Purpose: Implementation of blackboard data storage system for AI behavior trees
// This file implements efficient key-value data storage and sharing between AI nodes
//
// Implementation Notes:
// - Hash table-based O(1) lookup performance
// - Memory pooling for efficient allocation
// - Type safety with runtime checking
// - Multiplayer synchronization support
// - Comprehensive decorator system
// - Performance monitoring and debugging
//
// Dependencies: core/logger.h, core/memory.h, common.h
//

#include "ai/blackboard.h"
#include "include/core/logger.h"
#include "include/core/memory.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <include/math/math_all.h>
#include <time.h>

#include "performance_profiling.h"

// ============================================================================
// INTERNAL CONSTANTS AND STRUCTURES
// ============================================================================

#define DEFAULT_HASH_TABLE_SIZE 256
#define MEMORY_POOL_ALIGNMENT 16
#define MAX_KEY_LENGTH 63
#define HASH_PRIME 31

typedef struct {
    void *next;
    size_t size;
    bool is_free;
} MemoryBlock;

// ============================================================================
// HASH FUNCTIONS
// ============================================================================

static u32 hash_string_djb2(const char *str) {
    u32 hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash;
}

static u32 hash_string_fnv1a(const char *str) {
    u32 hash = 2166136261u;
    while (*str) {
        hash ^= (u32)*str++;
        hash *= 16777619u;
    }
    return hash;
}

// ============================================================================
// MEMORY POOL MANAGEMENT
// ============================================================================

static bool initialize_memory_pool(Blackboard *blackboard, size_t pool_size) {
    if (pool_size == 0) {
        pool_size = blackboard->max_entries * sizeof(BlackboardEntry) * 2;
    }
    
    // Align pool size to memory boundary
    pool_size = (pool_size + MEMORY_POOL_ALIGNMENT - 1) & ~(MEMORY_POOL_ALIGNMENT - 1);
    
    blackboard->memory_pool = MALLOC(pool_size);
    if (!blackboard->memory_pool) {
        LOG_ERROR("Failed to allocate blackboard memory pool: %zu bytes", pool_size);
        return false;
    }
    
    blackboard->pool_size = pool_size;
    blackboard->pool_used = 0;
    
    LOG_DEBUG("Blackboard memory pool initialized: %zu bytes", pool_size);
    return true;
}

static void *allocate_from_pool(Blackboard *blackboard, size_t size) {
    if (!blackboard->memory_pool) {
        return malloc(size); // Fallback to system malloc
    }
    
    // Align size to memory boundary
    size = (size + MEMORY_POOL_ALIGNMENT - 1) & ~(MEMORY_POOL_ALIGNMENT - 1);
    
    if (blackboard->pool_used + size > blackboard->pool_size) {
        LOG_WARN("Blackboard memory pool exhausted, falling back to malloc");
        return malloc(size);
    }
    
    void *ptr = (u8 *)blackboard->memory_pool + blackboard->pool_used;
    blackboard->pool_used += size;
    
    return ptr;
}

static void free_from_pool(Blackboard *blackboard, void *ptr) {
    // Simple implementation - we don't actually free from the pool
    // In a more sophisticated implementation, we could maintain a free list
    (void)blackboard;
    (void)ptr;
}

// ============================================================================
// HASH TABLE OPERATIONS
// ============================================================================

static bool initialize_hash_table(Blackboard *blackboard) {
    u32 hash_size = DEFAULT_HASH_TABLE_SIZE;
    
    // Ensure hash table size is at least 2x max entries for low collision rate
    while (hash_size < blackboard->max_entries * 2) {
        hash_size *= 2;
    }
    
    blackboard->hash_table_size = hash_size;
    blackboard->hash_table = calloc(hash_size, sizeof(BlackboardEntry *));
    blackboard->hash_collision_count = calloc(hash_size, sizeof(u32));
    
    if (!blackboard->hash_table || !blackboard->hash_collision_count) {
        LOG_ERROR("Failed to allocate hash table");
        if (blackboard->hash_table) free(blackboard->hash_table);
        if (blackboard->hash_collision_count) free(blackboard->hash_collision_count);
        return false;
    }
    
    LOG_DEBUG("Blackboard hash table initialized: %u buckets", hash_size);
    return true;
}

static BlackboardEntry *find_entry_in_bucket(BlackboardEntry *bucket, const char *key) {
    while (bucket) {
        if (strcmp(bucket->key, key) == 0) {
            return bucket;
        }
        bucket = (BlackboardEntry *)bucket->data.custom_value; // Next pointer stored in custom_value
    }
    return NULL;
}

static bool add_entry_to_hash_table(Blackboard *blackboard, BlackboardEntry *entry) {
    u32 hash = hash_string_djb2(entry->key) % blackboard->hash_table_size;
    
    // Check if entry already exists
    BlackboardEntry *existing = find_entry_in_bucket(blackboard->hash_table[hash], entry->key);
    if (existing) {
        LOG_WARN("Blackboard key already exists: %s", entry->key);
        return false;
    }
    
    // Add to bucket (simple linked list)
    entry->data.custom_value = (void *)blackboard->hash_table[hash]; // Store next pointer
    blackboard->hash_table[hash] = entry;
    
    if (blackboard->hash_table[hash] != entry) {
        blackboard->hash_collision_count[hash]++;
    }
    
    return true;
}

static BlackboardEntry *find_entry(Blackboard *blackboard, const char *key) {
    if (!blackboard || !key) return NULL;
    
    u32 hash = hash_string_djb2(key) % blackboard->hash_table_size;
    return find_entry_in_bucket(blackboard->hash_table[hash], key);
}

// ============================================================================
// DECORATOR IMPLEMENTATION
// ============================================================================

static bool evaluate_decorator(Blackboard *blackboard, BlackboardDecorator *decorator) {
    switch (decorator->type) {
        case BLACKBOARD_DECORATOR_EXISTS: {
            bool exists = blackboard_has_key(blackboard, decorator->key);
            return decorator->negate_result ? !exists : exists;
        }
        
        case BLACKBOARD_DECORATOR_NOT_EXISTS: {
            bool exists = blackboard_has_key(blackboard, decorator->key);
            return decorator->negate_result ? exists : !exists;
        }
        
        case BLACKBOARD_DECORATOR_EQUALS: {
            f32 value;
            if (!blackboard_get_float(blackboard, decorator->key, &value)) {
                return decorator->negate_result; // Key doesn't exist
            }
            bool equals = (fabsf(value - decorator->threshold.float_threshold) < 0.001f);
            return decorator->negate_result ? !equals : equals;
        }
        
        case BLACKBOARD_DECORATOR_GREATER: {
            f32 value;
            if (!blackboard_get_float(blackboard, decorator->key, &value)) {
                return decorator->negate_result; // Key doesn't exist
            }
            bool greater = (value > decorator->threshold.float_threshold);
            return decorator->negate_result ? !greater : greater;
        }
        
        case BLACKBOARD_DECORATOR_CHANGED: {
            BlackboardEntry *entry = find_entry(blackboard, decorator->key);
            if (!entry) {
                return decorator->negate_result; // Key doesn't exist
            }
            
            bool changed = (entry->version != decorator->last_check_version);
            decorator->last_check_version = entry->version;
            return decorator->negate_result ? !changed : changed;
        }
        
        case BLACKBOARD_DECORATOR_TIMEOUT: {
            f64 current_time = time(NULL);
            f64 elapsed = current_time - decorator->last_check_time;
            
            if (elapsed >= decorator->threshold.timeout_seconds) {
                decorator->last_check_time = current_time;
                return decorator->negate_result ? false : true;
            }
            
            return decorator->negate_result ? true : false;
        }
        
        default:
            LOG_ERROR("Unknown decorator type: %d", decorator->type);
            return false;
    }
}

// ============================================================================
// PUBLIC API IMPLEMENTATION
// ============================================================================

Blackboard *blackboard_create(u32 max_entries, const char *owner_name) {
    if (!owner_name) {
        LOG_ERROR("Blackboard create failed: owner_name is NULL");
        return NULL;
    }
    if (max_entries == 0) {
        LOG_ERROR("Blackboard create failed: max_entries is 0");
        return NULL;
    }
    
    Blackboard *blackboard = malloc(sizeof(Blackboard));
    if (!blackboard) {
        LOG_ERROR("Failed to allocate blackboard");
        return NULL;
    }
    
    memset(blackboard, 0, sizeof(Blackboard));
    
    blackboard->max_entries = max_entries;
    strncpy(blackboard->owner_name, owner_name, sizeof(blackboard->owner_name) - 1);
    
    // Initialize hash table
    if (!initialize_hash_table(blackboard)) {
        free(blackboard);
        return NULL;
    }
    
    // Initialize memory pool
    if (!initialize_memory_pool(blackboard, 0)) {
        free(blackboard->hash_table);
        free(blackboard->hash_collision_count);
        free(blackboard);
        return NULL;
    }
    
    // Allocate entries array
    blackboard->entries = calloc(max_entries, sizeof(BlackboardEntry));
    if (!blackboard->entries) {
        free(blackboard->memory_pool);
        free(blackboard->hash_table);
        free(blackboard->hash_collision_count);
        free(blackboard);
        return NULL;
    }
    
    // Allocate decorators array
    blackboard->max_decorators = max_entries / 4; // 25% of entries can have decorators
    blackboard->decorators = calloc(blackboard->max_decorators, sizeof(BlackboardDecorator));
    
    LOG_INFO("Blackboard created for '%s': max_entries=%u", owner_name, max_entries);
    return blackboard;
}

void blackboard_destroy(Blackboard *blackboard) {
    if (!blackboard) return;
    
    blackboard_shutdown(blackboard);
    
    if (blackboard->entries) free(blackboard->entries);
    if (blackboard->decorators) free(blackboard->decorators);
    if (blackboard->hash_table) free(blackboard->hash_table);
    if (blackboard->hash_collision_count) free(blackboard->hash_collision_count);
    if (blackboard->memory_pool) memory_free(blackboard->memory_pool);
    
    free(blackboard);
    LOG_INFO("Blackboard destroyed for '%s'", blackboard->owner_name);
}

bool blackboard_set_float(Blackboard *blackboard, const char *key, f32 value, bool persistent) {
    BLACKBOARD_CHECK_NULL_PARAM(blackboard);
    BLACKBOARD_CHECK_NULL_PARAM(key);
    BLACKBOARD_CHECK_ERROR(blackboard->entry_count < blackboard->max_entries, BLACKBOARD_ERROR_OUT_OF_MEMORY);
    BLACKBOARD_CHECK_ERROR(strlen(key) <= MAX_KEY_LENGTH, BLACKBOARD_ERROR_INVALID_PARAMETER);
    
    // Check if key already exists
    BlackboardEntry *entry = find_entry(blackboard, key);
    if (entry) {
        if (entry->is_read_only) {
            LOG_WARN("Attempted to modify read-only key: %s", key);
            return false;
        }
        
        entry->data.float_value = value;
        entry->version++;
        entry->last_access_time = time(NULL);
        return true;
    }
    
    // Create new entry
    entry = &blackboard->entries[blackboard->entry_count];
    memset(entry, 0, sizeof(BlackboardEntry));
    
    strncpy(entry->key, key, sizeof(entry->key) - 1);
    entry->type = BLACKBOARD_TYPE_FLOAT;
    entry->data.float_value = value;
    entry->is_persistent = persistent;
    entry->creation_time = time(NULL);
    entry->last_access_time = entry->creation_time;
    entry->version = 1;
    
    // Add to hash table
    if (!add_entry_to_hash_table(blackboard, entry)) {
        return false;
    }
    
    blackboard->entry_count++;
    
    if (blackboard->is_debug_enabled) {
        LOG_DEBUG("Blackboard set float: %s = %.3f", key, value);
    }
    
    return true;
}

bool blackboard_get_float(Blackboard *blackboard, const char *key, f32 *value) {
    BLACKBOARD_CHECK_NULL_PARAM(blackboard);
    BLACKBOARD_CHECK_NULL_PARAM(key);
    BLACKBOARD_CHECK_NULL_PARAM(value);
    
    BLACKBOARD_START_TIMER(blackboard);
    
    BlackboardEntry *entry = find_entry(blackboard, key);
    if (!entry) {
        blackboard->cache_misses++;
        BLACKBOARD_END_TIMER(blackboard, NULL);
        return false;
    }
    
    if (entry->type != BLACKBOARD_TYPE_FLOAT) {
        LOG_ERROR("Type mismatch for key '%s': expected FLOAT, got %s", 
                 key, blackboard_get_type_name(entry->type));
        blackboard->cache_misses++;
        BLACKBOARD_END_TIMER(blackboard, NULL);
        return false;
    }
    
    *value = entry->data.float_value;
    entry->last_access_time = time(NULL);
    entry->access_count++;
    blackboard->cache_hits++;
    
    BLACKBOARD_END_TIMER(blackboard, NULL);
    
    if (blackboard->is_debug_enabled) {
        LOG_DEBUG("Blackboard get float: %s = %.3f", key, *value);
    }
    
    return true;
}

bool blackboard_set_int(Blackboard *blackboard, const char *key, i32 value, bool persistent) {
    BLACKBOARD_CHECK_NULL_PARAM(blackboard);
    BLACKBOARD_CHECK_NULL_PARAM(key);
    BLACKBOARD_CHECK_ERROR(blackboard->entry_count < blackboard->max_entries, BLACKBOARD_ERROR_OUT_OF_MEMORY);
    
    // Check if key already exists
    BlackboardEntry *entry = find_entry(blackboard, key);
    if (entry) {
        if (entry->is_read_only) {
            LOG_WARN("Attempted to modify read-only key: %s", key);
            return false;
        }
        
        entry->data.int_value = value;
        entry->version++;
        entry->last_access_time = time(NULL);
        return true;
    }
    
    // Create new entry
    entry = &blackboard->entries[blackboard->entry_count];
    memset(entry, 0, sizeof(BlackboardEntry));
    
    strncpy(entry->key, key, sizeof(entry->key) - 1);
    entry->type = BLACKBOARD_TYPE_INT;
    entry->data.int_value = value;
    entry->is_persistent = persistent;
    entry->creation_time = time(NULL);
    entry->last_access_time = entry->creation_time;
    entry->version = 1;
    
    // Add to hash table
    if (!add_entry_to_hash_table(blackboard, entry)) {
        return false;
    }
    
    blackboard->entry_count++;
    
    if (blackboard->is_debug_enabled) {
        LOG_DEBUG("Blackboard set int: %s = %d", key, value);
    }
    
    return true;
}

bool blackboard_get_int(Blackboard *blackboard, const char *key, i32 *value) {
    BLACKBOARD_CHECK_NULL_PARAM(blackboard);
    BLACKBOARD_CHECK_NULL_PARAM(key);
    BLACKBOARD_CHECK_NULL_PARAM(value);
    
    BLACKBOARD_START_TIMER(blackboard);
    
    BlackboardEntry *entry = find_entry(blackboard, key);
    if (!entry) {
        blackboard->cache_misses++;
        BLACKBOARD_END_TIMER(blackboard, NULL);
        return false;
    }
    
    if (entry->type != BLACKBOARD_TYPE_INT) {
        LOG_ERROR("Type mismatch for key '%s': expected INT, got %s", 
                 key, blackboard_get_type_name(entry->type));
        blackboard->cache_misses++;
        BLACKBOARD_END_TIMER(blackboard, NULL);
        return false;
    }
    
    *value = entry->data.int_value;
    entry->last_access_time = time(NULL);
    entry->access_count++;
    blackboard->cache_hits++;
    
    BLACKBOARD_END_TIMER(blackboard, NULL);
    
    if (blackboard->is_debug_enabled) {
        LOG_DEBUG("Blackboard get int: %s = %d", key, *value);
    }
    
    return true;
}

bool blackboard_has_key(Blackboard *blackboard, const char *key) {
    BLACKBOARD_CHECK_NULL_PARAM(blackboard);
    BLACKBOARD_CHECK_NULL_PARAM(key);
    
    return find_entry(blackboard, key) != NULL;
}

BlackboardType blackboard_get_type(Blackboard *blackboard, const char *key) {
    if (!blackboard || !key) return BLACKBOARD_TYPE_CUSTOM;
    
    BlackboardEntry *entry = find_entry(blackboard, key);
    return entry ? entry->type : BLACKBOARD_TYPE_CUSTOM;
}

bool blackboard_add_decorator_exists(Blackboard *blackboard, const char *key, bool negate) {
    BLACKBOARD_CHECK_NULL_PARAM(blackboard);
    BLACKBOARD_CHECK_NULL_PARAM(key);
    BLACKBOARD_CHECK_ERROR(blackboard->decorator_count < blackboard->max_decorators, BLACKBOARD_ERROR_OUT_OF_MEMORY);
    
    BlackboardDecorator *decorator = &blackboard->decorators[blackboard->decorator_count];
    memset(decorator, 0, sizeof(BlackboardDecorator));
    
    strncpy(decorator->key, key, sizeof(decorator->key) - 1);
    decorator->type = BLACKBOARD_DECORATOR_EXISTS;
    decorator->negate_result = negate;
    decorator->last_check_time = time(NULL);
    
    blackboard->decorator_count++;
    
    LOG_DEBUG("Added EXISTS decorator for key '%s' (negate=%s)", key, negate ? "true" : "false");
    return true;
}

bool blackboard_evaluate_decorators(Blackboard *blackboard) {
    BLACKBOARD_CHECK_NULL_PARAM(blackboard);
    
    if (blackboard->decorator_count == 0) {
        return true; // No decorators means always pass
    }
    
    for (u32 i = 0; i < blackboard->decorator_count; i++) {
        if (!evaluate_decorator(blackboard, &blackboard->decorators[i])) {
            return false; // Any decorator failing means overall failure
        }
    }
    
    return true; // All decorators passed
}

BlackboardStats *blackboard_get_statistics(Blackboard *blackboard) {
    if (!blackboard) return NULL;
    
    static BlackboardStats stats;
    memset(&stats, 0, sizeof(BlackboardStats));
    
    stats.entry_count = blackboard->entry_count;
    stats.decorator_count = blackboard->decorator_count;
    stats.total_lookups = blackboard->total_lookups;
    stats.cache_hits = blackboard->cache_hits;
    stats.cache_misses = blackboard->cache_misses;
    stats.cache_hit_rate = (blackboard->total_lookups > 0) ? 
                          (f32)blackboard->cache_hits / blackboard->total_lookups : 0.0f;
    stats.average_lookup_time = (blackboard->total_lookups > 0) ? 
                               blackboard->total_lookup_time / blackboard->total_lookups : 0.0;
    stats.memory_usage = blackboard->pool_used;
    
    return &stats;
}

void blackboard_print_statistics(Blackboard *blackboard) {
    if (!blackboard) return;
    
    BlackboardStats *stats = blackboard_get_statistics(blackboard);
    
    LOG_INFO("=== Blackboard Statistics for '%s' ===", blackboard->owner_name);
    LOG_INFO("Entries: %u / %u", stats->entry_count, blackboard->max_entries);
    LOG_INFO("Decorators: %u / %u", stats->decorator_count, blackboard->max_decorators);
    LOG_INFO("Total Lookups: %u", stats->total_lookups);
    LOG_INFO("Cache Hit Rate: %.2f%%", stats->cache_hit_rate * 100.0f);
    LOG_INFO("Average Lookup Time: %.3fs", stats->average_lookup_time * 1000000.0);
    LOG_INFO("Memory Usage: %zu / %zu bytes", stats->memory_usage, blackboard->pool_size);
    LOG_INFO("=============================================");
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

const char *blackboard_get_type_name(BlackboardType type) {
    switch (type) {
        case BLACKBOARD_TYPE_FLOAT: return "FLOAT";
        case BLACKBOARD_TYPE_INT: return "INT";
        case BLACKBOARD_TYPE_BOOL: return "BOOL";
        case BLACKBOARD_TYPE_VECTOR3: return "VECTOR3";
        case BLACKBOARD_TYPE_VECTOR4: return "VECTOR4";
        case BLACKBOARD_TYPE_ENTITY: return "ENTITY";
        case BLACKBOARD_TYPE_STRING: return "STRING";
        case BLACKBOARD_TYPE_TRANSFORM: return "TRANSFORM";
        case BLACKBOARD_TYPE_COLOR: return "COLOR";
        case BLACKBOARD_TYPE_QUATERNION: return "QUATERNION";
        case BLACKBOARD_TYPE_ARRAY: return "ARRAY";
        case BLACKBOARD_TYPE_CUSTOM: return "CUSTOM";
        default: return "UNKNOWN";
    }
}

const char *blackboard_get_error_string(BlackboardError error) {
    switch (error) {
        case BLACKBOARD_ERROR_NONE: return "No error";
        case BLACKBOARD_ERROR_INVALID_PARAMETER: return "Invalid parameter";
        case BLACKBOARD_ERROR_KEY_NOT_FOUND: return "Key not found";
        case BLACKBOARD_ERROR_TYPE_MISMATCH: return "Type mismatch";
        case BLACKBOARD_ERROR_OUT_OF_MEMORY: return "Out of memory";
        case BLACKBOARD_ERROR_KEY_EXISTS: return "Key already exists";
        case BLACKBOARD_ERROR_READ_ONLY: return "Key is read-only";
        case BLACKBOARD_ERROR_NETWORK_ERROR: return "Network error";
        default: return "Unknown error";
    }
}

bool blackboard_is_valid_key(const char *key) {
    if (!key || strlen(key) == 0 || strlen(key) > MAX_KEY_LENGTH) {
        return false;
    }
    
    // Check for valid characters (alphanumeric, underscore)
    for (const char *p = key; *p; p++) {
        if (!((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || 
              (*p >= '0' && *p <= '9') || *p == '_')) {
            return false;
        }
    }
    
    return true;
}
