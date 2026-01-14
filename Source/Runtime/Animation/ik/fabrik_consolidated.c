/*
 * fabrik_consolidated.c
 * Consolidated FABRIK IK solver implementation
 *
 * Merges the best features from both implementations:
 * - Proper API structure from character/animation/ik/fabrik_solver.c
 * - Enhanced constraint handling from animation/ik_advanced/fabrik_solver.c
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 */

#include "character/animation/ik/fabrik_solver.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include "include/math/math_all.h"
#include <math.h>

// FABRIK solver specific data structures
typedef struct FabrikChain {
    Vec3 positions[16];
    float lengths[15];
    uint32_t joint_count;
    float total_length;
    bool constraints[16];
    float min_angles[16];
    float max_angles[16];
} FabrikChain;

typedef struct animation_fabrik_solver_internal {
    uint32_t id;
    uint32_t flags;
    FabrikChain* chains;
    uint32_t chain_count;
    uint32_t max_chains;
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} animation_fabrik_solver_internal_t;

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define ANIMATION_FABRIK_SOLVER_MAX_COUNT 4096
#define ANIMATION_FABRIK_SOLVER_DEFAULT_CAPACITY 256
#define ANIMATION_FABRIK_SOLVER_ALIGNMENT 16
#define FABRIK_TOLERANCE 0.001f
#define FABRIK_MAX_ITERATIONS 10

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct animation_fabrik_solver_context {
    animation_fabrik_solver_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
} animation_fabrik_solver_context_t;

static animation_fabrik_solver_context_t g_fabrik_solver_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static Vec3 rotate_point_around_axis(const Vec3* point, const Vec3* axis_origin, 
                                     const Vec3* axis, float angle) {
    // Rodrigues' rotation formula
    Vec3 p = vec3_sub(*point, *axis_origin);
    Vec3 k = vec3_normalize(*axis);
    
    float cos_angle = cosf(angle);
    float sin_angle = sinf(angle);
    
    Vec3 term1 = vec3_mul(p, cos_angle);
    Vec3 term2 = vec3_mul(vec3_cross(&k, &p), sin_angle);
    Vec3 term3 = vec3_mul(k, vec3_dot(&k, &p) * (1.0f - cos_angle));
    
    return vec3_add(*axis_origin, vec3_add(vec3_add(term1, term2), term3));
}

static void apply_joint_constraints(FabrikChain* chain, int joint_index) {
    if (!chain->constraints[joint_index] || 
        joint_index <= 0 || joint_index >= (int)chain->joint_count - 1) {
        return;
    }
    
    // Calculate angle between adjacent bones
    Vec3 bone1 = vec3_normalize(vec3_sub(chain->positions[joint_index], 
                                         chain->positions[joint_index-1]));
    Vec3 bone2 = vec3_normalize(vec3_sub(chain->positions[joint_index+1], 
                                         chain->positions[joint_index]));
    float angle = acosf(fmaxf(-1.0f, fminf(1.0f, vec3_dot(&bone1, &bone2))));
    float angle_deg = angle * 180.0f / M_PI;
    
    // Enforce angle constraints
    if (angle_deg < chain->min_angles[joint_index]) {
        // Rotate to minimum angle
        float rotation_needed = (chain->min_angles[joint_index] - angle_deg) * M_PI / 180.0f;
        Vec3 axis = vec3_normalize(vec3_cross(&bone1, &bone2));
        chain->positions[joint_index+1] = rotate_point_around_axis(
            &chain->positions[joint_index+1],
            &chain->positions[joint_index],
            &axis, rotation_needed);
    } else if (angle_deg > chain->max_angles[joint_index]) {
        // Rotate to maximum angle
        float rotation_needed = (angle_deg - chain->max_angles[joint_index]) * M_PI / 180.0f;
        Vec3 axis = vec3_normalize(vec3_cross(&bone1, &bone2));
        chain->positions[joint_index+1] = rotate_point_around_axis(
            &chain->positions[joint_index+1],
            &chain->positions[joint_index],
            &axis, -rotation_needed);
    }
}

static bool animation_fabrik_solver_validate(const animation_fabrik_solver_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    return true;
}

