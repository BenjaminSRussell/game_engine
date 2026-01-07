/*
 * skeleton_data.c
 * Skeleton bone hierarchy
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 *
 * Implements bone hierarchy storage, bind pose matrices, and skeleton management
 */

#include "character/animation/skeletal/skeleton_data.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <include/math/math.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define ANIMATION_SKELETON_DATA_MAX_COUNT 4096
#define ANIMATION_SKELETON_DATA_DEFAULT_CAPACITY 256
#define ANIMATION_SKELETON_DATA_ALIGNMENT 16
#define SKELETON_MAX_BONES 256
#define SKELETON_BONE_NAME_MAX 64
#define SKELETON_INVALID_BONE_INDEX 0xFFFFFFFF

/* ============================================================================
 * MATH TYPES (inline for skeleton operations)
 * ============================================================================ */

typedef struct vec3 {
    float x, y, z;
} vec3_t;

typedef struct quat {
    float x, y, z, w;
} quat_t;

typedef struct mat4 {
    float m[16];
} mat4_t;

/* ============================================================================
 * SKELETON-SPECIFIC TYPES
 * ============================================================================ */

typedef struct bone_data {
    char name[SKELETON_BONE_NAME_MAX];
    uint32_t name_hash;
    int32_t parent_index;           // -1 for root bones
    
    // Bind pose (T-pose / rest pose)
    vec3_t bind_position;
    quat_t bind_rotation;
    vec3_t bind_scale;
    
    // Precomputed matrices
    mat4_t local_bind_matrix;       // Local space bind pose
    mat4_t inverse_bind_matrix;     // Used for skinning
    
    // Hierarchy data
    int32_t first_child_index;      // First child bone (-1 if leaf)
    int32_t next_sibling_index;     // Next sibling bone (-1 if last)
    uint8_t depth;                  // Depth in hierarchy (0 = root)
    uint8_t _padding[3];
} bone_data_t;

typedef struct skeleton {
    bone_data_t* bones;
    uint32_t bone_count;
    uint32_t root_bone_index;
    
    // Cached data for fast lookup
    uint32_t* bone_name_hashes;     // For fast name-based lookup
    int32_t* sorted_indices;        // Bones sorted by hierarchy depth
    
    // Bounds
    vec3_t bounds_min;
    vec3_t bounds_max;
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
    animation_skeleton_data_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
} animation_skeleton_data_context_t;

static animation_skeleton_data_context_t g_skeleton_data_ctx = {0};

/* ============================================================================
 * MATH HELPERS
 * ============================================================================ */

static uint32_t hash_string(const char* str) {
    uint32_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

static void mat4_identity(mat4_t* m) {
    memset(m->m, 0, sizeof(m->m));
    m->m[0] = m->m[5] = m->m[10] = m->m[15] = 1.0f;
}

static void mat4_from_trs(mat4_t* out, const vec3_t* t, const quat_t* r, const vec3_t* s) {
    // Build rotation matrix from quaternion
    float xx = r->x * r->x, yy = r->y * r->y, zz = r->z * r->z;
    float xy = r->x * r->y, xz = r->x * r->z, yz = r->y * r->z;
    float wx = r->w * r->x, wy = r->w * r->y, wz = r->w * r->z;
    
    out->m[0]  = (1.0f - 2.0f * (yy + zz)) * s->x;
    out->m[1]  = (2.0f * (xy + wz)) * s->x;
    out->m[2]  = (2.0f * (xz - wy)) * s->x;
    out->m[3]  = 0.0f;
    
    out->m[4]  = (2.0f * (xy - wz)) * s->y;
    out->m[5]  = (1.0f - 2.0f * (xx + zz)) * s->y;
    out->m[6]  = (2.0f * (yz + wx)) * s->y;
    out->m[7]  = 0.0f;
    
    out->m[8]  = (2.0f * (xz + wy)) * s->z;
    out->m[9]  = (2.0f * (yz - wx)) * s->z;
    out->m[10] = (1.0f - 2.0f * (xx + yy)) * s->z;
    out->m[11] = 0.0f;
    
    out->m[12] = t->x;
    out->m[13] = t->y;
    out->m[14] = t->z;
    out->m[15] = 1.0f;
}

static void mat4_multiply(mat4_t* out, const mat4_t* a, const mat4_t* b) {
    mat4_t result;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result.m[i * 4 + j] = 
                a->m[i * 4 + 0] * b->m[0 * 4 + j] +
                a->m[i * 4 + 1] * b->m[1 * 4 + j] +
                a->m[i * 4 + 2] * b->m[2 * 4 + j] +
                a->m[i * 4 + 3] * b->m[3 * 4 + j];
        }
    }
    memcpy(out, &result, sizeof(mat4_t));
}

