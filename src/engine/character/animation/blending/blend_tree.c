/*
 * blend_tree.c
 * Animation blend tree evaluation
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 *
 * Implements blend nodes, state machines, and pose transitions
 */

#include "character/animation/blending/blend_tree.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <include/math/math.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define BLEND_TREE_MAX_COUNT 64
#define BLEND_TREE_DEFAULT_CAPACITY 16
#define BLEND_TREE_MAX_NODES 128
#define BLEND_TREE_MAX_PARAMETERS 32

/* ============================================================================
 * MATH TYPES
 * ============================================================================ */

typedef struct vec3 {
    float x, y, z;
} vec3_t;

typedef struct quat {
    float x, y, z, w;
} quat_t;

typedef struct pose {
    vec3_t* translations;
    quat_t* rotations;
    vec3_t* scales;
    uint32_t bone_count;
} pose_t;

/* ============================================================================
 * BLEND TREE TYPES
 * ============================================================================ */

typedef enum blend_node_type {
    NODE_TYPE_CLIP = 0,         // Single animation clip
    NODE_TYPE_LERP,             // Linear interpolation between two inputs
    NODE_TYPE_BLEND_1D,         // 1D Blend space (e.g. idle -> walk -> run)
    NODE_TYPE_BLEND_2D,         // 2D Blend space (e.g. strafe magnitude/direction)
    NODE_TYPE_ADDITIVE,         // Additive blending
    NODE_TYPE_OUTPUT            // Final output
} blend_node_type_t;

typedef struct blend_node_clip {
    uint32_t clip_handle;       // Handle to animation clip
    float speed;
    bool loop;
} blend_node_clip_t;

typedef struct blend_node_lerp {
    uint16_t input_a;           // Index of input node A
    uint16_t input_b;           // Index of input node B
    uint32_t parameter_index;   // Index of float parameter [0,1]
} blend_node_lerp_t;

typedef struct blend_point_1d {
    float value;                // Parameter value (e.g., speed)
    uint16_t input_node;        // Node to blend at this value
} blend_point_1d_t;

typedef struct blend_node_1d {
    blend_point_1d_t points[8];
    uint32_t point_count;
    uint32_t parameter_index;
} blend_node_1d_t;

typedef struct blend_node {
    blend_node_type_t type;
    union {
        blend_node_clip_t clip;
        blend_node_lerp_t lerp;
        blend_node_1d_t blend1d;
    } data;
    
    // Runtime state
    float time;
    float weight;
    pose_t cached_pose;
} blend_node_t;

typedef struct blend_parameter {
    char name[32];
    float value;
} blend_parameter_t;

typedef struct animation_blend_tree_internal {
    uint32_t id;
    uint32_t flags;
    
    blend_node_t nodes[BLEND_TREE_MAX_NODES];
    uint32_t node_count;
    uint16_t root_node;
    
    blend_parameter_t parameters[BLEND_TREE_MAX_PARAMETERS];
    uint32_t parameter_count;
    
    pose_t final_pose;
    
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} animation_blend_tree_internal_t;

typedef struct animation_blend_tree_context {
    animation_blend_tree_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} animation_blend_tree_context_t;

static animation_blend_tree_context_t g_blend_tree_ctx = {0};

/* ============================================================================
 * MATH HELPERS
 * ============================================================================ */

static float clampf(float v, float min, float max) {
    if (v < min) return min;
    if (v > max) return max;
    return v;
}

static float lerpf(float a, float b, float t) {
    return a + (b - a) * t;
}

static vec3_t vec3_lerp(const vec3_t* a, const vec3_t* b, float t) {
    vec3_t result;
    result.x = lerpf(a->x, b->x, t);
    result.y = lerpf(a->y, b->y, t);
    result.z = lerpf(a->z, b->z, t);
    return result;
}

static quat_t quat_slerp(const quat_t* a, const quat_t* b, float t) {
    // Simplified SLERP
    float dot = a->x * b->x + a->y * b->y + a->z * b->z + a->w * b->w;
    float sign = 1.0f;
    if (dot < 0.0f) {
        dot = -dot;
        sign = -1.0f;
    }
    
    // Linear approximation for small angles/performance
    quat_t result;
    result.x = lerpf(a->x, b->x * sign, t);
    result.y = lerpf(a->y, b->y * sign, t);
    result.z = lerpf(a->z, b->z * sign, t);
    result.w = lerpf(a->w, b->w * sign, t);
    
    // Normalize
    float len_sq = result.x*result.x + result.y*result.y + result.z*result.z + result.w*result.w;
    if (len_sq > 0.0001f) {
        float inv_len = 1.0f / sqrtf(len_sq);
        result.x *= inv_len;
        result.y *= inv_len;
        result.z *= inv_len;
        result.w *= inv_len;
    }
    
    return result;
}

