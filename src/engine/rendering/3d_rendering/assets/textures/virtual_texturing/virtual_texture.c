/*
 * virtual_texture.c
 * Virtual texturing system implementation
 */

#include "virtual_texture.h"
#include "page_table.h"
#include "page_cache.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * GLOBAL STATE
 * ============================================================================ */

static bool g_vt_system_initialized = false;

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int virtual_texture_system_init(void) {
    if (g_vt_system_initialized) return 0;
    
    // Initialize sub-systems if needed
    g_vt_system_initialized = true;
    return 0;
}

void virtual_texture_system_shutdown(void) {
    if (!g_vt_system_initialized) return;
    g_vt_system_initialized = false;
}

int virtual_texture_create(virtual_texture_t* vt, const virtual_texture_config_t* config) {
    if (!g_vt_system_initialized || !vt || !config) return -1;
    
    vt->config = *config;
    
    // Calculate page table dimensions
    uint32_t pt_width = config->virtual_width / config->page_size;
    uint32_t pt_height = config->virtual_height / config->page_size;
    
    // Initialize page table
    // For now, we'll use a simplified internal pointer rather than handles
    page_table_t* pt = malloc(sizeof(page_table_t));
    if (page_table_init(pt, pt_width, pt_height) != 0) {
        free(pt);
        return -2;
    }
    vt->page_table_handle = (uint32_t)(uintptr_t)pt; // Hacky handle for now
    
    // Initialize page cache
    // Assuming a fixed physical cache size for now (e.g., 2048x2048)
    page_cache_t* cache = malloc(sizeof(page_cache_t));
    if (page_cache_init(cache, 16, 16) != 0) { // 16x16 pages = 2048x2048 if 128x128
        page_table_shutdown(pt);
        free(pt);
        free(cache);
        return -3;
    }
    vt->physical_cache_handle = (uint32_t)(uintptr_t)cache;
    
    vt->initialized = true;
    return 0;
}

void virtual_texture_destroy(virtual_texture_t* vt) {
    if (!vt || !vt->initialized) return;
    
    page_table_t* pt = (page_table_t*)(uintptr_t)vt->page_table_handle;
    page_cache_t* cache = (page_cache_t*)(uintptr_t)vt->physical_cache_handle;
    
    page_table_shutdown(pt);
    page_cache_shutdown(cache);
    
    free(pt);
    free(cache);
    
    vt->initialized = false;
}

void virtual_texture_update(virtual_texture_t* vt) {
    if (!vt || !vt->initialized) return;
    
    // 1. Analyze feedback (placeholder)
    // 2. Request missing pages
    // 3. Update page table
    
    page_table_t* pt = (page_table_t*)(uintptr_t)vt->page_table_handle;
    page_table_update_gpu(pt);
}

int virtual_texture_translate_coord(const virtual_texture_t* vt, float u, float v, uint32_t* out_page_x, uint32_t* out_page_y) {
    if (!vt || !vt->initialized) return -1;
    
    // Simple wrapping
    u = u - (int)u; if (u < 0) u += 1.0f;
    v = v - (int)v; if (v < 0) v += 1.0f;
    
    uint32_t pt_width = vt->config.virtual_width / vt->config.page_size;
    uint32_t pt_height = vt->config.virtual_height / vt->config.page_size;
    
    *out_page_x = (uint32_t)(u * pt_width);
    *out_page_y = (uint32_t)(v * pt_height);
    
    return 0;
}

int virtual_texture_get_info(const virtual_texture_t* vt, texture_virtual_texture_info_t* out_info) {
    if (!vt || !out_info) return -1;
    
    out_info->virtual_width = vt->config.virtual_width;
    out_info->virtual_height = vt->config.virtual_height;
    out_info->page_count = (vt->config.virtual_width / vt->config.page_size) * (vt->config.virtual_height / vt->config.page_size);
    out_info->resident_pages = 0; // TODO: Track this in cache
    
    return 0;
}

/* Original stub compatibility */
int texture_virtual_texture_init(void) {
    return virtual_texture_system_init();
}

void texture_virtual_texture_shutdown(void) {
    virtual_texture_system_shutdown();
}

int texture_virtual_texture_create(texture_virtual_texture_handle_t* out_handle, const texture_virtual_texture_desc_t* desc) {
    static virtual_texture_t g_global_vt = {0};
    if (!g_global_vt.initialized) {
        virtual_texture_create(&g_global_vt, &desc->config);
    }
    out_handle->id = 0;
    return 0;
}

void texture_virtual_texture_destroy(texture_virtual_texture_handle_t handle) {
    (void)handle;
}

