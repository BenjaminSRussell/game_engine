// src/engine/rendering/animation/gpu_skinning.c
//
// Purpose: GPU-based skeletal animation skinning implementation
// Provides high-performance vertex skinning using compute shaders

#include "rendering/animation/gpu_skinning.h"
#include "core/memory/unified_allocator.h"
#include "core/logging/unified_logger.h"
#include <string.h>
#include <stdlib.h>

// ============================================================================
// INTERNAL STATE
// ============================================================================

static GPUSkinningContext* g_gpu_skinning_context = NULL;

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

static mat3x4 matrix_to_skinning_matrix(const mat4* matrix) {
    mat3x4 result;
    result.m[0][0] = matrix->m[0][0];
    result.m[0][1] = matrix->m[0][1];
    result.m[0][2] = matrix->m[0][2];
    result.m[0][3] = matrix->m[0][3];
    result.m[1][0] = matrix->m[1][0];
    result.m[1][1] = matrix->m[1][1];
    result.m[1][2] = matrix->m[1][2];
    result.m[1][3] = matrix->m[1][3];
    result.m[2][0] = matrix->m[2][0];
    result.m[2][1] = matrix->m[2][1];
    result.m[2][2] = matrix->m[2][2];
    result.m[2][3] = matrix->m[2][3];
    return result;
}

static void normalize_bone_weights(f32* weights, u8* indices, u32 count) {
    f32 total = 0.0f;
    for (u32 i = 0; i < count; i++) {
        total += weights[i];
    }
    
    if (total > 0.0001f) {
        f32 inv_total = 1.0f / total;
        for (u32 i = 0; i < count; i++) {
            weights[i] *= inv_total;
        }
    }
}

// ============================================================================
// SYSTEM MANAGEMENT
// ============================================================================

bool gpu_skinning_initialize(u32 max_meshes, u32 max_skeletons) {
    if (g_gpu_skinning_context) {
        LOG_WARN(LOG_CAT_RENDERER, "GPU skinning already initialized");
        return true;
    }
    
    LOG_INFO(LOG_CAT_RENDERER, "Initializing GPU skinning system");
    
    g_gpu_skinning_context = MALLOC_PERSISTENT(sizeof(GPUSkinningContext));
    if (!g_gpu_skinning_context) {
        LOG_ERROR(LOG_CAT_RENDERER, "Failed to allocate GPU skinning context");
        return false;
    }
    
    memset(g_gpu_skinning_context, 0, sizeof(GPUSkinningContext));
    
    // Load compute shader
    g_gpu_skinning_context->skinning_compute_shader = 
        shader_load_compute("shaders/animation/gpu_skinning.comp");
    if (!g_gpu_skinning_context->skinning_compute_shader) {
        LOG_ERROR(LOG_CAT_RENDERER, "Failed to load GPU skinning compute shader");
        FREE(g_gpu_skinning_context);
        g_gpu_skinning_context = NULL;
        return false;
    }
    
    // Create global bone buffer
    BufferDesc bone_buffer_desc = {
        .size = GPU_SKINNING_MAX_BONES * max_skeletons * sizeof(GPUBoneTransform),
        .usage = BUFFER_USAGE_STORAGE | BUFFER_USAGE_DYNAMIC,
        .memory_type = MEMORY_TYPE_DEVICE_LOCAL
    };
    
    g_gpu_skinning_context->global_bone_buffer = buffer_create(&bone_buffer_desc);
    if (!g_gpu_skinning_context->global_bone_buffer) {
        LOG_ERROR(LOG_CAT_RENDERER, "Failed to create global bone buffer");
        shader_destroy(g_gpu_skinning_context->skinning_compute_shader);
        FREE(g_gpu_skinning_context);
        g_gpu_skinning_context = NULL;
        return false;
    }
    
    // Create bone texture for compute shader access
    TextureDesc bone_texture_desc = {
        .width = GPU_SKINNING_MAX_BONES * 4,  // 4 components per bone
        .height = max_skeletons,
        .format = TEXTURE_FORMAT_RGBA32F,
        .usage = TEXTURE_USAGE_STORAGE | TEXTURE_USAGE_SAMPLED
    };
    
    g_gpu_skinning_context->global_bone_texture = texture_create(&bone_texture_desc);
    if (!g_gpu_skinning_context->global_bone_texture) {
        LOG_ERROR(LOG_CAT_RENDERER, "Failed to create global bone texture");
        buffer_destroy(g_gpu_skinning_context->global_bone_buffer);
        shader_destroy(g_gpu_skinning_context->skinning_compute_shader);
        FREE(g_gpu_skinning_context);
        g_gpu_skinning_context = NULL;
        return false;
    }
    
    g_gpu_skinning_context->initialized = true;
    
    LOG_INFO(LOG_CAT_RENDERER, "GPU skinning system initialized successfully");
    return true;
}

