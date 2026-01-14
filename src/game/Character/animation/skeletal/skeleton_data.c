/*
 * skeleton_data.c
 * Skeleton bone hierarchy
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 *
 * Implements bone hierarchy storage, bind pose matrices, and skeleton
 * management
 */

#include "character/animation/skeletal/skeleton_data.h"
#include <common.h>
#include "include/math/mat4.h"
#include "include/math/quat.h"
#include "include/math/vec3.h"
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================
 */

#define ANIMATION_SKELETON_DATA_MAX_COUNT 4096
#define ANIMATION_SKELETON_DATA_DEFAULT_CAPACITY 256
#define ANIMATION_SKELETON_DATA_ALIGNMENT 16
#define SKELETON_MAX_BONES 256
#define SKELETON_BONE_NAME_MAX 64
#define SKELETON_INVALID_BONE_INDEX 0xFFFFFFFF

/* ============================================================================
 * SKELETON-SPECIFIC TYPES
 * ============================================================================
 */

typedef struct bone_data {
  char name[SKELETON_BONE_NAME_MAX];
  uint32_t name_hash;
  int32_t parent_index; // -1 for root bones

  // Bind pose (T-pose / rest pose)
  Vec3 bind_position;
  Quat bind_rotation;
  Vec3 bind_scale;

  // Precomputed matrices
  Mat4 local_bind_matrix;   // Local space bind pose
  Mat4 inverse_bind_matrix; // Used for skinning

  // Hierarchy data
  int32_t first_child_index;  // First child bone (-1 if leaf)
  int32_t next_sibling_index; // Next sibling bone (-1 if last)
  uint8_t depth;              // Depth in hierarchy (0 = root)
  uint8_t _padding[3];
} bone_data_t;

typedef struct skeleton {
  bone_data_t *bones;
  uint32_t bone_count;
  uint32_t root_bone_index;

  // Cached data for fast lookup
  uint32_t *bone_name_hashes; // For fast name-based lookup
  int32_t *sorted_indices;    // Bones sorted by hierarchy depth

  // Bounds
  Vec3 bounds_min;
  Vec3 bounds_max;
} skeleton_t;

typedef struct animation_skeleton_data_internal {
  uint32_t id;
  uint32_t flags;
  skeleton_t skeleton;
  bool initialized;
  bool dirty;
  uint64_t frame_updated;
} animation_skeleton_data_internal_t;

typedef struct animation_skeleton_data_context {
  animation_skeleton_data_internal_t *items;
  uint32_t count;
  uint32_t capacity;
  void *allocator;
  bool initialized;
} animation_skeleton_data_context_t;

static animation_skeleton_data_context_t g_skeleton_data_ctx = {0};

/* ============================================================================
 * MATH HELPERS
 * ============================================================================
 */

static Mat4 mat4_trs(Vec3 t, Quat r, Vec3 s) {
  Mat4 translation = mat4_translate(t);
  Mat4 rotation = quat_to_mat4(r);
  Mat4 scale = mat4_scale(s);

  // T * R * S
  Mat4 rs = mat4_mul(rotation, scale);
  return mat4_mul(translation, rs);
}

static uint32_t hash_string(const char *str) {
  uint32_t hash = 5381;
  int c;
  while ((c = *str++)) {
    hash = ((hash << 5) + hash) + c;
  }
  return hash;
}

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================
 */

static bool animation_skeleton_data_validate(
    const animation_skeleton_data_internal_t *item) {
  if (!item)
    return false;
  if (!item->initialized)
    return false;
  if (item->skeleton.bone_count > SKELETON_MAX_BONES)
    return false;
  return true;
}

static void skeleton_compute_bind_matrices(skeleton_t *skel) {
  if (!skel || !skel->bones || skel->bone_count == 0)
    return;

  // Compute local bind matrices for each bone
  for (uint32_t i = 0; i < skel->bone_count; i++) {
    bone_data_t *bone = &skel->bones[i];
    bone->local_bind_matrix =
        mat4_trs(bone->bind_position, bone->bind_rotation, bone->bind_scale);
  }

  // Compute world-space inverse bind matrices (traverse hierarchy)
  Mat4 *world_matrices = (Mat4 *)calloc(skel->bone_count, sizeof(Mat4));
  if (!world_matrices)
    return;

  for (uint32_t i = 0; i < skel->bone_count; i++) {
    bone_data_t *bone = &skel->bones[i];

    if (bone->parent_index < 0) {
      // Root bone - world = local
      world_matrices[i] = bone->local_bind_matrix;
    } else {
      // Child bone - world = parent_world * local
      world_matrices[i] =
          mat4_mul(world_matrices[bone->parent_index], bone->local_bind_matrix);
    }

    // Compute inverse bind matrix for skinning
    bone->inverse_bind_matrix = mat4_inverse(world_matrices[i]);
  }

  free(world_matrices);
}