static void animation_fabrik_solver_cleanup_internal(animation_fabrik_solver_internal_t* item) {
    if (!item) return;
    if (item->chains) {
        free(item->chains);
        item->chains = NULL;
    }
    if (item->data) {
        free(item->data);
        item->data = NULL;
    }
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int animation_fabrik_solver_init(void) {
    if (g_fabrik_solver_ctx.initialized) {
        return 0; // Already initialized
    }

    g_fabrik_solver_ctx.capacity = ANIMATION_FABRIK_SOLVER_DEFAULT_CAPACITY;
    g_fabrik_solver_ctx.items = calloc(g_fabrik_solver_ctx.capacity, sizeof(animation_fabrik_solver_internal_t));
    if (!g_fabrik_solver_ctx.items) {
        return -1;
    }

    g_fabrik_solver_ctx.count = 0;
    g_fabrik_solver_ctx.initialized = true;

    return 0;
}

void animation_fabrik_solver_shutdown(void) {
    if (!g_fabrik_solver_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_fabrik_solver_ctx.count; i++) {
        animation_fabrik_solver_cleanup_internal(&g_fabrik_solver_ctx.items[i]);
    }

    free(g_fabrik_solver_ctx.items);
    g_fabrik_solver_ctx.items = NULL;
    g_fabrik_solver_ctx.count = 0;
    g_fabrik_solver_ctx.capacity = 0;
    g_fabrik_solver_ctx.initialized = false;
}

int animation_fabrik_solver_create(animation_fabrik_solver_handle_t* out_handle, const animation_fabrik_solver_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_fabrik_solver_ctx.initialized) {
        return -2;
    }

    if (g_fabrik_solver_ctx.count >= g_fabrik_solver_ctx.capacity) {
        return -3;
    }

    uint32_t index = g_fabrik_solver_ctx.count++;
    animation_fabrik_solver_internal_t* item = &g_fabrik_solver_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->max_chains = 16; // Default max chains per solver
    item->chains = calloc(item->max_chains, sizeof(FabrikChain));
    item->chain_count = 0;
    item->data = desc->user_data;
    item->data_size = 0;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void animation_fabrik_solver_destroy(animation_fabrik_solver_handle_t handle) {
    if (handle.id >= g_fabrik_solver_ctx.count) {
        return;
    }

    animation_fabrik_solver_cleanup_internal(&g_fabrik_solver_ctx.items[handle.id]);
}

int animation_fabrik_solver_update(animation_fabrik_solver_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_fabrik_solver_ctx.count) {
        return -1;
    }

    animation_fabrik_solver_internal_t* item = &g_fabrik_solver_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    item->dirty = true;
    return 0;
}

bool animation_fabrik_solver_is_valid(animation_fabrik_solver_handle_t handle) {
    if (handle.id >= g_fabrik_solver_ctx.count) {
        return false;
    }
    return g_fabrik_solver_ctx.items[handle.id].initialized;
}

int animation_fabrik_solver_get_info(animation_fabrik_solver_handle_t handle, animation_fabrik_solver_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_fabrik_solver_ctx.count) {
        return -2;
    }

    const animation_fabrik_solver_internal_t* item = &g_fabrik_solver_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void animation_fabrik_solver_mark_dirty(animation_fabrik_solver_handle_t handle) {
    if (handle.id < g_fabrik_solver_ctx.count) {
        g_fabrik_solver_ctx.items[handle.id].dirty = true;
    }
}