/* ============================================================================
 * POSE MANAGEMENT
 * ============================================================================ */

static void pose_init(pose_t* pose, uint32_t bone_count) {
    pose->translations = calloc(bone_count, sizeof(vec3_t));
    pose->rotations = calloc(bone_count, sizeof(quat_t));
    pose->scales = calloc(bone_count, sizeof(vec3_t));
    pose->bone_count = bone_count;
    
    for (uint32_t i = 0; i < bone_count; i++) {
        pose->scales[i] = (vec3_t){1, 1, 1};
        pose->rotations[i] = (quat_t){0, 0, 0, 1};
    }
}

static void pose_cleanup(pose_t* pose) {
    if (pose->translations) free(pose->translations);
    if (pose->rotations) free(pose->rotations);
    if (pose->scales) free(pose->scales);
    memset(pose, 0, sizeof(pose_t));
}

static void pose_blend(pose_t* out, const pose_t* a, const pose_t* b, float t) {
    uint32_t count = (a->bone_count < b->bone_count) ? a->bone_count : b->bone_count;
    if (out->bone_count < count) count = out->bone_count;
    
    for (uint32_t i = 0; i < count; i++) {
        out->translations[i] = vec3_lerp(&a->translations[i], &b->translations[i], t);
        out->rotations[i] = quat_slerp(&a->rotations[i], &b->rotations[i], t);
        out->scales[i] = vec3_lerp(&a->scales[i], &b->scales[i], t);
    }
}

/* ============================================================================
 * TREE EVALUATION
 * ============================================================================ */