void gpu_skinning_shutdown(void) {
    if (!g_gpu_skinning_context) return;
    
    LOG_INFO(LOG_CAT_RENDERER, "Shutting down GPU skinning system");
    
    // Destroy all instances
    for (u32 i = 0; i < g_gpu_skinning_context->instance_count; i++) {
        GPUAnimationInstance* instance = &g_gpu_skinning_context->instances[i];
        if (instance->skinned_vertices) {
            buffer_destroy(instance->skinned_vertices);
        }
        if (instance->instance_buffer) {
            buffer_destroy(instance->instance_buffer);
        }
    }
    
    // Destroy all meshes
    for (u32 i = 0; i < g_gpu_skinning_context->mesh_count; i++) {
        GPUSkinningMesh* mesh = &g_gpu_skinning_context->meshes[i];
        if (mesh->vertex_buffer) buffer_destroy(mesh->vertex_buffer);
        if (mesh->index_buffer) buffer_destroy(mesh->index_buffer);
        if (mesh->output_vertex_buffer) buffer_destroy(mesh->output_vertex_buffer);
        if (mesh->output_index_buffer) buffer_destroy(mesh->output_index_buffer);
        if (mesh->bone_buffer) buffer_destroy(mesh->bone_buffer);
        if (mesh->vertices) FREE(mesh->vertices);
        if (mesh->indices) FREE(mesh->indices);
    }
    
    // Destroy all skeletons
    for (u32 i = 0; i < g_gpu_skinning_context->skeleton_count; i++) {
        GPUSkeleton* skeleton = &g_gpu_skinning_context->skeletons[i];
        if (skeleton->bone_buffer) buffer_destroy(skeleton->bone_buffer);
        if (skeleton->bone_texture) texture_destroy(skeleton->bone_texture);
    }
    
    // Destroy global resources
    if (g_gpu_skinning_context->global_bone_buffer) {
        buffer_destroy(g_gpu_skinning_context->global_bone_buffer);
    }
    if (g_gpu_skinning_context->global_bone_texture) {
        texture_destroy(g_gpu_skinning_context->global_bone_texture);
    }
    if (g_gpu_skinning_context->skinning_compute_shader) {
        shader_destroy(g_gpu_skinning_context->skinning_compute_shader);
    }
    
    FREE(g_gpu_skinning_context);
    g_gpu_skinning_context = NULL;
    
    LOG_INFO(LOG_CAT_RENDERER, "GPU skinning system shutdown complete");
}

GPUSkinningContext* gpu_skinning_get_context(void) {
    return g_gpu_skinning_context;
}

// ============================================================================
// SKELETON MANAGEMENT
// ============================================================================

