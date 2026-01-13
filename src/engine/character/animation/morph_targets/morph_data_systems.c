/*
 * morph_data_systems.c
 * Advanced morph target systems (procedural, ragdoll, retargeting, etc.)
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 */

#include "character/animation/morph_targets/morph_data.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <errno.h>
#include <fcntl.h>

/* External context reference */
extern animation_morph_context_t g_morph_ctx;

/* ============================================================================
 * PROCEDURAL ANIMATION
 * ============================================================================ */

static void generate_noise_3d(float* noise, uint32_t width, uint32_t height, uint32_t depth, 
                             float frequency, float amplitude, uint32_t seed) {
    srand(seed);
    
    for (uint32_t z = 0; z < depth; z++) {
        for (uint32_t y = 0; y < height; y++) {
            for (uint32_t x = 0; x < width; x++) {
                float nx = (float)x * frequency;
                float ny = (float)y * frequency;
                float nz = (float)z * frequency;
                
                // Simple noise function (in real implementation, use Perlin or Simplex noise)
                float value = sinf(nx + seed) * cosf(ny + seed) * sinf(nz + seed);
                value = (value + 1.0f) * 0.5f * amplitude; // Normalize to [0, amplitude]
                
                noise[(z * height + y) * width + x] = value;
            }
        }
    }
}

int animation_morph_data_generate_procedural_targets(animation_morph_data_handle_t handle, uint32_t seed, float complexity) {
    if (!g_morph_ctx.initialized || handle.id >= g_morph_ctx.count) {
        return ANIMATION_MORPH_ERROR_INVALID_HANDLE;
    }
    
    animation_morph_internal_t* item = &g_morph_ctx.items[handle.id];
    if (!item->initialized || item->vertex_count == 0) {
        return ANIMATION_MORPH_ERROR_NOT_INITIALIZED;
    }
    
    pthread_mutex_lock(&item->mutex);
    
    // Generate procedural morph targets based on complexity
    uint32_t num_targets = (uint32_t)(complexity * 5.0f) + 1; // 1-6 targets based on complexity
    num_targets = (num_targets > 6) ? 6 : num_targets;
    
    for (uint32_t target_idx = 0; target_idx < num_targets; target_idx++) {
        char target_name[64];
        snprintf(target_name, sizeof(target_name), "procedural_%u", target_idx);
        
        // Allocate target vertices
        animation_morph_vertex_t* target_vertices = malloc(item->vertex_count * sizeof(animation_morph_vertex_t));
        if (!target_vertices) {
            pthread_mutex_unlock(&item->mutex);
            return ANIMATION_MORPH_ERROR_OUT_OF_MEMORY;
        }
        
        // Generate noise for this target
        float* noise = malloc(item->vertex_count * sizeof(float));
        if (!noise) {
            free(target_vertices);
            pthread_mutex_unlock(&item->mutex);
            return ANIMATION_MORPH_ERROR_OUT_OF_MEMORY;
        }
        
        // Generate 3D noise (using vertex indices as pseudo-3D coordinates)
        generate_noise_3d(noise, item->vertex_count, 1, 1, complexity, 0.1f, seed + target_idx);
        
        // Apply noise to create morph target
        for (uint32_t i = 0; i < item->vertex_count; i++) {
            // Copy base vertex
            memcpy(&target_vertices[i], &item->base_vertices[i], sizeof(animation_morph_vertex_t));
            
            // Apply noise to position (create deformation)
            float noise_value = noise[i];
            target_vertices[i].position[0] += noise_value * 0.1f;
            target_vertices[i].position[1] += noise_value * 0.05f;
            target_vertices[i].position[2] += noise_value * 0.08f;
            
            // Apply subtle normal changes
            target_vertices[i].normal[0] += noise_value * 0.02f;
            target_vertices[i].normal[1] += noise_value * 0.02f;
            target_vertices[i].normal[2] += noise_value * 0.02f;
            
            // Renormalize
            vector_normalize(target_vertices[i].normal);
        }
        
        free(noise);
        
        // Add the target
        int result = animation_morph_data_add_target(handle, target_name, target_vertices, 
                                                 item->vertex_count, NULL, 
                                                 ANIMATION_MORPH_TARGET_POSITION | ANIMATION_MORPH_TARGET_NORMAL);
        
        free(target_vertices);
        
        if (result != ANIMATION_MORPH_ERROR_NONE) {
            pthread_mutex_unlock(&item->mutex);
            return result;
        }
    }
    
    item->dirty = true;
    item->frame_updated = get_current_time_ms();
    
    pthread_mutex_unlock(&item->mutex);
    return ANIMATION_MORPH_ERROR_NONE;
}

