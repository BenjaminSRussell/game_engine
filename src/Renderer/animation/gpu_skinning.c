// src/engine/rendering/animation/gpu_skinning.c
//
// Purpose: GPU-based skeletal animation skinning implementation
// Provides high-performance vertex skinning using compute shaders

#include "rendering/animation/gpu_skinning.h"
#include "core/logger/unified_logger.h"
#include "core/memory/unified_allocator.h"
#include "math/mat4.h"              // For mat4_identity
#include "rendering/core/shader.h"  // For shader types
#include "rendering/core/texture.h" // For TextureCreateInfo
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef TEXTURE_FORMAT_RGBA32F
#define TEXTURE_FORMAT_RGBA32F 30 // Placeholder
#endif

// ============================================================================
// INTERNAL STATE
// ============================================================================

static GPUSkinningContext *g_gpu_skinning_context = NULL;

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

static mat3x4 matrix_to_skinning_matrix(const mat4 *matrix) {
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

static void normalize_bone_weights(f32 *weights, u8 *indices, u32 count) {
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
    LOG_WARN_CAT(LOG_CAT_RENDERER, "GPU skinning already initialized");
    return true;
  }

  LOG_INFO_CAT(LOG_CAT_RENDERER, "Initializing GPU skinning system");

  g_gpu_skinning_context = MALLOC_PERSISTENT(sizeof(GPUSkinningContext));
  if (!g_gpu_skinning_context) {
    LOG_ERROR_CAT(LOG_CAT_RENDERER, "Failed to allocate GPU skinning context");
    return false;
  }

  memset(g_gpu_skinning_context, 0, sizeof(GPUSkinningContext));

  // Load compute shader
  g_gpu_skinning_context->skinning_compute_shader =
      shader_load_compute("shaders/animation/gpu_skinning.comp");
  if (!g_gpu_skinning_context->skinning_compute_shader) {
    LOG_ERROR_CAT(LOG_CAT_RENDERER,
                  "Failed to load GPU skinning compute shader");
    FREE(g_gpu_skinning_context);
    g_gpu_skinning_context = NULL;
    return false;
  }

  // Create global bone buffer
  BufferCreateInfo bone_buffer_desc = {
      .size = GPU_SKINNING_MAX_BONES * max_skeletons * sizeof(GPUBoneTransform),
      .usage = BUFFER_USAGE_STORAGE,
      .flags = 0,
      .name = "Global_Bone_Buffer"};

  g_gpu_skinning_context->global_bone_buffer = buffer_create(&bone_buffer_desc);
  if (!g_gpu_skinning_context->global_bone_buffer) {
    LOG_ERROR_CAT(LOG_CAT_RENDERER, "Failed to create global bone buffer");
    FREE(g_gpu_skinning_context);
    g_gpu_skinning_context = NULL;
    return false;
  }

  // Create global bone texture
  TextureCreateInfo bone_texture_desc = {.width = GPU_SKINNING_MAX_BONES * 4,
                                         .height = max_skeletons,
                                         .depth = 1,
                                         .format = TEXTURE_FORMAT_RGBA32F,
                                         .usage = TEXTURE_USAGE_STORAGE |
                                                  TEXTURE_USAGE_SAMPLED,
                                         .name = "Global_Bone_Texture"};

  g_gpu_skinning_context->global_bone_texture =
      texture_create(&bone_texture_desc);
  if (!g_gpu_skinning_context->global_bone_texture) {
    LOG_ERROR_CAT(LOG_CAT_RENDERER, "Failed to create global bone texture");
    buffer_destroy(g_gpu_skinning_context->global_bone_buffer);
    FREE(g_gpu_skinning_context);
    g_gpu_skinning_context = NULL;
    return false;
  }

  g_gpu_skinning_context->initialized = true;

  LOG_INFO_CAT(LOG_CAT_RENDERER,
               "GPU skinning system initialized successfully");
  return true;
}