GPUSkeleton* gpu_skinning_create_skeleton(const char* name, u32 bone_count) {
    if (!g_gpu_skinning_context || !g_gpu_skinning_context->initialized) {
        LOG_ERROR(LOG_CAT_RENDERER, "GPU skinning not initialized");
        return NULL;
    }
    
    if (g_gpu_skinning_context->skeleton_count >= GPU_SKINNING_MAX_MESHES) {
        LOG_ERROR(LOG_CAT_RENDERER, "Maximum skeleton count reached");
        return NULL;
    }
    
    if (bone_count > GPU_SKINNING_MAX_BONES) {
        LOG_ERROR(LOG_CAT_RENDERER, "Bone count exceeds maximum: %u > %u", bone_count, GPU_SKINNING_MAX_BONES);
        return NULL;
    }
    
    GPUSkeleton* skeleton = &g_gpu_skinning_context->skeletons[g_gpu_skinning_context->skeleton_count++];
    
    strncpy(skeleton->name, name, sizeof(skeleton->name) - 1);
    skeleton->name[sizeof(skeleton->name) - 1] = '\0';
    skeleton->bone_count = bone_count;
    skeleton->is_dirty = true;
    
    // Initialize bones to identity
    for (u32 i = 0; i < bone_count; i++) {
        skeleton->bones[i].transform = mat4_identity();
        skeleton->bones[i].skinning_matrix = matrix_to_skinning_matrix(&skeleton->bones[i].transform);
        skeleton->parent_indices[i] = i;  // Self-parent by default
    }
    
    // Create bone buffer
    BufferDesc bone_buffer_desc = {
        .size = bone_count * sizeof(GPUBoneTransform),
        .usage = BUFFER_USAGE_STORAGE | BUFFER_USAGE_DYNAMIC,
        .memory_type = MEMORY_TYPE_DEVICE_LOCAL
    };
    
    skeleton->bone_buffer = buffer_create(&bone_buffer_desc);
    if (!skeleton->bone_buffer) {
        LOG_ERROR(LOG_CAT_RENDERER, "Failed to create skeleton bone buffer");
        g_gpu_skinning_context->skeleton_count--;
        return NULL;
    }
    
    // Create bone texture
    TextureDesc bone_texture_desc = {
        .width = GPU_SKINNING_MAX_BONES * 4,  // 4 components per bone
        .height = 1,
        .format = TEXTURE_FORMAT_RGBA32F,
        .usage = TEXTURE_USAGE_STORAGE | TEXTURE_USAGE_SAMPLED
    };
    
    skeleton->bone_texture = texture_create(&bone_texture_desc);
    if (!skeleton->bone_texture) {
        LOG_ERROR(LOG_CAT_RENDERER, "Failed to create skeleton bone texture");
        buffer_destroy(skeleton->bone_buffer);
        g_gpu_skinning_context->skeleton_count--;
        return NULL;
    }
    
    LOG_INFO(LOG_CAT_RENDERER, "Created skeleton '%s' with %u bones", name, bone_count);
    return skeleton;
}

void gpu_skinning_destroy_skeleton(GPUSkeleton* skeleton) {
    if (!skeleton || !g_gpu_skinning_context) return;
    
    // Find skeleton in array and mark as invalid
    for (u32 i = 0; i < g_gpu_skinning_context->skeleton_count; i++) {
        if (&g_gpu_skinning_context->skeletons[i] == skeleton) {
            // Destroy resources
            if (skeleton->bone_buffer) buffer_destroy(skeleton->bone_buffer);
            if (skeleton->bone_texture) texture_destroy(skeleton->bone_texture);
            
            // Shift remaining skeletons
            for (u32 j = i; j < g_gpu_skinning_context->skeleton_count - 1; j++) {
                g_gpu_skinning_context->skeletons[j] = g_gpu_skinning_context->skeletons[j + 1];
            }
            g_gpu_skinning_context->skeleton_count--;
            
            LOG_INFO(LOG_CAT_RENDERER, "Destroyed skeleton");
            return;
        }
    }
}

void gpu_skinning_update_skeleton(GPUSkeleton* skeleton, const GPUBoneTransform* bone_transforms) {
    if (!skeleton || !bone_transforms) return;
    
    memcpy(skeleton->bones, bone_transforms, skeleton->bone_count * sizeof(GPUBoneTransform));
    
    // Update skinning matrices
    for (u32 i = 0; i < skeleton->bone_count; i++) {
        skeleton->bones[i].skinning_matrix = matrix_to_skinning_matrix(&skeleton->bones[i].transform);
    }
    
    skeleton->is_dirty = true;
}

void gpu_skinning_set_bone_parents(GPUSkeleton* skeleton, const u32* parent_indices) {
    if (!skeleton || !parent_indices) return;
    
    memcpy(skeleton->parent_indices, parent_indices, skeleton->bone_count * sizeof(u32));
    skeleton->is_dirty = true;
}

// ============================================================================
// MESH MANAGEMENT
// ============================================================================