int animation_morph_data_apply_noise(animation_morph_data_handle_t handle, float amplitude, float frequency) {
    if (amplitude < 0.0f || frequency <= 0.0f) {
        return ANIMATION_MORPH_ERROR_INVALID_PARAMETER;
    }
    
    if (!g_morph_ctx.initialized || handle.id >= g_morph_ctx.count) {
        return ANIMATION_MORPH_ERROR_INVALID_HANDLE;
    }
    
    animation_morph_internal_t* item = &g_morph_ctx.items[handle.id];
    if (!item->initialized || item->vertex_count == 0) {
        return ANIMATION_MORPH_ERROR_NOT_INITIALIZED;
    }
    
    pthread_mutex_lock(&item->mutex);
    
    // Generate noise
    float* noise = malloc(item->vertex_count * sizeof(float));
    if (!noise) {
        pthread_mutex_unlock(&item->mutex);
        return ANIMATION_MORPH_ERROR_OUT_OF_MEMORY;
    }
    
    generate_noise_3d(noise, item->vertex_count, 1, 1, frequency, amplitude, (uint32_t)get_current_time_ms());
    
    // Apply noise to all active targets
    for (uint32_t target_idx = 0; target_idx < item->target_count; target_idx++) {
        animation_morph_target_t* target = &item->targets[target_idx];
        
        if (target->influence > 0.0f) {
            for (uint32_t i = 0; i < item->vertex_count; i++) {
                float noise_value = noise[i];
                
                // Apply noise to target deltas
                target->vertices[i].position[0] += noise_value * 0.01f;
                target->vertices[i].position[1] += noise_value * 0.01f;
                target->vertices[i].position[2] += noise_value * 0.01f;
            }
        }
    }
    
    free(noise);
    
    item->dirty = true;
    item->frame_updated = get_current_time_ms();
    
    pthread_mutex_unlock(&item->mutex);
    return ANIMATION_MORPH_ERROR_NONE;
}

/* ============================================================================
 * RAGDOLL PHYSICS
 * ============================================================================ */

typedef struct ragdoll_body {
    float position[3];
    float velocity[3];
    float angular_velocity[3];
    float orientation[4]; // quaternion
    float mass;
    float radius;
    bool constrained;
} ragdoll_body_t;

int animation_morph_data_enable_ragdoll(animation_morph_data_handle_t handle, const float* gravity) {
    if (!gravity) {
        return ANIMATION_MORPH_ERROR_INVALID_PARAMETER;
    }
    
    if (!g_morph_ctx.initialized || handle.id >= g_morph_ctx.count) {
        return ANIMATION_MORPH_ERROR_INVALID_HANDLE;
    }
    
    animation_morph_internal_t* item = &g_morph_ctx.items[handle.id];
    if (!item->initialized) {
        return ANIMATION_MORPH_ERROR_NOT_INITIALIZED;
    }
    
    pthread_mutex_lock(&item->mutex);
    
    // Store gravity
    memcpy(item->gravity, gravity, 3 * sizeof(float));
    
    // Initialize ragdoll bodies based on skeleton bones
    if (item->skeleton && item->skeleton->bone_count > 0) {
        // In a real implementation, create physics bodies for each bone
        // For now, just enable ragdoll flag
        item->ragdoll_enabled = true;
    }
    
    item->dirty = true;
    item->frame_updated = get_current_time_ms();
    
    pthread_mutex_unlock(&item->mutex);
    return ANIMATION_MORPH_ERROR_NONE;
}

