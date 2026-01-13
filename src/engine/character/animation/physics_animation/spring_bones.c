/*
 * spring_bones.c
 * Spring constraint bones
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 */

#include "character/animation/physics_animation/spring_bones.h"
#include "math/mat4.h"
#include "math/quat.h"
#include "math/vec3.h"
#include <animation/skeleton_system.h>
#include <core/types.h>
#include <math.h>
#include "engine/include/math/math.h"
#include <math/quat.h>
#include <math/vec3.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================
 */

#define ANIMATION_SPRING_BONES_MAX_COUNT 4096
#define ANIMATION_SPRING_BONES_DEFAULT_CAPACITY 256
#define ANIMATION_SPRING_BONES_ALIGNMENT 16
#define SPRING_BONE_DT_SUBSTEPS 1

/* ============================================================================
 * INTERNAL TYPES
 * ============================================================================
 */

typedef struct animation_spring_bones_internal {
  uint32_t id;
  uint32_t flags;
  // Skeleton reference
  Skeleton *skeleton;

  // Configuration & State (SoA for SIMD/Batching)
  uint32_t count;
  uint32_t *bone_indices; // Index in skeleton
  float *bone_lengths;    // Rest length of the bone

  Vec3 *current_tails; // World space tail position
  Vec3 *prev_tails;    // Previous world space tail position
  Vec3 *bone_axes;     // Local axis that points to tail (usually +Y or +Z)

  // Physics properties
  Vec3 *gravities;   // Gravity vector (dir * power)
  float *stiffnesss; // Stiffness
  float *drags;      // Drag
  float *radii;      // Collision radius

  // Caching / Optimization
  Quat *cached_rotations; // Last computed local rotations
  Mat4 *parent_matrices;  // Cached parent world matrices

  // GPU Data
  animation_spring_bones_gpu_data_t *gpu_buffer;

  // Settings
  uint32_t lod_level;
  bool culling_enabled;
  bool is_visible;
  bool initialized;
  bool dirty;
  uint64_t frame_updated;
} SpringBoneInstance;

typedef struct animation_spring_bones_context {
  SpringBoneInstance *items;
  uint32_t count;
  uint32_t capacity;
  bool initialized;

  // Scratch buffers for batch processing
  Vec3 *scratch_vec_a;
  Vec3 *scratch_vec_b;
  Vec3 *scratch_vec_c;
} animation_spring_bones_context_t;

static animation_spring_bones_context_t g_spring_bones_ctx = {0};

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================
 */

static void *aligned_alloc_wrapper(size_t size) {
  // Simple wrapper, in a real engine use a proper aligned allocator
  void *ptr = malloc(size);
  // memset(ptr, 0, size); // Optional
  return ptr;
}

static void ensure_scratch_buffers(uint32_t count) {
  static uint32_t current_size = 0;
  if (count > current_size) {
    current_size = (count + 255) & ~255; // Align to 256
    g_spring_bones_ctx.scratch_vec_a =
        realloc(g_spring_bones_ctx.scratch_vec_a, current_size * sizeof(Vec3));
    g_spring_bones_ctx.scratch_vec_b =
        realloc(g_spring_bones_ctx.scratch_vec_b, current_size * sizeof(Vec3));
    g_spring_bones_ctx.scratch_vec_c =
        realloc(g_spring_bones_ctx.scratch_vec_c, current_size * sizeof(Vec3));
  }
}

static void free_instance_data(SpringBoneInstance *inst) {
  if (inst->bone_indices)
    free(inst->bone_indices);
  if (inst->bone_lengths)
    free(inst->bone_lengths);
  if (inst->current_tails)
    free(inst->current_tails);
  if (inst->prev_tails)
    free(inst->prev_tails);
  if (inst->bone_axes)
    free(inst->bone_axes);
  if (inst->gravities)
    free(inst->gravities);
  if (inst->stiffnesss)
    free(inst->stiffnesss);
  if (inst->drags)
    free(inst->drags);
  if (inst->radii)
    free(inst->radii);
  if (inst->cached_rotations)
    free(inst->cached_rotations);
  if (inst->parent_matrices)
    free(inst->parent_matrices);
  if (inst->gpu_buffer)
    free(inst->gpu_buffer);

  memset(inst, 0, sizeof(SpringBoneInstance));
}