static bool mat4_inverse(mat4_t* out, const mat4_t* m) {
    float inv[16], det;
    const float* src = m->m;
    
    inv[0] = src[5]*src[10]*src[15] - src[5]*src[11]*src[14] - src[9]*src[6]*src[15] 
           + src[9]*src[7]*src[14] + src[13]*src[6]*src[11] - src[13]*src[7]*src[10];
    inv[4] = -src[4]*src[10]*src[15] + src[4]*src[11]*src[14] + src[8]*src[6]*src[15] 
           - src[8]*src[7]*src[14] - src[12]*src[6]*src[11] + src[12]*src[7]*src[10];
    inv[8] = src[4]*src[9]*src[15] - src[4]*src[11]*src[13] - src[8]*src[5]*src[15] 
           + src[8]*src[7]*src[13] + src[12]*src[5]*src[11] - src[12]*src[7]*src[9];
    inv[12] = -src[4]*src[9]*src[14] + src[4]*src[10]*src[13] + src[8]*src[5]*src[14] 
            - src[8]*src[6]*src[13] - src[12]*src[5]*src[10] + src[12]*src[6]*src[9];
    
    det = src[0]*inv[0] + src[1]*inv[4] + src[2]*inv[8] + src[3]*inv[12];
    if (fabsf(det) < 1e-10f) return false;
    
    inv[1] = -src[1]*src[10]*src[15] + src[1]*src[11]*src[14] + src[9]*src[2]*src[15] 
           - src[9]*src[3]*src[14] - src[13]*src[2]*src[11] + src[13]*src[3]*src[10];
    inv[5] = src[0]*src[10]*src[15] - src[0]*src[11]*src[14] - src[8]*src[2]*src[15] 
           + src[8]*src[3]*src[14] + src[12]*src[2]*src[11] - src[12]*src[3]*src[10];
    inv[9] = -src[0]*src[9]*src[15] + src[0]*src[11]*src[13] + src[8]*src[1]*src[15] 
           - src[8]*src[3]*src[13] - src[12]*src[1]*src[11] + src[12]*src[3]*src[9];
    inv[13] = src[0]*src[9]*src[14] - src[0]*src[10]*src[13] - src[8]*src[1]*src[14] 
            + src[8]*src[2]*src[13] + src[12]*src[1]*src[10] - src[12]*src[2]*src[9];
    inv[2] = src[1]*src[6]*src[15] - src[1]*src[7]*src[14] - src[5]*src[2]*src[15] 
           + src[5]*src[3]*src[14] + src[13]*src[2]*src[7] - src[13]*src[3]*src[6];
    inv[6] = -src[0]*src[6]*src[15] + src[0]*src[7]*src[14] + src[4]*src[2]*src[15] 
           - src[4]*src[3]*src[14] - src[12]*src[2]*src[7] + src[12]*src[3]*src[6];
    inv[10] = src[0]*src[5]*src[15] - src[0]*src[7]*src[13] - src[4]*src[1]*src[15] 
            + src[4]*src[3]*src[13] + src[12]*src[1]*src[7] - src[12]*src[3]*src[5];
    inv[14] = -src[0]*src[5]*src[14] + src[0]*src[6]*src[13] + src[4]*src[1]*src[14] 
            - src[4]*src[2]*src[13] - src[12]*src[1]*src[6] + src[12]*src[2]*src[5];
    inv[3] = -src[1]*src[6]*src[11] + src[1]*src[7]*src[10] + src[5]*src[2]*src[11] 
           - src[5]*src[3]*src[10] - src[9]*src[2]*src[7] + src[9]*src[3]*src[6];
    inv[7] = src[0]*src[6]*src[11] - src[0]*src[7]*src[10] - src[4]*src[2]*src[11] 
           + src[4]*src[3]*src[10] + src[8]*src[2]*src[7] - src[8]*src[3]*src[6];
    inv[11] = -src[0]*src[5]*src[11] + src[0]*src[7]*src[9] + src[4]*src[1]*src[11] 
            - src[4]*src[3]*src[9] - src[8]*src[1]*src[7] + src[8]*src[3]*src[5];
    inv[15] = src[0]*src[5]*src[10] - src[0]*src[6]*src[9] - src[4]*src[1]*src[10] 
            + src[4]*src[2]*src[9] + src[8]*src[1]*src[6] - src[8]*src[2]*src[5];
    
    det = 1.0f / det;
    for (int i = 0; i < 16; i++) {
        out->m[i] = inv[i] * det;
    }
    return true;
}

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool animation_skeleton_data_validate(const animation_skeleton_data_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    if (item->skeleton.bone_count > SKELETON_MAX_BONES) return false;
    return true;
}