int animation_morph_data_update_ragdoll_physics(animation_morph_data_handle_t handle, float delta_time) {
    if (delta_time <= 0.0f) {
        return ANIMATION_MORPH_ERROR_INVALID_PARAMETER;
    }
    
    if (!g_morph_ctx.initialized || handle.id >= g_morph_ctx.count) {
        return ANIMATION_MORPH_ERROR_INVALID_HANDLE;
    }
    
    animation_morph_internal_t* item = &g_morph_ctx.items[handle.id];
    if (!item->initialized || !item->ragdoll_enabled) {
        return ANIMATION_MORPH_ERROR_NOT_INITIALIZED;
    }
    
    pthread_mutex_lock(&item->mutex);
    
    // Simple ragdoll physics simulation
    // In a real implementation, this would integrate with a physics engine
    
    if (item->skeleton) {
        for (uint32_t i = 0; i < item->skeleton->bone_count; i++) {
            // Apply gravity to bone positions
            float* transform = item->skeleton->bones[i].transform;
            
            // Simple gravity effect on translation part of matrix
            transform[12] += item->gravity[0] * delta_time * delta_time * 0.5f;
            transform[13] += item->gravity[1] * delta_time * delta_time * 0.5f;
            transform[14] += item->gravity[2] * delta_time * delta_time * 0.5f;
        }
        
        // Update skinning matrices
        for (uint32_t i = 0; i < item->skeleton->bone_count; i++) {
            const float* bone_transform = item->skeleton->bones[i].transform;
            const float* inverse_bind = item->skeleton->bones[i].inverse_bind;
            float* skinning_matrix = &item->skeleton->skinning_matrices[i * 16];
            
            matrix_multiply(bone_transform, inverse_bind, skinning_matrix);
        }
    }
    
    item->dirty = true;
    item->frame_updated = get_current_time_ms();
    
    pthread_mutex_unlock(&item->mutex);
    return ANIMATION_MORPH_ERROR_NONE;
}

/* ============================================================================
 * ANIMATION RETARGETING
 * ============================================================================ */

static float calculate_bone_length(const float* matrix) {
    float translation[3] = {matrix[12], matrix[13], matrix[14]};
    return vector_length(translation);
}

static void retarget_bone_chain(const animation_morph_bone_t* source_bones, uint32_t source_count,
                               animation_morph_bone_t* target_bones, uint32_t target_count,
                               const uint32_t* mapping, uint32_t mapping_count) {
    for (uint32_t i = 0; i < mapping_count; i++) {
        uint32_t source_idx = mapping[i * 2];
        uint32_t target_idx = mapping[i * 2 + 1];
        
        if (source_idx < source_count && target_idx < target_count) {
            const animation_morph_bone_t* src_bone = &source_bones[source_idx];
            animation_morph_bone_t* tgt_bone = &target_bones[target_idx];
            
            // Copy rotation (3x3 part of matrix)
            for (int row = 0; row < 3; row++) {
                for (int col = 0; col < 3; col++) {
                    tgt_bone->transform[row * 4 + col] = src_bone->transform[row * 4 + col];
                }
            }
            
            // Scale translation based on bone length ratio
            float src_length = calculate_bone_length(src_bone->transform);
            float tgt_length = calculate_bone_length(tgt_bone->transform);
            
            if (tgt_length > 0.0f) {
                float scale_ratio = src_length / tgt_length;
                for (int col = 0; col < 3; col++) {
                    tgt_bone->transform[3 * 4 + col] *= scale_ratio;
                }
            }
        }
    }
}

