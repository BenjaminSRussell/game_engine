/*
 * morph_data_gpu.c
 * GPU integration, SIMD, batch processing, streaming, LOD, culling, render graph
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

/* External context reference */
extern animation_morph_context_t g_morph_ctx;

/* ============================================================================
 * GPU INTEGRATION
 * ============================================================================ */

static uint32_t g_next_gpu_buffer_id = 1;

int animation_morph_data_upload_to_gpu(animation_morph_data_handle_t handle) {
    if (!g_morph_ctx.initialized || handle.id >= g_morph_ctx.count) {
        return ANIMATION_MORPH_ERROR_INVALID_HANDLE;
    }
    
    animation_morph_internal_t* item = &g_morph_ctx.items[handle.id];
    if (!item->initialized) {
        return ANIMATION_MORPH_ERROR_NOT_INITIALIZED;
    }
    
    pthread_mutex_lock(&item->mutex);
    
    // Simulate GPU buffer creation
    if (item->gpu_buffer_id == 0) {
        item->gpu_buffer_id = g_next_gpu_buffer_id++;
    }
    
    // Calculate total GPU memory needed
    size_t total_size = item->vertex_count * sizeof(animation_morph_vertex_t);
    for (uint32_t i = 0; i < item->target_count; i++) {
        total_size += item->targets[i].vertex_count * sizeof(animation_morph_vertex_t);
        if (item->targets[i].weights) {
            total_size += item->targets[i].vertex_count * sizeof(float);
        }
    }
    
    if (item->skeleton) {
        total_size += item->skeleton->bone_count * 16 * sizeof(float) * 2; // bone + skinning matrices
    }
    
    // Simulate GPU upload
    item->gpu_uploaded = true;
    item->stats.gpu_memory_usage = total_size;
    
    item->dirty = false;
    item->frame_updated = get_current_time_ms();
    
    pthread_mutex_unlock(&item->mutex);
    return ANIMATION_MORPH_ERROR_NONE;
}

int animation_morph_data_download_from_gpu(animation_morph_data_handle_t handle) {
    if (!g_morph_ctx.initialized || handle.id >= g_morph_ctx.count) {
        return ANIMATION_MORPH_ERROR_INVALID_HANDLE;
    }
    
    animation_morph_internal_t* item = &g_morph_ctx.items[handle.id];
    if (!item->initialized || !item->gpu_uploaded) {
        return ANIMATION_MORPH_ERROR_NOT_INITIALIZED;
    }
    
    pthread_mutex_lock(&item->mutex);
    
    // Simulate GPU download
    // In real implementation, would read back from GPU buffers
    
    item->dirty = true;
    item->frame_updated = get_current_time_ms();
    
    pthread_mutex_unlock(&item->mutex);
    return ANIMATION_MORPH_ERROR_NONE;
}

uint32_t animation_morph_data_get_gpu_buffer_id(animation_morph_data_handle_t handle) {
    if (!g_morph_ctx.initialized || handle.id >= g_morph_ctx.count) {
        return 0;
    }
    
    animation_morph_internal_t* item = &g_morph_ctx.items[handle.id];
    if (!item->initialized) {
        return 0;
    }
    
    pthread_mutex_lock(&item->mutex);
    uint32_t buffer_id = item->gpu_buffer_id;
    pthread_mutex_unlock(&item->mutex);
    
    return buffer_id;
}

/* ============================================================================
 * SIMD OPTIMIZATION
 * ============================================================================ */

#if defined(__SSE2__)
static void simd_vector_add_sse2(const float* a, const float* b, float* result) {
    __m128 va = _mm_load_ps(a);
    __m128 vb = _mm_load_ps(b);
    __m128 vr = _mm_add_ps(va, vb);
    _mm_store_ps(result, vr);
}

static void simd_vector_scale_sse2(const float* v, float scale, float* result) {
    __m128 vs = _mm_set1_ps(scale);
    __m128 vv = _mm_load_ps(v);
    __m128 vr = _mm_mul_ps(vv, vs);
    _mm_store_ps(result, vr);
}
#endif