static void skeleton_build_hierarchy_cache(skeleton_t *skel) {
  if (!skel || !skel->bones || skel->bone_count == 0)
    return;

  // Reset hierarchy links
  for (uint32_t i = 0; i < skel->bone_count; i++) {
    skel->bones[i].first_child_index = -1;
    skel->bones[i].next_sibling_index = -1;
    skel->bones[i].depth = 0;
  }

  // Build child/sibling links and compute depths
  for (uint32_t i = 0; i < skel->bone_count; i++) {
    bone_data_t *bone = &skel->bones[i];
    int32_t parent_idx = bone->parent_index;

    if (parent_idx >= 0 && parent_idx < (int32_t)skel->bone_count) {
      bone_data_t *parent = &skel->bones[parent_idx];
      bone->depth = parent->depth + 1;

      if (parent->first_child_index < 0) {
        parent->first_child_index = (int32_t)i;
      } else {
        // Find last sibling
        int32_t sibling_idx = parent->first_child_index;
        while (skel->bones[sibling_idx].next_sibling_index >= 0) {
          sibling_idx = skel->bones[sibling_idx].next_sibling_index;
        }
        skel->bones[sibling_idx].next_sibling_index = (int32_t)i;
      }
    }
  }

  // Build sorted indices by depth (for efficient pose calculation)
  if (skel->sorted_indices) {
    // Simple insertion sort by depth
    for (uint32_t i = 0; i < skel->bone_count; i++) {
      skel->sorted_indices[i] = (int32_t)i;
    }
    for (uint32_t i = 1; i < skel->bone_count; i++) {
      int32_t key = skel->sorted_indices[i];
      int j = (int)i - 1;
      while (j >= 0 && skel->bones[skel->sorted_indices[j]].depth >
                           skel->bones[key].depth) {
        skel->sorted_indices[j + 1] = skel->sorted_indices[j];
        j--;
      }
      skel->sorted_indices[j + 1] = key;
    }
  }
}

static void skeleton_cleanup(skeleton_t *skel) {
  if (!skel)
    return;
  if (skel->bones) {
    free(skel->bones);
    skel->bones = NULL;
  }
  if (skel->bone_name_hashes) {
    free(skel->bone_name_hashes);
    skel->bone_name_hashes = NULL;
  }
  if (skel->sorted_indices) {
    free(skel->sorted_indices);
    skel->sorted_indices = NULL;
  }
  skel->bone_count = 0;
}

