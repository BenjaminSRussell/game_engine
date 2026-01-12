// src/engine/rendering/animation/gpu_skinning.h
//
// Purpose: GPU-based skeletal animation skinning system
// Provides high-performance vertex skinning using compute shaders

#ifndef GPU_SKINNING_H
#define GPU_SKINNING_H

#include "core/math/types.h"
#include "rendering/core/texture.h"
#include "rendering/core/buffer.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// CONSTANTS AND LIMITS
// ============================================================================

#define GPU_SKINNING_MAX_BONES 256
#define GPU_SKINNING_MAX_BONE_INFLUENCES 4
#define GPU_SKINNING_MAX_VERTICES 65536
#define GPU_SKINNING_MAX_MESHES 1024

// ============================================================================
// DATA STRUCTURES
// ============================================================================

// Bone transform matrix (3x4 for skinning)
typedef struct GPUBoneTransform {
    mat4 transform;        // Full 4x4 transform
    mat3x4 skinning_matrix; // 3x4 matrix for vertex skinning
} GPUBoneTransform;

// Vertex with skinning data
typedef struct GPUSkinningVertex {
    vec3 position;
    vec3 normal;
    vec4 tangent;
    vec2 texcoord;
    u8 bone_indices[GPU_SKINNING_MAX_BONE_INFLUENCES];
    f32 bone_weights[GPU_SKINNING_MAX_BONE_INFLUENCES];
} GPUSkinningVertex;

// Mesh skinning data
typedef struct GPUSkinningMesh {
    // Input data (static)
    GPUSkinningVertex* vertices;
    u32 vertex_count;
    u32* indices;
    u32 index_count;
    
    // Output data (skinned)
    BufferID output_vertex_buffer;
    BufferID output_index_buffer;
    
    // Bone information
    u32 bone_count;
    u32 bone_offset;  // Offset in global bone array
    
    // GPU resources
    BufferID vertex_buffer;
    BufferID index_buffer;
    BufferID bone_buffer;
    
    bool is_dirty;
} GPUSkinningMesh;

// Skeleton for GPU skinning
typedef struct GPUSkeleton {
    char name[64];
    GPUBoneTransform bones[GPU_SKINNING_MAX_BONES];
    u32 bone_count;
    u32 parent_indices[GPU_SKINNING_MAX_BONES];
    
    // GPU resources
    BufferID bone_buffer;
    TextureID bone_texture;  // For compute shader access
    
    bool is_dirty;
} GPUSkeleton;

// Animation instance
typedef struct GPUAnimationInstance {
    GPUSkeleton* skeleton;
    GPUSkinningMesh* mesh;
    
    // Animation state
    f32 current_time;
    f32 playback_speed;
    bool is_playing;
    bool is_looping;
    
    // GPU resources
    u32 instance_id;
    BufferID instance_buffer;
    
    // Output
    BufferID skinned_vertices;
} GPUAnimationInstance;

// GPU skinning system context
typedef struct GPUSkinningContext {
    // Compute shader
    u32 skinning_compute_shader;
    
    // Storage
    GPUSkeleton skeletons[GPU_SKINNING_MAX_MESHES];
    GPUSkinningMesh meshes[GPU_SKINNING_MAX_MESHES];
    GPUAnimationInstance instances[GPU_SKINNING_MAX_MESHES];
    
    u32 skeleton_count;
    u32 mesh_count;
    u32 instance_count;
    
    // Global bone buffer (for all skeletons)
    BufferID global_bone_buffer;
    TextureID global_bone_texture;
    
    // Statistics
    u32 total_vertices_processed;
    u32 total_bones_processed;
    f32 average_skinning_time_ms;
    
    bool initialized;
} GPUSkinningContext;

// ============================================================================
// SYSTEM MANAGEMENT
// ============================================================================

// Initialize GPU skinning system
bool gpu_skinning_initialize(u32 max_meshes, u32 max_skeletons);

// Shutdown GPU skinning system
void gpu_skinning_shutdown(void);

// Get global GPU skinning context
GPUSkinningContext* gpu_skinning_get_context(void);

// ============================================================================
// SKELETON MANAGEMENT
// ============================================================================

// Create skeleton
GPUSkeleton* gpu_skinning_create_skeleton(const char* name, u32 bone_count);