int animation_morph_data_enable_simd_optimization(animation_morph_data_handle_t handle, bool enable) {
    if (!g_morph_ctx.initialized || handle.id >= g_morph_ctx.count) {
        return ANIMATION_MORPH_ERROR_INVALID_HANDLE;
    }
    
    animation_morph_internal_t* item = &g_morph_ctx.items[handle.id];
    if (!item->initialized) {
        return ANIMATION_MORPH_ERROR_NOT_INITIALIZED;
    }
    
    pthread_mutex_lock(&item->mutex);
    item->simd_optimized = enable;
    item->dirty = true;
    item->frame_updated = get_current_time_ms();
    pthread_mutex_unlock(&item->mutex);
    
    return ANIMATION_MORPH_ERROR_NONE;
}

bool animation_morph_data_is_simd_optimized(animation_morph_data_handle_t handle) {
    if (!g_morph_ctx.initialized || handle.id >= g_morph_ctx.count) {
        return false;
    }
    
    animation_morph_internal_t* item = &g_morph_ctx.items[handle.id];
    if (!item->initialized) {
        return false;
    }
    
    pthread_mutex_lock(&item->mutex);
    bool optimized = item->simd_optimized;
    pthread_mutex_unlock(&item->mutex);
    
    return optimized;
}

/* ============================================================================
 * BATCH PROCESSING
 * ============================================================================ */

int animation_morph_data_process_batch(animation_morph_data_handle_t* handles, uint32_t handle_count) {
    if (!handles || handle_count == 0) {
        return ANIMATION_MORPH_ERROR_INVALID_PARAMETER;
    }
    
    if (!g_morph_ctx.initialized) {
        return ANIMATION_MORPH_ERROR_NOT_INITIALIZED;
    }
    
    int processed = 0;
    uint32_t batch_size = g_morph_ctx.batch_size;
    
    for (uint32_t i = 0; i < handle_count; i += batch_size) {
        uint32_t current_batch = (i + batch_size > handle_count) ? (handle_count - i) : batch_size;
        
        // Process batch in parallel (simplified - would use thread pool in real implementation)
        for (uint32_t j = 0; j < current_batch; j++) {
            animation_morph_data_handle_t handle = handles[i + j];
            
            if (handle.id < g_morph_ctx.count) {
                animation_morph_internal_t* item = &g_morph_ctx.items[handle.id];
                
                if (item->initialized && item->dirty) {
                    pthread_mutex_lock(&item->mutex);
                    
                    // Process morph targets
                    for (uint32_t k = 0; k < item->target_count; k++) {
                        animation_morph_target_t* target = &item->targets[k];
                        if (target->influence > 0.0f && target->vertices) {
                            for (uint32_t m = 0; m < item->vertex_count; m++) {
                                float weight = target->weights ? target->weights[m] : 1.0f;
                                float influence = target->influence * weight;
                                
                                if (item->simd_optimized && (m + 3) < item->vertex_count) {
                                    // Use SIMD for 4 vertices at once
                                    #if defined(__SSE2__)
                                    simd_vector_add_sse2(item->base_vertices[m].position,
                                                        target->vertices[m].position,
                                                        item->base_vertices[m].position);
                                    simd_vector_scale_sse2(item->base_vertices[m].position,
                                                         influence,
                                                         item->base_vertices[m].position);
                                    #else
                                    vector_add(item->base_vertices[m].position,
                                              target->vertices[m].position,
                                              item->base_vertices[m].position);
                                    vector_scale(item->base_vertices[m].position,
                                               influence,
                                               item->base_vertices[m].position);
                                    #endif
                                } else {
                                    vector_add(item->base_vertices[m].position,
                                              target->vertices[m].position,
                                              item->base_vertices[m].position);
                                    vector_scale(item->base_vertices[m].position,
                                               influence,
                                               item->base_vertices[m].position);
                                }
                            }
                        }
                    }
                    
                    item->dirty = false;
                    processed++;
                    
                    pthread_mutex_unlock(&item->mutex);
                }
            }
        }
    }
    
    return processed;
}