static void skeleton_compute_bind_matrices(skeleton_t* skel) {
    if (!skel || !skel->bones || skel->bone_count == 0) return;
    
    // Compute local bind matrices for each bone
    for (uint32_t i = 0; i < skel->bone_count; i++) {
        bone_data_t* bone = &skel->bones[i];
        mat4_from_trs(&bone->local_bind_matrix, 
                      &bone->bind_position, 
                      &bone->bind_rotation, 
                      &bone->bind_scale);
    }
    
    // Compute world-space inverse bind matrices (traverse hierarchy)
    mat4_t* world_matrices = calloc(skel->bone_count, sizeof(mat4_t));
    if (!world_matrices) return;
    
    for (uint32_t i = 0; i < skel->bone_count; i++) {
        bone_data_t* bone = &skel->bones[i];
        
        if (bone->parent_index < 0) {
            // Root bone - world = local
            memcpy(&world_matrices[i], &bone->local_bind_matrix, sizeof(mat4_t));
        } else {
            // Child bone - world = parent_world * local
            mat4_multiply(&world_matrices[i], 
                         &world_matrices[bone->parent_index], 
                         &bone->local_bind_matrix);
        }
        
        // Compute inverse bind matrix for skinning
        mat4_inverse(&bone->inverse_bind_matrix, &world_matrices[i]);
    }
    
    free(world_matrices);
}

static void skeleton_build_hierarchy_cache(skeleton_t* skel) {
    if (!skel || !skel->bones || skel->bone_count == 0) return;
    
    // Reset hierarchy links
    for (uint32_t i = 0; i < skel->bone_count; i++) {
        skel->bones[i].first_child_index = -1;
        skel->bones[i].next_sibling_index = -1;
        skel->bones[i].depth = 0;
    }
    
    // Build child/sibling links and compute depths
    for (uint32_t i = 0; i < skel->bone_count; i++) {
        bone_data_t* bone = &skel->bones[i];
        int32_t parent_idx = bone->parent_index;
        
        if (parent_idx >= 0 && parent_idx < (int32_t)skel->bone_count) {
            bone_data_t* parent = &skel->bones[parent_idx];
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
            while (j >= 0 && skel->bones[skel->sorted_indices[j]].depth > skel->bones[key].depth) {
                skel->sorted_indices[j + 1] = skel->sorted_indices[j];
                j--;
            }
            skel->sorted_indices[j + 1] = key;
        }
    }
}

