/*
 * water_material.h
 * Water surface material
 *
 * Part of the Materials subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef MATERIALS_WATER_MATERIAL_H
#define MATERIALS_WATER_MATERIAL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "rendering/forward/refraction.h"
#include "rendering/forward/transparency.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct materials_water_material_handle {
    uint32_t id;
} materials_water_material_handle_t;

typedef struct {
    float direction[2];
    float steepness;
    float wavelength;
    float speed;
} WaveParameters;

typedef struct materials_water_material_desc {
    // Visual Properties
    float base_color[4];
    float shallow_color[4];
    float deep_color[4];
    
    // Wave Simulation
    WaveParameters waves[4]; // Support up to 4 Gerstner waves
    uint32_t wave_count;
    float global_wave_speed;
    
    // Foam & Fog
    float foam_threshold;
    float foam_scale;
    float fog_density;
    float fog_color[3];
    
    // PBR & Lighting
    float roughness;
    float metallic;
    float specular;
    
    // Refraction/Transparency
    RefractionParams refraction;
    BlendMode blend_mode;
    
    uint32_t flags;
    void* user_data;
} materials_water_material_desc_t;

typedef struct materials_water_material_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} materials_water_material_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int materials_water_material_init(void);
void materials_water_material_shutdown(void);

/* Lifecycle */
int materials_water_material_create(materials_water_material_handle_t* out_handle, const materials_water_material_desc_t* desc);
void materials_water_material_destroy(materials_water_material_handle_t handle);

/* Operations */
int materials_water_material_update(materials_water_material_handle_t handle, const materials_water_material_desc_t* desc);
bool materials_water_material_is_valid(materials_water_material_handle_t handle);
int materials_water_material_get_info(materials_water_material_handle_t handle, materials_water_material_info_t* out_info);

#ifdef __cplusplus
}
#endif

#endif /* MATERIALS_WATER_MATERIAL_H */
