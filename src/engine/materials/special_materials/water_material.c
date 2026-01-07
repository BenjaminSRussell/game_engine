/*
 * water_material.c
 * Water surface material Implementation
 */

#include "materials/special_materials/water_material.h"
#include <core/logger.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * INTERNAL STATE
 * ============================================================================ */

#define MAX_WATER_MATERIALS 16 // Generally fewer water types than glass

typedef struct {
    uint32_t id;
    materials_water_material_desc_t desc;
    bool in_use;
} WaterMaterialData;

static struct {
    WaterMaterialData materials[MAX_WATER_MATERIALS];
    uint32_t count;
    bool initialized;
} g_water_system;

/* ============================================================================
 * IMPLEMENTATION
 * ============================================================================ */

int materials_water_material_init(void) {
    if (g_water_system.initialized) return 0;
    
    memset(&g_water_system, 0, sizeof(g_water_system));
    g_water_system.initialized = true;
    
    LOG_INFO("WaterMaterialSystem: Initialized.");
    return 0;
}

void materials_water_material_shutdown(void) {
    if (!g_water_system.initialized) return;
    g_water_system.initialized = false;
}

int materials_water_material_create(materials_water_material_handle_t* out_handle, const materials_water_material_desc_t* desc) {
    if (!g_water_system.initialized || !out_handle || !desc) return -1;
    
    // Find free slot
    for (int i = 0; i < MAX_WATER_MATERIALS; i++) {
        if (!g_water_system.materials[i].in_use) {
            g_water_system.materials[i].in_use = true;
            g_water_system.materials[i].id = i + 1;
            g_water_system.materials[i].desc = *desc;
            
            out_handle->id = g_water_system.materials[i].id;
            g_water_system.count++;
            
            return 0;
        }
    }
    
    LOG_ERROR("WaterMaterialSystem: Out of slots!");
    return -2;
}

void materials_water_material_destroy(materials_water_material_handle_t handle) {
    if (!g_water_system.initialized || handle.id == 0) return;
    
    uint32_t index = handle.id - 1;
    if (index < MAX_WATER_MATERIALS && g_water_system.materials[index].in_use) {
        g_water_system.materials[index].in_use = false;
        g_water_system.count--;
    }
}

int materials_water_material_update(materials_water_material_handle_t handle, const materials_water_material_desc_t* desc) {
    if (!g_water_system.initialized || handle.id == 0 || !desc) return -1;
    
    uint32_t index = handle.id - 1;
    if (index < MAX_WATER_MATERIALS && g_water_system.materials[index].in_use) {
        g_water_system.materials[index].desc = *desc;
        return 0;
    }
    return -1;
}

bool materials_water_material_is_valid(materials_water_material_handle_t handle) {
    if (!g_water_system.initialized || handle.id == 0) return false;
    uint32_t index = handle.id - 1;
    return index < MAX_WATER_MATERIALS && g_water_system.materials[index].in_use;
}

int materials_water_material_get_info(materials_water_material_handle_t handle, materials_water_material_info_t* out_info) {
    if (!g_water_system.initialized || handle.id == 0 || !out_info) return -1;
    uint32_t index = handle.id - 1;
    if (index < MAX_WATER_MATERIALS && g_water_system.materials[index].in_use) {
        out_info->id = handle.id;
        out_info->flags = g_water_system.materials[index].desc.flags;
        out_info->initialized = true;
        return 0;
    }
    return -1;
}
