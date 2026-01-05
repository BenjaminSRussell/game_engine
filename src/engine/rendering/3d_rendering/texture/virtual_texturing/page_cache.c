/*
 * page_cache.c
 * Virtual texture physical page cache implementation
 */

#include "page_cache.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int page_cache_init(page_cache_t* cache, uint32_t width, uint32_t height) {
    if (!cache) return -1;
    
    cache->width = width;
    cache->height = height;
    cache->entries = calloc(width * height, sizeof(cache_entry_t));
    if (!cache->entries) return -2;
    
    // Physical texture handle should be created here
    cache->physical_texture_handle = 0; 
    
    return 0;
}

void page_cache_shutdown(page_cache_t* cache) {
    if (!cache) return;
    if (cache->entries) free(cache->entries);
    memset(cache, 0, sizeof(page_cache_t));
}

int page_cache_allocate(page_cache_t* cache, uint32_t virtual_x, uint32_t virtual_y, uint32_t* out_phys_x, uint32_t* out_phys_y) {
    if (!cache) return -1;
    
    // Find an empty or LRU page
    uint32_t lru_index = 0;
    uint32_t min_frame = 0xFFFFFFFF;
    bool found_empty = false;
    
    for (uint32_t i = 0; i < cache->width * cache->height; i++) {
        if (!cache->entries[i].active) {
            lru_index = i;
            found_empty = true;
            break;
        }
        if (cache->entries[i].last_used_frame < min_frame) {
            min_frame = cache->entries[i].last_used_frame;
            lru_index = i;
        }
    }
    
    *out_phys_x = lru_index % cache->width;
    *out_phys_y = lru_index / cache->width;
    
    cache->entries[lru_index].virtual_page_x = virtual_x;
    cache->entries[lru_index].virtual_page_y = virtual_y;
    cache->entries[lru_index].active = true;
    
    return 0;
}

void page_cache_free(page_cache_t* cache, uint32_t phys_x, uint32_t phys_y) {
    if (!cache || phys_x >= cache->width || phys_y >= cache->height) return;
    
    uint32_t index = phys_y * cache->width + phys_x;
    cache->entries[index].active = false;
}

void page_cache_touch(page_cache_t* cache, uint32_t phys_x, uint32_t phys_y, uint32_t frame) {
    if (!cache || phys_x >= cache->width || phys_y >= cache->height) return;
    
    uint32_t index = phys_y * cache->width + phys_x;
    cache->entries[index].last_used_frame = frame;
}

int page_cache_upload_page(page_cache_t* cache, uint32_t phys_x, uint32_t phys_y, const void* data) {
    if (!cache || !data) return -1;
    
    // Placeholder for GPU texture sub-resource update
    // update_gpu_texture(cache->physical_texture_handle, phys_x * PAGE_SIZE, phys_y * PAGE_SIZE, data);
    
    return 0;
}

/* Original stub compatibility */
int texture_page_cache_init(void) {
    return 0;
}

void texture_page_cache_shutdown(void) {
}