GPUSkinningMesh* gpu_skinning_create_mesh(const GPUSkinningVertex* vertices, u32 vertex_count,
                                          const u32* indices, u32 index_count, u32 bone_count) {
    if (!g_gpu_skinning_context || !g_gpu_skinning_context->initialized) {
        LOG_ERROR(LOG_CAT_RENDERER, "GPU skinning not initialized");
        return NULL;
    }
    
    if (g_gpu_skinning_context->mesh_count >= GPU_SKINNING_MAX_MESHES) {
        LOG_ERROR(LOG_CAT_RENDERER, "Maximum mesh count reached");
        return NULL;
    }
    
    GPUSkinningMesh* mesh = &g_gpu_skinning_context->meshes[g_gpu_skinning_context->mesh_count++];
    
    mesh->vertex_count = vertex_count;
    mesh->index_count = index_count;
    mesh->bone_count = bone_count;
    mesh->bone_offset = 0;  // Will be set when instance is created
    mesh->is_dirty = true;
    
    // Copy vertex data
    mesh->vertices = MALLOC_PERSISTENT(vertex_count * sizeof(GPUSkinningVertex));
    if (!mesh->vertices) {
        LOG_ERROR(LOG_CAT_RENDERER, "Failed to allocate mesh vertices");
        g_gpu_skinning_context->mesh_count--;
        return NULL;
    }
    
    memcpy(mesh->vertices, vertices, vertex_count * sizeof(GPUSkinningVertex));
    
    // Normalize bone weights
    for (u32 i = 0; i < vertex_count; i++) {
        normalize_bone_weights(mesh->vertices[i].bone_weights, mesh->vertices[i].bone_indices, 
                              GPU_SKINNING_MAX_BONE_INFLUENCES);
    }
    
    // Copy index data
    mesh->indices = MALLOC_PERSISTENT(index_count * sizeof(u32));
    if (!mesh->indices) {
        LOG_ERROR(LOG_CAT_RENDERER, "Failed to allocate mesh indices");
        FREE(mesh->vertices);
        g_gpu_skinning_context->mesh_count--;
        return NULL;
    }
    
    memcpy(mesh->indices, indices, index_count * sizeof(u32));
    
    // Create input vertex buffer
    BufferDesc vertex_buffer_desc = {
        .size = vertex_count * sizeof(GPUSkinningVertex),
        .usage = BUFFER_USAGE_STORAGE | BUFFER_USAGE_VERTEX,
        .memory_type = MEMORY_TYPE_DEVICE_LOCAL
    };
    
    mesh->vertex_buffer = buffer_create(&vertex_buffer_desc);
    if (!mesh->vertex_buffer) {
        LOG_ERROR(LOG_CAT_RENDERER, "Failed to create mesh vertex buffer");
        FREE(mesh->vertices);
        FREE(mesh->indices);
        g_gpu_skinning_context->mesh_count--;
        return NULL;
    }
    
    // Create input index buffer
    BufferDesc index_buffer_desc = {
        .size = index_count * sizeof(u32),
        .usage = BUFFER_USAGE_INDEX,
        .memory_type = MEMORY_TYPE_DEVICE_LOCAL
    };
    
    mesh->index_buffer = buffer_create(&index_buffer_desc);
    if (!mesh->index_buffer) {
        LOG_ERROR(LOG_CAT_RENDERER, "Failed to create mesh index buffer");
        buffer_destroy(mesh->vertex_buffer);
        FREE(mesh->vertices);
        FREE(mesh->indices);
        g_gpu_skinning_context->mesh_count--;
        return NULL;
    }
    
    // Create output vertex buffer (skinned vertices)
    BufferDesc output_vertex_desc = {
        .size = vertex_count * sizeof(GPUSkinningVertex),
        .usage = BUFFER_USAGE_STORAGE | BUFFER_USAGE_VERTEX,
        .memory_type = MEMORY_TYPE_DEVICE_LOCAL
    };
    
    mesh->output_vertex_buffer = buffer_create(&output_vertex_desc);
    if (!mesh->output_vertex_buffer) {
        LOG_ERROR(LOG_CAT_RENDERER, "Failed to create mesh output vertex buffer");
        buffer_destroy(mesh->vertex_buffer);
        buffer_destroy(mesh->index_buffer);
        FREE(mesh->vertices);
        FREE(mesh->indices);
        g_gpu_skinning_context->mesh_count--;
        return NULL;
    }
    
    // Upload data to GPU
    buffer_upload_data(mesh->vertex_buffer, mesh->vertices, vertex_count * sizeof(GPUSkinningVertex));
    buffer_upload_data(mesh->index_buffer, mesh->indices, index_count * sizeof(u32));
    
    LOG_INFO(LOG_CAT_RENDERER, "Created skinned mesh with %u vertices, %u indices, %u bones", 
             vertex_count, index_count, bone_count);
    return mesh;
}