int animation_morph_data_set_batch_size(uint32_t batch_size) {
    if (batch_size == 0) {
        return ANIMATION_MORPH_ERROR_INVALID_PARAMETER;
    }
    
    if (!g_morph_ctx.initialized) {
        return ANIMATION_MORPH_ERROR_NOT_INITIALIZED;
    }
    
    pthread_mutex_lock(&g_morph_ctx.global_mutex);
    g_morph_ctx.batch_size = batch_size;
    pthread_mutex_unlock(&g_morph_ctx.global_mutex);
    
    return ANIMATION_MORPH_ERROR_NONE;
}

/* ============================================================================
 * STREAMING SUPPORT
 * ============================================================================ */

int animation_morph_data_stream_init(animation_morph_data_handle_t handle, uint32_t chunk_size) {
    if (chunk_size == 0) {
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
    
    item->chunk_size = chunk_size;
    item->current_chunk = 0;
    item->streaming_enabled = true;
    
    pthread_mutex_unlock(&item->mutex);
    return ANIMATION_MORPH_ERROR_NONE;
}

int animation_morph_data_stream_chunk(animation_morph_data_handle_t handle, uint32_t chunk_index) {
    if (!g_morph_ctx.initialized || handle.id >= g_morph_ctx.count) {
        return ANIMATION_MORPH_ERROR_INVALID_HANDLE;
    }
    
    animation_morph_internal_t* item = &g_morph_ctx.items[handle.id];
    if (!item->initialized || !item->streaming_enabled) {
        return ANIMATION_MORPH_ERROR_NOT_INITIALIZED;
    }
    
    pthread_mutex_lock(&item->mutex);
    
    // Calculate chunk range
    uint32_t start_vertex = chunk_index * item->chunk_size;
    uint32_t end_vertex = (start_vertex + item->chunk_size > item->vertex_count) ? 
                         item->vertex_count : start_vertex + item->chunk_size;
    
    // Process chunk
    for (uint32_t i = start_vertex; i < end_vertex; i++) {
        for (uint32_t j = 0; j < item->target_count; j++) {
            animation_morph_target_t* target = &item->targets[j];
            if (target->influence > 0.0f && target->vertices) {
                float weight = target->weights ? target->weights[i] : 1.0f;
                float influence = target->influence * weight;
                
                vector_add(item->base_vertices[i].position,
                          target->vertices[i].position,
                          item->base_vertices[i].position);
                vector_scale(item->base_vertices[i].position,
                           influence,
                           item->base_vertices[i].position);
            }
        }
    }
    
    item->current_chunk = chunk_index;
    item->dirty = false;
    item->frame_updated = get_current_time_ms();
    
    pthread_mutex_unlock(&item->mutex);
    return ANIMATION_MORPH_ERROR_NONE;
}

void animation_morph_data_stream_cleanup(animation_morph_data_handle_t handle) {
    if (!g_morph_ctx.initialized || handle.id >= g_morph_ctx.count) {
        return;
    }
    
    animation_morph_internal_t* item = &g_morph_ctx.items[handle.id];
    if (!item->initialized) {
        return;
    }
    
    pthread_mutex_lock(&item->mutex);
    item->streaming_enabled = false;
    item->chunk_size = 0;
    item->current_chunk = 0;
    pthread_mutex_unlock(&item->mutex);
}

/* ============================================================================
 * LOD SUPPORT
 * ============================================================================ */

int animation_morph_data_set_lod_level(animation_morph_data_handle_t handle, animation_morph_lod_level_t lod_level) {
    if (lod_level >= ANIMATION_MORPH_LOD_COUNT) {
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
    item->current_lod = lod_level;
    item->dirty = true;
    item->frame_updated = get_current_time_ms();
    pthread_mutex_unlock(&item->mutex);
    
    return ANIMATION_MORPH_ERROR_NONE;
}

animation_morph_lod_level_t animation_morph_data_get_lod_level(animation_morph_data_handle_t handle) {
    if (!g_morph_ctx.initialized || handle.id >= g_morph_ctx.count) {
        return ANIMATION_MORPH_LOD_HIGH;
    }
    
    animation_morph_internal_t* item = &g_morph_ctx.items[handle.id];
    if (!item->initialized) {
        return ANIMATION_MORPH_LOD_HIGH;
    }
    
    pthread_mutex_lock(&item->mutex);
    animation_morph_lod_level_t lod = item->current_lod;
    pthread_mutex_unlock(&item->mutex);
    
    return lod;
}

int animation_morph_data_generate_lod_data(animation_morph_data_handle_t handle, animation_morph_lod_level_t lod_level, float reduction_ratio) {
    if (lod_level >= ANIMATION_MORPH_LOD_COUNT || reduction_ratio <= 0.0f || reduction_ratio > 1.0f) {
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
    
    // Calculate reduced vertex count
    uint32_t reduced_count = (uint32_t)(item->vertex_count * (1.0f - reduction_ratio));
    if (reduced_count < 1) reduced_count = 1;
    
    // Free existing LOD data
    if (item->lod_data[lod_level].vertices) {
        free(item->lod_data[lod_level].vertices);
    }
    if (item->lod_data[lod_level].targets) {
        for (uint32_t i = 0; i < item->lod_data[lod_level].target_count; i++) {
            if (item->lod_data[lod_level].targets[i].vertices) {
                free(item->lod_data[lod_level].targets[i].vertices);
            }
        }
        free(item->lod_data[lod_level].targets);
    }
    
    // Allocate LOD vertices
    item->lod_data[lod_level].vertices = malloc(reduced_count * sizeof(animation_morph_vertex_t));
    if (!item->lod_data[lod_level].vertices) {
        pthread_mutex_unlock(&item->mutex);
        return ANIMATION_MORPH_ERROR_OUT_OF_MEMORY;
    }
    
    // Simple vertex reduction (sample every Nth vertex)
    uint32_t step = item->vertex_count / reduced_count;
    for (uint32_t i = 0; i < reduced_count; i++) {
        uint32_t src_index = i * step;
        if (src_index >= item->vertex_count) src_index = item->vertex_count - 1;
        
        item->lod_data[lod_level].vertices[i] = item->base_vertices[src_index];
    }
    
    // Allocate LOD targets
    item->lod_data[lod_level].targets = malloc(item->target_count * sizeof(animation_morph_target_t));
    if (!item->lod_data[lod_level].targets) {
        free(item->lod_data[lod_level].vertices);
        item->lod_data[lod_level].vertices = NULL;
        pthread_mutex_unlock(&item->mutex);
        return ANIMATION_MORPH_ERROR_OUT_OF_MEMORY;
    }
    
    // Generate LOD targets
    for (uint32_t i = 0; i < item->target_count; i++) {
        animation_morph_target_t* src_target = &item->targets[i];
        animation_morph_target_t* lod_target = &item->lod_data[lod_level].targets[i];
        
        strncpy(lod_target->name, src_target->name, sizeof(lod_target->name) - 1);
        lod_target->name[sizeof(lod_target->name) - 1] = '\0';
        
        lod_target->vertex_count = reduced_count;
        lod_target->flags = src_target->flags;
        lod_target->influence = src_target->influence;
        
        lod_target->vertices = malloc(reduced_count * sizeof(animation_morph_vertex_t));
        if (!lod_target->vertices) {
            // Cleanup partially allocated LOD data
            for (uint32_t j = 0; j < i; j++) {
                if (item->lod_data[lod_level].targets[j].vertices) {
                    free(item->lod_data[lod_level].targets[j].vertices);
                }
            }
            free(item->lod_data[lod_level].targets);
            free(item->lod_data[lod_level].vertices);
            item->lod_data[lod_level].targets = NULL;
            item->lod_data[lod_level].vertices = NULL;
            pthread_mutex_unlock(&item->mutex);
            return ANIMATION_MORPH_ERROR_OUT_OF_MEMORY;
        }
        
        // Sample target vertices
        for (uint32_t j = 0; j < reduced_count; j++) {
            uint32_t src_index = j * step;
            if (src_index >= src_target->vertex_count) src_index = src_target->vertex_count - 1;
            
            lod_target->vertices[j] = src_target->vertices[src_index];
        }
        
        lod_target->weights = NULL; // Simplified LOD doesn't use weights
    }
    
    item->lod_data[lod_level].vertex_count = reduced_count;
    item->lod_data[lod_level].target_count = item->target_count;
    item->lod_data[lod_level].reduction_ratio = reduction_ratio;
    
    item->dirty = true;
    item->frame_updated = get_current_time_ms();
    
    pthread_mutex_unlock(&item->mutex);
    return ANIMATION_MORPH_ERROR_NONE;
}

/* ============================================================================
 * CULLING INTEGRATION
 * ============================================================================ */

bool animation_morph_data_is_visible(animation_morph_data_handle_t handle, const float* view_matrix, const float* projection_matrix) {
    if (!view_matrix || !projection_matrix) {
        return false;
    }
    
    if (!g_morph_ctx.initialized || handle.id >= g_morph_ctx.count) {
        return false;
    }
    
    animation_morph_internal_t* item = &g_morph_ctx.items[handle.id];
    if (!item->initialized) {
        return false;
    }
    
    pthread_mutex_lock(&item->mutex);
    
    // Simple bounding box culling
    // Transform bounds to view space and check against frustum
    float center[3] = {
        (item->min_bounds[0] + item->max_bounds[0]) * 0.5f,
        (item->min_bounds[1] + item->max_bounds[1]) * 0.5f,
        (item->min_bounds[2] + item->max_bounds[2]) * 0.5f
    };
    
    float radius = fmaxf(fmaxf(
        item->max_bounds[0] - item->min_bounds[0],
        item->max_bounds[1] - item->min_bounds[1]),
        item->max_bounds[2] - item->min_bounds[2]) * 0.5f;
    
    // Transform center to view space (simplified)
    float view_center[3];
    for (int i = 0; i < 3; i++) {
        view_center[i] = view_matrix[i * 4 + 0] * center[0] +
                        view_matrix[i * 4 + 1] * center[1] +
                        view_matrix[i * 4 + 2] * center[2] +
                        view_matrix[i * 4 + 3];
    }
    
    // Simple distance culling
    bool visible = (view_center[2] + radius > 0.0f); // In front of camera
    
    pthread_mutex_unlock(&item->mutex);
    return visible;
}

int animation_morph_data_set_culling_bounds(animation_morph_data_handle_t handle, const float* min_bounds, const float* max_bounds) {
    if (!min_bounds || !max_bounds) {
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
    
    memcpy(item->min_bounds, min_bounds, 3 * sizeof(float));
    memcpy(item->max_bounds, max_bounds, 3 * sizeof(float));
    
    pthread_mutex_unlock(&item->mutex);
    return ANIMATION_MORPH_ERROR_NONE;
}

/* ============================================================================
 * RENDER GRAPH NODE
 * ============================================================================ */

int animation_morph_data_add_render_graph_dependency(animation_morph_data_handle_t handle, const char* node_name) {
    if (!node_name) {
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
    
    // Reallocate render nodes array
    animation_morph_render_graph_node_t* new_nodes = realloc(item->render_nodes,
        (item->render_node_count + 1) * sizeof(animation_morph_render_graph_node_t));
    if (!new_nodes) {
        pthread_mutex_unlock(&item->mutex);
        return ANIMATION_MORPH_ERROR_OUT_OF_MEMORY;
    }
    
    item->render_nodes = new_nodes;
    animation_morph_render_graph_node_t* node = &item->render_nodes[item->render_node_count];
    
    strncpy(node->name, node_name, sizeof(node->name) - 1);
    node->name[sizeof(node->name) - 1] = '\0';
    node->dependencies = NULL;
    node->dependency_count = 0;
    node->execute_func = NULL;
    
    item->render_node_count++;
    item->dirty = true;
    item->frame_updated = get_current_time_ms();
    
    pthread_mutex_unlock(&item->mutex);
    return ANIMATION_MORPH_ERROR_NONE;
}

int animation_morph_data_remove_render_graph_dependency(animation_morph_data_handle_t handle, const char* node_name) {
    if (!node_name) {
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
    
    // Find and remove node
    for (uint32_t i = 0; i < item->render_node_count; i++) {
        if (strcmp(item->render_nodes[i].name, node_name) == 0) {
            // Free dependencies
            if (item->render_nodes[i].dependencies) {
                free(item->render_nodes[i].dependencies);
            }
            
            // Shift remaining nodes
            for (uint32_t j = i; j < item->render_node_count - 1; j++) {
                item->render_nodes[j] = item->render_nodes[j + 1];
            }
            
            item->render_node_count--;
            item->dirty = true;
            item->frame_updated = get_current_time_ms();
            
            pthread_mutex_unlock(&item->mutex);
            return ANIMATION_MORPH_ERROR_NONE;
        }
    }
    
    pthread_mutex_unlock(&item->mutex);
    return ANIMATION_MORPH_ERROR_INVALID_PARAMETER;
}

int animation_morph_data_execute_render_graph_node(animation_morph_data_handle_t handle, const char* node_name) {
    if (!node_name) {
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
    
    // Find and execute node
    for (uint32_t i = 0; i < item->render_node_count; i++) {
        if (strcmp(item->render_nodes[i].name, node_name) == 0) {
            if (item->render_nodes[i].execute_func) {
                item->render_nodes[i].execute_func(handle);
            }
            
            item->dirty = true;
            item->frame_updated = get_current_time_ms();
            
            pthread_mutex_unlock(&item->mutex);
            return ANIMATION_MORPH_ERROR_NONE;
        }
    }
    
    pthread_mutex_unlock(&item->mutex);
    return ANIMATION_MORPH_ERROR_INVALID_PARAMETER;
}

/* ============================================================================
 * DEBUG OUTPUT
 * ============================================================================ */

void animation_morph_data_debug_print(void) {
    if (!g_morph_ctx.initialized) {
        printf("Morph data system not initialized\\n");
        return;
    }
    
    printf("=== Morph Data System Debug Info ===\\n");
    printf("Total morph data items: %u\\n", g_morph_ctx.count);
    printf("Capacity: %u\\n", g_morph_ctx.capacity);
    printf("Batch size: %u\\n", g_morph_ctx.batch_size);
    printf("Peak memory usage: %zu bytes\\n", g_morph_ctx.peak_memory_usage);
    printf("Cache entries: %u / %u\\n", g_morph_ctx.cache_size, g_morph_ctx.cache_max_entries);
    printf("Async queue: %u -> %u\\n", g_morph_ctx.async_queue_head, g_morph_ctx.async_queue_tail);
    printf("\\n");
    
    for (uint32_t i = 0; i < g_morph_ctx.count; i++) {
        animation_morph_internal_t* item = &g_morph_ctx.items[i];
        if (item->initialized) {
            printf("Item %u:\\n", i);
            printf("  Vertices: %u\\n", item->vertex_count);
            printf("  Targets: %u\\n", item->target_count);
            printf("  Bones: %u\\n", item->bone_count);
            printf("  Flags: 0x%08x\\n", item->flags);
            printf("  Dirty: %s\\n", item->dirty ? "Yes" : "No");
            printf("  GPU uploaded: %s\\n", item->gpu_uploaded ? "Yes" : "No");
            printf("  SIMD optimized: %s\\n", item->simd_optimized ? "Yes" : "No");
            printf("  LOD level: %d\\n", item->current_lod);
            printf("  Streaming: %s\\n", item->streaming_enabled ? "Yes" : "No");
            printf("  Ragdoll: %s\\n", item->ragdoll_enabled ? "Yes" : "No");
            printf("  Memory usage: %llu bytes\\n", (unsigned long long)item->stats.memory_usage);
            printf("  GPU memory: %llu bytes\\n", (unsigned long long)item->stats.gpu_memory_usage);
            printf("  Cache hits: %u, misses: %u\\n", item->stats.cache_hits, item->stats.cache_misses);
            printf("  Async pending: %u\\n", item->stats.async_operations_pending);
            printf("  Avg processing time: %.3f ms\\n", item->stats.average_processing_time * 1000.0f);
            printf("\\n");
        }
    }
}

/* End of morph_data_gpu.c */
