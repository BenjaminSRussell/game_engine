/**
 * =================================================================================================
 *                           BODY MORPH BLENDING SYSTEM
 *                               Agent: AGENT_TEMPLATE_1
 * =================================================================================================
 *
 * Implementation of body morph blending system for real-time character
 * customization through morph targets and blend shapes.
 *
 * =================================================================================================
 */

#include "character_templates.h"
#include <core/logger.h>
#include <core/memory.h>
#include "engine/include/math/math.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

// Forward declaration
static void body_morph_system_cleanup(void);

// Body morph blending system
#define MAX_MORPH_TARGETS 128
#define MAX_BLEND_SHAPES 64
#define MAX_MORPH_INFLUENCERS 32

typedef struct {
    char name[64];
    u32 mesh_id;
    u32 vertex_count;
    Vec3* delta_vertices;     // Vertex position deltas
    Vec3* delta_normals;      // Vertex normal deltas
    Vec3* delta_tangents;     // Vertex tangent deltas
    f32* delta_uvs;           // UV coordinate deltas
    u32 vertex_stride;
    bool is_active;
    f32 current_weight;
    f32 target_weight;
    f32 blend_speed;
} MorphTarget;

typedef struct {
    char name[64];
    u32 target_indices[MAX_MORPH_INFLUENCERS];
    f32 weights[MAX_MORPH_INFLUENCERS];
    u32 influence_count;
    f32 current_value;
    f32 target_value;
    f32 min_value;
    f32 max_value;
    f32 blend_speed;
    bool is_active;
} BlendShape;

typedef struct {
    MorphTarget morph_targets[MAX_MORPH_TARGETS];
    u32 morph_count;
    BlendShape blend_shapes[MAX_BLEND_SHAPES];
    u32 blend_count;
    u32 base_vertex_count;
    Vec3* base_vertices;
    Vec3* base_normals;
    Vec3* base_tangents;
    f32* base_uvs;
    Vec3* blended_vertices;
    Vec3* blended_normals;
    Vec3* blended_tangents;
    f32* blended_uvs;
    bool needs_update;
    bool is_initialized;
} BodyMorphSystem;

static BodyMorphSystem g_morph_system = {0};

// Initialize body morph system
bool body_morph_system_init(u32 vertex_count) {
    if (g_morph_system.is_initialized) {
        return true;
    }
    
    memset(&g_morph_system, 0, sizeof(BodyMorphSystem));
    g_morph_system.base_vertex_count = vertex_count;
    
    // Allocate base vertex data
    g_morph_system.base_vertices = (Vec3*)calloc(vertex_count, sizeof(Vec3));
    g_morph_system.base_normals = (Vec3*)calloc(vertex_count, sizeof(Vec3));
    g_morph_system.base_tangents = (Vec3*)calloc(vertex_count, sizeof(Vec3));
    g_morph_system.base_uvs = (f32*)calloc(vertex_count * 2, sizeof(f32));
    
    // Allocate blended vertex data
    g_morph_system.blended_vertices = (Vec3*)calloc(vertex_count, sizeof(Vec3));
    g_morph_system.blended_normals = (Vec3*)calloc(vertex_count, sizeof(Vec3));
    g_morph_system.blended_tangents = (Vec3*)calloc(vertex_count, sizeof(Vec3));
    g_morph_system.blended_uvs = (f32*)calloc(vertex_count * 2, sizeof(f32));
    
    if (!g_morph_system.base_vertices || !g_morph_system.base_normals ||
        !g_morph_system.base_tangents || !g_morph_system.base_uvs ||
        !g_morph_system.blended_vertices || !g_morph_system.blended_normals ||
        !g_morph_system.blended_tangents || !g_morph_system.blended_uvs) {
        LOG_ERROR("Failed to allocate memory for morph system");
        body_morph_system_cleanup();
        return false;
    }
    
    g_morph_system.is_initialized = true;
    LOG_INFO("Body morph system initialized with %u vertices", vertex_count);
    return true;
}

