/*
 * fog_volume.c
 * Fog volume primitives and management implementation
 */

#include "lighting/volumetric/fog_volume.h"
#include <stdlib.h>
#include <string.h>
#include <include/math/math.h>

// Helper: Calculate transform matrix from position, rotation, scale
static void calculate_transform_matrix(fog_volume_transform_t* transform) {
    if (!transform->dirty) return;
    
    // For simplicity, create translation * rotation *scale
    // Full implementation would need proper euler-to-matrix or quaternion conversion
    mat4_t translation = mat4_translate(transform->position.x, transform->position.y, transform->position.z);
    mat4_t scale = mat4_scale(transform->scale.x, transform->scale.y, transform->scale.z);
    
    // Rotation  (simplified - would need proper euler angle conversion)
    mat4_t rotation_x = mat4_rotate(transform->rotation.x, 1.0f, 0.0f, 0.0f);
    mat4_t rotation_y = mat4_rotate(transform->rotation.y, 0.0f, 1.0f, 0.0f);
    mat4_t rotation_z = mat4_rotate(transform->rotation.z, 0.0f, 0.0f, 1.0f);
    
    mat4_t rotation = mat4_mul(mat4_mul(rotation_z, rotation_y), rotation_x);
    transform->matrix = mat4_mul(translation, mat4_mul(rotation, scale));
    transform->inv_matrix = mat4_inverse(transform->matrix);
    transform->dirty = false;
}

void fog_volume_manager_init(fog_volume_manager_t* manager) {
    memset(manager, 0, sizeof(fog_volume_manager_t));
    manager->next_id = 1;  // ID 0 is reserved for invalid
    
    // Default global fog parameters
    manager->global_params.density = 0.01f;
    manager->global_params.scattering = 0.1f;
    manager->global_params.absorption = 0.01f;
    manager->global_params.anisotropy = 0.0f;
    manager->global_params.color = vec3_set(0.7f, 0.8f, 0.9f);
    manager->global_params.emission = vec3_zero();
    manager->global_params.height_falloff = 0.0f;
    manager->global_params.distance_falloff = 0.0f;
    manager->global_fog_enabled = false;
}

void fog_volume_manager_shutdown(fog_volume_manager_t* manager) {
    memset(manager, 0, sizeof(fog_volume_manager_t));
}

uint32_t fog_volume_create(fog_volume_manager_t* manager, fog_volume_shape_t shape,
                            const fog_volume_params_t* params) {
    if (manager->active_count >= MAX_FOG_VOLUMES) {
        return 0;  // No space available
    }
    
    // Find first free slot
    fog_volume_t* volume = NULL;
    for (uint32_t i = 0; i < MAX_FOG_VOLUMES; i++) {
        if (manager->volumes[i].id == 0) {
            volume = &manager->volumes[i];
            break;
        }
    }
    
    if (!volume) return 0;
    
    // Initialize volume
    memset(volume, 0, sizeof(fog_volume_t));
    volume->id = manager->next_id++;
    volume->shape = shape;
    volume->params = *params;
    volume->active = true;
    volume->priority = 0;
    
    // Initialize transform
    volume->transform.position = vec3_zero();
    volume->transform.rotation = vec3_zero();
    volume->transform.scale = vec3_set(1.0f, 1.0f, 1.0f);
    volume->transform.dirty = true;
    
    // Set default shape parameters
    switch (shape) {
        case FOG_VOLUME_SHAPE_BOX:
            volume->box.extents = vec3_set(1.0f, 1.0f, 1.0f);
            break;
        case FOG_VOLUME_SHAPE_SPHERE:
            volume->sphere.radius = 1.0f;
            break;
        case FOG_VOLUME_SHAPE_CAPSULE:
            volume->capsule.radius = 0.5f;
            volume->capsule.height = 2.0f;
            break;
        case FOG_VOLUME_SHAPE_GLOBAL:
            // Global fog has no shape parameters
            break;
    }
    
    fog_volume_calculate_bounds(volume);
    manager->active_count++;
    
    return volume->id;
}

