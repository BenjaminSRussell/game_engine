/*
 * virtual_texture.h
 * Virtual texture system
 *
 * Part of the Texture subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef TEXTURE_VIRTUAL_TEXTURE_H
#define TEXTURE_VIRTUAL_TEXTURE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define VT_PAGE_SIZE 128
#define VT_VIRTUAL_SIZE (16384)
#define VT_MAX_PAGES ((VT_VIRTUAL_SIZE / VT_PAGE_SIZE) * (VT_VIRTUAL_SIZE / VT_PAGE_SIZE))

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct virtual_texture_config {
    uint32_t virtual_width;
    uint32_t virtual_height;
    uint32_t page_size;
    uint32_t border_size;
} virtual_texture_config_t;

typedef struct virtual_texture {
    virtual_texture_config_t config;
    uint32_t page_table_handle;
    uint32_t physical_cache_handle;
    bool initialized;
} virtual_texture_t;

typedef struct texture_virtual_texture_handle {
    uint32_t id;
} texture_virtual_texture_handle_t;

typedef struct texture_virtual_texture_desc {
    virtual_texture_config_t config;
} texture_virtual_texture_desc_t;

typedef struct texture_virtual_texture_info {
    uint32_t virtual_width;
    uint32_t virtual_height;
    uint32_t page_count;
    uint32_t resident_pages;
} texture_virtual_texture_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization & Cleanup */
int virtual_texture_system_init(void);
void virtual_texture_system_shutdown(void);

/* Lifecycle */
int virtual_texture_create(virtual_texture_t* vt, const virtual_texture_config_t* config);
void virtual_texture_destroy(virtual_texture_t* vt);

/* Operations */
void virtual_texture_update(virtual_texture_t* vt);
int virtual_texture_translate_coord(const virtual_texture_t* vt, float u, float v, uint32_t* out_page_x, uint32_t* out_page_y);

/* Statistics */
int virtual_texture_get_info(const virtual_texture_t* vt, texture_virtual_texture_info_t* out_info);

/* Original stub compatibility */
int texture_virtual_texture_init(void);
void texture_virtual_texture_shutdown(void);
int texture_virtual_texture_create(texture_virtual_texture_handle_t* out_handle, const texture_virtual_texture_desc_t* desc);
void texture_virtual_texture_destroy(texture_virtual_texture_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* TEXTURE_VIRTUAL_TEXTURE_H */

