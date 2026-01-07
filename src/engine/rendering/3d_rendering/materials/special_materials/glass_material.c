/*
 * glass_material.c
 * Glass/transparent materials Implementation
 */

#include "glass_material.h"
#include <core/logger.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * INTERNAL STATE
 * ============================================================================ */

#define MAX_GLASS_MATERIALS 64

typedef struct {
    uint32_t id;
    materials_glass_material_desc_t desc;
    bool in_use;
} GlassMaterialData;

static struct {
    GlassMaterialData materials[MAX_GLASS_MATERIALS];
    uint32_t count;
    bool initialized;
} g_glass_system;

/* ============================================================================
 * IMPLEMENTATION
 * ============================================================================ */

int materials_glass_material_init(void) {
    if (g_glass_system.initialized) return 0;
    
    memset(&g_glass_system, 0, sizeof(g_glass_system));
    g_glass_system.initialized = true;
    
    LOG_INFO("GlassMaterialSystem: Initialized.");
    return 0;
}

void materials_glass_material_shutdown(void) {
    if (!g_glass_system.initialized) return;
    
    g_glass_system.initialized = false;
    // TODO: Release any GPU resources
}

int materials_glass_material_create(materials_glass_material_handle_t* out_handle, const materials_glass_material_desc_t* desc) {
    if (!g_glass_system.initialized || !out_handle || !desc) return -1;
    
    // Find free slot
    for (int i = 0; i < MAX_GLASS_MATERIALS; i++) {
        if (!g_glass_system.materials[i].in_use) {
            g_glass_system.materials[i].in_use = true;
            g_glass_system.materials[i].id = i + 1; // 1-based ID
            g_glass_system.materials[i].desc = *desc;
            
            out_handle->id = g_glass_system.materials[i].id;
            g_glass_system.count++;
            
            // TODO: Upload material data to GPU uniform buffer
            
            return 0;
        }
    }
    
    LOG_ERROR("GlassMaterialSystem: Out of slots!");
    return -2;
}

void materials_glass_material_destroy(materials_glass_material_handle_t handle) {
    if (!g_glass_system.initialized || handle.id == 0) return;
    
    uint32_t index = handle.id - 1;
    if (index < MAX_GLASS_MATERIALS && g_glass_system.materials[index].in_use) {
        g_glass_system.materials[index].in_use = false;
        g_glass_system.count--;
    }
}

int materials_glass_material_update(materials_glass_material_handle_t handle, const materials_glass_material_desc_t* desc) {
    if (!g_glass_system.initialized || handle.id == 0 || !desc) return -1;
    
    uint32_t index = handle.id - 1;
    if (index < MAX_GLASS_MATERIALS && g_glass_system.materials[index].in_use) {
        g_glass_system.materials[index].desc = *desc;
        // TODO: Update GPU buffer
        return 0;
    }
    return -1;
}

bool materials_glass_material_is_valid(materials_glass_material_handle_t handle) {
    if (!g_glass_system.initialized || handle.id == 0) return false;
    uint32_t index = handle.id - 1;
    return index < MAX_GLASS_MATERIALS && g_glass_system.materials[index].in_use;
}

int materials_glass_material_get_info(materials_glass_material_handle_t handle, materials_glass_material_info_t* out_info) {
     if (!g_glass_system.initialized || handle.id == 0 || !out_info) return -1;
     uint32_t index = handle.id - 1;
     
     if (index < MAX_GLASS_MATERIALS && g_glass_system.materials[index].in_use) {
         out_info->id = handle.id;
         out_info->flags = g_glass_system.materials[index].desc.flags;
         out_info->initialized = true;
         return 0;
     }
     return -1;
}