void fog_volume_destroy(fog_volume_manager_t* manager, uint32_t id) {
    for (uint32_t i = 0; i < MAX_FOG_VOLUMES; i++) {
        if (manager->volumes[i].id == id) {
            memset(&manager->volumes[i], 0, sizeof(fog_volume_t));
            manager->active_count--;
            return;
        }
    }
}

fog_volume_t* fog_volume_get(fog_volume_manager_t* manager, uint32_t id) {
    for (uint32_t i = 0; i < MAX_FOG_VOLUMES; i++) {
        if (manager->volumes[i].id == id) {
            return &manager->volumes[i];
        }
    }
    return NULL;
}

void fog_volume_set_params(fog_volume_t* volume, const fog_volume_params_t* params) {
    volume->params = *params;
}

void fog_volume_set_transform(fog_volume_t* volume, vec3_t position, vec3_t rotation, vec3_t scale) {
    volume->transform.position = position;
    volume->transform.rotation = rotation;
    volume->transform.scale = scale;
    volume->transform.dirty = true;
    fog_volume_calculate_bounds(volume);
}

void fog_volume_set_box_extents(fog_volume_t* volume, vec3_t extents) {
    if (volume->shape == FOG_VOLUME_SHAPE_BOX) {
        volume->box.extents = extents;
        fog_volume_calculate_bounds(volume);
    }
}

void fog_volume_set_sphere_radius(fog_volume_t* volume, float radius) {
    if (volume->shape == FOG_VOLUME_SHAPE_SPHERE) {
        volume->sphere.radius = radius;
        fog_volume_calculate_bounds(volume);
    }
}

void fog_volume_set_capsule(fog_volume_t* volume, float radius, float height) {
    if (volume->shape == FOG_VOLUME_SHAPE_CAPSULE) {
        volume->capsule.radius = radius;
        volume->capsule.height = height;
        fog_volume_calculate_bounds(volume);
    }
}

void fog_volume_set_active(fog_volume_t* volume, bool active) {
    volume->active = active;
}

void fog_volume_set_priority(fog_volume_t* volume, int32_t priority) {
    volume->priority = priority;
}

void fog_volume_calculate_bounds(fog_volume_t* volume) {
    calculate_transform_matrix(&volume->transform);
    
    switch (volume->shape) {
        case FOG_VOLUME_SHAPE_BOX: {
            // Transform AABB corners to find world-space bounds
            vec3_t extents = volume->box.extents;
            vec3_t local_min = vec3_set(-extents.x, -extents.y, -extents.z);
            vec3_t local_max = vec3_set(extents.x, extents.y, extents.z);
            
            // Transform all 8 corners and find min/max
            volume->bounds_min = vec3_set(FLT_MAX, FLT_MAX, FLT_MAX);
            volume->bounds_max = vec3_set(-FLT_MAX, -FLT_MAX, -FLT_MAX);
            
            for (int i = 0; i < 8; i++) {
                vec3_t corner = vec3_set(
                    (i & 1) ? local_max.x : local_min.x,
                    (i & 2) ? local_max.y : local_min.y,
                    (i & 4) ? local_max.z : local_min.z
                );
                vec3_t world_corner = mat4_mul_vec3(volume->transform.matrix, corner, 1.0f);
                volume->bounds_min = vec3_min(volume->bounds_min, world_corner);
                volume->bounds_max = vec3_max(volume->bounds_max, world_corner);
            }
            break;
        }
        
        case FOG_VOLUME_SHAPE_SPHERE: {
            float radius = volume->sphere.radius * fmaxf(fmaxf(volume->transform.scale.x, 
                                                                 volume->transform.scale.y), 
                                                                volume->transform.scale.z);
            volume->bounds_min = vec3_sub(volume->transform.position, vec3_set(radius, radius, radius));
            volume->bounds_max = vec3_add(volume->transform.position, vec3_set(radius, radius, radius));
            break;
        }
        
        case FOG_VOLUME_SHAPE_CAPSULE: {
            // Simplified: treat as sphere with radius = max(radius, height/2)
            float extent = fmaxf(volume->capsule.radius, volume->capsule.height * 0.5f);
            extent *= fmaxf(fmaxf(volume->transform.scale.x, volume->transform.scale.y), 
                           volume->transform.scale.z);
            volume->bounds_min = vec3_sub(volume->transform.position, vec3_set(extent, extent, extent));
            volume->bounds_max = vec3_add(volume->transform.position, vec3_set(extent, extent, extent));
            break;
        }
        
        case FOG_VOLUME_SHAPE_GLOBAL:
            // Infinite bounds
            volume->bounds_min = vec3_set(-FLT_MAX, -FLT_MAX, -FLT_MAX);
            volume->bounds_max = vec3_set(FLT_MAX, FLT_MAX, FLT_MAX);
            break;
    }
}