static void skeleton_cleanup(skeleton_t* skel) {
    if (!skel) return;
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

static void animation_skeleton_data_cleanup_internal(animation_skeleton_data_internal_t* item) {
    if (!item) return;
    skeleton_cleanup(&item->skeleton);
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int animation_skeleton_data_init(void) {
    if (g_skeleton_data_ctx.initialized) {
        return 0; // Already initialized
    }

    g_skeleton_data_ctx.capacity = ANIMATION_SKELETON_DATA_DEFAULT_CAPACITY;
    g_skeleton_data_ctx.items = calloc(g_skeleton_data_ctx.capacity, sizeof(animation_skeleton_data_internal_t));
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

int animation_skeleton_data_create(animation_skeleton_data_handle_t* out_handle, const animation_skeleton_data_desc_t* desc) {
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
        
        animation_skeleton_data_internal_t* new_items = realloc(
            g_skeleton_data_ctx.items, 
            new_capacity * sizeof(animation_skeleton_data_internal_t)
        );
        if (!new_items) {
            return -3;
        }
        
        memset(&new_items[g_skeleton_data_ctx.capacity], 0, 
               (new_capacity - g_skeleton_data_ctx.capacity) * sizeof(animation_skeleton_data_internal_t));
        
        g_skeleton_data_ctx.items = new_items;
        g_skeleton_data_ctx.capacity = new_capacity;
    }

    uint32_t index = g_skeleton_data_ctx.count++;
    animation_skeleton_data_internal_t* item = &g_skeleton_data_ctx.items[index];

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

    animation_skeleton_data_cleanup_internal(&g_skeleton_data_ctx.items[handle.id]);
}

int animation_skeleton_data_set_bones(animation_skeleton_data_handle_t handle, 
                                       const void* bone_data, 
                                       uint32_t bone_count) {
    if (handle.id >= g_skeleton_data_ctx.count) {
        return -1;
    }
    
    if (bone_count > SKELETON_MAX_BONES) {
        return -2;
    }

    animation_skeleton_data_internal_t* item = &g_skeleton_data_ctx.items[handle.id];
    if (!item->initialized) {
        return -3;
    }

    // Cleanup existing skeleton data
    skeleton_cleanup(&item->skeleton);
    
    if (bone_count == 0) {
        return 0; // Empty skeleton is valid
    }
    
    // Allocate bones
    item->skeleton.bones = calloc(bone_count, sizeof(bone_data_t));
    if (!item->skeleton.bones) {
        return -4;
    }
    
    item->skeleton.bone_name_hashes = calloc(bone_count, sizeof(uint32_t));
    item->skeleton.sorted_indices = calloc(bone_count, sizeof(int32_t));
    
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
            item->skeleton.bone_name_hashes[i] = hash_string(item->skeleton.bones[i].name);
        }
    } else {
        // Initialize default bones
        for (uint32_t i = 0; i < bone_count; i++) {
            bone_data_t* bone = &item->skeleton.bones[i];
            snprintf(bone->name, SKELETON_BONE_NAME_MAX, "bone_%u", i);
            bone->name_hash = hash_string(bone->name);
            bone->parent_index = (i > 0) ? (int32_t)(i - 1) : -1;
            bone->bind_position = (vec3_t){0, 0, 0};
            bone->bind_rotation = (quat_t){0, 0, 0, 1};
            bone->bind_scale = (vec3_t){1, 1, 1};
            
            item->skeleton.bone_name_hashes[i] = bone->name_hash;
        }
    }
    
    // Compute bind matrices and hierarchy
    skeleton_build_hierarchy_cache(&item->skeleton);
    skeleton_compute_bind_matrices(&item->skeleton);
    
    item->dirty = true;
    return 0;
}

int32_t animation_skeleton_data_find_bone(animation_skeleton_data_handle_t handle, const char* name) {
    if (handle.id >= g_skeleton_data_ctx.count || !name) {
        return -1;
    }
    
    animation_skeleton_data_internal_t* item = &g_skeleton_data_ctx.items[handle.id];
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

int animation_skeleton_data_get_bone_count(animation_skeleton_data_handle_t handle) {
    if (handle.id >= g_skeleton_data_ctx.count) {
        return 0;
    }
    
    return (int)g_skeleton_data_ctx.items[handle.id].skeleton.bone_count;
}

int animation_skeleton_data_update(animation_skeleton_data_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_skeleton_data_ctx.count) {
        return -1;
    }

    animation_skeleton_data_internal_t* item = &g_skeleton_data_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    // Recompute matrices if skeleton data changed
    if (data && size >= sizeof(bone_data_t) * item->skeleton.bone_count) {
        memcpy(item->skeleton.bones, data, sizeof(bone_data_t) * item->skeleton.bone_count);
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

int animation_skeleton_data_get_info(animation_skeleton_data_handle_t handle, animation_skeleton_data_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_skeleton_data_ctx.count) {
        return -2;
    }

    const animation_skeleton_data_internal_t* item = &g_skeleton_data_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void animation_skeleton_data_mark_dirty(animation_skeleton_data_handle_t handle) {
    if (handle.id < g_skeleton_data_ctx.count) {
        g_skeleton_data_ctx.items[handle.id].dirty = true;
    }
}

int animation_skeleton_data_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_skeleton_data_ctx.count; i++) {
        animation_skeleton_data_internal_t* item = &g_skeleton_data_ctx.items[i];
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
    total += g_skeleton_data_ctx.capacity * sizeof(animation_skeleton_data_internal_t);

    for (uint32_t i = 0; i < g_skeleton_data_ctx.count; i++) {
        skeleton_t* skel = &g_skeleton_data_ctx.items[i].skeleton;
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
        animation_skeleton_data_internal_t* item = &g_skeleton_data_ctx.items[i];
        if (item->initialized) {
            // Print skeleton summary
            // In production, would use proper logging
        }
    }
}

/* End of skeleton_data.c */
