/*
 * spring_bones.c
 * Spring constraint bones
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 *
 * Implementation TODOs:
 * TODO: Implement skeletal animation
 * TODO: Add animation blending
 * TODO: Implement IK solvers
 * TODO: Add morph target support
 * TODO: Implement GPU skinning
 * TODO: Add animation compression
 * TODO: Implement state machine
 * TODO: Add procedural animation
 * TODO: Implement ragdoll physics
 * TODO: Add animation retargeting
 * TODO: Implement spring bones initialization
 * TODO: Add spring bones cleanup/shutdown
 * TODO: Implement spring bones validation
 * TODO: Add spring bones error handling
 * TODO: Implement spring bones serialization
 * TODO: Add spring bones debug output
 * TODO: Implement spring bones unit tests
 * TODO: Add spring bones performance counters
 * TODO: Implement spring bones hot-reload
 * TODO: Add spring bones thread safety
 * TODO: Implement spring bones memory pooling
 * TODO: Add spring bones caching layer
 * TODO: Implement spring bones async operations
 * TODO: Add spring bones GPU integration
 * TODO: Implement spring bones SIMD optimization
 * TODO: Add spring bones batch processing
 * TODO: Implement spring bones streaming support
 * TODO: Add spring bones LOD support
 * TODO: Implement spring bones culling integration
 * TODO: Add spring bones render graph node
 */

#include "character/animation/physics_animation/spring_bones.h"
#include "math/vec3.h"
#include "math/mat4.h"
#include "math/quat.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define ANIMATION_SPRING_BONES_MAX_COUNT 4096
#define ANIMATION_SPRING_BONES_DEFAULT_CAPACITY 256
#define ANIMATION_SPRING_BONES_ALIGNMENT 16

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct {
    animation_spring_bone_settings_t settings;
    Vec3 current_tail;
    Vec3 prev_tail;
    bool valid;
} spring_bone_runtime_t;

typedef struct animation_spring_bones_internal {
    uint32_t id;
    uint32_t flags;
    spring_bone_runtime_t* bones;
    uint32_t bone_count;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} animation_spring_bones_internal_t;

typedef struct animation_spring_bones_context {
    animation_spring_bones_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
    // For free list optimization, we could add:
    // uint32_t* free_indices;
    // uint32_t free_count;
} animation_spring_bones_context_t;

static animation_spring_bones_context_t g_spring_bones_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void animation_spring_bones_cleanup_internal(animation_spring_bones_internal_t* item) {
    if (!item) return;
    if (item->bones) {
        free(item->bones);
        item->bones = NULL;
    }
    item->bone_count = 0;
    item->initialized = false;
    item->flags = 0;
}

// Helper to construct a rotation from two vectors
static Quat quat_from_to_rotation(Vec3 from, Vec3 to) {
    Vec3 axis = vec3_cross(from, to);
    float dot = vec3_dot(from, to);
    float len_from = vec3_length(from);
    float len_to = vec3_length(to);

    // Normalize logic baked in
    float denom = len_from * len_to;
    if (denom < 0.0001f) return quat_identity();

    dot /= denom;

    // Handle parallel cases
    if (dot > 0.9999f) {
        return quat_identity();
    }

    // Handle anti-parallel case
    if (dot < -0.9999f) {
        // Rotate 180 degrees around any orthogonal axis
        Vec3 ortho = vec3_orthogonal(from, vec3_create(1, 0, 0));
        if (vec3_length_sq(ortho) < 0.001f) {
            ortho = vec3_orthogonal(from, vec3_create(0, 1, 0));
        }
        ortho = vec3_normalize(ortho);
        return quat_from_axis_angle(ortho, 3.14159265f);
    }

    // Standard case
    float angle = acosf(dot);
    axis = vec3_normalize(axis);
    return quat_from_axis_angle(axis, angle);
}

