/*
 * page_cache.h
 * Virtual texture physical page cache
 *
 * Part of the Texture subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef TEXTURE_PAGE_CACHE_H
#define TEXTURE_PAGE_CACHE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct cache_entry {
    uint32_t virtual_page_x;
    uint32_t virtual_page_y;
    uint32_t last_used_frame;
    bool active;
} cache_entry_t;

typedef struct page_cache {
    cache_entry_t* entries;
    uint32_t width;
    uint32_t height;
    uint32_t physical_texture_handle;
    uint32_t free_list_head;
} page_cache_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Lifecycle */
int page_cache_init(page_cache_t* cache, uint32_t width, uint32_t height);
void page_cache_shutdown(page_cache_t* cache);

/* Operations */
int page_cache_allocate(page_cache_t* cache, uint32_t virtual_x, uint32_t virtual_y, uint32_t* out_phys_x, uint32_t* out_phys_y);
void page_cache_free(page_cache_t* cache, uint32_t phys_x, uint32_t phys_y);
void page_cache_touch(page_cache_t* cache, uint32_t phys_x, uint32_t phys_y, uint32_t frame);

/* Page Upload */
int page_cache_upload_page(page_cache_t* cache, uint32_t phys_x, uint32_t phys_y, const void* data);

/* Original stub compatibility */
int texture_page_cache_init(void);
void texture_page_cache_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* TEXTURE_PAGE_CACHE_H */