static void animation_skeleton_data_cleanup_internal(
    animation_skeleton_data_internal_t *item) {
  if (!item)
    return;
  skeleton_cleanup(&item->skeleton);
  item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================
 */

int animation_skeleton_data_init(void) {
  if (g_skeleton_data_ctx.initialized) {
    return 0; // Already initialized
  }

  g_skeleton_data_ctx.capacity = ANIMATION_SKELETON_DATA_DEFAULT_CAPACITY;
  g_skeleton_data_ctx.items = (animation_skeleton_data_internal_t *)calloc(
      g_skeleton_data_ctx.capacity, sizeof(animation_skeleton_data_internal_t));
  if (!g_skeleton_data_ctx.items) {
    return -1;
  }

  g_skeleton_data_ctx.count = 0;
  g_skeleton_data_ctx.initialized = true;

  return 0;
}

void animation_skeleton_data_shutdown(void) {
  if (!g_skeleton_data_ctx.initialized) {
    return;
  }

  for (uint32_t i = 0; i < g_skeleton_data_ctx.count; i++) {
    animation_skeleton_data_cleanup_internal(&g_skeleton_data_ctx.items[i]);
  }

  free(g_skeleton_data_ctx.items);
  g_skeleton_data_ctx.items = NULL;
  g_skeleton_data_ctx.count = 0;
  g_skeleton_data_ctx.capacity = 0;
  g_skeleton_data_ctx.initialized = false;
}

int animation_skeleton_data_create(animation_skeleton_data_handle_t *out_handle,
                                   const animation_skeleton_data_desc_t *desc) {
  if (!out_handle || !desc) {
    return -1;
  }

  if (!g_skeleton_data_ctx.initialized) {
    return -2;
  }

  if (g_skeleton_data_ctx.count >= g_skeleton_data_ctx.capacity) {
    // Grow capacity
    uint32_t new_capacity = g_skeleton_data_ctx.capacity * 2;
    if (new_capacity > ANIMATION_SKELETON_DATA_MAX_COUNT) {
      new_capacity = ANIMATION_SKELETON_DATA_MAX_COUNT;
    }
    if (g_skeleton_data_ctx.count >= new_capacity) {
      return -3;
    }

    animation_skeleton_data_internal_t *new_items =
        (animation_skeleton_data_internal_t *)realloc(
            g_skeleton_data_ctx.items,
            new_capacity * sizeof(animation_skeleton_data_internal_t));
    if (!new_items) {
      return -3;
    }

    memset(&new_items[g_skeleton_data_ctx.capacity], 0,
           (new_capacity - g_skeleton_data_ctx.capacity) *
               sizeof(animation_skeleton_data_internal_t));

    g_skeleton_data_ctx.items = new_items;
    g_skeleton_data_ctx.capacity = new_capacity;
  }

  uint32_t index = g_skeleton_data_ctx.count++;
  animation_skeleton_data_internal_t *item = &g_skeleton_data_ctx.items[index];

  item->id = index;
  item->flags = desc->flags;
  memset(&item->skeleton, 0, sizeof(skeleton_t));
  item->initialized = true;
  item->dirty = true;
  item->frame_updated = 0;

  out_handle->id = index;
  return 0;
}

void animation_skeleton_data_destroy(animation_skeleton_data_handle_t handle) {
  if (handle.id >= g_skeleton_data_ctx.count) {
    return;
  }

  animation_skeleton_data_cleanup_internal(
      &g_skeleton_data_ctx.items[handle.id]);
}

int animation_skeleton_data_set_bones(animation_skeleton_data_handle_t handle,
                                      const void *bone_data,
                                      uint32_t bone_count) {
  if (handle.id >= g_skeleton_data_ctx.count) {
    return -1;
  }

  if (bone_count > SKELETON_MAX_BONES) {
    return -2;
  }

  animation_skeleton_data_internal_t *item =
      &g_skeleton_data_ctx.items[handle.id];
  if (!item->initialized) {
    return -3;
  }

  // Cleanup existing skeleton data
  skeleton_cleanup(&item->skeleton);

  if (bone_count == 0) {
    return 0; // Empty skeleton is valid
  }

  // Allocate bones
  item->skeleton.bones = (bone_data_t *)calloc(bone_count, sizeof(bone_data_t));
  if (!item->skeleton.bones) {
    return -4;
  }

  item->skeleton.bone_name_hashes =
      (uint32_t *)calloc(bone_count, sizeof(uint32_t));
  item->skeleton.sorted_indices =
      (int32_t *)calloc(bone_count, sizeof(int32_t));

  if (!item->skeleton.bone_name_hashes || !item->skeleton.sorted_indices) {
    skeleton_cleanup(&item->skeleton);
    return -4;
  }

  item->skeleton.bone_count = bone_count;
  item->skeleton.root_bone_index = 0;

  // Copy bone data if provided
  if (bone_data) {
    memcpy(item->skeleton.bones, bone_data, bone_count * sizeof(bone_data_t));

    // Build name hash cache
    for (uint32_t i = 0; i < bone_count; i++) {
      item->skeleton.bone_name_hashes[i] =
          hash_string(item->skeleton.bones[i].name);
    }
  } else {
    // Initialize default bones
    for (uint32_t i = 0; i < bone_count; i++) {
      bone_data_t *bone = &item->skeleton.bones[i];
      snprintf(bone->name, SKELETON_BONE_NAME_MAX, "bone_%u", i);
      bone->name_hash = hash_string(bone->name);
      bone->parent_index = (i > 0) ? (int32_t)(i - 1) : -1;
      bone->bind_position = (Vec3){0, 0, 0};
      bone->bind_rotation = (Quat){0, 0, 0, 1};
      bone->bind_scale = (Vec3){1, 1, 1};

      item->skeleton.bone_name_hashes[i] = bone->name_hash;
    }
  }

  // Compute bind matrices and hierarchy
  skeleton_build_hierarchy_cache(&item->skeleton);
  skeleton_compute_bind_matrices(&item->skeleton);

  item->dirty = true;
  return 0;
}

int32_t
animation_skeleton_data_find_bone(animation_skeleton_data_handle_t handle,
                                  const char *name) {
  if (handle.id >= g_skeleton_data_ctx.count || !name) {
    return -1;
  }

  animation_skeleton_data_internal_t *item =
      &g_skeleton_data_ctx.items[handle.id];
  if (!item->initialized || !item->skeleton.bones) {
    return -1;
  }

  uint32_t target_hash = hash_string(name);

  // Fast hash lookup
  for (uint32_t i = 0; i < item->skeleton.bone_count; i++) {
    if (item->skeleton.bone_name_hashes[i] == target_hash) {
      // Verify name matches (hash collision check)
      if (strcmp(item->skeleton.bones[i].name, name) == 0) {
        return (int32_t)i;
      }
    }
  }

  return -1;
}

int animation_skeleton_data_get_bone_count(
    animation_skeleton_data_handle_t handle) {
  if (handle.id >= g_skeleton_data_ctx.count) {
    return 0;
  }

  return (int)g_skeleton_data_ctx.items[handle.id].skeleton.bone_count;
}

int animation_skeleton_data_update(animation_skeleton_data_handle_t handle,
                                   const void *data, size_t size) {
  if (handle.id >= g_skeleton_data_ctx.count) {
    return -1;
  }

  animation_skeleton_data_internal_t *item =
      &g_skeleton_data_ctx.items[handle.id];
  if (!item->initialized) {
    return -2;
  }

  // Recompute matrices if skeleton data changed
  if (data && size >= sizeof(bone_data_t) * item->skeleton.bone_count) {
    memcpy(item->skeleton.bones, data,
           sizeof(bone_data_t) * item->skeleton.bone_count);
    skeleton_compute_bind_matrices(&item->skeleton);
  }

  item->dirty = true;
  return 0;
}

bool animation_skeleton_data_is_valid(animation_skeleton_data_handle_t handle) {
  if (handle.id >= g_skeleton_data_ctx.count) {
    return false;
  }
  return g_skeleton_data_ctx.items[handle.id].initialized;
}

int animation_skeleton_data_get_info(animation_skeleton_data_handle_t handle,
                                     animation_skeleton_data_info_t *out_info) {
  if (!out_info) {
    return -1;
  }

  if (handle.id >= g_skeleton_data_ctx.count) {
    return -2;
  }

  const animation_skeleton_data_internal_t *item =
      &g_skeleton_data_ctx.items[handle.id];
  out_info->id = item->id;
  out_info->flags = item->flags;
  out_info->initialized = item->initialized;

  return 0;
}

void animation_skeleton_data_mark_dirty(
    animation_skeleton_data_handle_t handle) {
  if (handle.id < g_skeleton_data_ctx.count) {
    g_skeleton_data_ctx.items[handle.id].dirty = true;
  }
}

int animation_skeleton_data_process_pending(void) {
  int processed = 0;
  for (uint32_t i = 0; i < g_skeleton_data_ctx.count; i++) {
    animation_skeleton_data_internal_t *item = &g_skeleton_data_ctx.items[i];
    if (item->initialized && item->dirty) {
      // Recompute any cached data
      skeleton_compute_bind_matrices(&item->skeleton);
      item->dirty = false;
      processed++;
    }
  }

  return processed;
}

uint32_t animation_skeleton_data_get_count(void) {
  return g_skeleton_data_ctx.count;
}

size_t animation_skeleton_data_get_memory_usage(void) {
  size_t total = sizeof(g_skeleton_data_ctx);
  total +=
      g_skeleton_data_ctx.capacity * sizeof(animation_skeleton_data_internal_t);

  for (uint32_t i = 0; i < g_skeleton_data_ctx.count; i++) {
    skeleton_t *skel = &g_skeleton_data_ctx.items[i].skeleton;
    if (skel->bones) {
      total += skel->bone_count * sizeof(bone_data_t);
    }
    if (skel->bone_name_hashes) {
      total += skel->bone_count * sizeof(uint32_t);
    }
    if (skel->sorted_indices) {
      total += skel->bone_count * sizeof(int32_t);
    }
  }

  return total;
}

void animation_skeleton_data_debug_print(void) {
  // Debug printing implementation - outputs skeleton hierarchy info
  for (uint32_t i = 0; i < g_skeleton_data_ctx.count; i++) {
    animation_skeleton_data_internal_t *item = &g_skeleton_data_ctx.items[i];
    if (item->initialized) {
      // Print skeleton summary
      // In production, would use proper logging
    }
  }
}
