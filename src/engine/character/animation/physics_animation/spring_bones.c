/*
 * spring_bones.c
 * Spring constraint bones
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 */

#include "character/animation/physics_animation/spring_bones.h"
#include "math/vec3.h"
#include "math/mat4.h"
#include "math/quat.h"
#include <animation/skeleton_system.h>
#include <math/math.h>
#include <math/vec3.h>
#include <math/quat.h>
#include <core/types.h>
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
#define SPRING_BONE_DT_SUBSTEPS 1

/* ============================================================================
 * INTERNAL TYPES
 * ============================================================================ */

<<<<<<< HEAD
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
    SpringBoneInstance instance;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;

    // Skeleton reference
    Skeleton* skeleton;

    // Configuration & State (SoA for SIMD/Batching)
    uint32_t count;
    uint32_t* bone_indices;      // Index in skeleton
    float* bone_lengths;         // Rest length of the bone

    Vec3* current_tails;         // World space tail position
    Vec3* prev_tails;            // Previous world space tail position
    Vec3* bone_axes;             // Local axis that points to tail (usually +Y or +Z)

    // Physics properties
    Vec3* gravities;             // Gravity vector (dir * power)
    float* stiffnesss;           // Stiffness
    float* drags;                // Drag
    float* radii;                // Collision radius

    // Caching / Optimization
    Quat* cached_rotations;      // Last computed local rotations
    Mat4* parent_matrices;       // Cached parent world matrices

    // GPU Data
    animation_spring_bones_gpu_data_t* gpu_buffer;

    // Settings
    uint32_t lod_level;
    bool culling_enabled;
    bool is_visible;

} SpringBoneInstance;

typedef struct animation_spring_bones_context {
    SpringBoneInstance* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
    
    // For free list optimization, we could add:
    // uint32_t* free_indices;
    // uint32_t free_count;

    // Scratch buffers for batch processing
    Vec3* scratch_vec_a;
    Vec3* scratch_vec_b;
    Vec3* scratch_vec_c;
} animation_spring_bones_context_t;

static animation_spring_bones_context_t g_spring_bones_ctx = {0};

/* ============================================================================
 * HELPER FUNCTIONS
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
    free_instance_data(&item->instance);
}

static void* aligned_alloc_wrapper(size_t size) {
    // Simple wrapper, in a real engine use a proper aligned allocator
    void* ptr = malloc(size);
    // memset(ptr, 0, size); // Optional
    return ptr;
}

static void ensure_scratch_buffers(uint32_t count) {
    static uint32_t current_size = 0;
    if (count > current_size) {
        current_size = (count + 255) & ~255; // Align to 256
        g_spring_bones_ctx.scratch_vec_a = realloc(g_spring_bones_ctx.scratch_vec_a, current_size * sizeof(Vec3));
        g_spring_bones_ctx.scratch_vec_b = realloc(g_spring_bones_ctx.scratch_vec_b, current_size * sizeof(Vec3));
        g_spring_bones_ctx.scratch_vec_c = realloc(g_spring_bones_ctx.scratch_vec_c, current_size * sizeof(Vec3));
    }
}

static void free_instance_data(SpringBoneInstance* inst) {
    if (inst->bone_indices) free(inst->bone_indices);
    if (inst->bone_lengths) free(inst->bone_lengths);
    if (inst->current_tails) free(inst->current_tails);
    if (inst->prev_tails) free(inst->prev_tails);
    if (inst->bone_axes) free(inst->bone_axes);
    if (inst->gravities) free(inst->gravities);
    if (inst->stiffnesss) free(inst->stiffnesss);
    if (inst->drags) free(inst->drags);
    if (inst->radii) free(inst->radii);
    if (inst->cached_rotations) free(inst->cached_rotations);
    if (inst->parent_matrices) free(inst->parent_matrices);
    if (inst->gpu_buffer) free(inst->gpu_buffer);

    memset(inst, 0, sizeof(SpringBoneInstance));
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
=======
static void* aligned_alloc_wrapper(size_t size) {
    // Simple wrapper, in a real engine use a proper aligned allocator
    void* ptr = malloc(size);
    // memset(ptr, 0, size); // Optional
    return ptr;
}

static void ensure_scratch_buffers(uint32_t count) {
    static uint32_t current_size = 0;
    if (count > current_size) {
        current_size = (count + 255) & ~255; // Align to 256
        g_spring_bones_ctx.scratch_vec_a = realloc(g_spring_bones_ctx.scratch_vec_a, current_size * sizeof(Vec3));
        g_spring_bones_ctx.scratch_vec_b = realloc(g_spring_bones_ctx.scratch_vec_b, current_size * sizeof(Vec3));
        g_spring_bones_ctx.scratch_vec_c = realloc(g_spring_bones_ctx.scratch_vec_c, current_size * sizeof(Vec3));
    }
>>>>>>> origin/spring-bones-implementation-10769064037362822729
}

static void free_instance_data(SpringBoneInstance* inst) {
    if (inst->bone_indices) free(inst->bone_indices);
    if (inst->bone_lengths) free(inst->bone_lengths);
    if (inst->current_tails) free(inst->current_tails);
    if (inst->prev_tails) free(inst->prev_tails);
    if (inst->bone_axes) free(inst->bone_axes);
    if (inst->gravities) free(inst->gravities);
    if (inst->stiffnesss) free(inst->stiffnesss);
    if (inst->drags) free(inst->drags);
    if (inst->radii) free(inst->radii);
    if (inst->cached_rotations) free(inst->cached_rotations);
    if (inst->parent_matrices) free(inst->parent_matrices);
    if (inst->gpu_buffer) free(inst->gpu_buffer);

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
 * ============================================================================ */