void gpu_skinning_destroy_mesh(GPUSkinningMesh* mesh) {
    if (!mesh || !g_gpu_skinning_context) return;
    
    // Find mesh in array and destroy
    for (u32 i = 0; i < g_gpu_skinning_context->mesh_count; i++) {
        if (&g_gpu_skinning_context->meshes[i] == mesh) {
            // Destroy resources
            if (mesh->vertex_buffer) buffer_destroy(mesh->vertex_buffer);
            if (mesh->index_buffer) buffer_destroy(mesh->index_buffer);
            if (mesh->output_vertex_buffer) buffer_destroy(mesh->output_vertex_buffer);
            if (mesh->bone_buffer) buffer_destroy(mesh->bone_buffer);
            if (mesh->vertices) FREE(mesh->vertices);
            if (mesh->indices) FREE(mesh->indices);
            
            // Shift remaining meshes
            for (u32 j = i; j < g_gpu_skinning_context->mesh_count - 1; j++) {
                g_gpu_skinning_context->meshes[j] = g_gpu_skinning_context->meshes[j + 1];
            }
            g_gpu_skinning_context->mesh_count--;
            
            LOG_INFO(LOG_CAT_RENDERER, "Destroyed skinned mesh");
            return;
        }
    }
}

void gpu_skinning_update_mesh_vertices(GPUSkinningMesh* mesh, const GPUSkinningVertex* vertices) {
    if (!mesh || !vertices) return;
    
    memcpy(mesh->vertices, vertices, mesh->vertex_count * sizeof(GPUSkinningVertex));
    
    // Normalize bone weights
    for (u32 i = 0; i < mesh->vertex_count; i++) {
        normalize_bone_weights(mesh->vertices[i].bone_weights, mesh->vertices[i].bone_indices, 
                              GPU_SKINNING_MAX_BONE_INFLUENCES);
    }
    
    // Upload to GPU
    buffer_upload_data(mesh->vertex_buffer, mesh->vertices, 
                     mesh->vertex_count * sizeof(GPUSkinningVertex));
    
    mesh->is_dirty = true;
}

// ============================================================================
// ANIMATION INSTANCE MANAGEMENT
// ============================================================================

GPUAnimationInstance* gpu_skinning_create_instance(GPUSkeleton* skeleton, GPUSkinningMesh* mesh) {
    if (!g_gpu_skinning_context || !skeleton || !mesh) {
        LOG_ERROR(LOG_CAT_RENDERER, "Invalid parameters for animation instance");
        return NULL;
    }
    
    if (g_gpu_skinning_context->instance_count >= GPU_SKINNING_MAX_MESHES) {
        LOG_ERROR(LOG_CAT_RENDERER, "Maximum instance count reached");
        return NULL;
    }
    
    GPUAnimationInstance* instance = &g_gpu_skinning_context->instances[g_gpu_skinning_context->instance_count++];
    
    instance->skeleton = skeleton;
    instance->mesh = mesh;
    instance->current_time = 0.0f;
    instance->playback_speed = 1.0f;
    instance->is_playing = false;
    instance->is_looping = true;
    instance->instance_id = g_gpu_skinning_context->instance_count - 1;
    
    // Set mesh bone offset
    mesh->bone_offset = 0;  // Will be calculated based on skeleton position
    
    // Create instance buffer for animation parameters
    BufferDesc instance_buffer_desc = {
        .size = sizeof(f32) * 4,  // time, speed, playing state, padding
        .usage = BUFFER_USAGE_UNIFORM | BUFFER_USAGE_DYNAMIC,
        .memory_type = MEMORY_TYPE_HOST_VISIBLE
    };
    
    instance->instance_buffer = buffer_create(&instance_buffer_desc);
    if (!instance->instance_buffer) {
        LOG_ERROR(LOG_CAT_RENDERER, "Failed to create instance buffer");
        g_gpu_skinning_context->instance_count--;
        return NULL;
    }
    
    LOG_INFO(LOG_CAT_RENDERER, "Created animation instance %u", instance->instance_id);
    return instance;
}

void gpu_skinning_destroy_instance(GPUAnimationInstance* instance) {
    if (!instance || !g_gpu_skinning_context) return;
    
    // Find instance in array and destroy
    for (u32 i = 0; i < g_gpu_skinning_context->instance_count; i++) {
        if (&g_gpu_skinning_context->instances[i] == instance) {
            // Destroy resources
            if (instance->instance_buffer) buffer_destroy(instance->instance_buffer);
            if (instance->skinned_vertices) buffer_destroy(instance->skinned_vertices);
            
            // Shift remaining instances
            for (u32 j = i; j < g_gpu_skinning_context->instance_count - 1; j++) {
                g_gpu_skinning_context->instances[j] = g_gpu_skinning_context->instances[j + 1];
            }
            g_gpu_skinning_context->instance_count--;
            
            LOG_INFO(LOG_CAT_RENDERER, "Destroyed animation instance");
            return;
        }
    }
}

