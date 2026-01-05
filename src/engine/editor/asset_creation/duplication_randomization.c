/**
 * =================================================================================================
 *                          DUPLICATION & RANDOMIZATION
 * =================================================================================================
 */

#include "duplication_randomization.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

static float randf(float min, float max) {
    return min + (max - min) * ((float)rand() / (float)RAND_MAX);
}

void duplication_init(void) {
    // Initialize random seed or settings
}

void* duplication_duplicate_entity(void *entity, DuplicationSettings *settings) {
    if (!entity) return NULL;
    
    // In a real implementation:
    // if (settings->use_instances) {
    //     return create_entity_instance(entity);
    // } else {
    //     return deep_copy_entity(entity);
    // }
    
    // Stub: would need entity system integration
    return NULL;
}

void randomize_transform(float *pos, float *rot, float *scale, const RandomizationParams *params) {
    if (pos) {
        pos[0] += randf(params->pos_offset_min[0], params->pos_offset_max[0]);
        pos[1] += randf(params->pos_offset_min[1], params->pos_offset_max[1]);
        pos[2] += randf(params->pos_offset_min[2], params->pos_offset_max[2]);
    }
    
    if (rot) {
        rot[0] += randf(params->rot_offset_min[0], params->rot_offset_max[0]);
        rot[1] += randf(params->rot_offset_min[1], params->rot_offset_max[1]);
        rot[2] += randf(params->rot_offset_min[2], params->rot_offset_max[2]);
    }
    
    if (scale) {
        scale[0] *= randf(params->scale_offset_min[0], params->scale_offset_max[0]);
        scale[1] *= randf(params->scale_offset_min[1], params->scale_offset_max[1]);
        scale[2] *= randf(params->scale_offset_min[2], params->scale_offset_max[2]);
    }
}

void duplication_scatter_on_surface(void *prefab, const float *surface_point, 
                                     const float *surface_normal, float radius, int density) {
    // Poisson disk sampling or random placement within radius
    for (int i = 0; i < density; i++) {
        float angle = randf(0, 2.0f * 3.14159f);
        float r = sqrtf(randf(0, 1.0f)) * radius; // sqrt for uniform distribution
        
        // Generate random point on disk
        float offset_x = r * cosf(angle);
        float offset_z = r * sinf(angle);
        
        float spawn_pos[3] = {
            surface_point[0] + offset_x,
            surface_point[1],
            surface_point[2] + offset_z
        };
        
        // Align to surface normal (simplified, assume Y-up surface)
        // In real implementation, would raycast down to find actual surface height
        
        // Create instance at spawn_pos
        // entity_create_from_prefab(prefab, spawn_pos, surface_normal);
    }
}
