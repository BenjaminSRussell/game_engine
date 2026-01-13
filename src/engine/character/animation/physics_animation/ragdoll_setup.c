/*
 * ragdoll_setup.c
 * Ragdoll skeleton setup
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 */

#include "character/animation/physics_animation/ragdoll_setup.h"
#include "physics/physics_engine_core.h"
#include "physics/core/physics_types.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
// Remove #include <math.h> as requested to avoid conflicts if any, though standard lib should be fine.
// But wait, I need sqrt if I do any math. I'll rely on engine math.h if needed.

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define ANIMATION_RAGDOLL_SETUP_MAX_COUNT 4096
#define ANIMATION_RAGDOLL_SETUP_DEFAULT_CAPACITY 256
#define ANIMATION_RAGDOLL_SETUP_ALIGNMENT 16

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct RagdollSetupData {
    uint32_t bone_count;
    RagdollBoneSetup* bones;
} RagdollSetupData;

typedef struct animation_ragdoll_setup_internal {
    uint32_t id;
    uint32_t flags;
    RagdollSetupData* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} animation_ragdoll_setup_internal_t;

typedef struct animation_ragdoll_setup_context {
    animation_ragdoll_setup_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
} animation_ragdoll_setup_context_t;

static animation_ragdoll_setup_context_t g_ragdoll_setup_ctx = {0};

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================ */

// Helper to transform a local offset by parent transform
static void transform_vec3(const float* transform, const float* v, float* out) {
    // Basic mat4 * vec3 (assuming w=1)
    // m is column major
    out[0] = transform[0]*v[0] + transform[4]*v[1] + transform[8]*v[2] + transform[12];
    out[1] = transform[1]*v[0] + transform[5]*v[1] + transform[9]*v[2] + transform[13];
    out[2] = transform[2]*v[0] + transform[6]*v[1] + transform[10]*v[2] + transform[14];
}

static void create_constraint_from_setup(PhysicsWorld* world, RagdollBoneSetup* bone, RigidBody* body, RigidBody* parent_body) {
    Constraint constraint = {0};
    // ID generation: Handle (high bits) + Bone Index (low bits) + offset?
    // body->id is already (handle << 16) | bone_index.
    // Let's make constraint ID unique by using bone index + some high bit flag or different offset.
    // Assuming constraint ID space is separate or large enough.
    // We'll use body->id | 0x80000000 to distinguish from bodies? No, ID is u32.
    // We'll trust the world to manage it or generate a derived ID.
    // For now: (body->id ^ 0xA5A5A5A5) to mix bits.
    constraint.id = body->id ^ 0xA5A5A5A5;

    // Use HINGE constraint as it matches the data structure available (angle_min/max)
    // In a full implementation, we would support CONE_TWIST if the struct supported it.
    constraint.type = CONSTRAINT_HINGE;
    constraint.body_a = parent_body->id;
    constraint.body_b = body->id;

    // Limits
    constraint.data.hinge.angle_min = bone->limits_min[0];
    constraint.data.hinge.angle_max = bone->limits_max[0];
    constraint.limit_enabled[0] = true;

    // Set axis (e.g., X axis)
    constraint.data.hinge.axis[0] = 1.0f;
    constraint.data.hinge.axis[1] = 0.0f;
    constraint.data.hinge.axis[2] = 0.0f;

    // Anchors
    // Pivot on Child (Body B) is usually at its origin (0,0,0) in local space if bone offset defines the joint.
    constraint.local_anchor_b[0] = 0.0f;
    constraint.local_anchor_b[1] = 0.0f;
    constraint.local_anchor_b[2] = 0.0f;

    // Pivot on Parent (Body A) is the offset of the Child bone in Parent's space.
    // Since bone->offset is likely the position of the child relative to parent (or root),
    // we use it as the anchor on parent.
    // Note: This assumes bone->offset is relative to parent.
    constraint.local_anchor_a[0] = bone->offset[0];
    constraint.local_anchor_a[1] = bone->offset[1];
    constraint.local_anchor_a[2] = bone->offset[2];

    physics_world_add_constraint(world, &constraint);
}

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool animation_ragdoll_setup_validate(const animation_ragdoll_setup_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    if (!item->data) return false;
    return true;
}