// Set base vertex data
bool body_morph_set_base_data(const Vec3* vertices, const Vec3* normals, 
                               const Vec3* tangents, const f32* uvs) {
    if (!g_morph_system.is_initialized) {
        LOG_ERROR("Morph system not initialized");
        return false;
    }
    
    if (vertices) {
        memcpy(g_morph_system.base_vertices, vertices, 
               g_morph_system.base_vertex_count * sizeof(Vec3));
    }
    
    if (normals) {
        memcpy(g_morph_system.base_normals, normals, 
               g_morph_system.base_vertex_count * sizeof(Vec3));
    }
    
    if (tangents) {
        memcpy(g_morph_system.base_tangents, tangents, 
               g_morph_system.base_vertex_count * sizeof(Vec3));
    }
    
    if (uvs) {
        memcpy(g_morph_system.base_uvs, uvs, 
               g_morph_system.base_vertex_count * 2 * sizeof(f32));
    }
    
    // Reset blended data to base data
    memcpy(g_morph_system.blended_vertices, g_morph_system.base_vertices, 
           g_morph_system.base_vertex_count * sizeof(Vec3));
    memcpy(g_morph_system.blended_normals, g_morph_system.base_normals, 
           g_morph_system.base_vertex_count * sizeof(Vec3));
    memcpy(g_morph_system.blended_tangents, g_morph_system.base_tangents, 
           g_morph_system.base_vertex_count * sizeof(Vec3));
    memcpy(g_morph_system.blended_uvs, g_morph_system.base_uvs, 
           g_morph_system.base_vertex_count * 2 * sizeof(f32));
    
    g_morph_system.needs_update = false;
    
    LOG_DEBUG("Set base vertex data for morph system");
    return true;
}

// Add morph target
u32 body_morph_add_target(const char* name, u32 mesh_id, u32 vertex_count) {
    if (!g_morph_system.is_initialized || !name) {
        LOG_ERROR("Morph system not initialized or invalid name");
        return 0;
    }
    
    if (g_morph_system.morph_count >= MAX_MORPH_TARGETS) {
        LOG_ERROR("Maximum morph targets reached");
        return 0;
    }
    
    MorphTarget* target = &g_morph_system.morph_targets[g_morph_system.morph_count];
    
    // Initialize morph target
    strncpy(target->name, name, sizeof(target->name) - 1);
    target->name[sizeof(target->name) - 1] = '\0';
    target->mesh_id = mesh_id;
    target->vertex_count = vertex_count;
    target->vertex_stride = sizeof(Vec3) * 3 + sizeof(f32) * 2; // pos + normal + tangent + uv
    target->is_active = false;
    target->current_weight = 0.0f;
    target->target_weight = 0.0f;
    target->blend_speed = 1.0f;
    
    // Allocate delta arrays
    target->delta_vertices = (Vec3*)calloc(vertex_count, sizeof(Vec3));
    target->delta_normals = (Vec3*)calloc(vertex_count, sizeof(Vec3));
    target->delta_tangents = (Vec3*)calloc(vertex_count, sizeof(Vec3));
    target->delta_uvs = (f32*)calloc(vertex_count * 2, sizeof(f32));
    
    if (!target->delta_vertices || !target->delta_normals || 
        !target->delta_tangents || !target->delta_uvs) {
        LOG_ERROR("Failed to allocate memory for morph target deltas");
        
        if (target->delta_vertices) free(target->delta_vertices);
        if (target->delta_normals) free(target->delta_normals);
        if (target->delta_tangents) free(target->delta_tangents);
        if (target->delta_uvs) free(target->delta_uvs);
        
        memset(target, 0, sizeof(MorphTarget));
        return 0;
    }
    
    u32 target_index = g_morph_system.morph_count++;
    LOG_DEBUG("Added morph target '%s' with %u vertices", name, vertex_count);
    return target_index;
}

// Set morph target delta data
bool body_morph_set_target_deltas(u32 target_index, const Vec3* vertex_deltas,
                                   const Vec3* normal_deltas, const Vec3* tangent_deltas,
                                   const f32* uv_deltas) {
    if (!g_morph_system.is_initialized || target_index >= g_morph_system.morph_count) {
        LOG_ERROR("Invalid morph target index");
        return false;
    }
    
    MorphTarget* target = &g_morph_system.morph_targets[target_index];
    
    if (vertex_deltas) {
        memcpy(target->delta_vertices, vertex_deltas, 
               target->vertex_count * sizeof(Vec3));
    }
    
    if (normal_deltas) {
        memcpy(target->delta_normals, normal_deltas, 
               target->vertex_count * sizeof(Vec3));
    }
    
    if (tangent_deltas) {
        memcpy(target->delta_tangents, tangent_deltas, 
               target->vertex_count * sizeof(Vec3));
    }
    
    if (uv_deltas) {
        memcpy(target->delta_uvs, uv_deltas, 
               target->vertex_count * 2 * sizeof(f32));
    }
    
    LOG_DEBUG("Set delta data for morph target '%s'", target->name);
    return true;
}

// Add blend shape
u32 body_morph_add_blend_shape(const char* name, f32 min_value, f32 max_value) {
    if (!g_morph_system.is_initialized || !name) {
        LOG_ERROR("Morph system not initialized or invalid name");
        return 0;
    }
    
    if (g_morph_system.blend_count >= MAX_BLEND_SHAPES) {
        LOG_ERROR("Maximum blend shapes reached");
        return 0;
    }
    
    BlendShape* shape = &g_morph_system.blend_shapes[g_morph_system.blend_count];
    
    // Initialize blend shape
    strncpy(shape->name, name, sizeof(shape->name) - 1);
    shape->name[sizeof(shape->name) - 1] = '\0';
    shape->current_value = 0.0f;
    shape->target_value = 0.0f;
    shape->min_value = min_value;
    shape->max_value = max_value;
    shape->blend_speed = 1.0f;
    shape->is_active = false;
    shape->influence_count = 0;
    
    u32 shape_index = g_morph_system.blend_count++;
    LOG_DEBUG("Added blend shape '%s' [%.2f, %.2f]", name, min_value, max_value);
    return shape_index;
}

