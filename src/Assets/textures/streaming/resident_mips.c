/*
 * resident_mips.c
 * Resident mip tracking implementation
 */

#include "assets/textures/streaming/resident_mips.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct texture_residency {
    uint32_t handle;
    uint32_t resident_mask; // Bitmask of resident mips
    uint32_t max_mips;
} texture_residency_t;

typedef struct residency_context {
    texture_residency_t* items;
    uint32_t count;
    uint32_t capacity;
    size_t total_memory_used;
    bool initialized;
} residency_context_t;

static residency_context_t g_residency_ctx = {0};

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int texture_resident_mips_init(void) {
    if (g_residency_ctx.initialized) return 0;
    
    g_residency_ctx.capacity = 1024;
    g_residency_ctx.items = calloc(g_residency_ctx.capacity, sizeof(texture_residency_t));
    if (!g_residency_ctx.items) return -1;
    
    g_residency_ctx.count = 0;
    g_residency_ctx.total_memory_used = 0;
    g_residency_ctx.initialized = true;
    
    return 0;
}

void texture_resident_mips_shutdown(void) {
    if (!g_residency_ctx.initialized) return;
    
    if (g_residency_ctx.items) free(g_residency_ctx.items);
    memset(&g_residency_ctx, 0, sizeof(residency_context_t));
}

int texture_residency_set_mip_status(uint32_t handle, uint32_t mip, bool resident) {
    if (!g_residency_ctx.initialized) return -1;
    
    texture_residency_t* entry = NULL;
    for (uint32_t i = 0; i < g_residency_ctx.count; i++) {
        if (g_residency_ctx.items[i].handle == handle) {
            entry = &g_residency_ctx.items[i];
            break;
        }
    }
    
    if (!entry) {
        if (g_residency_ctx.count >= g_residency_ctx.capacity) return -2;
        entry = &g_residency_ctx.items[g_residency_ctx.count++];
        entry->handle = handle;
        entry->resident_mask = 0;
        entry->max_mips = 16; // Default
    }
    
    bool current_status = (entry->resident_mask & (1 << mip)) != 0;
    if (current_status == resident) return 0; // No change
    
    size_t mip_size = texture_residency_get_mip_memory_size(handle, mip);
    
    if (resident) {
        entry->resident_mask |= (1 << mip);
        g_residency_ctx.total_memory_used += mip_size;
    } else {
        entry->resident_mask &= ~(1 << mip);
        g_residency_ctx.total_memory_used -= mip_size;
    }
    
    return 0;
}

int texture_residency_get_highest_resident_mip(uint32_t handle) {
    if (!g_residency_ctx.initialized) return -1;
    
    for (uint32_t i = 0; i < g_residency_ctx.count; i++) {
        if (g_residency_ctx.items[i].handle == handle) {
            uint32_t mask = g_residency_ctx.items[i].resident_mask;
            if (mask == 0) return -1;
            
            // Find lowest bit set (mip 0 is highest resolution)
            for (int m = 0; m < 32; m++) {
                if (mask & (1 << m)) return m;
            }
        }
    }
    
    return -1;
}

bool texture_residency_is_mip_resident(uint32_t handle, uint32_t mip) {
    if (!g_residency_ctx.initialized) return false;
    
    for (uint32_t i = 0; i < g_residency_ctx.count; i++) {
        if (g_residency_ctx.items[i].handle == handle) {
            return (g_residency_ctx.items[i].resident_mask & (1 << mip)) != 0;
        }
    }
    
    return false;
}

size_t texture_residency_get_mip_memory_size(uint32_t handle, uint32_t mip) {
    (void)handle;
    // VERY simplified: assume 1024x1024 base mip, RGBA8
    // Mip 0: 4MB, Mip 1: 1MB, Mip 2: 256KB, etc.
    size_t base_size = 4 * 1024 * 1024;
    return base_size >> (mip * 2);
}

size_t texture_residency_get_total_memory_usage(void) {
    return g_residency_ctx.total_memory_used;
}

void texture_residency_update_all(void) {
    // Background garbage collection or similar could go here
}