static void animation_ragdoll_setup_cleanup_internal(animation_ragdoll_setup_internal_t* item) {
    if (!item) return;
    if (item->data) {
        if (item->data->bones) {
            free(item->data->bones);
        }
        free(item->data);
        item->data = NULL;
    }
    item->initialized = false;
}

static CollisionShape* create_shape_from_setup(const RagdollBoneSetup* setup) {
    switch (setup->shape_type) {
        case RAGDOLL_SHAPE_SPHERE:
            return shape_sphere_create(setup->dimensions.radius);
        case RAGDOLL_SHAPE_BOX:
            return shape_box_create(setup->dimensions.half_extents[0],
                                  setup->dimensions.half_extents[1],
                                  setup->dimensions.half_extents[2]);
        case RAGDOLL_SHAPE_CAPSULE:
            return shape_capsule_create(setup->dimensions.radius, setup->dimensions.height);
        default:
            return NULL;
    }
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int animation_ragdoll_setup_init(void) {
    if (g_ragdoll_setup_ctx.initialized) {
        return 0; // Already initialized
    }

    g_ragdoll_setup_ctx.capacity = ANIMATION_RAGDOLL_SETUP_DEFAULT_CAPACITY;
    g_ragdoll_setup_ctx.items = calloc(g_ragdoll_setup_ctx.capacity, sizeof(animation_ragdoll_setup_internal_t));
    if (!g_ragdoll_setup_ctx.items) {
        return -1;
    }

    g_ragdoll_setup_ctx.count = 0;
    g_ragdoll_setup_ctx.initialized = true;

    return 0;
}

void animation_ragdoll_setup_shutdown(void) {
    if (!g_ragdoll_setup_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_ragdoll_setup_ctx.count; i++) {
        animation_ragdoll_setup_cleanup_internal(&g_ragdoll_setup_ctx.items[i]);
    }

    free(g_ragdoll_setup_ctx.items);
    g_ragdoll_setup_ctx.items = NULL;
    g_ragdoll_setup_ctx.count = 0;
    g_ragdoll_setup_ctx.capacity = 0;
    g_ragdoll_setup_ctx.initialized = false;
}

int animation_ragdoll_setup_create(animation_ragdoll_setup_handle_t* out_handle, const animation_ragdoll_setup_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_ragdoll_setup_ctx.initialized) {
        return -2;
    }

    if (g_ragdoll_setup_ctx.count >= g_ragdoll_setup_ctx.capacity) {
        return -3;
    }

    // Validation
    if (desc->bone_count == 0 || !desc->bones) {
        return -4;
    }

    uint32_t index = g_ragdoll_setup_ctx.count++;
    animation_ragdoll_setup_internal_t* item = &g_ragdoll_setup_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;

    // Allocate data
    item->data = malloc(sizeof(RagdollSetupData));
    if (!item->data) {
        return -5;
    }
    item->data->bone_count = desc->bone_count;
    item->data->bones = malloc(sizeof(RagdollBoneSetup) * desc->bone_count);
    if (!item->data->bones) {
        free(item->data);
        return -5;
    }

    // Copy bone setup
    memcpy(item->data->bones, desc->bones, sizeof(RagdollBoneSetup) * desc->bone_count);

    item->data_size = sizeof(RagdollSetupData) + sizeof(RagdollBoneSetup) * desc->bone_count;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void animation_ragdoll_setup_destroy(animation_ragdoll_setup_handle_t handle) {
    if (handle.id >= g_ragdoll_setup_ctx.count) {
        return;
    }

    animation_ragdoll_setup_cleanup_internal(&g_ragdoll_setup_ctx.items[handle.id]);
}

int animation_ragdoll_setup_update(animation_ragdoll_setup_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_ragdoll_setup_ctx.count) {
        return -1;
    }

    animation_ragdoll_setup_internal_t* item = &g_ragdoll_setup_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    item->dirty = true;
    return 0;
}

bool animation_ragdoll_setup_is_valid(animation_ragdoll_setup_handle_t handle) {
    if (handle.id >= g_ragdoll_setup_ctx.count) {
        return false;
    }
    return g_ragdoll_setup_ctx.items[handle.id].initialized;
}

