/*
 * page_cache.h
 * LRU cache for Nanite cluster data
 */

#ifndef PAGE_CACHE_H
#define PAGE_CACHE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Cache settings
#define MAX_CACHE_PAGES 1024

// Page status
typedef enum page_status {
    PAGE_EMPTY,
    PAGE_LOADING,
    PAGE_RESIDENT
} page_status_t;

// Cache entry
typedef struct cache_entry {
    uint32_t page_id;
    page_status_t status;
    uint32_t last_used_frame;
} cache_entry_t;

// Cache API
void page_cache_init(size_t total_memory);
void page_cache_shutdown(void);

// Request a page from cache
void* page_cache_acquire(uint32_t page_id);
void page_cache_release(uint32_t page_id);

// Eviction
void page_cache_evict_lru(void);

#endif // PAGE_CACHE_H