// Local helper to create a rotation from two vectors
static Quat quat_from_vectors_local(Vec3 u, Vec3 v) {
  float dot = vec3_dot(u, v);
  if (dot >= 1.0f - 1e-6f) {
    return quat_identity();
  }
  if (dot < -1.0f + 1e-6f) {
    // Vectors are opposite. Rotate 180 degrees around any orthogonal axis.
    Vec3 axis = vec3_cross(vec3_create(1, 0, 0), u);
    if (vec3_length_sq(axis) < 1e-6f) {
      axis = vec3_cross(vec3_create(0, 1, 0), u);
    }
    axis = vec3_normalize(axis);
    return quat_from_axis_angle(axis, PI);
  }

  Vec3 axis = vec3_cross(u, v);
  float s = sqrtf((1.0f + dot) * 2.0f);
  float inv_s = 1.0f / s;

  return quat_create(s * 0.5f, axis.x * inv_s, axis.y * inv_s, axis.z * inv_s);
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================
 */

int animation_spring_bones_init(void) {
  if (g_spring_bones_ctx.initialized)
    return 0;

  g_spring_bones_ctx.capacity = ANIMATION_SPRING_BONES_DEFAULT_CAPACITY;
  g_spring_bones_ctx.items =
      calloc(g_spring_bones_ctx.capacity, sizeof(SpringBoneInstance));
  if (!g_spring_bones_ctx.items)
    return -1;

  g_spring_bones_ctx.count = 0;
  g_spring_bones_ctx.initialized = true;

  // Pre-allocate some scratch memory
  ensure_scratch_buffers(256);

  return 0;
}

void animation_spring_bones_shutdown(void) {
  if (!g_spring_bones_ctx.initialized)
    return;

  for (uint32_t i = 0; i < g_spring_bones_ctx.count; i++) {
    free_instance_data(&g_spring_bones_ctx.items[i]);
  }

  free(g_spring_bones_ctx.items);
  if (g_spring_bones_ctx.scratch_vec_a)
    free(g_spring_bones_ctx.scratch_vec_a);
  if (g_spring_bones_ctx.scratch_vec_b)
    free(g_spring_bones_ctx.scratch_vec_b);
  if (g_spring_bones_ctx.scratch_vec_c)
    free(g_spring_bones_ctx.scratch_vec_c);

  memset(&g_spring_bones_ctx, 0, sizeof(g_spring_bones_ctx));
}

int animation_spring_bones_create(animation_spring_bones_handle_t *out_handle,
                                  const animation_spring_bones_desc_t *desc) {
  if (!out_handle || !desc || !desc->skeleton || desc->bone_count == 0)
    return -1;
  if (!g_spring_bones_ctx.initialized)
    return -2;

  if (g_spring_bones_ctx.count >= g_spring_bones_ctx.capacity) {
    // Simple expansion logic could go here
    return -3;
  }

  uint32_t id = g_spring_bones_ctx.count++;
  SpringBoneInstance *inst = &g_spring_bones_ctx.items[id];

  inst->id = id;
  inst->flags = desc->flags;
  inst->skeleton = (Skeleton *)desc->skeleton;
  inst->count = desc->bone_count;
  inst->initialized = true;
  inst->is_visible = true; // Default visible

  // Allocate arrays
  size_t count = inst->count;
  inst->bone_indices = malloc(count * sizeof(uint32_t));
  inst->bone_lengths = malloc(count * sizeof(float));
  inst->current_tails = aligned_alloc_wrapper(count * sizeof(Vec3));
  inst->prev_tails = aligned_alloc_wrapper(count * sizeof(Vec3));
  inst->bone_axes = aligned_alloc_wrapper(count * sizeof(Vec3));
  inst->gravities = aligned_alloc_wrapper(count * sizeof(Vec3));
  inst->stiffnesss = malloc(count * sizeof(float));
  inst->drags = malloc(count * sizeof(float));
  inst->radii = malloc(count * sizeof(float));
  inst->cached_rotations = malloc(count * sizeof(Quat));
  inst->parent_matrices = malloc(count * sizeof(Mat4));
  inst->gpu_buffer = malloc(count * sizeof(animation_spring_bones_gpu_data_t));

  // Initialize data from config
  Skeleton *skel = inst->skeleton;
  for (uint32_t i = 0; i < count; i++) {
    const animation_spring_bone_config_t *cfg = &desc->bones[i];

    // Find bone index
    Bone *bone = skeleton_get_bone(skel, cfg->bone_name);
    if (!bone) {
      inst->bone_indices[i] = 0;
    } else {
      inst->bone_indices[i] = bone->bone_id;
    }

    inst->stiffnesss[i] = cfg->stiffness;
    inst->drags[i] = cfg->drag;
    inst->radii[i] = cfg->collider_radius;

    Vec3 g_dir = vec3_create(cfg->gravity_dir[0], cfg->gravity_dir[1],
                             cfg->gravity_dir[2]);
    inst->gravities[i] = vec3_mul(g_dir, cfg->gravity_power);

    // Initialize state
    Bone *b = skeleton_get_bone_by_id(skel, inst->bone_indices[i]);
    if (b) {
      inst->bone_lengths[i] = (b->length > 0.001f) ? b->length : 1.0f;

      // Assume +Y axis for now
      inst->bone_axes[i] = vec3_create(0, 1, 0);

      inst->current_tails[i] = vec3_zero(); // Will be set in update
      inst->prev_tails[i] = vec3_zero();
    }
  }

  out_handle->id = id;
  return 0;
}

void animation_spring_bones_destroy(animation_spring_bones_handle_t handle) {
  if (handle.id >= g_spring_bones_ctx.count)
    return;
  free_instance_data(&g_spring_bones_ctx.items[handle.id]);
  g_spring_bones_ctx.items[handle.id].initialized = false;
}

static void update_instance(SpringBoneInstance *inst, float delta_time) {
  if (!inst->initialized || !inst->skeleton)
    return;

  // LOD & Culling check
  if (inst->culling_enabled && !inst->is_visible)
    return;
  if (inst->lod_level > 2)
    return; // Skip update for low LOD

  Skeleton *skel = inst->skeleton;
  uint32_t count = inst->count;

  // Retrieve scratch buffers
  ensure_scratch_buffers(count);
  Vec3 *forces = g_spring_bones_ctx.scratch_vec_a;

  // Physics Sub-stepping
  float dt = delta_time;

  // 1. Calculate Forces (Gravity + External)
  memcpy(forces, inst->gravities, count * sizeof(Vec3));

  for (uint32_t i = 0; i < count; i++) {
    // If first frame or reset, snap to target
    if (vec3_length_sq(inst->current_tails[i]) < 0.0001f) {
      Bone *bone = skeleton_get_bone_by_id(skel, inst->bone_indices[i]);
      if (!bone)
        continue;

      // Initial tail position in Model Space
      Vec3 tail_local = vec3_mul(inst->bone_axes[i], inst->bone_lengths[i]);
      // Use mat4_transform_point for model space transform
      Vec3 tail_model =
          mat4_transform_point(bone->current_pose_matrix, tail_local);

      inst->current_tails[i] = tail_model;
      inst->prev_tails[i] = tail_model;
    }

    // Verlet
    Vec3 curr = inst->current_tails[i];
    Vec3 prev = inst->prev_tails[i];

    // Inertia
    Vec3 velocity = vec3_sub(curr, prev);
    velocity = vec3_mul(velocity, 1.0f - inst->drags[i]);

    // Force
    Vec3 force = forces[i]; // Gravity

    // Stiffness (Force pulling back to rest pose)
    Bone *bone = skeleton_get_bone_by_id(skel, inst->bone_indices[i]);

    Vec3 tail_local = vec3_mul(inst->bone_axes[i], inst->bone_lengths[i]);
    Vec3 target_tail =
        mat4_transform_point(bone->current_pose_matrix, tail_local);

    Vec3 to_target = vec3_sub(target_tail, curr);
    Vec3 spring_force = vec3_mul(to_target, inst->stiffnesss[i] * dt);

    Vec3 total_move =
        vec3_add(velocity, vec3_mul(vec3_add(force, spring_force), dt * dt));
    Vec3 next = vec3_add(curr, total_move);

    // Constraints
    // 1. Length Constraint (distance from bone origin)
    // Access matrix elements carefully using column-major assumption (m[12],
    // m[13], m[14] are translation)
    Vec3 origin = vec3_create(bone->current_pose_matrix.m[12],
                              bone->current_pose_matrix.m[13],
                              bone->current_pose_matrix.m[14]);

    Vec3 dir = vec3_sub(next, origin);
    dir = vec3_normalize(dir);
    next = vec3_add(origin, vec3_mul(dir, inst->bone_lengths[i]));

    // Collision (Simple floor at y=0)
    if (next.y < inst->radii[i]) {
      next.y = inst->radii[i];
    }

    // Store state
    inst->prev_tails[i] = curr;
    inst->current_tails[i] = next;

    // Apply rotation to bone
    Vec3 target_dir = vec3_sub(target_tail, origin);
    target_dir = vec3_normalize(target_dir);

    Vec3 current_dir = vec3_sub(next, origin);
    current_dir = vec3_normalize(current_dir);

    Quat delta_rot = quat_from_vectors_local(target_dir, current_dir);

    // Apply delta rotation
    Bone *parent = skeleton_get_bone_by_id(skel, bone->parent_id);
    Quat parent_rot = parent ? parent->model_rotation : quat_identity();
    Quat parent_inv = quat_inverse(parent_rot);

    Quat model_rot = bone->model_rotation;
    Quat new_model_rot = quat_mul(delta_rot, model_rot);

    Quat new_local = quat_mul(parent_inv, new_model_rot);
    new_local = quat_normalize(new_local);

    bone->local_rotation = new_local;

    // Update GPU data
    inst->gpu_buffer[i].bone_index = inst->bone_indices[i];
    inst->gpu_buffer[i].position[0] = next.x;
    inst->gpu_buffer[i].position[1] = next.y;
    inst->gpu_buffer[i].position[2] = next.z;
    inst->gpu_buffer[i].rotation[0] = new_local.x;
    inst->gpu_buffer[i].rotation[1] = new_local.y;
    inst->gpu_buffer[i].rotation[2] = new_local.z;
    inst->gpu_buffer[i].rotation[3] = new_local.w;
  }

  // Finalize: re-evaluate skeleton hierarchy
  skeleton_update_bone_hierarchy(skel);

  inst->dirty = false;
}

int animation_spring_bones_update(animation_spring_bones_handle_t handle,
                                  float delta_time) {
  if (handle.id >= g_spring_bones_ctx.count)
    return -1;
  update_instance(&g_spring_bones_ctx.items[handle.id], delta_time);
  return 0;
}

int animation_spring_bones_update_batch(
    const animation_spring_bones_handle_t *handles, uint32_t count,
    float delta_time) {
  if (!handles)
    return -1;
  for (uint32_t i = 0; i < count; i++) {
    animation_spring_bones_update(handles[i], delta_time);
  }
  return 0;
}

int animation_spring_bones_update_async(animation_spring_bones_handle_t handle,
                                        float delta_time) {
  // Stub for async
  return animation_spring_bones_update(handle, delta_time);
}

void animation_spring_bones_set_lod(animation_spring_bones_handle_t handle,
                                    uint32_t lod_level) {
  if (handle.id < g_spring_bones_ctx.count) {
    g_spring_bones_ctx.items[handle.id].lod_level = lod_level;
  }
}

void animation_spring_bones_set_culling(animation_spring_bones_handle_t handle,
                                        bool enabled) {
  if (handle.id < g_spring_bones_ctx.count) {
    g_spring_bones_ctx.items[handle.id].culling_enabled = enabled;
  }
}

void animation_spring_bones_reset(animation_spring_bones_handle_t handle) {
  if (handle.id < g_spring_bones_ctx.count) {
    SpringBoneInstance *inst = &g_spring_bones_ctx.items[handle.id];
    if (inst->current_tails) {
      memset(inst->current_tails, 0,
             inst->count * sizeof(Vec3)); // Will trigger reset in update
    }
  }
}

bool animation_spring_bones_is_valid(animation_spring_bones_handle_t handle) {
  if (handle.id >= g_spring_bones_ctx.count)
    return false;
  return g_spring_bones_ctx.items[handle.id].initialized;
}

int animation_spring_bones_get_info(animation_spring_bones_handle_t handle,
                                    animation_spring_bones_info_t *out_info) {
  if (!out_info || handle.id >= g_spring_bones_ctx.count)
    return -1;
  SpringBoneInstance *inst = &g_spring_bones_ctx.items[handle.id];
  out_info->id = inst->id;
  out_info->flags = inst->flags;
  out_info->initialized = inst->initialized;
  out_info->active_bone_count = inst->count;
  return 0;
}

void animation_spring_bones_mark_dirty(animation_spring_bones_handle_t handle) {
  if (handle.id < g_spring_bones_ctx.count) {
    g_spring_bones_ctx.items[handle.id].dirty = true;
  }
}