int animation_morph_data_retarget_from_skeleton(animation_morph_data_handle_t handle, const animation_morph_skeleton_t* source_skeleton) {
    if (!source_skeleton) {
        return ANIMATION_MORPH_ERROR_INVALID_PARAMETER;
    }
    
    if (!g_morph_ctx.initialized || handle.id >= g_morph_ctx.count) {
        return ANIMATION_MORPH_ERROR_INVALID_HANDLE;
    }
    
    animation_morph_internal_t* item = &g_morph_ctx.items[handle.id];
    if (!item->initialized || !item->skeleton) {
        return ANIMATION_MORPH_ERROR_NOT_INITIALIZED;
    }
    
    pthread_mutex_lock(&item->mutex);
    
    // Simple name-based bone mapping
    uint32_t mapping[ANIMATION_MORPH_DATA_MAX_RETARGET_MAPPINGS * 2];
    uint32_t mapping_count = 0;
    
    for (uint32_t i = 0; i < source_skeleton->bone_count && mapping_count < ANIMATION_MORPH_DATA_MAX_RETARGET_MAPPINGS; i++) {
        const char* source_name = source_skeleton->bones[i].name;
        
        // Find matching bone in target skeleton
        for (uint32_t j = 0; j < item->skeleton->bone_count && mapping_count < ANIMATION_MORPH_DATA_MAX_RETARGET_MAPPINGS; j++) {
            if (strcmp(source_name, item->skeleton->bones[j].name) == 0) {
                mapping[mapping_count * 2] = i;
                mapping[mapping_count * 2 + 1] = j;
                mapping_count++;
                break;
            }
        }
    }
    
    // Retarget bone chains
    retarget_bone_chain(source_skeleton->bones, source_skeleton->bone_count,
                       item->skeleton->bones, item->skeleton->bone_count,
                       mapping, mapping_count);
    
    // Update skinning matrices
    for (uint32_t i = 0; i < item->skeleton->bone_count; i++) {
        const float* bone_transform = item->skeleton->bones[i].transform;
        const float* inverse_bind = item->skeleton->bones[i].inverse_bind;
        float* skinning_matrix = &item->skeleton->skinning_matrices[i * 16];
        
        matrix_multiply(bone_transform, inverse_bind, skinning_matrix);
    }
    
    item->dirty = true;
    item->frame_updated = get_current_time_ms();
    
    pthread_mutex_unlock(&item->mutex);
    return ANIMATION_MORPH_ERROR_NONE;
}

int animation_morph_data_retarget_to_skeleton(animation_morph_data_handle_t handle, const animation_morph_skeleton_t* target_skeleton) {
    if (!target_skeleton) {
        return ANIMATION_MORPH_ERROR_INVALID_PARAMETER;
    }
    
    if (!g_morph_ctx.initialized || handle.id >= g_morph_ctx.count) {
        return ANIMATION_MORPH_ERROR_INVALID_HANDLE;
    }
    
    animation_morph_internal_t* item = &g_morph_ctx.items[handle.id];
    if (!item->initialized || !item->skeleton) {
        return ANIMATION_MORPH_ERROR_NOT_INITIALIZED;
    }
    
    pthread_mutex_lock(&item->mutex);
    
    // Simple name-based bone mapping (reverse of from_skeleton)
    uint32_t mapping[ANIMATION_MORPH_DATA_MAX_RETARGET_MAPPINGS * 2];
    uint32_t mapping_count = 0;
    
    for (uint32_t i = 0; i < item->skeleton->bone_count && mapping_count < ANIMATION_MORPH_DATA_MAX_RETARGET_MAPPINGS; i++) {
        const char* source_name = item->skeleton->bones[i].name;
        
        // Find matching bone in target skeleton
        for (uint32_t j = 0; j < target_skeleton->bone_count && mapping_count < ANIMATION_MORPH_DATA_MAX_RETARGET_MAPPINGS; j++) {
            if (strcmp(source_name, target_skeleton->bones[j].name) == 0) {
                mapping[mapping_count * 2] = i;
                mapping[mapping_count * 2 + 1] = j;
                mapping_count++;
                break;
            }
        }
    }
    
    // Retarget bone chains
    retarget_bone_chain(item->skeleton->bones, item->skeleton->bone_count,
                       (animation_morph_bone_t*)target_skeleton->bones, target_skeleton->bone_count,
                       mapping, mapping_count);
    
    item->dirty = true;
    item->frame_updated = get_current_time_ms();
    
    pthread_mutex_unlock(&item->mutex);
    return ANIMATION_MORPH_ERROR_NONE;
}