int animation_fabrik_solver_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_fabrik_solver_ctx.count; i++) {
        animation_fabrik_solver_internal_t* item = &g_fabrik_solver_ctx.items[i];
        if (item->initialized && item->dirty) {
            // Process item
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t animation_fabrik_solver_get_count(void) {
    return g_fabrik_solver_ctx.count;
}

size_t animation_fabrik_solver_get_memory_usage(void) {
    size_t total = sizeof(g_fabrik_solver_ctx);
    total += g_fabrik_solver_ctx.capacity * sizeof(animation_fabrik_solver_internal_t);

    for (uint32_t i = 0; i < g_fabrik_solver_ctx.count; i++) {
        total += g_fabrik_solver_ctx.items[i].data_size;
    }

    return total;
}

void animation_fabrik_solver_debug_print(void) {
    if (!g_fabrik_solver_ctx.initialized) {
        return;
    }
    
    printf("FABRIK Solver Debug Info:\n");
    printf("  Active solvers: %u/%u\n", g_fabrik_solver_ctx.count, g_fabrik_solver_ctx.capacity);
    printf("  Memory usage: %zu bytes\n", animation_fabrik_solver_get_memory_usage());
    
    for (uint32_t i = 0; i < g_fabrik_solver_ctx.count; i++) {
        const animation_fabrik_solver_internal_t* item = &g_fabrik_solver_ctx.items[i];
        if (item->initialized) {
            printf("  Solver %u: %u chains, dirty=%s\n", 
                   item->id, item->chain_count, item->dirty ? "true" : "false");
        }
    }
}

// FABRIK solving functions
uint32_t animation_fabrik_solver_add_chain(animation_fabrik_solver_handle_t handle, 
                                           const Vec3* positions, uint32_t joint_count) {
    if (handle.id >= g_fabrik_solver_ctx.count || !positions || joint_count < 2 || joint_count > 16) {
        return UINT32_MAX;
    }
    
    animation_fabrik_solver_internal_t* item = &g_fabrik_solver_ctx.items[handle.id];
    if (!item->initialized || item->chain_count >= item->max_chains) {
        return UINT32_MAX;
    }
    
    uint32_t chain_id = item->chain_count++;
    FabrikChain* chain = &item->chains[chain_id];
    
    chain->joint_count = joint_count;
    chain->total_length = 0.0f;
    
    // Copy positions and calculate bone lengths
    for (uint32_t i = 0; i < joint_count; i++) {
        chain->positions[i] = positions[i];
        chain->constraints[i] = false;
        chain->min_angles[i] = 0.0f;
        chain->max_angles[i] = 180.0f;
        
        if (i > 0) {
            Vec3 bone = vec3_sub(positions[i], positions[i-1]);
            chain->lengths[i-1] = vec3_length(&bone);
            chain->total_length += chain->lengths[i-1];
        }
    }
    
    return chain_id;
}

bool animation_fabrik_solver_solve_chain(animation_fabrik_solver_handle_t handle, 
                                        uint32_t chain_id, const Vec3* target, 
                                        int max_iterations) {
    if (handle.id >= g_fabrik_solver_ctx.count || !target) {
        return false;
    }
    
    animation_fabrik_solver_internal_t* item = &g_fabrik_solver_ctx.items[handle.id];
    if (!item->initialized || chain_id >= item->chain_count) {
        return false;
    }
    
    FabrikChain* chain = &item->chains[chain_id];
    if (chain->joint_count < 2) {
        return false;
    }
    
    // Use default max iterations if not specified
    if (max_iterations <= 0) {
        max_iterations = FABRIK_MAX_ITERATIONS;
    }
    
    // Check if target is reachable
    float target_distance = vec3_distance(&chain->positions[0], target);
    if (target_distance > chain->total_length) {
        // Target unreachable - stretch towards it
        Vec3 direction = vec3_normalize(vec3_sub(*target, chain->positions[0]));
        for (uint32_t i = 1; i < chain->joint_count; i++) {
            chain->positions[i] = vec3_add(chain->positions[i-1], 
                                          vec3_mul(direction, chain->lengths[i-1]));
        }
        return true;
    }
    
    // FABRIK algorithm iterations
    for (int iter = 0; iter < max_iterations; iter++) {
        // Forward reaching
        chain->positions[chain->joint_count - 1] = *target;
        
        for (int32_t i = chain->joint_count - 2; i >= 0; i--) {
            Vec3 direction = vec3_normalize(vec3_sub(chain->positions[i], 
                                                   chain->positions[i + 1]));
            chain->positions[i] = vec3_add(chain->positions[i + 1], 
                                          vec3_mul(direction, chain->lengths[i]));
            
            // Apply joint constraints using the helper function
            apply_joint_constraints(chain, i);
        }
        
        // Backward reaching
        for (uint32_t i = 1; i < chain->joint_count; i++) {
            Vec3 direction = vec3_normalize(vec3_sub(chain->positions[i], 
                                                   chain->positions[i - 1]));
            chain->positions[i] = vec3_add(chain->positions[i - 1], 
                                          vec3_mul(direction, chain->lengths[i - 1]));
        }
        
        // Check convergence
        float end_error = vec3_distance(&chain->positions[chain->joint_count - 1], target);
        if (end_error < FABRIK_TOLERANCE) {
            break;
        }
    }
    
    return true;
}

Vec3 animation_fabrik_solver_get_joint_position(animation_fabrik_solver_handle_t handle, 
                                                uint32_t chain_id, uint32_t joint_index) {
    if (handle.id >= g_fabrik_solver_ctx.count) {
        return (Vec3){0, 0, 0};
    }
    
    animation_fabrik_solver_internal_t* item = &g_fabrik_solver_ctx.items[handle.id];
    if (!item->initialized || chain_id >= item->chain_count || joint_index >= 16) {
        return (Vec3){0, 0, 0};
    }
    
    FabrikChain* chain = &item->chains[chain_id];
    if (joint_index >= chain->joint_count) {
        return (Vec3){0, 0, 0};
    }
    
    return chain->positions[joint_index];
}

void animation_fabrik_solver_set_joint_constraint(animation_fabrik_solver_handle_t handle, 
                                                  uint32_t chain_id, uint32_t joint_index,
                                                  float min_angle, float max_angle) {
    if (handle.id >= g_fabrik_solver_ctx.count) {
        return;
    }
    
    animation_fabrik_solver_internal_t* item = &g_fabrik_solver_ctx.items[handle.id];
    if (!item->initialized || chain_id >= item->chain_count || joint_index >= 16) {
        return;
    }
    
    FabrikChain* chain = &item->chains[chain_id];
    if (joint_index >= chain->joint_count) {
        return;
    }
    
    chain->constraints[joint_index] = true;
    chain->min_angles[joint_index] = min_angle;
    chain->max_angles[joint_index] = max_angle;
}

/* End of fabrik_consolidated.c */
