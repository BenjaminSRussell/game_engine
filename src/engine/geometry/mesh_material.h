#ifndef GEOMETRY_MESH_MATERIAL_H
#define GEOMETRY_MESH_MATERIAL_H

#include "geometry_types.h"
#include <stdbool.h>

/**
 * Material Binding Helpers
 * 
 * Simplifies material management for meshes
 */

// Material slot validation
bool mesh_validate_material_slot(const mesh_t* mesh, u32 slot);

// Default material handling
void mesh_set_default_material(mesh_t* mesh, u32 material_id);
void mesh_clear_materials(mesh_t* mesh);

// Per-instance material overrides (for instanced rendering)
typedef struct mesh_instance_material {
    u32 instance_id;
    u32 material_id;
    u32 slot;
} mesh_instance_material_t;

// Material change tracking
typedef struct mesh_material_change {
    u32 slot;
    u32 old_material;
    u32 new_material;
    u64 frame_changed;
} mesh_material_change_t;

void mesh_track_material_change(mesh_t* mesh, u32 slot, u32 new_material, u64 current_frame);

// Material parameter binding helpers
typedef struct mesh_material_params {
    u32 material_id;
    float metallic;
    float roughness;
    float ao;
    float emissive_strength;
} mesh_material_params_t;

void mesh_apply_material_params(mesh_t* mesh, u32 slot, const mesh_material_params_t* params);

#endif // GEOMETRY_MESH_MATERIAL_H