void gpu_skinning_shutdown(void) {
  if (!g_gpu_skinning_context)
    return;

  LOG_INFO_CAT(LOG_CAT_RENDERER, "Shutting down GPU skinning system");

  // Destroy all instances
  for (u32 i = 0; i < g_gpu_skinning_context->instance_count; i++) {
    GPUAnimationInstance *instance = &g_gpu_skinning_context->instances[i];
    if (instance->instance_buffer)
      buffer_destroy(instance->instance_buffer);
  }

  // Destroy meshes
  for (u32 i = 0; i < g_gpu_skinning_context->mesh_count; i++) {
    GPUSkinningMesh *mesh = &g_gpu_skinning_context->meshes[i];
    if (mesh->vertex_buffer)
      buffer_destroy(mesh->vertex_buffer);
    if (mesh->index_buffer)
      buffer_destroy(mesh->index_buffer);
    if (mesh->output_vertex_buffer)
      buffer_destroy(mesh->output_vertex_buffer);
    if (mesh->vertices)
      FREE(mesh->vertices);
    if (mesh->indices)
      FREE(mesh->indices);
  }

  // Destroy skeletons
  for (u32 i = 0; i < g_gpu_skinning_context->skeleton_count; i++) {
    GPUSkeleton *skeleton = &g_gpu_skinning_context->skeletons[i];
    if (skeleton->bone_buffer)
      buffer_destroy(skeleton->bone_buffer);
    if (skeleton->bone_texture)
      texture_destroy(skeleton->bone_texture);
  }

  // Global
  if (g_gpu_skinning_context->global_bone_buffer)
    buffer_destroy(g_gpu_skinning_context->global_bone_buffer);
  if (g_gpu_skinning_context->global_bone_texture)
    texture_destroy(g_gpu_skinning_context->global_bone_texture);

  FREE(g_gpu_skinning_context);
  g_gpu_skinning_context = NULL;

  LOG_INFO_CAT(LOG_CAT_RENDERER, "GPU skinning system shutdown complete");
}

GPUSkinningContext *gpu_skinning_get_context(void) {
  return g_gpu_skinning_context;
}

// ... SKELETON ...
GPUSkeleton *gpu_skinning_create_skeleton(const char *name, u32 bone_count) {
  if (!g_gpu_skinning_context)
    return NULL;
  if (g_gpu_skinning_context->skeleton_count >= GPU_SKINNING_MAX_MESHES)
    return NULL;

  GPUSkeleton *skeleton =
      &g_gpu_skinning_context
           ->skeletons[g_gpu_skinning_context->skeleton_count++];
  strncpy(skeleton->name, name, sizeof(skeleton->name) - 1);
  skeleton->bone_count = bone_count;
  skeleton->is_dirty = true;

  for (u32 i = 0; i < bone_count; i++) {
    memset(&skeleton->bones[i].transform, 0, sizeof(Mat4));
    skeleton->bones[i].transform.m00 = 1;
    skeleton->bones[i].transform.m11 = 1;
    skeleton->bones[i].transform.m22 = 1;
    skeleton->bones[i].transform.m33 = 1;

    skeleton->bones[i].skinning_matrix =
        matrix_to_skinning_matrix(&skeleton->bones[i].transform);
    skeleton->parent_indices[i] = i;
  }

  BufferCreateInfo bone_buffer_desc = {.size = bone_count *
                                               sizeof(GPUBoneTransform),
                                       .usage = BUFFER_USAGE_STORAGE,
                                       .flags = 0,
                                       .name = "Skeleton_Bone_Buffer"};
  skeleton->bone_buffer = buffer_create(&bone_buffer_desc);

  TextureCreateInfo bone_tex_desc = {.width = GPU_SKINNING_MAX_BONES * 4,
                                     .height = 1,
                                     .depth = 1,
                                     .format = TEXTURE_FORMAT_RGBA32F,
                                     .usage = TEXTURE_USAGE_STORAGE |
                                              TEXTURE_USAGE_SAMPLED,
                                     .name = "Skeleton_Bone_Texture"};
  skeleton->bone_texture = texture_create(&bone_tex_desc);

  return skeleton;
}

// ... Destroy skeleton ...
void gpu_skinning_destroy_skeleton(GPUSkeleton *skeleton) {
  if (skeleton && skeleton->bone_buffer)
    buffer_destroy(skeleton->bone_buffer);
  if (skeleton && skeleton->bone_texture)
    texture_destroy(skeleton->bone_texture);
}

void gpu_skinning_update_skeleton(GPUSkeleton *skeleton,
                                  const GPUBoneTransform *transforms) {
  if (skeleton) {
    memcpy(skeleton->bones, transforms,
           skeleton->bone_count * sizeof(GPUBoneTransform));
    for (u32 i = 0; i < skeleton->bone_count; ++i) {
      skeleton->bones[i].skinning_matrix =
          matrix_to_skinning_matrix(&skeleton->bones[i].transform);
    }
    skeleton->is_dirty = true;
  }
}

void gpu_skinning_set_bone_parents(GPUSkeleton *skeleton, const u32 *parents) {
  if (skeleton) {
    memcpy(skeleton->parent_indices, parents,
           skeleton->bone_count * sizeof(u32));
    skeleton->is_dirty = true;
  }
}

