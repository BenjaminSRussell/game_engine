/*
 * glass_material.h
 * Glass/transparent materials
 *
 * Part of the Materials subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef MATERIALS_GLASS_MATERIAL_H
#define MATERIALS_GLASS_MATERIAL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../../../forward/refraction.h"
#include "../../../forward/transparency.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct materials_glass_material_handle {
    uint32_t id;
} materials_glass_material_handle_t;

typedef struct materials_glass_material_desc {
    // Visual Properties
    float base_color[4];    // r, g, b, a (alpha = opacity)
    float emissive_factor[3];
    
    // PBR Properties
    float roughness;
    float metallic;
    
    // Glass Specifics
    RefractionParams refraction;
    float fresnel_scale;    // Adjusts the Fresnel intensity
    float tint_absorption;  // How much the glass color absorbs light passing through (Beer's Law)
    
    // Transparency Settings
    BlendMode blend_mode;
    
    uint32_t flags;
    void* user_data;
} materials_glass_material_desc_t;

typedef struct materials_glass_material_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} materials_glass_material_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int materials_glass_material_init(void);
void materials_glass_material_shutdown(void);

/* Lifecycle */
int materials_glass_material_create(materials_glass_material_handle_t* out_handle, const materials_glass_material_desc_t* desc);
void materials_glass_material_destroy(materials_glass_material_handle_t handle);

/* Operations */
int materials_glass_material_update(materials_glass_material_handle_t handle, const materials_glass_material_desc_t* desc);
bool materials_glass_material_is_valid(materials_glass_material_handle_t handle);
int materials_glass_material_get_info(materials_glass_material_handle_t handle, materials_glass_material_info_t* out_info);

#ifdef __cplusplus
}
#endif

#endif /* MATERIALS_GLASS_MATERIAL_H */