static int find_free_slot(void) {
    for (uint32_t i = 0; i < g_spring_bones_ctx.count; i++) {
        if (!g_spring_bones_ctx.items[i].initialized) {
            return (int)i;
        }
    }
    return -1;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int animation_spring_bones_init(void) {
    if (g_spring_bones_ctx.initialized) {
        return 0; // Already initialized
    }

    g_spring_bones_ctx.capacity = ANIMATION_SPRING_BONES_DEFAULT_CAPACITY;
    g_spring_bones_ctx.items = calloc(g_spring_bones_ctx.capacity, sizeof(animation_spring_bones_internal_t));
    if (!g_spring_bones_ctx.items) {
        return -1;
    }

    g_spring_bones_ctx.count = 0;
    g_spring_bones_ctx.initialized = true;

    return 0;
}

void animation_spring_bones_shutdown(void) {
    if (!g_spring_bones_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_spring_bones_ctx.count; i++) {
        animation_spring_bones_cleanup_internal(&g_spring_bones_ctx.items[i]);
    }

    free(g_spring_bones_ctx.items);
    g_spring_bones_ctx.items = NULL;
    g_spring_bones_ctx.count = 0;
    g_spring_bones_ctx.capacity = 0;
    g_spring_bones_ctx.initialized = false;
}

int animation_spring_bones_create(animation_spring_bones_handle_t* out_handle, const animation_spring_bones_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_spring_bones_ctx.initialized) {
        return -2;
    }

    // Try to reuse a slot
    int free_idx = find_free_slot();
    uint32_t index;

    if (free_idx >= 0) {
        index = (uint32_t)free_idx;
    } else {
        if (g_spring_bones_ctx.count >= g_spring_bones_ctx.capacity) {
            // Expand capacity
            uint32_t new_capacity = g_spring_bones_ctx.capacity * 2;
            animation_spring_bones_internal_t* new_items = realloc(g_spring_bones_ctx.items, new_capacity * sizeof(animation_spring_bones_internal_t));
            if (!new_items) {
                return -3;
            }
            g_spring_bones_ctx.items = new_items;
            // Zero out new memory
            memset(g_spring_bones_ctx.items + g_spring_bones_ctx.capacity, 0, (new_capacity - g_spring_bones_ctx.capacity) * sizeof(animation_spring_bones_internal_t));
            g_spring_bones_ctx.capacity = new_capacity;
        }
        index = g_spring_bones_ctx.count++;
    }

    animation_spring_bones_internal_t* item = &g_spring_bones_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->bone_count = desc->bone_count;

    if (desc->bone_count > 0 && desc->bones) {
        item->bones = calloc(desc->bone_count, sizeof(spring_bone_runtime_t));
        if (!item->bones) {
            // Rollback if we just added it
            if (free_idx < 0) g_spring_bones_ctx.count--;
            return -4;
        }

        for (uint32_t i = 0; i < desc->bone_count; i++) {
            item->bones[i].settings = desc->bones[i];
            item->bones[i].valid = false; // Will be initialized on first update
        }
    } else {
        item->bones = NULL;
    }

    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void animation_spring_bones_destroy(animation_spring_bones_handle_t handle) {
    if (handle.id >= g_spring_bones_ctx.count) {
        return;
    }

    // We cleanup but keep the context count same, just marking slot as uninitialized
    // so it can be reused.
    animation_spring_bones_cleanup_internal(&g_spring_bones_ctx.items[handle.id]);
}

int animation_spring_bones_update(animation_spring_bones_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_spring_bones_ctx.count) {
        return -1;
    }

    if (size != sizeof(animation_spring_bones_update_data_t)) {
        return -4;
    }

    animation_spring_bones_internal_t* item = &g_spring_bones_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    const animation_spring_bones_update_data_t* update_data = (const animation_spring_bones_update_data_t*)data;
    float dt = update_data->delta_time;
    if (dt <= 0.0001f) return 0; // Skip small steps

    const Mat4* bone_transforms = update_data->input_pose;
    if (!bone_transforms) {
        return -3;
    }

    // Physics Simulation (Verlet Integration)
    for (uint32_t i = 0; i < item->bone_count; i++) {
        spring_bone_runtime_t* bone = &item->bones[i];

        Mat4 bone_mat = bone_transforms[bone->settings.bone_index];

        // Extract translation (bone head position)
        Vec3 head_pos = vec3_create(bone_mat.m30, bone_mat.m31, bone_mat.m32);

        // Use user-defined local offset
        Vec3 target_tail_local = bone->settings.local_tail_offset;
        float bone_length = vec3_length(target_tail_local);
        if (bone_length < 0.0001f) {
            bone_length = 0.0001f; // Prevent division by zero
        }

        // Transform target tail to world space (using 3x3 rotation part of matrix)
        Vec3 target_tail_world;
        target_tail_world.x = bone_mat.m00 * target_tail_local.x + bone_mat.m10 * target_tail_local.y + bone_mat.m20 * target_tail_local.z + bone_mat.m30;
        target_tail_world.y = bone_mat.m01 * target_tail_local.x + bone_mat.m11 * target_tail_local.y + bone_mat.m21 * target_tail_local.z + bone_mat.m31;
        target_tail_world.z = bone_mat.m02 * target_tail_local.x + bone_mat.m12 * target_tail_local.y + bone_mat.m22 * target_tail_local.z + bone_mat.m32;

        if (!bone->valid) {
            // First frame initialization
            bone->current_tail = target_tail_world;
            bone->prev_tail = target_tail_world;
            bone->valid = true;
            continue;
        }

        // Apply Forces
        Vec3 force = vec3_zero();

        // 1. Spring Force (Hooks Law towards target)
        Vec3 displacement = vec3_sub(target_tail_world, bone->current_tail);
        Vec3 spring_force = vec3_mul(displacement, bone->settings.stiffness);
        force = vec3_add(force, spring_force);

        // 2. Gravity
        Vec3 gravity = vec3_mul(bone->settings.gravity_dir, bone->settings.gravity_power);
        force = vec3_add(force, gravity);

        // Verlet Integration
        Vec3 velocity = vec3_sub(bone->current_tail, bone->prev_tail);
        // Apply Drag
        velocity = vec3_mul(velocity, (1.0f - bone->settings.drag));

        Vec3 next_tail = vec3_add(bone->current_tail, velocity);
        next_tail = vec3_add(next_tail, vec3_mul(force, dt * dt));

        // Length Constraint (Keep tail at fixed distance from head)
        Vec3 direction = vec3_sub(next_tail, head_pos);
        float current_dist = vec3_length(direction);
        if (current_dist > 0.0001f) {
            direction = vec3_div(direction, current_dist);
        } else {
            direction = vec3_create(0, 1, 0); // Fallback
        }
        next_tail = vec3_add(head_pos, vec3_mul(direction, bone_length));

        // Update state
        bone->prev_tail = bone->current_tail;
        bone->current_tail = next_tail;
    }

    item->dirty = false;
    item->frame_updated++;

    return 0;
}

