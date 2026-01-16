/*
 * morph_data_advanced.c
 * Advanced morph target operations
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
 * MORPH TARGET OPERATIONS
 * ============================================================================ */

int animation_morph_data_add_target(animation_morph_data_handle_t handle, const char* name, 
                                   const animation_morph_vertex_t* vertices, uint32_t vertex_count, 
                                   const float* weights, uint32_t flags) {
    if (!name || !vertices) {
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
    
    // Check if we have space for more targets
    if (item->target_count >= ANIMATION_MORPH_DATA_MAX_TARGETS) {
        pthread_mutex_unlock(&item->mutex);
        return ANIMATION_MORPH_ERROR_OUT_OF_MEMORY;
    }
    
    // Find existing target with same name
    for (uint32_t i = 0; i < item->target_count; i++) {
        if (strcmp(item->targets[i].name, name) == 0) {
            // Update existing target
            animation_morph_target_t* target = &item->targets[i];
            
            if (target->vertices) {
                free(target->vertices);
            }
            if (target->weights) {
                free(target->weights);
            }
            
            target->vertices = malloc(vertex_count * sizeof(animation_morph_vertex_t));
            if (!target->vertices) {
                pthread_mutex_unlock(&item->mutex);
                return ANIMATION_MORPH_ERROR_OUT_OF_MEMORY;
            }
            
            target->weights = weights ? malloc(vertex_count * sizeof(float)) : NULL;
            if (weights && !target->weights) {
                free(target->vertices);
                pthread_mutex_unlock(&item->mutex);
                return ANIMATION_MORPH_ERROR_OUT_OF_MEMORY;
            }
            
            memcpy(target->vertices, vertices, vertex_count * sizeof(animation_morph_vertex_t));
            if (weights) {
                memcpy(target->weights, weights, vertex_count * sizeof(float));
            }
            
            target->vertex_count = vertex_count;
            target->flags = flags;
            target->influence = 0.0f;
            
            item->dirty = true;
            item->frame_updated = get_current_time_ms();
            
            pthread_mutex_unlock(&item->mutex);
            return ANIMATION_MORPH_ERROR_NONE;
        }
    }
    
    // Add new target
    animation_morph_target_t* target = &item->targets[item->target_count];
    
    strncpy(target->name, name, sizeof(target->name) - 1);
    target->name[sizeof(target->name) - 1] = '\0';
    
    target->vertices = malloc(vertex_count * sizeof(animation_morph_vertex_t));
    if (!target->vertices) {
        pthread_mutex_unlock(&item->mutex);
        return ANIMATION_MORPH_ERROR_OUT_OF_MEMORY;
    }
    
    target->weights = weights ? malloc(vertex_count * sizeof(float)) : NULL;
    if (weights && !target->weights) {
        free(target->vertices);
        pthread_mutex_unlock(&item->mutex);
        return ANIMATION_MORPH_ERROR_OUT_OF_MEMORY;
    }
    
    memcpy(target->vertices, vertices, vertex_count * sizeof(animation_morph_vertex_t));
    if (weights) {
        memcpy(target->weights, weights, vertex_count * sizeof(float));
    }
    
    target->vertex_count = vertex_count;
    target->flags = flags;
    target->influence = 0.0f;
    
    item->target_count++;
    item->stats.total_targets = item->target_count;
    item->dirty = true;
    item->frame_updated = get_current_time_ms();
    
    pthread_mutex_unlock(&item->mutex);
    return ANIMATION_MORPH_ERROR_NONE;
}

int animation_morph_data_remove_target(animation_morph_data_handle_t handle, const char* name) {
    if (!name) {
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
    
    // Find target to remove
    uint32_t target_index = UINT32_MAX;
    for (uint32_t i = 0; i < item->target_count; i++) {
        if (strcmp(item->targets[i].name, name) == 0) {
            target_index = i;
            break;
        }
    }
    
    if (target_index == UINT32_MAX) {
        pthread_mutex_unlock(&item->mutex);
        return ANIMATION_MORPH_ERROR_INVALID_PARAMETER;
    }
    
    // Free target data
    animation_morph_target_t* target = &item->targets[target_index];
    if (target->vertices) {
        free(target->vertices);
    }
    if (target->weights) {
        free(target->weights);
    }
    
    // Shift remaining targets
    for (uint32_t i = target_index; i < item->target_count - 1; i++) {
        item->targets[i] = item->targets[i + 1];
    }
    
    item->target_count--;
    item->stats.total_targets = item->target_count;
    item->dirty = true;
    item->frame_updated = get_current_time_ms();
    
    pthread_mutex_unlock(&item->mutex);
    return ANIMATION_MORPH_ERROR_NONE;
}

int animation_morph_data_set_target_influence(animation_morph_data_handle_t handle, const char* name, float influence) {
    if (!name) {
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
    
    // Find target
    for (uint32_t i = 0; i < item->target_count; i++) {
        if (strcmp(item->targets[i].name, name) == 0) {
            item->targets[i].influence = influence;
            item->dirty = true;
            item->frame_updated = get_current_time_ms();
            
            pthread_mutex_unlock(&item->mutex);
            return ANIMATION_MORPH_ERROR_NONE;
        }
    }
    
    pthread_mutex_unlock(&item->mutex);
    return ANIMATION_MORPH_ERROR_INVALID_PARAMETER;
}

float animation_morph_data_get_target_influence(animation_morph_data_handle_t handle, const char* name) {
    if (!name) {
        return 0.0f;
    }
    
    if (!g_morph_ctx.initialized || handle.id >= g_morph_ctx.count) {
        return 0.0f;
    }
    
    animation_morph_internal_t* item = &g_morph_ctx.items[handle.id];
    if (!item->initialized) {
        return 0.0f;
    }
    
    pthread_mutex_lock(&item->mutex);
    
    // Find target
    for (uint32_t i = 0; i < item->target_count; i++) {
        if (strcmp(item->targets[i].name, name) == 0) {
            float influence = item->targets[i].influence;
            pthread_mutex_unlock(&item->mutex);
            return influence;
        }
    }
    
    pthread_mutex_unlock(&item->mutex);
    return 0.0f;
}

/* ============================================================================
 * SKELETAL ANIMATION
 * ============================================================================ */

int animation_morph_data_set_skeleton(animation_morph_data_handle_t handle, const animation_morph_skeleton_t* skeleton) {
    if (!skeleton) {
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
    
    // Free existing skeleton
    if (item->skeleton) {
        if (item->skeleton->bones) {
            for (uint32_t i = 0; i < item->skeleton->bone_count; i++) {
                if (item->skeleton->bones[i].child_indices) {
                    free(item->skeleton->bones[i].child_indices);
                }
            }
            free(item->skeleton->bones);
        }
        if (item->skeleton->bone_matrices) {
            free(item->skeleton->bone_matrices);
        }
        if (item->skeleton->skinning_matrices) {
            free(item->skeleton->skinning_matrices);
        }
        free(item->skeleton);
    }
    
    // Allocate new skeleton
    item->skeleton = malloc(sizeof(animation_morph_skeleton_t));
    if (!item->skeleton) {
        pthread_mutex_unlock(&item->mutex);
        return ANIMATION_MORPH_ERROR_OUT_OF_MEMORY;
    }
    
    item->skeleton->bone_count = skeleton->bone_count;
    item->skeleton->bones = malloc(skeleton->bone_count * sizeof(animation_morph_bone_t));
    if (!item->skeleton->bones) {
        free(item->skeleton);
        item->skeleton = NULL;
        pthread_mutex_unlock(&item->mutex);
        return ANIMATION_MORPH_ERROR_OUT_OF_MEMORY;
    }
    
    // Copy bones
    for (uint32_t i = 0; i < skeleton->bone_count; i++) {
        const animation_morph_bone_t* src_bone = &skeleton->bones[i];
        animation_morph_bone_t* dst_bone = &item->skeleton->bones[i];
        
        strncpy(dst_bone->name, src_bone->name, sizeof(dst_bone->name) - 1);
        dst_bone->name[sizeof(dst_bone->name) - 1] = '\0';
        
        memcpy(dst_bone->transform, src_bone->transform, 16 * sizeof(float));
        memcpy(dst_bone->inverse_bind, src_bone->inverse_bind, 16 * sizeof(float));
        
        dst_bone->parent_index = src_bone->parent_index;
        dst_bone->child_count = src_bone->child_count;
        
        if (src_bone->child_count > 0) {
            dst_bone->child_indices = malloc(src_bone->child_count * sizeof(uint32_t));
            if (!dst_bone->child_indices) {
                // Cleanup allocated memory
                for (uint32_t j = 0; j < i; j++) {
                    if (item->skeleton->bones[j].child_indices) {
                        free(item->skeleton->bones[j].child_indices);
                    }
                }
                free(item->skeleton->bones);
                free(item->skeleton);
                item->skeleton = NULL;
                pthread_mutex_unlock(&item->mutex);
                return ANIMATION_MORPH_ERROR_OUT_OF_MEMORY;
            }
            memcpy(dst_bone->child_indices, src_bone->child_indices, 
                   src_bone->child_count * sizeof(uint32_t));
        } else {
            dst_bone->child_indices = NULL;
        }
    }
    
    // Allocate matrices
    item->skeleton->bone_matrices = malloc(skeleton->bone_count * 16 * sizeof(float));
    item->skeleton->skinning_matrices = malloc(skeleton->bone_count * 16 * sizeof(float));
    
    if (!item->skeleton->bone_matrices || !item->skeleton->skinning_matrices) {
        // Cleanup
        for (uint32_t i = 0; i < skeleton->bone_count; i++) {
            if (item->skeleton->bones[i].child_indices) {
                free(item->skeleton->bones[i].child_indices);
            }
        }
        free(item->skeleton->bones);
        free(item->skeleton->bone_matrices);
        free(item->skeleton->skinning_matrices);
        free(item->skeleton);
        item->skeleton = NULL;
        pthread_mutex_unlock(&item->mutex);
        return ANIMATION_MORPH_ERROR_OUT_OF_MEMORY;
    }
    
    if (skeleton->bone_matrices) {
        memcpy(item->skeleton->bone_matrices, skeleton->bone_matrices, 
               skeleton->bone_count * 16 * sizeof(float));
    }
    
    if (skeleton->skinning_matrices) {
        memcpy(item->skeleton->skinning_matrices, skeleton->skinning_matrices, 
               skeleton->bone_count * 16 * sizeof(float));
    }
    
    item->bone_count = skeleton->bone_count;
    item->stats.total_bones = item->bone_count;
    item->dirty = true;
    item->frame_updated = get_current_time_ms();
    
    pthread_mutex_unlock(&item->mutex);
    return ANIMATION_MORPH_ERROR_NONE;
}

int animation_morph_data_update_bone_transforms(animation_morph_data_handle_t handle, const float* transforms, uint32_t bone_count) {
    if (!transforms) {
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
    
    if (bone_count != item->skeleton->bone_count) {
        pthread_mutex_unlock(&item->mutex);
        return ANIMATION_MORPH_ERROR_INVALID_PARAMETER;
    }
    
    // Update bone transforms
    for (uint32_t i = 0; i < bone_count; i++) {
        memcpy(item->skeleton->bones[i].transform, &transforms[i * 16], 16 * sizeof(float));
    }
    
    // Update bone matrices
    for (uint32_t i = 0; i < bone_count; i++) {
        const float* bone_transform = item->skeleton->bones[i].transform;
        const float* inverse_bind = item->skeleton->bones[i].inverse_bind;
        float* skinning_matrix = &item->skeleton->skinning_matrices[i * 16];
        
        // Calculate skinning matrix: bone_transform * inverse_bind
        matrix_multiply(bone_transform, inverse_bind, skinning_matrix);
    }
    
    item->dirty = true;
    item->frame_updated = get_current_time_ms();
    
    pthread_mutex_unlock(&item->mutex);
    return ANIMATION_MORPH_ERROR_NONE;
}

int animation_morph_data_apply_gpu_skinning(animation_morph_data_handle_t handle, const float* bone_matrices, uint32_t bone_count) {
    if (!bone_matrices) {
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
    
    if (bone_count != item->skeleton->bone_count) {
        pthread_mutex_unlock(&item->mutex);
        return ANIMATION_MORPH_ERROR_INVALID_PARAMETER;
    }
    
    // Update skinning matrices for GPU
    memcpy(item->skeleton->skinning_matrices, bone_matrices, bone_count * 16 * sizeof(float));
    
    // Mark for GPU upload
    item->gpu_uploaded = false;
    item->dirty = true;
    item->frame_updated = get_current_time_ms();
    
    pthread_mutex_unlock(&item->mutex);
    return ANIMATION_MORPH_ERROR_NONE;
}

/* ============================================================================
 * ANIMATION COMPRESSION
 * ============================================================================ */

static float compress_keyframe_value(float value, float tolerance) {
    float rounded = roundf(value / tolerance) * tolerance;
    return rounded;
}

int animation_morph_data_compress_keyframes(animation_morph_data_handle_t handle, float tolerance) {
    if (tolerance <= 0.0f) {
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
    
    // Free existing compressed data
    if (item->compressed_data) {
        free(item->compressed_data);
        item->compressed_data = NULL;
    }
    
    // Calculate compressed size
    size_t base_size = item->vertex_count * sizeof(animation_morph_vertex_t);
    size_t targets_size = 0;
    
    for (uint32_t i = 0; i < item->target_count; i++) {
        targets_size += item->targets[i].vertex_count * sizeof(animation_morph_vertex_t);
        if (item->targets[i].weights) {
            targets_size += item->targets[i].vertex_count * sizeof(float);
        }
    }
    
    item->compressed_size = base_size + targets_size;
    item->compressed_data = malloc(item->compressed_size);
    
    if (!item->compressed_data) {
        pthread_mutex_unlock(&item->mutex);
        return ANIMATION_MORPH_ERROR_OUT_OF_MEMORY;
    }
    
    uint8_t* ptr = (uint8_t*)item->compressed_data;
    
    // Compress base vertices
    for (uint32_t i = 0; i < item->vertex_count; i++) {
        const animation_morph_vertex_t* vertex = &item->base_vertices[i];
        animation_morph_vertex_t* compressed_vertex = (animation_morph_vertex_t*)ptr;
        
        // Compress position
        for (int j = 0; j < 3; j++) {
            compressed_vertex->position[j] = compress_keyframe_value(vertex->position[j], tolerance);
        }
        
        // Compress normal
        for (int j = 0; j < 3; j++) {
            compressed_vertex->normal[j] = compress_keyframe_value(vertex->normal[j], tolerance);
        }
        
        // Compress tangent
        for (int j = 0; j < 3; j++) {
            compressed_vertex->tangent[j] = compress_keyframe_value(vertex->tangent[j], tolerance);
        }
        
        // Copy texcoords (usually don't compress)
        memcpy(compressed_vertex->texcoord, vertex->texcoord, 2 * sizeof(float));
        
        ptr += sizeof(animation_morph_vertex_t);
    }
    
    // Compress targets
    for (uint32_t i = 0; i < item->target_count; i++) {
        const animation_morph_target_t* target = &item->targets[i];
        
        for (uint32_t j = 0; j < target->vertex_count; j++) {
            const animation_morph_vertex_t* vertex = &target->vertices[j];
            animation_morph_vertex_t* compressed_vertex = (animation_morph_vertex_t*)ptr;
            
            // Compress position deltas
            for (int k = 0; k < 3; k++) {
                compressed_vertex->position[k] = compress_keyframe_value(vertex->position[k], tolerance);
            }
            
            // Compress normal deltas
            for (int k = 0; k < 3; k++) {
                compressed_vertex->normal[k] = compress_keyframe_value(vertex->normal[k], tolerance);
            }
            
            // Compress tangent deltas
            for (int k = 0; k < 3; k++) {
                compressed_vertex->tangent[k] = compress_keyframe_value(vertex->tangent[k], tolerance);
            }
            
            ptr += sizeof(animation_morph_vertex_t);
        }
        
        // Compress weights
        if (target->weights) {
            for (uint32_t j = 0; j < target->vertex_count; j++) {
                float* compressed_weight = (float*)ptr;
                *compressed_weight = compress_keyframe_value(target->weights[j], tolerance);
                ptr += sizeof(float);
            }
        }
    }
    
    item->compression_tolerance = tolerance;
    item->dirty = true;
    item->frame_updated = get_current_time_ms();
    
    pthread_mutex_unlock(&item->mutex);
    return ANIMATION_MORPH_ERROR_NONE;
}

int animation_morph_data_decompress_keyframes(animation_morph_data_handle_t handle) {
    if (!g_morph_ctx.initialized || handle.id >= g_morph_ctx.count) {
        return ANIMATION_MORPH_ERROR_INVALID_HANDLE;
    }
    
    animation_morph_internal_t* item = &g_morph_ctx.items[handle.id];
    if (!item->initialized || !item->compressed_data) {
        return ANIMATION_MORPH_ERROR_NOT_INITIALIZED;
    }
    
    pthread_mutex_lock(&item->mutex);
    
    // Decompression is essentially just copying the compressed data back
    // In a real implementation, we might apply smoothing or interpolation
    
    const uint8_t* ptr = (const uint8_t*)item->compressed_data;
    
    // Decompress base vertices
    for (uint32_t i = 0; i < item->vertex_count; i++) {
        const animation_morph_vertex_t* compressed_vertex = (const animation_morph_vertex_t*)ptr;
        memcpy(&item->base_vertices[i], compressed_vertex, sizeof(animation_morph_vertex_t));
        ptr += sizeof(animation_morph_vertex_t);
    }
    
    // Decompress targets
    for (uint32_t i = 0; i < item->target_count; i++) {
        animation_morph_target_t* target = &item->targets[i];
        
        for (uint32_t j = 0; j < target->vertex_count; j++) {
            const animation_morph_vertex_t* compressed_vertex = (const animation_morph_vertex_t*)ptr;
            memcpy(&target->vertices[j], compressed_vertex, sizeof(animation_morph_vertex_t));
            ptr += sizeof(animation_morph_vertex_t);
        }
        
        // Decompress weights
        if (target->weights) {
            for (uint32_t j = 0; j < target->vertex_count; j++) {
                float compressed_weight = *(const float*)ptr;
                target->weights[j] = compressed_weight;
                ptr += sizeof(float);
            }
        }
    }
    
    item->dirty = true;
    item->frame_updated = get_current_time_ms();
    
    pthread_mutex_unlock(&item->mutex);
    return ANIMATION_MORPH_ERROR_NONE;
}

/* End of morph_data_advanced.c */
