/*
 * page_cache.c
 * Implementation of LRU cache for cluster data
 */

#include "geometry/nanite/nanite/streaming/page_cache.h"
#include <stdlib.h>
#include <string.h>

static cache_entry_t g_cache_entries[MAX_CACHE_PAGES];
static uint32_t g_current_frame = 0;

void page_cache_init(size_t total_memory) {
    memset(g_cache_entries, 0, sizeof(g_cache_entries));
    for (int i = 0; i < MAX_CACHE_PAGES; i++) {
        g_cache_entries[i].status = PAGE_EMPTY;
        g_cache_entries[i].page_id = 0xFFFFFFFF;
    }
}

void page_cache_shutdown(void) {
    // Free resources
}

void* page_cache_acquire(uint32_t page_id) {
    // Find in cache
    for (int i = 0; i < MAX_CACHE_PAGES; i++) {
        if (g_cache_entries[i].page_id == page_id) {
            g_cache_entries[i].last_used_frame = g_current_frame;
            return (void*)0xCA123; // Placeholder data
        }
    }

    // Not found: evict if necessary and load
    page_cache_evict_lru();
    return NULL;
}

void page_cache_release(uint32_t page_id) {
    // Release lock if implemented
}

void page_cache_evict_lru(void) {
    uint32_t oldest_frame = 0xFFFFFFFF;
    int oldest_idx = -1;

    for (int i = 0; i < MAX_CACHE_PAGES; i++) {
        if (g_cache_entries[i].status == PAGE_RESIDENT && g_cache_entries[i].last_used_frame < oldest_frame) {
            oldest_frame = g_cache_entries[i].last_used_frame;
            oldest_idx = i;
        }
    }

    if (oldest_idx != -1) {
        g_cache_entries[oldest_idx].status = PAGE_EMPTY;
        g_cache_entries[oldest_idx].page_id = 0xFFFFFFFF;
    }
}
