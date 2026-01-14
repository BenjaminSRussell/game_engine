#include "core/string/string_pool.h"
#include "core/containers_advanced/hash_map.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/**
 * =================================================================================================
 *                          INTERNED STRING POOL IMPLEMENTATION
 * =================================================================================================
 */

// Simple FNV-1a for strings
static uint64_t str_hash(const char* str) {
    uint64_t hash = 14695981039346656037ULL;
    while (*str) {
        hash ^= (unsigned char)*str++;
        hash *= 1099511628211ULL;
    }
    return hash;
}

typedef struct StringNode {
    char* str;
    uint32_t length;
    uint32_t ref_count;
    struct StringNode* next;
} StringNode;

struct StringPool {
    // Simple bucket array
    StringNode** buckets;
    size_t capacity;
    size_t count;
};

StringPool* string_pool_create(size_t capacity) {
    StringPool* pool = (StringPool*)malloc(sizeof(StringPool));
    if (!pool) return NULL;
    
    if (capacity < 16) capacity = 16;
    pool->capacity = capacity;
    pool->count = 0;
    pool->buckets = (StringNode**)calloc(capacity, sizeof(StringNode*));
    
    return pool;
}

void string_pool_destroy(StringPool* pool) {
    if (!pool) return;
    
    for (size_t i = 0; i < pool->capacity; i++) {
        StringNode* node = pool->buckets[i];
        while (node) {
            StringNode* next = node->next;
            free(node->str);
            free(node);
            node = next;
        }
    }
    free(pool->buckets);
    free(pool);
}

const char* string_intern(StringPool* pool, const char* str) {
    if (!pool || !str) return NULL;
    
    uint64_t hash = str_hash(str);
    size_t idx = hash % pool->capacity;
    
    // Search existing
    StringNode* node = pool->buckets[idx];
    while (node) {
        if (strcmp(node->str, str) == 0) {
            node->ref_count++;
            return node->str;
        }
        node = node->next;
    }
    
    // Create new
    node = (StringNode*)malloc(sizeof(StringNode));
    node->str = strdup(str);
    node->length = (uint32_t)strlen(str);
    node->ref_count = 1;
    node->next = pool->buckets[idx];
    pool->buckets[idx] = node;
    pool->count++;
    
    return node->str;
}

void string_release(StringPool* pool, const char* str) {
    if (!pool || !str) return;
    
    uint64_t hash = str_hash(str);
    size_t idx = hash % pool->capacity;
    
    StringNode* node = pool->buckets[idx];
    StringNode* prev = NULL;
    
    while (node) {
        if (strcmp(node->str, str) == 0) { // Pointer cmp might suffice if strictly interned
            node->ref_count--;
            if (node->ref_count == 0) {
                // Remove
                if (prev) prev->next = node->next;
                else pool->buckets[idx] = node->next;
                
                free(node->str);
                free(node);
                pool->count--;
            }
            return;
        }
        prev = node;
        node = node->next;
    }
}