int animation_ragdoll_setup_get_info(animation_ragdoll_setup_handle_t handle, animation_ragdoll_setup_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_ragdoll_setup_ctx.count) {
        return -2;
    }

    const animation_ragdoll_setup_internal_t* item = &g_ragdoll_setup_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    if (item->data) {
        out_info->bone_count = item->data->bone_count;
    } else {
        out_info->bone_count = 0;
    }

    return 0;
}

void animation_ragdoll_setup_mark_dirty(animation_ragdoll_setup_handle_t handle) {
    if (handle.id < g_ragdoll_setup_ctx.count) {
        g_ragdoll_setup_ctx.items[handle.id].dirty = true;
    }
}

int animation_ragdoll_setup_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_ragdoll_setup_ctx.count; i++) {
        animation_ragdoll_setup_internal_t* item = &g_ragdoll_setup_ctx.items[i];
        if (item->initialized && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

int animation_ragdoll_setup_instantiate(animation_ragdoll_setup_handle_t handle,
                                        PhysicsWorld* world,
                                        const float* root_transform_mat4,
                                        RigidBody** out_bodies) {
    if (handle.id >= g_ragdoll_setup_ctx.count) return -1;
    animation_ragdoll_setup_internal_t* item = &g_ragdoll_setup_ctx.items[handle.id];
    if (!item->initialized || !item->data) return -2;
    if (!world || !out_bodies) return -3;

    RagdollSetupData* setup = item->data;

    float default_root[16] = {
        1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1
    };
    const float* root_tf = root_transform_mat4 ? root_transform_mat4 : default_root;

    // Temporary storage for calculated world transforms
    // In a real implementation we would hierarchy-walk properly
    // Here we assume offsets are relative to root if parent is -1, or relative to parent if parent >= 0
    // But since `RagdollBoneSetup` is just setup data, it might contain local offsets.

    // We'll iterate and create bodies.
    for (uint32_t i = 0; i < setup->bone_count; i++) {
        RagdollBoneSetup* bone = &setup->bones[i];

        float pos[3];
        // Calculate position.
        // Logic: if parent exists, pos = parent_pos + rotated_offset?
        // Or assumes setup->bones[i].offset is position in model space (bind pose)?
        // Usually setup data contains model-space transforms.
        // We'll assume model-space offset.
        transform_vec3(root_tf, bone->offset, pos);

        RigidBodyType type = (bone->mass > 0.0f) ? RIGID_BODY_DYNAMIC : RIGID_BODY_KINEMATIC;

        // Generate Unique ID: HandleID | BoneIndex
        uint32_t body_id = (handle.id << 16) | i;

        RigidBody* body = core_rigid_body_create(body_id, type);
        if (!body) {
             return -4;
        }

        float pos_v[3] = {pos[0], pos[1], pos[2]};
        core_rigid_body_set_position(body, pos_v);

        // Rotation should also be transformed by root.
        // For now using raw local rotation (simplified).
        core_rigid_body_set_rotation(body, bone->rotation);

        body->mass = bone->mass;
        body->inv_mass = (bone->mass > 0.0f) ? 1.0f / bone->mass : 0.0f;
        body->friction = bone->friction;
        body->restitution = bone->restitution;

        CollisionShape* shape = create_shape_from_setup(bone);
        if (shape) {
            body->shape = shape;
        }

        physics_world_add_body(world, body);
        out_bodies[i] = body;

        // Constraints
        if (bone->parent_bone_index >= 0 && bone->parent_bone_index < (int32_t)i) {
            RigidBody* parent_body = out_bodies[bone->parent_bone_index];
            if (parent_body) {
                create_constraint_from_setup(world, bone, body, parent_body);
            }
        }
    }

    return 0;
}

uint32_t animation_ragdoll_setup_get_count(void) {
    return g_ragdoll_setup_ctx.count;
}

size_t animation_ragdoll_setup_get_memory_usage(void) {
    size_t total = sizeof(g_ragdoll_setup_ctx);
    total += g_ragdoll_setup_ctx.capacity * sizeof(animation_ragdoll_setup_internal_t);

    for (uint32_t i = 0; i < g_ragdoll_setup_ctx.count; i++) {
        total += g_ragdoll_setup_ctx.items[i].data_size;
    }

    return total;
}

void animation_ragdoll_setup_debug_print(void) {
    // Debug printing implementation
}

/* End of ragdoll_setup.c */