int animation_spring_bones_apply(animation_spring_bones_handle_t handle, Mat4* output_pose, uint32_t bone_count) {
    if (handle.id >= g_spring_bones_ctx.count) {
        return -1;
    }

    animation_spring_bones_internal_t* item = &g_spring_bones_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    if (!output_pose) {
        return -3;
    }

    for (uint32_t i = 0; i < item->bone_count; i++) {
        spring_bone_runtime_t* bone = &item->bones[i];

        if (!bone->valid) continue;

        uint32_t bone_idx = bone->settings.bone_index;
        if (bone_idx >= bone_count) continue; // Boundary check

        Mat4* bone_mat = &output_pose[bone_idx];

        Vec3 head_pos = vec3_create(bone_mat->m30, bone_mat->m31, bone_mat->m32);

        // Current direction (from head to simulated tail)
        Vec3 current_dir = vec3_sub(bone->current_tail, head_pos);
        current_dir = vec3_normalize(current_dir);

        // Target direction (from head to original/animated tail)
        // We need to re-calculate where the tail would be without simulation to compute the rotation delta

        Vec3 local_offset = bone->settings.local_tail_offset;

        Vec3 animated_dir;
        animated_dir.x = bone_mat->m00 * local_offset.x + bone_mat->m10 * local_offset.y + bone_mat->m20 * local_offset.z;
        animated_dir.y = bone_mat->m01 * local_offset.x + bone_mat->m11 * local_offset.y + bone_mat->m21 * local_offset.z;
        animated_dir.z = bone_mat->m02 * local_offset.x + bone_mat->m12 * local_offset.y + bone_mat->m22 * local_offset.z;

        animated_dir = vec3_normalize(animated_dir);

        // Calculate shortest arc rotation to align animated_dir to current_dir
        Quat delta_rotation = quat_from_to_rotation(animated_dir, current_dir);

        Mat4 delta_mat = quat_to_mat4(delta_rotation);

        // We want to rotate around the head position.

        Mat4 final_mat = mat4_mul(delta_mat, *bone_mat);

        // Restore position (it might have drifted due to precision or if rotation wasn't pure)
        final_mat.m30 = head_pos.x;
        final_mat.m31 = head_pos.y;
        final_mat.m32 = head_pos.z;

        *bone_mat = final_mat;
    }

    return 0;
}

bool animation_spring_bones_is_valid(animation_spring_bones_handle_t handle) {
    if (handle.id >= g_spring_bones_ctx.count) {
        return false;
    }
    return g_spring_bones_ctx.items[handle.id].initialized;
}

int animation_spring_bones_get_info(animation_spring_bones_handle_t handle, animation_spring_bones_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_spring_bones_ctx.count) {
        return -2;
    }

    const animation_spring_bones_internal_t* item = &g_spring_bones_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    out_info->bone_count = item->bone_count;

    return 0;
}

void animation_spring_bones_mark_dirty(animation_spring_bones_handle_t handle) {
    if (handle.id < g_spring_bones_ctx.count) {
        g_spring_bones_ctx.items[handle.id].dirty = true;
    }
}

int animation_spring_bones_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_spring_bones_ctx.count; i++) {
        animation_spring_bones_internal_t* item = &g_spring_bones_ctx.items[i];
        if (item->initialized && item->dirty) {
            processed++;
        }
    }

    return processed;
}

uint32_t animation_spring_bones_get_count(void) {
    return g_spring_bones_ctx.count;
}

size_t animation_spring_bones_get_memory_usage(void) {
    size_t total = sizeof(g_spring_bones_ctx);
    total += g_spring_bones_ctx.capacity * sizeof(animation_spring_bones_internal_t);

    for (uint32_t i = 0; i < g_spring_bones_ctx.count; i++) {
        if (g_spring_bones_ctx.items[i].initialized) {
             total += g_spring_bones_ctx.items[i].bone_count * sizeof(spring_bone_runtime_t);
        }
    }

    return total;
}

void animation_spring_bones_debug_print(void) {
    // Debug printing implementation placeholder
}