float fog_volume_density_at_point(const fog_volume_t* volume, vec3_t world_pos) {
    if (!volume->active) return 0.0f;
    
    // Transform to local space
    vec3_t local_pos = mat4_mul_vec3(volume->transform.inv_matrix, world_pos, 1.0f);
    
    float density_factor = 0.0f;
    
    switch (volume->shape) {
        case FOG_VOLUME_SHAPE_BOX: {
            vec3_t extents = volume->box.extents;
            if (fabsf(local_pos.x) <= extents.x &&
                fabsf(local_pos.y) <= extents.y &&
                fabsf(local_pos.z) <= extents.z) {
                density_factor = 1.0f;
            }
            break;
        }
        
        case FOG_VOLUME_SHAPE_SPHERE: {
            float dist = vec3_length(local_pos);
            if (dist <= volume->sphere.radius) {
                density_factor = 1.0f;
            }
            break;
        }
        
        case FOG_VOLUME_SHAPE_CAPSULE: {
            // Simplified capsule test
            float half_height = volume->capsule.height * 0.5f;
            float clamped_y = fmaxf(-half_height, fminf(half_height, local_pos.y));
            vec3_t closest = vec3_set(0.0f, clamped_y, 0.0f);
            float dist = vec3_length(vec3_sub(local_pos, closest));
            if (dist <= volume->capsule.radius) {
                density_factor = 1.0f;
            }
            break;
        }
        
        case FOG_VOLUME_SHAPE_GLOBAL:
            density_factor = 1.0f;
            break;
    }
    
    // Apply height falloff
    if (volume->params.height_falloff > 0.0f) {
        float height_factor = expf(-world_pos.y * volume->params.height_falloff);
        density_factor *= height_factor;
    }
    
    return density_factor * volume->params.density;
}

bool fog_volume_intersects_froxel(const fog_volume_t* volume, const froxel_grid_t* grid,
                                   uint32_t x, uint32_t y, uint32_t z, mat4_t view_matrix) {
    if (!volume->active) return false;
    
    // Get froxel bounds in view space
    vec3_t froxel_min, froxel_max;
    froxel_grid_get_froxel_bounds(grid, x, y, z, &froxel_min, &froxel_max);
    
    // Transform volume bounds to view space
    vec3_t view_min = mat4_mul_vec3(view_matrix, volume->bounds_min, 1.0f);
    vec3_t view_max = mat4_mul_vec3(view_matrix, volume->bounds_max, 1.0f);
    
    // AABB intersection test (simplified)
    return !(view_max.z < froxel_min.z || view_min.z > froxel_max.z);
}

void fog_volume_manager_set_global_fog(fog_volume_manager_t* manager, bool enabled,
                                        const fog_volume_params_t* params) {
    manager->global_fog_enabled = enabled;
    if (params) {
        manager->global_params = *params;
    }
}

void fog_volume_manager_update(fog_volume_manager_t* manager) {
    for (uint32_t i = 0; i < MAX_FOG_VOLUMES; i++) {
        if (manager->volumes[i].id != 0 && manager->volumes[i].transform.dirty) {
            calculate_transform_matrix(&manager->volumes[i].transform);
            fog_volume_calculate_bounds(&manager->volumes[i]);
        }
    }
}