static void evaluate_node(animation_blend_tree_internal_t* tree, uint16_t node_idx, float dt) {
    if (node_idx >= tree->node_count) return;
    blend_node_t* node = &tree->nodes[node_idx];
    
    // Update local time
    node->time += dt;
    
    switch (node->type) {
        case NODE_TYPE_CLIP:
            // Fetch pose from animation system (simulated here)
            // sample_animation(node->data.clip.clip_handle, node->time, &node->cached_pose);
            break;
            
        case NODE_TYPE_LERP: {
            evaluate_node(tree, node->data.lerp.input_a, dt);
            evaluate_node(tree, node->data.lerp.input_b, dt);
            
            float alpha = 0.0f;
            uint32_t param_idx = node->data.lerp.parameter_index;
            if (param_idx < tree->parameter_count) {
                alpha = tree->parameters[param_idx].value;
            }
            alpha = clampf(alpha, 0.0f, 1.0f);
            
            pose_blend(&node->cached_pose, 
                       &tree->nodes[node->data.lerp.input_a].cached_pose,
                       &tree->nodes[node->data.lerp.input_b].cached_pose,
                       alpha);
            break;
        }
        
        case NODE_TYPE_BLEND_1D: {
            // Evaluates blend space
            float param = 0.0f;
            if (node->data.blend1d.parameter_index < tree->parameter_count) {
                param = tree->parameters[node->data.blend1d.parameter_index].value;
            }
            
            // Find inputs to blend between
            blend_node_1d_t* b1d = &node->data.blend1d;
            if (b1d->point_count == 0) break;
            
            if (b1d->point_count == 1 || param <= b1d->points[0].value) {
                evaluate_node(tree, b1d->points[0].input_node, dt);
                // Copy pose
                // pose_copy(&node->cached_pose, &tree->nodes[b1d->points[0].input_node].cached_pose);
            } else if (param >= b1d->points[b1d->point_count-1].value) {
                evaluate_node(tree, b1d->points[b1d->point_count-1].input_node, dt);
                // Copy pose
            } else {
                // Find span
                for (uint32_t i = 0; i < b1d->point_count - 1; i++) {
                    if (param >= b1d->points[i].value && param < b1d->points[i+1].value) {
                        evaluate_node(tree, b1d->points[i].input_node, dt);
                        evaluate_node(tree, b1d->points[i+1].input_node, dt);
                        
                        float range = b1d->points[i+1].value - b1d->points[i].value;
                        float t = (param - b1d->points[i].value) / range;
                        
                        pose_blend(&node->cached_pose,
                                   &tree->nodes[b1d->points[i].input_node].cached_pose,
                                   &tree->nodes[b1d->points[i+1].input_node].cached_pose,
                                   t);
                        break;
                    }
                }
            }
            break;
        }
        
        default:
            break;
    }
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int animation_blend_tree_init(void) {
    if (g_blend_tree_ctx.initialized) {
        return 0;
    }

    g_blend_tree_ctx.capacity = BLEND_TREE_DEFAULT_CAPACITY;
    g_blend_tree_ctx.items = calloc(g_blend_tree_ctx.capacity, sizeof(animation_blend_tree_internal_t));
    if (!g_blend_tree_ctx.items) {
        return -1;
    }

    g_blend_tree_ctx.count = 0;
    g_blend_tree_ctx.initialized = true;

    return 0;
}

void animation_blend_tree_shutdown(void) {
    if (!g_blend_tree_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_blend_tree_ctx.count; i++) {
        animation_blend_tree_internal_t* item = &g_blend_tree_ctx.items[i];
        pose_cleanup(&item->final_pose);
        
        for (uint32_t n = 0; n < item->node_count; n++) {
            pose_cleanup(&item->nodes[n].cached_pose);
        }
    }

    free(g_blend_tree_ctx.items);
    g_blend_tree_ctx.items = NULL;
    g_blend_tree_ctx.count = 0;
    g_blend_tree_ctx.capacity = 0;
    g_blend_tree_ctx.initialized = false;
}

int animation_blend_tree_create(animation_blend_tree_handle_t* out_handle, 
                                  const animation_blend_tree_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_blend_tree_ctx.initialized) {
        return -2;
    }

    if (g_blend_tree_ctx.count >= g_blend_tree_ctx.capacity) {
        return -3;
    }

    uint32_t index = g_blend_tree_ctx.count++;
    animation_blend_tree_internal_t* item = &g_blend_tree_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->node_count = 0;
    item->parameter_count = 0;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void animation_blend_tree_destroy(animation_blend_tree_handle_t handle) {
    if (handle.id >= g_blend_tree_ctx.count) {
        return;
    }

    animation_blend_tree_internal_t* item = &g_blend_tree_ctx.items[handle.id];
    pose_cleanup(&item->final_pose);
    for (uint32_t n = 0; n < item->node_count; n++) {
        pose_cleanup(&item->nodes[n].cached_pose);
    }
    
    item->initialized = false;
}

int animation_blend_tree_set_parameter(animation_blend_tree_handle_t handle,
                                         const char* name, float value) {
    if (handle.id >= g_blend_tree_ctx.count) {
        return -1;
    }
    
    animation_blend_tree_internal_t* item = &g_blend_tree_ctx.items[handle.id];
    
    // Find or add parameter
    for (uint32_t i = 0; i < item->parameter_count; i++) {
        if (strcmp(item->parameters[i].name, name) == 0) {
            item->parameters[i].value = value;
            return 0;
        }
    }
    
    if (item->parameter_count < BLEND_TREE_MAX_PARAMETERS) {
        uint32_t idx = item->parameter_count++;
        strncpy(item->parameters[idx].name, name, 31);
        item->parameters[idx].value = value;
        return 0;
    }
    
    return -2; // Full
}

int animation_blend_tree_update(animation_blend_tree_handle_t handle, 
                                  const void* data, size_t size) {
    // Treat data as delta time (float)
    if (handle.id >= g_blend_tree_ctx.count) {
        return -1;
    }
    
    float dt = 0.016f;
    if (data && size == sizeof(float)) {
        dt = *(const float*)data;
    }
    
    animation_blend_tree_internal_t* item = &g_blend_tree_ctx.items[handle.id];
    if (item->node_count > 0 && item->initialized) {
        evaluate_node(item, item->root_node, dt);
    }

    item->dirty = true;
    return 0;
}

bool animation_blend_tree_is_valid(animation_blend_tree_handle_t handle) {
    if (handle.id >= g_blend_tree_ctx.count) {
        return false;
    }
    return g_blend_tree_ctx.items[handle.id].initialized;
}

int animation_blend_tree_get_info(animation_blend_tree_handle_t handle, 
                                    animation_blend_tree_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_blend_tree_ctx.count) {
        return -2;
    }

    const animation_blend_tree_internal_t* item = &g_blend_tree_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void animation_blend_tree_mark_dirty(animation_blend_tree_handle_t handle) {
    if (handle.id < g_blend_tree_ctx.count) {
        g_blend_tree_ctx.items[handle.id].dirty = true;
    }
}

int animation_blend_tree_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_blend_tree_ctx.count; i++) {
        animation_blend_tree_internal_t* item = &g_blend_tree_ctx.items[i];
        if (item->initialized && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t animation_blend_tree_get_count(void) {
    return g_blend_tree_ctx.count;
}

size_t animation_blend_tree_get_memory_usage(void) {
    size_t total = sizeof(g_blend_tree_ctx);
    total += g_blend_tree_ctx.capacity * sizeof(animation_blend_tree_internal_t);
    return total;
}

void animation_blend_tree_debug_print(void) {
    // Debug output
}

/* End of blend_tree.c */
