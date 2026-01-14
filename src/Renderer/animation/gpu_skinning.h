#ifndef GPU_SKINNING_H
#define GPU_SKINNING_H

#include "core/math/types.h"
#include "rendering/core/buffer.h"
#include "rendering/core/texture.h"

#ifdef __cplusplus
extern "C" {
#endif

// Constants
#define GPU_SKINNING_MAX_BONES 128
#define GPU_SKINNING_MAX_BONE_INFLUENCES 4
#define GPU_SKINNING_MAX_MESHES 1024
#define GPU_SKINNING_MAX_TIME 1000.0f

// Helper struct for 3x4 matrix (12 floats) - cleaner for GPU upload
typedef struct mat3x4 {
  float m[3][4];
} mat3x4;

// Bone transform data for GPU
typedef struct GPUBoneTransform {
  mat4 transform;
  mat3x4 skinning_matrix;
} GPUBoneTransform;

// Vertex with skinning data
typedef struct GPUSkinningVertex {
  vec3 position;
  vec3 normal;
  vec2 uv;
  f32 bone_weights[GPU_SKINNING_MAX_BONE_INFLUENCES];
  u8 bone_indices[GPU_SKINNING_MAX_BONE_INFLUENCES];
} GPUSkinningVertex;

// Skeleton data
typedef struct GPUSkeleton {
  char name[64];
  GPUBoneTransform bones[GPU_SKINNING_MAX_BONES];
  u32 parent_indices[GPU_SKINNING_MAX_BONES]; // Parent index for each bone
  u32 bone_count;
  Buffer *bone_buffer;   // Storage buffer for bone data
  Texture *bone_texture; // Texture for bone data (tbo)
  bool is_dirty;
} GPUSkeleton;

// Mesh data
typedef struct GPUSkinningMesh {
  GPUSkinningVertex *vertices; // CPU copy
  u32 *indices;                // CPU copy
  u32 vertex_count;
  u32 index_count;

  Buffer *vertex_buffer;        // Input static vertices
  Buffer *index_buffer;         // Indices
  Buffer *output_vertex_buffer; // Transformed vertices (GPU output)
  Buffer *output_index_buffer;  // Transformed indices (GPU output)
  Buffer *bone_buffer;          // Per-mesh bone data if needed

  u32 bone_count;
  u32 bone_offset; // Offset into global bone buffer
  bool is_dirty;
} GPUSkinningMesh;

// Animation instance
typedef struct GPUAnimationInstance {
  GPUSkeleton *skeleton;
  GPUSkinningMesh *mesh;

  f32 current_time;
  f32 playback_speed;
  bool is_playing;
  bool is_looping;

  Buffer *instance_buffer;  // Uniform buffer for instance data
  Buffer *skinned_vertices; // Pointer to output buffer (alias)

  u32 instance_id;
} GPUAnimationInstance;

// Main context
typedef struct GPUSkinningContext {
  u32 skinning_compute_shader; // Shader ID
  Buffer *global_bone_buffer;
  Texture *global_bone_texture;

  GPUSkeleton skeletons[GPU_SKINNING_MAX_MESHES];
  u32 skeleton_count;

  GPUSkinningMesh meshes[GPU_SKINNING_MAX_MESHES];
  u32 mesh_count;

  GPUAnimationInstance instances[GPU_SKINNING_MAX_MESHES];
  u32 instance_count;

  // Statistics
  u32 total_vertices_processed;
  u32 total_bones_processed;
  f32 average_skinning_time_ms;

  bool initialized;
} GPUSkinningContext;

// Statistics struct
typedef struct GPUSkinningStats {
  u32 total_instances;
  u32 active_instances;
  u32 total_vertices;
  u32 total_bones;
  f32 average_skinning_time_ms;
  f32 gpu_utilization;
  u32 memory_usage_mb;
} GPUSkinningStats;

// Core System
bool gpu_skinning_initialize(u32 max_meshes, u32 max_skeletons);
void gpu_skinning_shutdown(void);
GPUSkinningContext *gpu_skinning_get_context(void);

// Skeleton Management
GPUSkeleton *gpu_skinning_create_skeleton(const char *name, u32 bone_count);
void gpu_skinning_destroy_skeleton(GPUSkeleton *skeleton);
void gpu_skinning_update_skeleton(GPUSkeleton *skeleton,
                                  const GPUBoneTransform *bone_transforms);
void gpu_skinning_set_bone_parents(GPUSkeleton *skeleton,
                                   const u32 *parent_indices);

// Mesh Management
GPUSkinningMesh *gpu_skinning_create_mesh(const GPUSkinningVertex *vertices,
                                          u32 vertex_count, const u32 *indices,
                                          u32 index_count, u32 bone_count);
void gpu_skinning_destroy_mesh(GPUSkinningMesh *mesh);
void gpu_skinning_update_mesh_vertices(GPUSkinningMesh *mesh,
                                       const GPUSkinningVertex *vertices);

// Instance Management
GPUAnimationInstance *gpu_skinning_create_instance(GPUSkeleton *skeleton,
                                                   GPUSkinningMesh *mesh);
void gpu_skinning_destroy_instance(GPUAnimationInstance *instance);
void gpu_skinning_update_instance(GPUAnimationInstance *instance,
                                  f32 delta_time);
void gpu_skinning_play_animation(GPUAnimationInstance *instance);
void gpu_skinning_pause_animation(GPUAnimationInstance *instance);
void gpu_skinning_set_animation_time(GPUAnimationInstance *instance, f32 time);

// Processing
void gpu_skinning_process_all(void);
void gpu_skinning_process_instance(GPUAnimationInstance *instance);

// Getters
BufferID gpu_skinning_get_vertex_buffer(GPUAnimationInstance *instance);
BufferID gpu_skinning_get_index_buffer(GPUAnimationInstance *instance);

// Utilities
void gpu_skinning_get_statistics(GPUSkinningStats *out_stats);
void gpu_skinning_print_statistics(void);
bool gpu_skinning_validate_instance(GPUAnimationInstance *instance);
mat3x4 gpu_skinning_matrix_to_skinning_matrix(const mat4 *matrix);
void gpu_skinning_calculate_bone_bounds(GPUSkeleton *skeleton, vec3 *out_min,
                                        vec3 *out_max);
bool gpu_skinning_instance_needs_update(GPUAnimationInstance *instance);

#ifdef __cplusplus
}
#endif

#endif // GPU_SKINNING_H