void gpu_skinning_update_instance(GPUAnimationInstance* instance, f32 delta_time) {
    if (!instance || !instance->is_playing) return;
    
    instance->current_time += delta_time * instance->playback_speed;
    
    // Handle looping
    if (instance->current_time >= GPU_SKINNING_MAX_TIME) {
        if (instance->is_looping) {
            instance->current_time = fmod(instance->current_time, GPU_SKINNING_MAX_TIME);
        } else {
            instance->current_time = GPU_SKINNING_MAX_TIME;
            instance->is_playing = false;
        }
    }
}

void gpu_skinning_play_animation(GPUAnimationInstance* instance) {
    if (instance) instance->is_playing = true;
}

void gpu_skinning_pause_animation(GPUAnimationInstance* instance) {
    if (instance) instance->is_playing = false;
}

void gpu_skinning_set_animation_time(GPUAnimationInstance* instance, f32 time) {
    if (instance) {
        instance->current_time = clamp(time, 0.0f, GPU_SKINNING_MAX_TIME);
    }
}

// ============================================================================
// GPU PROCESSING
// ============================================================================

void gpu_skinning_process_all(void) {
    if (!g_gpu_skinning_context || !g_gpu_skinning_context->initialized) return;
    
    // Update all skeleton bone buffers if dirty
    for (u32 i = 0; i < g_gpu_skinning_context->skeleton_count; i++) {
        GPUSkeleton* skeleton = &g_gpu_skinning_context->skeletons[i];
        if (skeleton->is_dirty) {
            buffer_upload_data(skeleton->bone_buffer, skeleton->bones, 
                             skeleton->bone_count * sizeof(GPUBoneTransform));
            skeleton->is_dirty = false;
        }
    }
    
    // Process all active instances
    for (u32 i = 0; i < g_gpu_skinning_context->instance_count; i++) {
        GPUAnimationInstance* instance = &g_gpu_skinning_context->instances[i];
        if (instance->is_playing) {
            gpu_skinning_process_instance(instance);
        }
    }
}

void gpu_skinning_process_instance(GPUAnimationInstance* instance) {
    if (!instance || !g_gpu_skinning_context || !g_gpu_skinning_context->initialized) return;
    
    // Bind compute shader
    shader_bind_compute(g_gpu_skinning_context->skinning_compute_shader);
    
    // Bind resources
    texture_bind_compute(instance->skeleton->bone_texture, 0);
    texture_bind_compute(g_gpu_skinning_context->global_bone_texture, 1);
    buffer_bind_compute(instance->mesh->vertex_buffer, 0);
    buffer_bind_compute(instance->mesh->bone_buffer, 1);
    buffer_bind_image_compute(instance->mesh->output_vertex_buffer, 0);
    
    // Set uniforms
    struct {
        u32 vertex_count;
        u32 bone_count;
        u32 bone_offset;
        f32 animation_time;
    } uniforms = {
        .vertex_count = instance->mesh->vertex_count,
        .bone_count = instance->skeleton->bone_count,
        .bone_offset = instance->mesh->bone_offset,
        .animation_time = instance->current_time
    };
    
    shader_set_uniform_compute(g_gpu_skinning_context->skinning_compute_shader, "params", 
                              &uniforms, sizeof(uniforms));
    
    // Dispatch compute shader
    u32 work_groups_x = (instance->mesh->vertex_count + 63) / 64;  // 64 threads per group
    shader_dispatch_compute(g_gpu_skinning_context->skinning_compute_shader, work_groups_x, 1, 1);
    
    // Memory barrier
    shader_memory_barrier_compute();
    
    // Update statistics
    g_gpu_skinning_context->total_vertices_processed += instance->mesh->vertex_count;
    g_gpu_skinning_context->total_bones_processed += instance->skeleton->bone_count;
}

BufferID gpu_skinning_get_vertex_buffer(GPUAnimationInstance* instance) {
    return instance ? instance->mesh->output_vertex_buffer : 0;
}