int animation_spring_bones_init(void) {
int animation_spring_bones_init(void) {
    if (g_spring_bones_ctx.initialized) return 0;

    g_spring_bones_ctx.capacity = ANIMATION_SPRING_BONES_DEFAULT_CAPACITY;
    g_spring_bones_ctx.items = calloc(g_spring_bones_ctx.capacity, sizeof(SpringBoneInstance));
    if (!g_spring_bones_ctx.items) return -1;

    g_spring_bones_ctx.count = 0;
    g_spring_bones_ctx.initialized = true;

    // Pre-allocate some scratch memory
    ensure_scratch_buffers(256);

    return 0;
}

void animation_spring_bones_shutdown(void) {
void animation_spring_bones_shutdown(void) {
    if (!g_spring_bones_ctx.initialized) return;

    for (uint32_t i = 0; i < g_spring_bones_ctx.count; i++) {
        free_instance_data(&g_spring_bones_ctx.items[i]);
    }

    free(g_spring_bones_ctx.items);
    if (g_spring_bones_ctx.scratch_vec_a) free(g_spring_bones_ctx.scratch_vec_a);
    if (g_spring_bones_ctx.scratch_vec_b) free(g_spring_bones_ctx.scratch_vec_b);
    if (g_spring_bones_ctx.scratch_vec_c) free(g_spring_bones_ctx.scratch_vec_c);

    memset(&g_spring_bones_ctx, 0, sizeof(g_spring_bones_ctx));
}

int animation_spring_bones_create(animation_spring_bones_handle_t* out_handle, const animation_spring_bones_desc_t* desc) {
<<<<<<< HEAD
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
=======
    if (!out_handle || !desc || !desc->skeleton || desc->bone_count == 0) return -1;
    if (!g_spring_bones_ctx.initialized) return -2;

    if (g_spring_bones_ctx.count >= g_spring_bones_ctx.capacity) {
        // Simple expansion logic could go here
        return -3;
    }

    uint32_t id = g_spring_bones_ctx.count++;
    SpringBoneInstance* inst = &g_spring_bones_ctx.items[id];

    inst->id = id;
    inst->flags = desc->flags;
    inst->skeleton = (Skeleton*)desc->skeleton;
    inst->count = desc->bone_count;
    inst->initialized = true;
    inst->is_visible = true; // Default visible
>>>>>>> origin/spring-bones-implementation-10769064037362822729

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
    Skeleton* skel = inst->skeleton;
    for (uint32_t i = 0; i < count; i++) {
        const animation_spring_bone_config_t* cfg = &desc->bones[i];

        // Find bone index
        Bone* bone = skeleton_get_bone(skel, cfg->bone_name);
        if (!bone) {
            inst->bone_indices[i] = 0;
        } else {
            inst->bone_indices[i] = bone->bone_id;
        }

        inst->stiffnesss[i] = cfg->stiffness;
        inst->drags[i] = cfg->drag;
        inst->radii[i] = cfg->collider_radius;

        Vec3 g_dir = vec3_create(cfg->gravity_dir[0], cfg->gravity_dir[1], cfg->gravity_dir[2]);
        inst->gravities[i] = vec3_mul(g_dir, cfg->gravity_power);

        // Initialize state
        Bone* b = skeleton_get_bone_by_id(skel, inst->bone_indices[i]);
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
<<<<<<< HEAD
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
=======
    if (handle.id >= g_spring_bones_ctx.count) return;
    free_instance_data(&g_spring_bones_ctx.items[handle.id]);
    g_spring_bones_ctx.items[handle.id].initialized = false;
}

static void update_instance(SpringBoneInstance* inst, float delta_time) {
    if (!inst->initialized || !inst->skeleton) return;

    // LOD & Culling check
    if (inst->culling_enabled && !inst->is_visible) return;
    if (inst->lod_level > 2) return; // Skip update for low LOD

    Skeleton* skel = inst->skeleton;
    uint32_t count = inst->count;

    // Retrieve scratch buffers
    ensure_scratch_buffers(count);
    Vec3* forces = g_spring_bones_ctx.scratch_vec_a;

    // Physics Sub-stepping
    float dt = delta_time;

    // 1. Calculate Forces (Gravity + External)
    memcpy(forces, inst->gravities, count * sizeof(Vec3));

    for (uint32_t i = 0; i < count; i++) {
        // If first frame or reset, snap to target
        if (vec3_length_sq(inst->current_tails[i]) < 0.0001f) {
             Bone* bone = skeleton_get_bone_by_id(skel, inst->bone_indices[i]);
             if (!bone) continue;

             // Initial tail position in Model Space
             Vec3 tail_local = vec3_mul(inst->bone_axes[i], inst->bone_lengths[i]);
             // Use mat4_transform_point for model space transform
             Vec3 tail_model = mat4_transform_point(bone->current_pose_matrix, tail_local);

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
        Bone* bone = skeleton_get_bone_by_id(skel, inst->bone_indices[i]);

        Vec3 tail_local = vec3_mul(inst->bone_axes[i], inst->bone_lengths[i]);
        Vec3 target_tail = mat4_transform_point(bone->current_pose_matrix, tail_local);

        Vec3 to_target = vec3_sub(target_tail, curr);
        Vec3 spring_force = vec3_mul(to_target, inst->stiffnesss[i] * dt);

        Vec3 total_move = vec3_add(velocity, vec3_mul(vec3_add(force, spring_force), dt * dt));
        Vec3 next = vec3_add(curr, total_move);

        // Constraints
        // 1. Length Constraint (distance from bone origin)
        // Access matrix elements carefully using column-major assumption (m[12], m[13], m[14] are translation)
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
        Bone* parent = skeleton_get_bone_by_id(skel, bone->parent_id);
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
>>>>>>> origin/spring-bones-implementation-10769064037362822729
    }

    // Finalize: re-evaluate skeleton hierarchy
    skeleton_update_bone_hierarchy(skel);

<<<<<<< HEAD
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
=======
    inst->dirty = false;
}

int animation_spring_bones_update(animation_spring_bones_handle_t handle, float delta_time) {
    if (handle.id >= g_spring_bones_ctx.count) return -1;
    update_instance(&g_spring_bones_ctx.items[handle.id], delta_time);
    return 0;
}

int animation_spring_bones_update_batch(const animation_spring_bones_handle_t* handles, uint32_t count, float delta_time) {
    if (!handles) return -1;
    for (uint32_t i = 0; i < count; i++) {
        animation_spring_bones_update(handles[i], delta_time);
>>>>>>> origin/spring-bones-implementation-10769064037362822729
    }
    return 0;
}

int animation_spring_bones_update_async(animation_spring_bones_handle_t handle, float delta_time) {
    // Stub for async
    return animation_spring_bones_update(handle, delta_time);
}

void animation_spring_bones_set_lod(animation_spring_bones_handle_t handle, uint32_t lod_level) {
    if (handle.id < g_spring_bones_ctx.count) {
        g_spring_bones_ctx.items[handle.id].lod_level = lod_level;
    }
}

void animation_spring_bones_set_culling(animation_spring_bones_handle_t handle, bool enabled) {
    if (handle.id < g_spring_bones_ctx.count) {
        g_spring_bones_ctx.items[handle.id].culling_enabled = enabled;
    }
}

void animation_spring_bones_reset(animation_spring_bones_handle_t handle) {
    if (handle.id < g_spring_bones_ctx.count) {
        SpringBoneInstance* inst = &g_spring_bones_ctx.items[handle.id];
        if (inst->current_tails) {
            memset(inst->current_tails, 0, inst->count * sizeof(Vec3)); // Will trigger reset in update
        }
    }
}

bool animation_spring_bones_is_valid(animation_spring_bones_handle_t handle) {
    if (handle.id >= g_spring_bones_ctx.count) return false;
    return g_spring_bones_ctx.items[handle.id].initialized;
}

int animation_spring_bones_get_info(animation_spring_bones_handle_t handle, animation_spring_bones_info_t* out_info) {
<<<<<<< HEAD
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

=======
    if (!out_info || handle.id >= g_spring_bones_ctx.count) return -1;
    SpringBoneInstance* inst = &g_spring_bones_ctx.items[handle.id];
    out_info->id = inst->id;
    out_info->flags = inst->flags;
    out_info->initialized = inst->initialized;
    out_info->active_bone_count = inst->count;
>>>>>>> origin/spring-bones-implementation-10769064037362822729
    return 0;
}

void animation_spring_bones_mark_dirty(animation_spring_bones_handle_t handle) {
    if (handle.id < g_spring_bones_ctx.count) {
        g_spring_bones_ctx.items[handle.id].dirty = true;
    }
}

int animation_spring_bones_process_pending(void) {
<<<<<<< HEAD
    int processed = 0;
    for (uint32_t i = 0; i < g_spring_bones_ctx.count; i++) {
        animation_spring_bones_internal_t* item = &g_spring_bones_ctx.items[i];
        if (item->initialized && item->dirty) {
            processed++;
        }
    }

    return processed;
=======
    return 0;
}

const animation_spring_bones_gpu_data_t* animation_spring_bones_get_gpu_data(animation_spring_bones_handle_t handle, uint32_t* out_count) {
    if (handle.id >= g_spring_bones_ctx.count) return NULL;
    SpringBoneInstance* inst = &g_spring_bones_ctx.items[handle.id];
    if (out_count) *out_count = inst->count;
    return inst->gpu_buffer;
>>>>>>> origin/spring-bones-implementation-10769064037362822729
}

uint32_t animation_spring_bones_get_count(void) {
    return g_spring_bones_ctx.count;
}

size_t animation_spring_bones_get_memory_usage(void) {
    size_t total = sizeof(g_spring_bones_ctx);
    total += g_spring_bones_ctx.capacity * sizeof(SpringBoneInstance);

    for (uint32_t i = 0; i < g_spring_bones_ctx.count; i++) {
<<<<<<< HEAD
        if (g_spring_bones_ctx.items[i].initialized) {
             total += g_spring_bones_ctx.items[i].bone_count * sizeof(spring_bone_runtime_t);
=======
        SpringBoneInstance* inst = &g_spring_bones_ctx.items[i];
        if (inst->initialized) {
            total += inst->count * (sizeof(uint32_t) + sizeof(float) * 4 + sizeof(Vec3) * 4 + sizeof(Quat) + sizeof(Mat4) + sizeof(animation_spring_bones_gpu_data_t));
>>>>>>> origin/spring-bones-implementation-10769064037362822729
        }
    }
    return total;
}

void animation_spring_bones_debug_print(void) {
<<<<<<< HEAD
    // Debug printing implementation placeholder
=======
    // Implementation of debug printing
>>>>>>> origin/spring-bones-implementation-10769064037362822729
}