/* ============================================================================
 * VALIDATION AND ERROR HANDLING
 * ============================================================================ */

int animation_morph_data_validate(animation_morph_data_handle_t handle) {
    if (!g_morph_ctx.initialized || handle.id >= g_morph_ctx.count) {
        return ANIMATION_MORPH_ERROR_INVALID_HANDLE;
    }
    
    animation_morph_internal_t* item = &g_morph_ctx.items[handle.id];
    if (!item->initialized) {
        return ANIMATION_MORPH_ERROR_NOT_INITIALIZED;
    }
    
    pthread_mutex_lock(&item->mutex);
    
    // Validate base vertices
    if (item->vertex_count == 0 || !item->base_vertices) {
        pthread_mutex_unlock(&item->mutex);
        return ANIMATION_MORPH_ERROR_INVALID_PARAMETER;
    }
    
    // Validate targets
    for (uint32_t i = 0; i < item->target_count; i++) {
        const animation_morph_target_t* target = &item->targets[i];
        
        if (!target->vertices || target->vertex_count != item->vertex_count) {
            pthread_mutex_unlock(&item->mutex);
            return ANIMATION_MORPH_ERROR_INVALID_PARAMETER;
        }
        
        // Validate vertex data
        for (uint32_t j = 0; j < target->vertex_count; j++) {
            const animation_morph_vertex_t* vertex = &target->vertices[j];
            
            // Check for NaN or infinite values
            for (int k = 0; k < 3; k++) {
                if (!isfinite(vertex->position[k]) || !isfinite(vertex->normal[k]) || !isfinite(vertex->tangent[k])) {
                    pthread_mutex_unlock(&item->mutex);
                    return ANIMATION_MORPH_ERROR_INVALID_PARAMETER;
                }
            }
        }
    }
    
    // Validate skeleton
    if (item->skeleton) {
        if (item->skeleton->bone_count == 0 || !item->skeleton->bones) {
            pthread_mutex_unlock(&item->mutex);
            return ANIMATION_MORPH_ERROR_INVALID_PARAMETER;
        }
        
        for (uint32_t i = 0; i < item->skeleton->bone_count; i++) {
            const animation_morph_bone_t* bone = &item->skeleton->bones[i];
            
            // Validate bone transform matrix
            for (int j = 0; j < 16; j++) {
                if (!isfinite(bone->transform[j]) || !isfinite(bone->inverse_bind[j])) {
                    pthread_mutex_unlock(&item->mutex);
                    return ANIMATION_MORPH_ERROR_INVALID_PARAMETER;
                }
            }
        }
    }
    
    pthread_mutex_unlock(&item->mutex);
    return ANIMATION_MORPH_ERROR_NONE;
}

const char* animation_morph_data_get_error_string(animation_morph_error_t error) {
    switch (error) {
        case ANIMATION_MORPH_ERROR_NONE:
            return "No error";
        case ANIMATION_MORPH_ERROR_INVALID_HANDLE:
            return "Invalid morph data handle";
        case ANIMATION_MORPH_ERROR_NOT_INITIALIZED:
            return "Morph data system not initialized";
        case ANIMATION_MORPH_ERROR_OUT_OF_MEMORY:
            return "Out of memory";
        case ANIMATION_MORPH_ERROR_INVALID_PARAMETER:
            return "Invalid parameter";
        case ANIMATION_MORPH_ERROR_SERIALIZATION_FAILED:
            return "Serialization failed";
        case ANIMATION_MORPH_ERROR_GPU_OPERATION_FAILED:
            return "GPU operation failed";
        case ANIMATION_MORPH_ERROR_THREAD_ERROR:
            return "Thread operation failed";
        default:
            return "Unknown error";
    }
}

/* End of morph_data_systems.c */
