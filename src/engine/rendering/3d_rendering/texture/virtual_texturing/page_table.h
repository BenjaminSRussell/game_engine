/*
 * page_table.h
 * Virtual texture page table
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

typedef struct texture_page_table_handle {
    uint32_t id;
} texture_page_table_handle_t;

typedef struct texture_page_table_desc {
    uint32_t flags;
    void* user_data;
} texture_page_table_desc_t;

typedef struct texture_page_table_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} texture_page_table_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int texture_page_table_init(void);
void texture_page_table_shutdown(void);

/* Lifecycle */
int texture_page_table_create(texture_page_table_handle_t* out_handle, const texture_page_table_desc_t* desc);
void texture_page_table_destroy(texture_page_table_handle_t handle);

/* Operations */
int texture_page_table_update(texture_page_table_handle_t handle, const void* data, size_t size);
bool texture_page_table_is_valid(texture_page_table_handle_t handle);
int texture_page_table_get_info(texture_page_table_handle_t handle, texture_page_table_info_t* out_info);
void texture_page_table_mark_dirty(texture_page_table_handle_t handle);
int texture_page_table_process_pending(void);

/* Statistics */
uint32_t texture_page_table_get_count(void);
size_t texture_page_table_get_memory_usage(void);
void texture_page_table_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* TEXTURE_PAGE_TABLE_H */