BufferID gpu_skinning_get_index_buffer(GPUAnimationInstance* instance) {
    return instance ? instance->mesh->index_buffer : 0;
}

// ============================================================================
// STATISTICS AND DEBUGGING
// ============================================================================

void gpu_skinning_get_statistics(GPUSkinningStats* out_stats) {
    if (!g_gpu_skinning_context || !out_stats) return;
    
    out_stats->total_instances = g_gpu_skinning_context->instance_count;
    out_stats->active_instances = 0;
    out_stats->total_vertices = 0;
    out_stats->total_bones = 0;
    out_stats->average_skinning_time_ms = g_gpu_skinning_context->average_skinning_time_ms;
    out_stats->gpu_utilization = 0.0f;  // Would need GPU profiling
    out_stats->memory_usage_mb = 0;     // Would need memory tracking
    
    // Count active instances and totals
    for (u32 i = 0; i < g_gpu_skinning_context->instance_count; i++) {
        GPUAnimationInstance* instance = &g_gpu_skinning_context->instances[i];
        if (instance->is_playing) {
            out_stats->active_instances++;
            out_stats->total_vertices += instance->mesh->vertex_count;
            out_stats->total_bones += instance->skeleton->bone_count;
        }
    }
}

void gpu_skinning_print_statistics(void) {
    if (!g_gpu_skinning_context) {
        LOG_WARN(LOG_CAT_RENDERER, "GPU skinning not initialized");
        return;
    }
    
    GPUSkinningStats stats;
    gpu_skinning_get_statistics(&stats);
    
    LOG_INFO(LOG_CAT_RENDERER, "=== GPU Skinning Statistics ===");
    LOG_INFO(LOG_CAT_RENDERER, "Instances: %u total, %u active", stats.total_instances, stats.active_instances);
    LOG_INFO(LOG_CAT_RENDERER, "Vertices: %u total", stats.total_vertices);
    LOG_INFO(LOG_CAT_RENDERER, "Bones: %u total", stats.total_bones);
    LOG_INFO(LOG_CAT_RENDERER, "Average skinning time: %.2f ms", stats.average_skinning_time_ms);
    LOG_INFO(LOG_CAT_RENDERER, "=== End Statistics ===");
}

bool gpu_skinning_validate_instance(GPUAnimationInstance* instance) {
    if (!instance) return false;
    
    if (!instance->skeleton || !instance->mesh) {
        LOG_ERROR(LOG_CAT_RENDERER, "Instance has null skeleton or mesh");
        return false;
    }
    
    if (instance->skeleton->bone_count > GPU_SKINNING_MAX_BONES) {
        LOG_ERROR(LOG_CAT_RENDERER, "Skeleton bone count exceeds maximum");
        return false;
    }
    
    if (instance->mesh->vertex_count == 0) {
        LOG_ERROR(LOG_CAT_RENDERER, "Mesh has no vertices");
        return false;
    }
    
    return true;
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

mat3x4 gpu_skinning_matrix_to_skinning_matrix(const mat4* matrix) {
    return matrix_to_skinning_matrix(matrix);
}

void gpu_skinning_calculate_bone_bounds(GPUSkeleton* skeleton, vec3* out_min, vec3* out_max) {
    if (!skeleton || !out_min || !out_max) return;
    
    *out_min = (vec3){FLT_MAX, FLT_MAX, FLT_MAX};
    *out_max = (vec3){-FLT_MAX, -FLT_MAX, -FLT_MAX};
    
    for (u32 i = 0; i < skeleton->bone_count; i++) {
        vec3 bone_pos = (vec3){
            skeleton->bones[i].transform.m[3][0],
            skeleton->bones[i].transform.m[3][1],
            skeleton->bones[i].transform.m[3][2]
        };
        
        if (bone_pos.x < out_min->x) out_min->x = bone_pos.x;
        if (bone_pos.y < out_min->y) out_min->y = bone_pos.y;
        if (bone_pos.z < out_min->z) out_min->z = bone_pos.z;
        
        if (bone_pos.x > out_max->x) out_max->x = bone_pos.x;
        if (bone_pos.y > out_max->y) out_max->y = bone_pos.y;
        if (bone_pos.z > out_max->z) out_max->z = bone_pos.z;
    }
}

bool gpu_skinning_instance_needs_update(GPUAnimationInstance* instance) {
    return instance && instance->is_playing;
}
