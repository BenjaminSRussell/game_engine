/*
 * shadow_cache.c
 * Cache static shadow maps
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#include "shadow_cache.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct { struct { uint32_t handle; } id; } texture_handle_t;

typedef struct shadow_cache_entry {
    uint32_t light_id;
    texture_handle_t cached_shadow_map;
    bool valid;
    uint64_t last_update_frame;
} shadow_cache_entry_t;

#define MAX_CACHED_SHADOWS 32

typedef struct shadow_cache_context {
    shadow_cache_entry_t entries[MAX_CACHED_SHADOWS];
    uint32_t entry_count;
    bool enabled;
    bool initialized;
} shadow_cache_context_t;

static shadow_cache_context_t g_cache_ctx = {0};

/* ============================================================================
 * SHADOW CACHE
 * ============================================================================ */

int lighting_shadow_cache_add(uint32_t light_id, texture_handle_t shadow_map) {
    if (!g_cache_ctx.enabled || !g_cache_ctx.initialized) {
        return -1;
    }
    
    if (g_cache_ctx.entry_count >= MAX_CACHED_SHADOWS) {
        return -2;  // Cache full
    }
    
    // Check if already cached
    for (uint32_t i = 0; i < g_cache_ctx.entry_count; i++) {
        if (g_cache_ctx.entries[i].light_id == light_id) {
            // Update existing entry
            g_cache_ctx.entries[i].cached_shadow_map = shadow_map;
            g_cache_ctx.entries[i].valid = true;
            return 0;
        }
    }
    
    // Add new entry
    shadow_cache_entry_t* entry = &g_cache_ctx.entries[g_cache_ctx.entry_count++];
    entry->light_id = light_id;
    entry->cached_shadow_map = shadow_map;
    entry->valid = true;
    entry->last_update_frame = 0;
    
    return 0;
}

bool lighting_shadow_cache_get(uint32_t light_id, texture_handle_t* out_shadow_map) {
    if (!g_cache_ctx.enabled || !g_cache_ctx.initialized || !out_shadow_map) {
        return false;
    }
    
    for (uint32_t i = 0; i < g_cache_ctx.entry_count; i++) {
        if (g_cache_ctx.entries[i].light_id == light_id && g_cache_ctx.entries[i].valid) {
            *out_shadow_map = g_cache_ctx.entries[i].cached_shadow_map;
            return true;
        }
    }
    
    return false;
}

void lighting_shadow_cache_invalidate(uint32_t light_id) {
    if (!g_cache_ctx.initialized) return;
    
    for (uint32_t i = 0; i < g_cache_ctx.entry_count; i++) {
        if (g_cache_ctx.entries[i].light_id == light_id) {
            g_cache_ctx.entries[i].valid = false;
            break;
        }
    }
}

void lighting_shadow_cache_invalidate_all(void) {
    if (!g_cache_ctx.initialized) return;
    
    for (uint32_t i = 0; i < g_cache_ctx.entry_count; i++) {
        g_cache_ctx.entries[i].valid = false;
    }
}

void lighting_shadow_cache_clear(void) {
    if (!g_cache_ctx.initialized) return;
    
    g_cache_ctx.entry_count = 0;
    memset(g_cache_ctx.entries, 0, sizeof(g_cache_ctx.entries));
}

void lighting_shadow_cache_enable(bool enabled) {
    if (g_cache_ctx.initialized) {
        g_cache_ctx.enabled = enabled;
    }
}

bool lighting_shadow_cache_is_enabled(void) {
    return g_cache_ctx.enabled;
}

uint32_t lighting_shadow_cache_get_cached_count(void) {
    if (!g_cache_ctx.initialized) return 0;
    
    uint32_t count = 0;
    for (uint32_t i = 0; i < g_cache_ctx.entry_count; i++) {
        if (g_cache_ctx.entries[i].valid) {
            count++;
        }
    }
    return count;
}

/* ============================================================================
 * PUBLIC API (Compatibility)
 * ============================================================================ */

int lighting_shadow_cache_init(void) {
    if (g_cache_ctx.initialized) {
        return 0;
    }
    
    g_cache_ctx.entry_count = 0;
    g_cache_ctx.enabled = true;
    memset(g_cache_ctx.entries, 0, sizeof(g_cache_ctx.entries));
    g_cache_ctx.initialized = true;
    
    return 0;
}

void lighting_shadow_cache_shutdown(void) {
    if (!g_cache_ctx.initialized) {
        return;
    }
    
    g_cache_ctx.entry_count = 0;
    g_cache_ctx.initialized = false;
}

int lighting_shadow_cache_create(lighting_shadow_cache_handle_t* out_handle, 
                                 const lighting_shadow_cache_desc_t* desc) {
    if (!out_handle || !desc) return -1;
    out_handle->id = 0;
    return 0;
}

void lighting_shadow_cache_destroy(lighting_shadow_cache_handle_t handle) {
    (void)handle;
}

int lighting_shadow_cache_update(lighting_shadow_cache_handle_t handle, 
                                 const void* data, size_t size) {
    (void)handle; (void)data; (void)size;
    return 0;
}

bool lighting_shadow_cache_is_valid(lighting_shadow_cache_handle_t handle) {
    (void)handle;
    return g_cache_ctx.initialized;
}

int lighting_shadow_cache_get_info(lighting_shadow_cache_handle_t handle, 
                                   lighting_shadow_cache_info_t* out_info) {
    if (!out_info) return -1;
    out_info->id = handle.id;
    out_info->flags = 0;
    out_info->initialized = g_cache_ctx.initialized;
    return 0;
}

void lighting_shadow_cache_mark_dirty(lighting_shadow_cache_handle_t handle) {
    (void)handle;
}

int lighting_shadow_cache_process_pending(void) {
    return 0;
}

uint32_t lighting_shadow_cache_get_count(void) {
    return g_cache_ctx.entry_count;
}

size_t lighting_shadow_cache_get_memory_usage(void) {
    return sizeof(shadow_cache_context_t);
}

void lighting_shadow_cache_debug_print(void) {
    // Debug output
}

/* End of shadow_cache.c */
