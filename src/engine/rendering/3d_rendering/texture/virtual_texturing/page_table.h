/*
 * page_table.h
 * Virtual texture page table management
 *
 * Part of the Texture subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef TEXTURE_PAGE_TABLE_H
#define TEXTURE_PAGE_TABLE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct page_entry {
    uint16_t physical_x;
    uint16_t physical_y;
    uint8_t mip_level;
    uint8_t flags;
} page_entry_t;

typedef struct page_table {
    page_entry_t* entries;
    uint32_t width;
    uint32_t height;
    uint32_t gpu_buffer_handle;
} page_table_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Lifecycle */
int page_table_init(page_table_t* pt, uint32_t width, uint32_t height);
void page_table_shutdown(page_table_t* pt);

/* Operations */
void page_table_map(page_table_t* pt, uint32_t virtual_x, uint32_t virtual_y, uint32_t physical_x, uint32_t physical_y, uint32_t mip);
void page_table_unmap(page_table_t* pt, uint32_t virtual_x, uint32_t virtual_y);
void page_table_update_gpu(page_table_t* pt);

/* Original stub compatibility */
int texture_page_table_init(void);
void texture_page_table_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* TEXTURE_PAGE_TABLE_H */