// ... MESH ...
GPUSkinningMesh *gpu_skinning_create_mesh(const GPUSkinningVertex *verts,
                                          u32 v_count, const u32 *indices,
                                          u32 i_count, u32 b_count) {
  if (!g_gpu_skinning_context)
    return NULL;
  GPUSkinningMesh *mesh =
      &g_gpu_skinning_context->meshes[g_gpu_skinning_context->mesh_count++];
  mesh->vertex_count = v_count;
  mesh->index_count = i_count;
  mesh->bone_count = b_count;
  mesh->vertices = MALLOC_PERSISTENT(v_count * sizeof(GPUSkinningVertex));
  memcpy(mesh->vertices, verts, v_count * sizeof(GPUSkinningVertex));
  mesh->indices = MALLOC_PERSISTENT(i_count * sizeof(u32));
  memcpy(mesh->indices, indices, i_count * sizeof(u32));

  BufferCreateInfo v_info = {.size = v_count * sizeof(GPUSkinningVertex),
                             .usage =
                                 BUFFER_USAGE_STORAGE | BUFFER_USAGE_VERTEX,
                             .name = "Mesh_Vertex_Buffer"};
  mesh->vertex_buffer = buffer_create(&v_info);

  BufferCreateInfo i_info = {.size = i_count * sizeof(u32),
                             .usage = BUFFER_USAGE_INDEX,
                             .name = "Mesh_Index_Buffer"};
  mesh->index_buffer = buffer_create(&i_info);

  BufferCreateInfo out_v_info = {.size = v_count * sizeof(GPUSkinningVertex),
                                 .usage =
                                     BUFFER_USAGE_STORAGE | BUFFER_USAGE_VERTEX,
                                 .name = "Mesh_Output_Buffer"};
  mesh->output_vertex_buffer = buffer_create(&out_v_info);

  return mesh;
}

void gpu_skinning_destroy_mesh(GPUSkinningMesh *mesh) {
  if (mesh->vertex_buffer)
    buffer_destroy(mesh->vertex_buffer);
  if (mesh->index_buffer)
    buffer_destroy(mesh->index_buffer);
  if (mesh->output_vertex_buffer)
    buffer_destroy(mesh->output_vertex_buffer);
  FREE(mesh->vertices);
  FREE(mesh->indices);
}

void gpu_skinning_update_mesh_vertices(GPUSkinningMesh *mesh,
                                       const GPUSkinningVertex *verts) {
  if (mesh) {
    memcpy(mesh->vertices, verts,
           mesh->vertex_count * sizeof(GPUSkinningVertex));
    mesh->is_dirty = true;
  }
}

// ... INSTANCE ...
GPUAnimationInstance *gpu_skinning_create_instance(GPUSkeleton *skeleton,
                                                   GPUSkinningMesh *mesh) {
  if (!g_gpu_skinning_context)
    return NULL;
  GPUAnimationInstance *inst =
      &g_gpu_skinning_context
           ->instances[g_gpu_skinning_context->instance_count++];
  inst->skeleton = skeleton;
  inst->mesh = mesh;
  inst->instance_id = g_gpu_skinning_context->instance_count - 1;

  BufferCreateInfo info = {.size = sizeof(f32) * 4,
                           .usage = BUFFER_USAGE_UNIFORM,
                           .name = "Instance_Buffer"};
  inst->instance_buffer = buffer_create(&info);
  inst->skinned_vertices = mesh->output_vertex_buffer;

  return inst;
}

void gpu_skinning_destroy_instance(GPUAnimationInstance *inst) {
  if (inst->instance_buffer)
    buffer_destroy(inst->instance_buffer);
}

void gpu_skinning_update_instance(GPUAnimationInstance *inst, f32 dt) {
  if (inst && inst->is_playing) {
    inst->current_time += dt * inst->playback_speed;
  }
}

void gpu_skinning_play_animation(GPUAnimationInstance *inst) {
  if (inst)
    inst->is_playing = true;
}
void gpu_skinning_pause_animation(GPUAnimationInstance *inst) {
  if (inst)
    inst->is_playing = false;
}
void gpu_skinning_set_animation_time(GPUAnimationInstance *inst, f32 t) {}

// ... PROCESS ...
void gpu_skinning_process_all(void) {} // Stub as upload logic missing
void gpu_skinning_process_instance(GPUAnimationInstance *inst) {} // Stub

BufferID gpu_skinning_get_vertex_buffer(GPUAnimationInstance *inst) {
  if (inst && inst->mesh && inst->mesh->output_vertex_buffer)
    return buffer_get_id(inst->mesh->output_vertex_buffer);
  return (BufferID){0};
}

BufferID gpu_skinning_get_index_buffer(GPUAnimationInstance *inst) {
  if (inst && inst->mesh && inst->mesh->index_buffer)
    return buffer_get_id(inst->mesh->index_buffer);
  return (BufferID){0};
}

void gpu_skinning_get_statistics(GPUSkinningStats *s) {}
void gpu_skinning_print_statistics(void) {}
bool gpu_skinning_validate_instance(GPUAnimationInstance *i) { return true; }
mat3x4 gpu_skinning_matrix_to_skinning_matrix(const mat4 *m) {
  return matrix_to_skinning_matrix(m);
}
void gpu_skinning_calculate_bone_bounds(GPUSkeleton *s, vec3 *min, vec3 *max) {}
bool gpu_skinning_instance_needs_update(GPUAnimationInstance *i) {
  return i && i->is_playing;
}