// Add morph target influence to blend shape
bool body_morph_add_influence(u32 blend_shape_index, u32 morph_target_index, f32 weight) {
    if (!g_morph_system.is_initialized || 
        blend_shape_index >= g_morph_system.blend_count ||
        morph_target_index >= g_morph_system.morph_count) {
        LOG_ERROR("Invalid blend shape or morph target index");
        return false;
    }
    
    BlendShape* shape = &g_morph_system.blend_shapes[blend_shape_index];
    
    if (shape->influence_count >= MAX_MORPH_INFLUENCERS) {
        LOG_ERROR("Maximum morph influences reached for blend shape");
        return false;
    }
    
    shape->target_indices[shape->influence_count] = morph_target_index;
    shape->weights[shape->influence_count] = weight;
    shape->influence_count++;
    
    LOG_DEBUG("Added morph target %u influence to blend shape '%s'", 
             morph_target_index, shape->name);
    return true;
}

// Set morph target weight
bool body_morph_set_target_weight(u32 target_index, f32 weight) {
    if (!g_morph_system.is_initialized || target_index >= g_morph_system.morph_count) {
        LOG_ERROR("Invalid morph target index");
        return false;
    }
    
    MorphTarget* target = &g_morph_system.morph_targets[target_index];
    target->target_weight = fmaxf(0.0f, fminf(1.0f, weight));
    target->is_active = target->target_weight > 0.001f;
    
    g_morph_system.needs_update = true;
    
    LOG_DEBUG("Set morph target '%s' weight to %.3f", target->name, target->target_weight);
    return true;
}

// Set blend shape value
bool body_morph_set_blend_value(u32 blend_shape_index, f32 value) {
    if (!g_morph_system.is_initialized || blend_shape_index >= g_morph_system.blend_count) {
        LOG_ERROR("Invalid blend shape index");
        return false;
    }
    
    BlendShape* shape = &g_morph_system.blend_shapes[blend_shape_index];
    shape->target_value = fmaxf(shape->min_value, fminf(shape->max_value, value));
    shape->is_active = fabsf(shape->target_value) > 0.001f;
    
    // Update influenced morph targets
    for (u32 i = 0; i < shape->influence_count; i++) {
        u32 target_index = shape->target_indices[i];
        f32 weight = shape->weights[i] * shape->target_value;
        body_morph_set_target_weight(target_index, weight);
    }
    
    LOG_DEBUG("Set blend shape '%s' value to %.3f", shape->name, shape->target_value);
    return true;
}