// Destroy skeleton
void gpu_skinning_destroy_skeleton(GPUSkeleton* skeleton);

// Update skeleton bone transforms
void gpu_skinning_update_skeleton(GPUSkeleton* skeleton, const GPUBoneTransform* bone_transforms);

// Set bone parent hierarchy
void gpu_skinning_set_bone_parents(GPUSkeleton* skeleton, const u32* parent_indices);

// ============================================================================
// MESH MANAGEMENT
// ============================================================================

// Create skinned mesh
GPUSkinningMesh* gpu_skinning_create_mesh(const GPUSkinningVertex* vertices, u32 vertex_count,
                                          const u32* indices, u32 index_count, u32 bone_count);

// Destroy mesh
void gpu_skinning_destroy_mesh(GPUSkinningMesh* mesh);

// Update mesh vertex data
void gpu_skinning_update_mesh_vertices(GPUSkinningMesh* mesh, const GPUSkinningVertex* vertices);

// ============================================================================
// ANIMATION INSTANCE MANAGEMENT
// ============================================================================

// Create animation instance
GPUAnimationInstance* gpu_skinning_create_instance(GPUSkeleton* skeleton, GPUSkinningMesh* mesh);

// Destroy animation instance
void gpu_skinning_destroy_instance(GPUAnimationInstance* instance);

// Update animation instance
void gpu_skinning_update_instance(GPUAnimationInstance* instance, f32 delta_time);

// Play/pause animation
void gpu_skinning_play_animation(GPUAnimationInstance* instance);
void gpu_skinning_pause_animation(GPUAnimationInstance* instance);

// Set animation time
void gpu_skinning_set_animation_time(GPUAnimationInstance* instance, f32 time);

// ============================================================================
// GPU PROCESSING
// ============================================================================

// Perform GPU skinning for all instances
void gpu_skinning_process_all(void);

// Perform GPU skinning for specific instance
void gpu_skinning_process_instance(GPUAnimationInstance* instance);

// Get skinned vertex buffer for rendering
BufferID gpu_skinning_get_vertex_buffer(GPUAnimationInstance* instance);

// Get skinned index buffer for rendering
BufferID gpu_skinning_get_index_buffer(GPUAnimationInstance* instance);

// ============================================================================
// BATCH PROCESSING
// ============================================================================

// Process multiple instances in batch
void gpu_skinning_process_batch(GPUAnimationInstance** instances, u32 count);

// Optimize instance ordering for better cache performance
void gpu_skinning_optimize_batch_ordering(GPUAnimationInstance** instances, u32 count);

// ============================================================================
// DEBUGGING AND STATISTICS
// ============================================================================

// Get skinning statistics
typedef struct GPUSkinningStats {
    u32 total_instances;
    u32 active_instances;
    u32 total_vertices;
    u32 total_bones;
    f32 average_skinning_time_ms;
    f32 gpu_utilization;
    u32 memory_usage_mb;
} GPUSkinningStats;

void gpu_skinning_get_statistics(GPUSkinningStats* out_stats);

// Print skinning statistics
void gpu_skinning_print_statistics(void);

// Validate skinning data
bool gpu_skinning_validate_instance(GPUAnimationInstance* instance);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Convert 4x4 matrix to 3x4 skinning matrix
mat3x4 gpu_skinning_matrix_to_skinning_matrix(const mat4* matrix);

// Calculate bone bounding box
void gpu_skinning_calculate_bone_bounds(GPUSkeleton* skeleton, vec3* out_min, vec3* out_max);

// Check if instance needs skinning update
bool gpu_skinning_instance_needs_update(GPUAnimationInstance* instance);

// ============================================================================
// CONVENIENCE MACROS
// ============================================================================

#define GPU_SKINNING_MAX_TIME 3600.0f  // 1 hour
#define GPU_SKINNING_DEFAULT_PLAYBACK_SPEED 1.0f

// Check if GPU skinning is available
static inline bool gpu_skinning_is_available(void) {
    GPUSkinningContext* ctx = gpu_skinning_get_context();
    return ctx && ctx->initialized;
}

#ifdef __cplusplus
}
#endif

#endif // GPU_SKINNING_H