// Update morph blending
void body_morph_update(f32 delta_time) {
    if (!g_morph_system.is_initialized || !g_morph_system.needs_update) {
        return;
    }
    
    // Reset blended vertices to base
    memcpy(g_morph_system.blended_vertices, g_morph_system.base_vertices, 
           g_morph_system.base_vertex_count * sizeof(Vec3));
    memcpy(g_morph_system.blended_normals, g_morph_system.base_normals, 
           g_morph_system.base_vertex_count * sizeof(Vec3));
    memcpy(g_morph_system.blended_tangents, g_morph_system.base_tangents, 
           g_morph_system.base_vertex_count * sizeof(Vec3));
    memcpy(g_morph_system.blended_uvs, g_morph_system.base_uvs, 
           g_morph_system.base_vertex_count * 2 * sizeof(f32));
    
    // Apply morph targets
    for (u32 i = 0; i < g_morph_system.morph_count; i++) {
        MorphTarget* target = &g_morph_system.morph_targets[i];
        
        if (!target->is_active) continue;
        
        // Smooth weight transition
        if (fabsf(target->target_weight - target->current_weight) > 0.001f) {
            f32 diff = target->target_weight - target->current_weight;
            f32 step = diff * (1.0f - expf(-target->blend_speed * delta_time));
            target->current_weight += step;
        } else {
            target->current_weight = target->target_weight;
        }
        
        // Apply morph deltas
        if (target->current_weight > 0.001f) {
            for (u32 v = 0; v < target->vertex_count && v < g_morph_system.base_vertex_count; v++) {
                // Apply vertex position delta
                g_morph_system.blended_vertices[v] = vec3_add(g_morph_system.blended_vertices[v],
                                                              vec3_scale(target->delta_vertices[v], vec3(target->current_weight, target->current_weight, target->current_weight)));
                
                // Apply normal delta
                if (target->delta_normals) {
                    g_morph_system.blended_normals[v] = vec3_add(g_morph_system.blended_normals[v],
                                                              vec3_scale(target->delta_normals[v], vec3(target->current_weight, target->current_weight, target->current_weight)));
                    g_morph_system.blended_normals[v] = vec3_normalize(g_morph_system.blended_normals[v]);
                }
                
                // Apply tangent delta
                if (target->delta_tangents) {
                    g_morph_system.blended_tangents[v] = vec3_add(g_morph_system.blended_tangents[v],
                                                              vec3_scale(target->delta_tangents[v], vec3(target->current_weight, target->current_weight, target->current_weight)));
                    g_morph_system.blended_tangents[v] = vec3_normalize(g_morph_system.blended_tangents[v]);
                }
                
                // Apply UV delta
                if (target->delta_uvs) {
                    g_morph_system.blended_uvs[v * 2] += target->delta_uvs[v * 2] * target->current_weight;
                    g_morph_system.blended_uvs[v * 2 + 1] += target->delta_uvs[v * 2 + 1] * target->current_weight;
                }
            }
        }
    }
    
    // Update blend shapes
    for (u32 i = 0; i < g_morph_system.blend_count; i++) {
        BlendShape* shape = &g_morph_system.blend_shapes[i];
        
        if (!shape->is_active) continue;
        
        // Smooth value transition
        if (fabsf(shape->target_value - shape->current_value) > 0.001f) {
            f32 diff = shape->target_value - shape->current_value;
            f32 step = diff * (1.0f - expf(-shape->blend_speed * delta_time));
            shape->current_value += step;
        } else {
            shape->current_value = shape->target_value;
        }
        
        // Update influenced morph targets
        for (u32 j = 0; j < shape->influence_count; j++) {
            u32 target_index = shape->target_indices[j];
            f32 weight = shape->weights[j] * shape->current_value;
            body_morph_set_target_weight(target_index, weight);
        }
    }
    
    g_morph_system.needs_update = false;
}

// Get blended vertex data
bool body_morph_get_blended_vertices(Vec3** vertices, Vec3** normals, Vec3** tangents, f32** uvs) {
    if (!g_morph_system.is_initialized) {
        return false;
    }
    
    if (vertices) *vertices = g_morph_system.blended_vertices;
    if (normals) *normals = g_morph_system.blended_normals;
    if (tangents) *tangents = g_morph_system.blended_tangents;
    if (uvs) *uvs = g_morph_system.blended_uvs;
    
    return true;
}

// Get morph target info
MorphTarget* body_morph_get_target(u32 target_index) {
    if (!g_morph_system.is_initialized || target_index >= g_morph_system.morph_count) {
        return NULL;
    }
    
    return &g_morph_system.morph_targets[target_index];
}

// Get blend shape info
BlendShape* body_morph_get_blend_shape(u32 blend_shape_index) {
    if (!g_morph_system.is_initialized || blend_shape_index >= g_morph_system.blend_count) {
        return NULL;
    }
    
    return &g_morph_system.blend_shapes[blend_shape_index];
}

// Public API functions
u32 body_morph_get_target_count(void) {
    return g_morph_system.morph_count;
}

u32 body_morph_get_blend_shape_count(void) {
    return g_morph_system.blend_count;
}

bool body_morph_is_initialized(void) {
    return g_morph_system.is_initialized;
}

void body_morph_cleanup(void) {
    if (!g_morph_system.is_initialized) {
        return;
    }
    
    // Free morph target data
    for (u32 i = 0; i < g_morph_system.morph_count; i++) {
        MorphTarget* target = &g_morph_system.morph_targets[i];
        
        if (target->delta_vertices) free(target->delta_vertices);
        if (target->delta_normals) free(target->delta_normals);
        if (target->delta_tangents) free(target->delta_tangents);
        if (target->delta_uvs) free(target->delta_uvs);
    }
    
    // Free base and blended data
    if (g_morph_system.base_vertices) free(g_morph_system.base_vertices);
    if (g_morph_system.base_normals) free(g_morph_system.base_normals);
    if (g_morph_system.base_tangents) free(g_morph_system.base_tangents);
    if (g_morph_system.base_uvs) free(g_morph_system.base_uvs);
    
    if (g_morph_system.blended_vertices) free(g_morph_system.blended_vertices);
    if (g_morph_system.blended_normals) free(g_morph_system.blended_normals);
    if (g_morph_system.blended_tangents) free(g_morph_system.blended_tangents);
    if (g_morph_system.blended_uvs) free(g_morph_system.blended_uvs);
    
    memset(&g_morph_system, 0, sizeof(BodyMorphSystem));
    LOG_INFO("Body morph system cleaned up");
}
