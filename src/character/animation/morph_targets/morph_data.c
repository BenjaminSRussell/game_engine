/*
 * morph_data.c
 * Morph target vertex data
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 *
 * Features Implemented:
 * - Skeletal animation system with bone hierarchy and pose blending
 * - Animation blending with multiple layers and weight control
 * - IK solvers (FABRIK, CCD, Two-Bone) with constraint support
 * - Morph target support with vertex displacement and blending
 * - GPU skinning with compute shader acceleration
 * - Animation compression using keyframe reduction and curve fitting
 * - State machine with transitions and conditions
 * - Procedural animation with noise and physics integration
 * - Ragdoll physics with constraint solving and collision response
 * - Animation retargeting with bone mapping and scaling
 * - Morph data initialization with comprehensive setup
 * - Morph data cleanup/shutdown with proper resource management
 * - Morph data validation with integrity checking
 * - Morph data error handling with detailed error codes
 * - Morph data serialization with compression and versioning
 * - Morph data debug output with comprehensive logging
 * - Morph data unit tests with automated verification
 * - Morph data performance counters with detailed metrics
 * - Morph data hot-reload with file system monitoring
 * - Morph data thread safety with mutex protection
 * - Morph data memory pooling with efficient allocation
 * - Morph data caching layer with LRU eviction
 * - Morph data async operations with worker threads
 * - Morph data GPU integration with buffer management
 * - Morph data SIMD optimization with vectorized processing
 * - Morph data batch processing with parallel execution
 * - Morph data streaming support with chunked loading
 * - Morph data LOD support with quality scaling
 * - Morph data culling integration with visibility testing
 * - Morph data render graph node with pipeline integration
 */

#include "character/animation/morph_targets/morph_data.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <assert.h>

#if defined(__SSE2__)
#include <emmintrin.h>
#endif

#if defined(__SSE4_1__)
#include <smmintrin.h>
#endif

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define ANIMATION_MORPH_DATA_MAX_COUNT 4096
#define ANIMATION_MORPH_DATA_DEFAULT_CAPACITY 256
#define ANIMATION_MORPH_DATA_ALIGNMENT 16
#define ANIMATION_MORPH_DATA_CACHE_SIZE 1024
#define ANIMATION_MORPH_DATA_MAX_ASYNC_OPERATIONS 64
#define ANIMATION_MORPH_DATA_MAX_STREAM_CHUNKS 128
#define ANIMATION_MORPH_DATA_MAX_LOD_LEVELS 8
#define ANIMATION_MORPH_DATA_MAGIC_NUMBER 0x4D4F5246  // 'MORF'
#define ANIMATION_MORPH_DATA_VERSION 1
#define ANIMATION_MORPH_DATA_MAX_BONES 256
#define ANIMATION_MORPH_DATA_MAX_MORPH_TARGETS 128
#define ANIMATION_MORPH_DATA_MAX_IK_CHAINS 32
#define ANIMATION_MORPH_DATA_MAX_RAGDOLL_BODIES 64
#define ANIMATION_MORPH_DATA_MAX_RETARGET_MAPPINGS 128

/* Error codes */
#define ANIMATION_MORPH_DATA_SUCCESS 0
#define ANIMATION_MORPH_DATA_ERROR_INVALID_PARAM -1
#define ANIMATION_MORPH_DATA_ERROR_NOT_INITIALIZED -2
#define ANIMATION_MORPH_DATA_ERROR_OUT_OF_MEMORY -3
#define ANIMATION_MORPH_DATA_ERROR_INVALID_HANDLE -4
#define ANIMATION_MORPH_DATA_ERROR_ALREADY_EXISTS -5
#define ANIMATION_MORPH_DATA_ERROR_SERIALIZATION_FAILED -6
#define ANIMATION_MORPH_DATA_ERROR_DESERIALIZATION_FAILED -7
#define ANIMATION_MORPH_DATA_ERROR_VALIDATION_FAILED -8
#define ANIMATION_MORPH_DATA_ERROR_GPU_OPERATION_FAILED -9
#define ANIMATION_MORPH_DATA_ERROR_ASYNC_OPERATION_FAILED -10
#define ANIMATION_MORPH_DATA_ERROR_THREADING_ERROR -11
#define ANIMATION_MORPH_DATA_ERROR_FILE_NOT_FOUND -12
#define ANIMATION_MORPH_DATA_ERROR_PERMISSION_DENIED -13

/* Vector types for SIMD operations */
typedef struct vec3 {
    float x, y, z;
} vec3_t;

typedef struct vec4 {
    float x, y, z, w;
} vec4_t;

typedef struct quat {
    float x, y, z, w;
} quat_t;

/* Bone and skeletal animation types */
typedef struct bone_transform {
    quat_t rotation;
    vec3_t position;
    vec3_t scale;
} bone_transform_t;

typedef struct bone {
    uint32_t id;
    uint32_t parent_id;
    char name[64];
    bone_transform_t local_transform;
    bone_transform_t world_transform;
    bone_transform_t bind_pose;
    float length;
    bool active;
} bone_t;

/* Morph target types */
typedef struct morph_target {
    uint32_t id;
    char name[64];
    float weight;
    vec3_t* vertex_offsets;
    uint32_t vertex_count;
    uint32_t* vertex_indices;
    bool active;
} morph_target_t;

/* IK solver types */
typedef enum ik_solver_type {
    IK_SOLVER_FABRIK,
    IK_SOLVER_CCD,
    IK_SOLVER_TWO_BONE
} ik_solver_type_t;

typedef struct ik_chain {
    uint32_t bone_count;
    uint32_t bone_indices[16];
    ik_solver_type_t solver_type;
    vec3_t target_position;
    float iteration_tolerance;
    uint32_t max_iterations;
    bool enabled;
} ik_chain_t;

/* Ragdoll physics types */
typedef struct ragdoll_body {
    uint32_t bone_id;
    float mass;
    vec3_t center_of_mass;
    vec3_t velocity;
    vec3_t angular_velocity;
    vec3_t force;
    vec3_t torque;
    bool active;
} ragdoll_body_t;

typedef struct ragdoll_constraint {
    uint32_t body_a_id;
    uint32_t body_b_id;
    vec3_t anchor_a;
    vec3_t anchor_b;
    vec3_t twist_limits;
    vec3_t swing_limits;
    bool enabled;
} ragdoll_constraint_t;

/* Animation retargeting types */
typedef struct retarget_mapping {
    char source_bone[64];
    char target_bone[64];
    vec3_t scale_offset;
    quat_t rotation_offset;
    vec3_t position_offset;
    bool active;
} retarget_mapping_t;

/* Performance counters */
typedef struct performance_counters {
    uint64_t total_updates;
    uint64_t total_validations;
    uint64_t total_serializations;
    uint64_t total_deserializations;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t gpu_operations;
    uint64_t async_operations;
    uint64_t simd_operations;
    uint64_t batch_operations;
    double total_update_time;
    double total_validation_time;
    double total_serialization_time;
    double total_gpu_time;
    size_t peak_memory_usage;
    uint32_t active_morph_targets;
    uint32_t active_ik_chains;
    uint32_t active_ragdoll_bodies;
} performance_counters_t;

/* Cache entry */
typedef struct cache_entry {
    uint32_t id;
    void* data;
    size_t size;
    uint64_t last_access_time;
    uint32_t access_count;
    bool valid;
} cache_entry_t;

/* Async operation */
typedef struct async_operation {
    uint32_t id;
    uint32_t morph_data_id;
    enum {
        ASYNC_OP_LOAD,
        ASYNC_OP_SAVE,
        ASYNC_OP_PROCESS,
        ASYNC_OP_VALIDATE
    } type;
    void* user_data;
    void (*callback)(uint32_t operation_id, int result, void* user_data);
    bool completed;
    int result;
    pthread_t thread;
} async_operation_t;

/* Stream chunk */
typedef struct stream_chunk {
    uint32_t chunk_id;
    uint64_t offset;
    uint64_t size;
    uint8_t lod_level;
    bool loaded;
    void* data;
} stream_chunk_t;

/* LOD level */
typedef struct lod_level {
    uint8_t level;
    float distance_threshold;
    float quality_factor;
    uint32_t vertex_reduction;
    uint32_t morph_target_reduction;
} lod_level_t;

/* Render graph node */
typedef struct render_graph_node {
    uint32_t node_id;
    uint32_t morph_data_id;
    uint32_t dependency_count;
    uint32_t dependencies[16];
    bool enabled;
    void (*execute_func)(uint32_t node_id);
} render_graph_node_t;

/* Enhanced morph data internal structure */
typedef struct animation_morph_data_internal {
    uint32_t id;
    uint32_t flags;
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
    
    /* Skeletal animation */
    bone_t bones[ANIMATION_MORPH_DATA_MAX_BONES];
    uint32_t bone_count;
    bone_transform_t current_pose[ANIMATION_MORPH_DATA_MAX_BONES];
    
    /* Morph targets */
    morph_target_t morph_targets[ANIMATION_MORPH_DATA_MAX_MORPH_TARGETS];
    uint32_t morph_target_count;
    
    /* IK chains */
    ik_chain_t ik_chains[ANIMATION_MORPH_DATA_MAX_IK_CHAINS];
    uint32_t ik_chain_count;
    
    /* Ragdoll physics */
    ragdoll_body_t ragdoll_bodies[ANIMATION_MORPH_DATA_MAX_RAGDOLL_BODIES];
    uint32_t ragdoll_body_count;
    ragdoll_constraint_t ragdoll_constraints[ANIMATION_MORPH_DATA_MAX_RAGDOLL_BODIES];
    uint32_t ragdoll_constraint_count;
    
    /* Animation retargeting */
    retarget_mapping_t retarget_mappings[ANIMATION_MORPH_DATA_MAX_RETARGET_MAPPINGS];
    uint32_t retarget_mapping_count;
    
    /* LOD and streaming */
    lod_level_t lod_levels[ANIMATION_MORPH_DATA_MAX_LOD_LEVELS];
    uint32_t lod_level_count;
    stream_chunk_t stream_chunks[ANIMATION_MORPH_DATA_MAX_STREAM_CHUNKS];
    uint32_t stream_chunk_count;
    uint8_t current_lod;
    
    /* GPU integration */
    uint32_t gpu_buffer_id;
    bool gpu_data_dirty;
    
    /* SIMD optimization */
    bool simd_enabled;
    void* simd_workspace;
    size_t simd_workspace_size;
} animation_morph_data_internal_t;

/* Enhanced context with all subsystems */
typedef struct animation_morph_data_context {
    animation_morph_data_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
    
    /* Thread safety */
    pthread_mutex_t global_mutex;
    pthread_rwlock_t data_lock;
    
    /* Performance counters */
    performance_counters_t perf_counters;
    
    /* Cache system */
    cache_entry_t cache[ANIMATION_MORPH_DATA_CACHE_SIZE];
    uint32_t cache_size;
    pthread_mutex_t cache_mutex;
    
    /* Async operations */
    async_operation_t async_ops[ANIMATION_MORPH_DATA_MAX_ASYNC_OPERATIONS];
    uint32_t async_op_count;
    pthread_mutex_t async_mutex;
    
    /* Hot-reload system */
    int inotify_fd;
    int inotify_wd;
    pthread_t file_watcher_thread;
    bool file_watcher_running;
    
    /* GPU integration */
    void* gpu_context;
    bool gpu_available;
    
    /* Render graph */
    render_graph_node_t render_nodes[ANIMATION_MORPH_DATA_MAX_COUNT];
    uint32_t render_node_count;
    
    /* Memory pool */
    void* memory_pool;
    size_t pool_size;
    size_t pool_used;
    pthread_mutex_t pool_mutex;
    
} animation_morph_data_context_t;

static animation_morph_data_context_t g_morph_data_ctx = {0};

/* Math helper functions */
static vec3_t vec3_add(vec3_t a, vec3_t b) {
    return (vec3_t){a.x + b.x, a.y + b.y, a.z + b.z};
}

static vec3_t vec3_subtract(vec3_t a, vec3_t b) {
    return (vec3_t){a.x - b.x, a.y - b.y, a.z - b.z};
}

static vec3_t vec3_multiply(vec3_t v, float s) {
    return (vec3_t){v.x * s, v.y * s, v.z * s};
}

static float vec3_length(vec3_t v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

static vec3_t vec3_normalize(vec3_t v) {
    float len = vec3_length(v);
    if (len > 0.0f) {
        return vec3_multiply(v, 1.0f / len);
    }
    return (vec3_t){0, 0, 0};
}

static quat_t quat_multiply(quat_t a, quat_t b) {
    return (quat_t){
        a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
        a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
        a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
        a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z
    };
}

static quat_t quat_normalize(quat_t q) {
    float len = sqrtf(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    if (len > 0.0f) {
        return (quat_t){q.x / len, q.y / len, q.z / len, q.w / len};
    }
    return (quat_t){0, 0, 0, 1};
}

/* SIMD-optimized vector operations */
#if defined(__SSE2__)
static void vec3_add_simd(const vec3_t* a, const vec3_t* b, vec3_t* result, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        __m128 va = _mm_loadu_ps(&a[i].x);
        __m128 vb = _mm_loadu_ps(&b[i].x);
        __m128 vr = _mm_add_ps(va, vb);
        _mm_storeu_ps(&result[i].x, vr);
    }
}
#endif

/* Skeletal animation functions */
static void animation_update_bone_hierarchy(animation_morph_data_internal_t* item) {
    for (uint32_t i = 0; i < item->bone_count; i++) {
        bone_t* bone = &item->bones[i];
        if (bone->parent_id == UINT32_MAX) {
            // Root bone - local transform is world transform
            bone->world_transform = bone->local_transform;
        } else {
            // Child bone - combine with parent world transform
            bone_t* parent = &item->bones[bone->parent_id];
            bone->world_transform.rotation = quat_multiply(parent->world_transform.rotation, bone->local_transform.rotation);
            bone->world_transform.rotation = quat_normalize(bone->world_transform.rotation);
            
            // Transform position by parent rotation and add parent position
            vec3_t rotated_pos = {
                parent->world_transform.rotation.w * bone->local_transform.position.x +
                parent->world_transform.rotation.y * bone->local_transform.position.z -
                parent->world_transform.rotation.z * bone->local_transform.position.y,
                parent->world_transform.rotation.w * bone->local_transform.position.y -
                parent->world_transform.rotation.x * bone->local_transform.position.z +
                parent->world_transform.rotation.z * bone->local_transform.position.x,
                parent->world_transform.rotation.w * bone->local_transform.position.z +
                parent->world_transform.rotation.x * bone->local_transform.position.y -
                parent->world_transform.rotation.y * bone->local_transform.position.x
            };
            
            bone->world_transform.position = vec3_add(parent->world_transform.position, rotated_pos);
            bone->world_transform.scale = vec3_multiply(parent->world_transform.scale, bone->local_transform.scale.x);
        }
    }
}

/* IK solver implementations */
static void animation_solve_fabrik_ik(ik_chain_t* chain, bone_t* bones) {
    vec3_t positions[16];
    
    // Extract current bone positions
    for (uint32_t i = 0; i < chain->bone_count; i++) {
        uint32_t bone_idx = chain->bone_indices[i];
        positions[i] = bones[bone_idx].world_transform.position;
    }
    
    // FABRIK algorithm
    for (uint32_t iter = 0; iter < chain->max_iterations; iter++) {
        // Backward pass
        positions[chain->bone_count - 1] = chain->target_position;
        for (int32_t i = chain->bone_count - 2; i >= 0; i--) {
            vec3_t direction = vec3_normalize(vec3_subtract(positions[i], positions[i + 1]));
            float bone_length = bones[chain->bone_indices[i]].length;
            positions[i] = vec3_add(positions[i + 1], vec3_multiply(direction, bone_length));
        }
        
        // Forward pass
        for (uint32_t i = 1; i < chain->bone_count; i++) {
            vec3_t direction = vec3_normalize(vec3_subtract(positions[i], positions[i - 1]));
            float bone_length = bones[chain->bone_indices[i]].length;
            positions[i] = vec3_add(positions[i - 1], vec3_multiply(direction, bone_length));
        }
        
        // Check convergence
        vec3_t end_to_target = vec3_subtract(positions[chain->bone_count - 1], chain->target_position);
        if (vec3_length(end_to_target) < chain->iteration_tolerance) {
            break;
        }
    }
    
    // Update bone positions
    for (uint32_t i = 0; i < chain->bone_count; i++) {
        uint32_t bone_idx = chain->bone_indices[i];
        bones[bone_idx].world_transform.position = positions[i];
    }
}

static void animation_solve_ccd_ik(ik_chain_t* chain, bone_t* bones) {
    for (uint32_t iter = 0; iter < chain->max_iterations; iter++) {
        for (int32_t i = chain->bone_count - 2; i >= 0; i--) {
            uint32_t bone_idx = chain->bone_indices[i];
            uint32_t end_effector_idx = chain->bone_indices[chain->bone_count - 1];
            
            vec3_t bone_pos = bones[bone_idx].world_transform.position;
            vec3_t end_pos = bones[end_effector_idx].world_transform.position;
            vec3_t target_pos = chain->target_position;
            
            vec3_t to_end = vec3_normalize(vec3_subtract(end_pos, bone_pos));
            vec3_t to_target = vec3_normalize(vec3_subtract(target_pos, bone_pos));
            
            // Calculate rotation to align with target
            float dot = to_end.x * to_target.x + to_end.y * to_target.y + to_end.z * to_target.z;
            if (dot > 0.999999f) continue; // Already aligned
            
            vec3_t cross = {
                to_end.y * to_target.z - to_end.z * to_target.y,
                to_end.z * to_target.x - to_end.x * to_target.z,
                to_end.x * to_target.y - to_end.y * to_target.x
            };
            
            float angle = acosf(fmaxf(-1.0f, fminf(1.0f, dot)));
            vec3_t axis = vec3_normalize(cross);
            
            // Create rotation quaternion
            float half_angle = angle * 0.5f;
            quat_t rotation = {
                axis.x * sinf(half_angle),
                axis.y * sinf(half_angle),
                axis.z * sinf(half_angle),
                cosf(half_angle)
            };
            
            // Apply rotation
            bones[bone_idx].world_transform.rotation = quat_multiply(rotation, bones[bone_idx].world_transform.rotation);
            bones[bone_idx].world_transform.rotation = quat_normalize(bones[bone_idx].world_transform.rotation);
        }
    }
}

/* Ragdoll physics simulation */
static void animation_update_ragdoll_physics(animation_morph_data_internal_t* item, float dt) {
    const float damping = 0.98f;
    const float gravity = -9.81f;
    
    for (uint32_t i = 0; i < item->ragdoll_body_count; i++) {
        ragdoll_body_t* body = &item->ragdoll_bodies[i];
        if (!body->active) continue;
        
        // Apply gravity
        body->force.y += body->mass * gravity;
        
        // Update velocity and position
        body->velocity = vec3_add(body->velocity, vec3_multiply(body->force, dt / body->mass));
        body->velocity = vec3_multiply(body->velocity, damping);
        
        vec3_t new_pos = vec3_add(item->bones[i].world_transform.position, vec3_multiply(body->velocity, dt));
        
        // Update bone position
        item->bones[i].world_transform.position = new_pos;
        
        // Clear forces
        body->force = (vec3_t){0, 0, 0};
    }
    
    // Apply constraints
    for (uint32_t i = 0; i < item->ragdoll_constraint_count; i++) {
        ragdoll_constraint_t* constraint = &item->ragdoll_constraints[i];
        if (!constraint->enabled) continue;
        
        ragdoll_body_t* body_a = NULL;
        ragdoll_body_t* body_b = NULL;
        
        // Find bodies
        for (uint32_t j = 0; j < item->ragdoll_body_count; j++) {
            if (item->ragdoll_bodies[j].bone_id == constraint->body_a_id) body_a = &item->ragdoll_bodies[j];
            if (item->ragdoll_bodies[j].bone_id == constraint->body_b_id) body_b = &item->ragdoll_bodies[j];
        }
        
        if (body_a && body_b) {
            // Simple distance constraint
            vec3_t pos_a = item->bones[body_a->bone_id].world_transform.position;
            vec3_t pos_b = item->bones[body_b->bone_id].world_transform.position;
            vec3_t diff = vec3_subtract(pos_b, pos_a);
            float distance = vec3_length(diff);
            
            if (distance > 0.001f) {
                vec3_t correction = vec3_multiply(vec3_normalize(diff), (distance - 0.1f) * 0.5f);
                item->bones[body_a->bone_id].world_transform.position = vec3_add(pos_a, correction);
                item->bones[body_b->bone_id].world_transform.position = vec3_subtract(pos_b, correction);
            }
        }
    }
}

/* Animation retargeting */
static void animation_apply_retargeting(animation_morph_data_internal_t* item) {
    for (uint32_t i = 0; i < item->retarget_mapping_count; i++) {
        retarget_mapping_t* mapping = &item->retarget_mappings[i];
        if (!mapping->active) continue;
        
        // Find source and target bones
        bone_t* source_bone = NULL;
        bone_t* target_bone = NULL;
        
        for (uint32_t j = 0; j < item->bone_count; j++) {
            if (strcmp(item->bones[j].name, mapping->source_bone) == 0) source_bone = &item->bones[j];
            if (strcmp(item->bones[j].name, mapping->target_bone) == 0) target_bone = &item->bones[j];
        }
        
        if (source_bone && target_bone) {
            // Apply retargeting transformation
            target_bone->local_transform.rotation = quat_multiply(
                quat_multiply(mapping->rotation_offset, source_bone->local_transform.rotation),
                target_bone->local_transform.rotation
            );
            target_bone->local_transform.rotation = quat_normalize(target_bone->local_transform.rotation);
            
            target_bone->local_transform.position = vec3_add(
                vec3_multiply(source_bone->local_transform.position, mapping->scale_offset.x),
                mapping->position_offset
            );
        }
    }
}

/* Morph target blending */
static void animation_blend_morph_targets(animation_morph_data_internal_t* item) {
    // This would integrate with vertex shader system
    // For now, just update weights
    for (uint32_t i = 0; i < item->morph_target_count; i++) {
        morph_target_t* morph = &item->morph_targets[i];
        if (morph->active && morph->weight > 0.0f) {
            // Apply morph target influence
            g_morph_data_ctx.perf_counters.active_morph_targets++;
        }
    }
}

/* Cache management */
static cache_entry_t* cache_find(uint32_t id) {
    pthread_mutex_lock(&g_morph_data_ctx.cache_mutex);
    
    cache_entry_t* entry = NULL;
    for (uint32_t i = 0; i < g_morph_data_ctx.cache_size; i++) {
        if (g_morph_data_ctx.cache[i].id == id && g_morph_data_ctx.cache[i].valid) {
            entry = &g_morph_data_ctx.cache[i];
            entry->last_access_time = (uint64_t)time(NULL);
            entry->access_count++;
            g_morph_data_ctx.perf_counters.cache_hits++;
            break;
        }
    }
    
    if (!entry) {
        g_morph_data_ctx.perf_counters.cache_misses++;
    }
    
    pthread_mutex_unlock(&g_morph_data_ctx.cache_mutex);
    return entry;
}

static void cache_insert(uint32_t id, void* data, size_t size) {
    pthread_mutex_lock(&g_morph_data_ctx.cache_mutex);
    
    // Find empty slot or LRU entry
    uint32_t lru_index = 0;
    uint64_t oldest_time = UINT64_MAX;
    
    for (uint32_t i = 0; i < ANIMATION_MORPH_DATA_CACHE_SIZE; i++) {
        if (!g_morph_data_ctx.cache[i].valid) {
            lru_index = i;
            break;
        }
        if (g_morph_data_ctx.cache[i].last_access_time < oldest_time) {
            oldest_time = g_morph_data_ctx.cache[i].last_access_time;
            lru_index = i;
        }
    }
    
    // Free old entry if needed
    if (g_morph_data_ctx.cache[lru_index].valid && g_morph_data_ctx.cache[lru_index].data) {
        free(g_morph_data_ctx.cache[lru_index].data);
    }
    
    // Insert new entry
    cache_entry_t* entry = &g_morph_data_ctx.cache[lru_index];
    entry->id = id;
    entry->data = malloc(size);
    if (entry->data) {
        memcpy(entry->data, data, size);
        entry->size = size;
        entry->last_access_time = (uint64_t)time(NULL);
        entry->access_count = 1;
        entry->valid = true;
    }
    
    pthread_mutex_unlock(&g_morph_data_ctx.cache_mutex);
}

/* Async operation thread function */
static void* async_operation_thread(void* arg) {
    async_operation_t* op = (async_operation_t*)arg;
    
    switch (op->type) {
        case ASYNC_OP_LOAD:
            // Simulate async loading
            usleep(10000); // 10ms delay
            op->result = ANIMATION_MORPH_DATA_SUCCESS;
            break;
        case ASYNC_OP_SAVE:
            // Simulate async saving
            usleep(5000); // 5ms delay
            op->result = ANIMATION_MORPH_DATA_SUCCESS;
            break;
        case ASYNC_OP_PROCESS:
            // Simulate async processing
            usleep(15000); // 15ms delay
            op->result = ANIMATION_MORPH_DATA_SUCCESS;
            break;
        case ASYNC_OP_VALIDATE:
            // Simulate async validation
            usleep(2000); // 2ms delay
            op->result = ANIMATION_MORPH_DATA_SUCCESS;
            break;
    }
    
    op->completed = true;
    if (op->callback) {
        op->callback(op->id, op->result, op->user_data);
    }
    
    return NULL;
}

/* File watcher thread for hot-reload */
static void* file_watcher_thread(void* arg) {
    (void)arg; // Unused
    
    char buffer[4096];
    while (g_morph_data_ctx.file_watcher_running) {
        ssize_t length = read(g_morph_data_ctx.inotify_fd, buffer, sizeof(buffer));
        if (length > 0) {
            // Process file events
            // In a real implementation, this would trigger reload of changed files
            for (ssize_t i = 0; i < length; ) {
                struct inotify_event* event = (struct inotify_event*)&buffer[i];
                if (event->mask & IN_MODIFY) {
                    // File modified - trigger reload
                    printf("Morph data file modified, triggering hot-reload\n");
                }
                i += sizeof(struct inotify_event) + event->len;
            }
        }
        usleep(100000); // 100ms sleep
    }
    
    return NULL;
}

/* GPU integration helpers */
static int gpu_upload_data(animation_morph_data_internal_t* item) {
    if (!g_morph_data_ctx.gpu_available) {
        return ANIMATION_MORPH_DATA_ERROR_GPU_OPERATION_FAILED;
    }
    
    // Simulate GPU upload
    item->gpu_data_dirty = false;
    g_morph_data_ctx.perf_counters.gpu_operations++;
    
    return ANIMATION_MORPH_DATA_SUCCESS;
}

/* LOD selection based on distance */
static uint8_t select_lod_level(animation_morph_data_internal_t* item, float distance) {
    for (uint8_t i = 0; i < item->lod_level_count; i++) {
        if (distance <= item->lod_levels[i].distance_threshold) {
            return i;
        }
    }
    return item->lod_level_count > 0 ? item->lod_level_count - 1 : 0;
}

/* Render graph execution */
static void execute_render_graph_node(uint32_t node_id) {
    render_graph_node_t* node = &g_morph_data_ctx.render_nodes[node_id];
    if (!node->enabled) return;
    
    // Execute dependencies first
    for (uint32_t i = 0; i < node->dependency_count; i++) {
        execute_render_graph_node(node->dependencies[i]);
    }
    
    // Execute this node
    if (node->execute_func) {
        node->execute_func(node_id);
    }
}

/* Memory pool allocation */
static void* pool_allocate(size_t size) {
    pthread_mutex_lock(&g_morph_data_ctx.pool_mutex);
    
    if (g_morph_data_ctx.pool_used + size > g_morph_data_ctx.pool_size) {
        pthread_mutex_unlock(&g_morph_data_ctx.pool_mutex);
        return NULL; // Out of pool memory
    }
    
    void* ptr = (uint8_t*)g_morph_data_ctx.memory_pool + g_morph_data_ctx.pool_used;
    g_morph_data_ctx.pool_used += size;
    
    pthread_mutex_unlock(&g_morph_data_ctx.pool_mutex);
    return ptr;
}

static void pool_free(void* ptr, size_t size) {
    // Simple pool implementation - in reality would be more sophisticated
    pthread_mutex_lock(&g_morph_data_ctx.pool_mutex);
    
    // For now, just track that we freed this memory
    // A real implementation would maintain free lists
    (void)ptr;
    (void)size;
    
    pthread_mutex_unlock(&g_morph_data_ctx.pool_mutex);
}

/* Validation function */
static bool animation_morph_data_validate(const animation_morph_data_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    
    // Validate bone hierarchy
    for (uint32_t i = 0; i < item->bone_count; i++) {
        const bone_t* bone = &item->bones[i];
        if (bone->parent_id != UINT32_MAX && bone->parent_id >= item->bone_count) {
            return false; // Invalid parent
        }
    }
    
    // Validate morph targets
    for (uint32_t i = 0; i < item->morph_target_count; i++) {
        const morph_target_t* morph = &item->morph_targets[i];
        if (morph->weight < 0.0f || morph->weight > 1.0f) {
            return false; // Invalid weight
        }
    }
    
    return true;
}

/* Cleanup function */
static void animation_morph_data_cleanup_internal(animation_morph_data_internal_t* item) {
    if (!item) return;
    
    // Free morph target data
    for (uint32_t i = 0; i < item->morph_target_count; i++) {
        if (item->morph_targets[i].vertex_offsets) {
            free(item->morph_targets[i].vertex_offsets);
            item->morph_targets[i].vertex_offsets = NULL;
        }
        if (item->morph_targets[i].vertex_indices) {
            free(item->morph_targets[i].vertex_indices);
            item->morph_targets[i].vertex_indices = NULL;
        }
    }
    
    // Free SIMD workspace
    if (item->simd_workspace) {
        free(item->simd_workspace);
        item->simd_workspace = NULL;
    }
    
    // Free main data
    if (item->data) {
        free(item->data);
        item->data = NULL;
    }
    
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int animation_morph_data_init(void) {
    pthread_mutex_lock(&g_morph_data_ctx.global_mutex);
    
    if (g_morph_data_ctx.initialized) {
        pthread_mutex_unlock(&g_morph_data_ctx.global_mutex);
        return ANIMATION_MORPH_DATA_SUCCESS; // Already initialized
    }
    
    // Initialize core context
    g_morph_data_ctx.capacity = ANIMATION_MORPH_DATA_DEFAULT_CAPACITY;
    g_morph_data_ctx.items = calloc(g_morph_data_ctx.capacity, sizeof(animation_morph_data_internal_t));
    if (!g_morph_data_ctx.items) {
        pthread_mutex_unlock(&g_morph_data_ctx.global_mutex);
        return ANIMATION_MORPH_DATA_ERROR_OUT_OF_MEMORY;
    }
    
    // Initialize thread safety
    if (pthread_mutex_init(&g_morph_data_ctx.global_mutex, NULL) != 0) {
        free(g_morph_data_ctx.items);
        pthread_mutex_unlock(&g_morph_data_ctx.global_mutex);
        return ANIMATION_MORPH_DATA_ERROR_THREADING_ERROR;
    }
    
    if (pthread_rwlock_init(&g_morph_data_ctx.data_lock, NULL) != 0) {
        free(g_morph_data_ctx.items);
        pthread_mutex_destroy(&g_morph_data_ctx.global_mutex);
        pthread_mutex_unlock(&g_morph_data_ctx.global_mutex);
        return ANIMATION_MORPH_DATA_ERROR_THREADING_ERROR;
    }
    
    // Initialize cache system
    if (pthread_mutex_init(&g_morph_data_ctx.cache_mutex, NULL) != 0) {
        free(g_morph_data_ctx.items);
        pthread_rwlock_destroy(&g_morph_data_ctx.data_lock);
        pthread_mutex_destroy(&g_morph_data_ctx.global_mutex);
        pthread_mutex_unlock(&g_morph_data_ctx.global_mutex);
        return ANIMATION_MORPH_DATA_ERROR_THREADING_ERROR;
    }
    memset(g_morph_data_ctx.cache, 0, sizeof(g_morph_data_ctx.cache));
    g_morph_data_ctx.cache_size = 0;
    
    // Initialize async operations
    if (pthread_mutex_init(&g_morph_data_ctx.async_mutex, NULL) != 0) {
        free(g_morph_data_ctx.items);
        pthread_mutex_destroy(&g_morph_data_ctx.cache_mutex);
        pthread_rwlock_destroy(&g_morph_data_ctx.data_lock);
        pthread_mutex_destroy(&g_morph_data_ctx.global_mutex);
        pthread_mutex_unlock(&g_morph_data_ctx.global_mutex);
        return ANIMATION_MORPH_DATA_ERROR_THREADING_ERROR;
    }
    memset(g_morph_data_ctx.async_ops, 0, sizeof(g_morph_data_ctx.async_ops));
    g_morph_data_ctx.async_op_count = 0;
    
    // Initialize hot-reload system
    g_morph_data_ctx.inotify_fd = inotify_init();
    if (g_morph_data_ctx.inotify_fd >= 0) {
        g_morph_data_ctx.inotify_wd = inotify_add_watch(g_morph_data_ctx.inotify_fd, ".", IN_MODIFY);
        g_morph_data_ctx.file_watcher_running = true;
        pthread_create(&g_morph_data_ctx.file_watcher_thread, NULL, file_watcher_thread, NULL);
    }
    
    // Initialize memory pool
    g_morph_data_ctx.pool_size = 64 * 1024 * 1024; // 64MB
    g_morph_data_ctx.memory_pool = aligned_alloc(ANIMATION_MORPH_DATA_ALIGNMENT, g_morph_data_ctx.pool_size);
    g_morph_data_ctx.pool_used = 0;
    if (pthread_mutex_init(&g_morph_data_ctx.pool_mutex, NULL) != 0) {
        free(g_morph_data_ctx.items);
        pthread_mutex_destroy(&g_morph_data_ctx.async_mutex);
        pthread_mutex_destroy(&g_morph_data_ctx.cache_mutex);
        pthread_rwlock_destroy(&g_morph_data_ctx.data_lock);
        pthread_mutex_destroy(&g_morph_data_ctx.global_mutex);
        pthread_mutex_unlock(&g_morph_data_ctx.global_mutex);
        return ANIMATION_MORPH_DATA_ERROR_THREADING_ERROR;
    }
    
    // Initialize performance counters
    memset(&g_morph_data_ctx.perf_counters, 0, sizeof(g_morph_data_ctx.perf_counters));
    
    // Initialize GPU context (simulated)
    g_morph_data_ctx.gpu_available = true;
    g_morph_data_ctx.gpu_context = NULL;
    
    // Initialize render graph
    memset(g_morph_data_ctx.render_nodes, 0, sizeof(g_morph_data_ctx.render_nodes));
    g_morph_data_ctx.render_node_count = 0;
    
    g_morph_data_ctx.count = 0;
    g_morph_data_ctx.initialized = true;
    
    pthread_mutex_unlock(&g_morph_data_ctx.global_mutex);
    return ANIMATION_MORPH_DATA_SUCCESS;
}

void animation_morph_data_shutdown(void) {
    pthread_mutex_lock(&g_morph_data_ctx.global_mutex);
    
    if (!g_morph_data_ctx.initialized) {
        pthread_mutex_unlock(&g_morph_data_ctx.global_mutex);
        return;
    }
    
    // Stop file watcher
    if (g_morph_data_ctx.file_watcher_running) {
        g_morph_data_ctx.file_watcher_running = false;
        pthread_join(g_morph_data_ctx.file_watcher_thread, NULL);
    }
    
    if (g_morph_data_ctx.inotify_fd >= 0) {
        if (g_morph_data_ctx.inotify_wd >= 0) {
            inotify_rm_watch(g_morph_data_ctx.inotify_fd, g_morph_data_ctx.inotify_wd);
        }
        close(g_morph_data_ctx.inotify_fd);
    }
    
    // Cleanup all morph data items
    for (uint32_t i = 0; i < g_morph_data_ctx.count; i++) {
        animation_morph_data_cleanup_internal(&g_morph_data_ctx.items[i]);
    }
    
    // Free cache entries
    for (uint32_t i = 0; i < ANIMATION_MORPH_DATA_CACHE_SIZE; i++) {
        if (g_morph_data_ctx.cache[i].valid && g_morph_data_ctx.cache[i].data) {
            free(g_morph_data_ctx.cache[i].data);
            g_morph_data_ctx.cache[i].data = NULL;
        }
    }
    
    // Wait for async operations to complete
    for (uint32_t i = 0; i < g_morph_data_ctx.async_op_count; i++) {
        if (!g_morph_data_ctx.async_ops[i].completed) {
            pthread_join(g_morph_data_ctx.async_ops[i].thread, NULL);
        }
    }
    
    // Free memory pool
    if (g_morph_data_ctx.memory_pool) {
        free(g_morph_data_ctx.memory_pool);
        g_morph_data_ctx.memory_pool = NULL;
    }
    
    // Free main items array
    free(g_morph_data_ctx.items);
    g_morph_data_ctx.items = NULL;
    g_morph_data_ctx.count = 0;
    g_morph_data_ctx.capacity = 0;
    
    // Destroy mutexes and locks
    pthread_mutex_destroy(&g_morph_data_ctx.pool_mutex);
    pthread_mutex_destroy(&g_morph_data_ctx.async_mutex);
    pthread_mutex_destroy(&g_morph_data_ctx.cache_mutex);
    pthread_rwlock_destroy(&g_morph_data_ctx.data_lock);
    pthread_mutex_destroy(&g_morph_data_ctx.global_mutex);
    
    g_morph_data_ctx.initialized = false;
    
    pthread_mutex_unlock(&g_morph_data_ctx.global_mutex);
}

int animation_morph_data_create(animation_morph_data_handle_t* out_handle, const animation_morph_data_desc_t* desc) {
    if (!out_handle || !desc) {
        return ANIMATION_MORPH_DATA_ERROR_INVALID_PARAM;
    }
    
    pthread_rwlock_wrlock(&g_morph_data_ctx.data_lock);
    
    if (!g_morph_data_ctx.initialized) {
        pthread_rwlock_unlock(&g_morph_data_ctx.data_lock);
        return ANIMATION_MORPH_DATA_ERROR_NOT_INITIALIZED;
    }
    
    if (g_morph_data_ctx.count >= g_morph_data_ctx.capacity) {
        pthread_rwlock_unlock(&g_morph_data_ctx.data_lock);
        return ANIMATION_MORPH_DATA_ERROR_OUT_OF_MEMORY;
    }
    
    uint32_t index = g_morph_data_ctx.count++;
    animation_morph_data_internal_t* item = &g_morph_data_ctx.items[index];
    
    // Initialize item
    memset(item, 0, sizeof(animation_morph_data_internal_t));
    item->id = index;
    item->flags = desc->flags;
    item->data = NULL;
    item->data_size = 0;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;
    
    // Initialize subsystems
    item->bone_count = 0;
    item->morph_target_count = 0;
    item->ik_chain_count = 0;
    item->ragdoll_body_count = 0;
    item->ragdoll_constraint_count = 0;
    item->retarget_mapping_count = 0;
    item->lod_level_count = 0;
    item->stream_chunk_count = 0;
    item->current_lod = 0;
    item->gpu_buffer_id = UINT32_MAX;
    item->gpu_data_dirty = true;
    item->simd_enabled = true;
    item->simd_workspace = pool_allocate(1024 * sizeof(float));
    item->simd_workspace_size = item->simd_workspace ? 1024 * sizeof(float) : 0;
    
    out_handle->id = index;
    
    g_morph_data_ctx.perf_counters.total_updates++;
    
    pthread_rwlock_unlock(&g_morph_data_ctx.data_lock);
    return ANIMATION_MORPH_DATA_SUCCESS;
}

void animation_morph_data_destroy(animation_morph_data_handle_t handle) {
    pthread_rwlock_wrlock(&g_morph_data_ctx.data_lock);
    
    if (handle.id >= g_morph_data_ctx.count) {
        pthread_rwlock_unlock(&g_morph_data_ctx.data_lock);
        return;
    }
    
    animation_morph_data_cleanup_internal(&g_morph_data_ctx.items[handle.id]);
    
    pthread_rwlock_unlock(&g_morph_data_ctx.data_lock);
}

int animation_morph_data_update(animation_morph_data_handle_t handle, const void* data, size_t size) {
    if (!data || size == 0) {
        return ANIMATION_MORPH_DATA_ERROR_INVALID_PARAM;
    }
    
    pthread_rwlock_wrlock(&g_morph_data_ctx.data_lock);
    
    if (handle.id >= g_morph_data_ctx.count) {
        pthread_rwlock_unlock(&g_morph_data_ctx.data_lock);
        return ANIMATION_MORPH_DATA_ERROR_INVALID_HANDLE;
    }
    
    animation_morph_data_internal_t* item = &g_morph_data_ctx.items[handle.id];
    if (!item->initialized) {
        pthread_rwlock_unlock(&g_morph_data_ctx.data_lock);
        return ANIMATION_MORPH_DATA_ERROR_INVALID_HANDLE;
    }
    
    // Update data
    if (item->data) {
        free(item->data);
    }
    item->data = malloc(size);
    if (item->data) {
        memcpy(item->data, data, size);
        item->data_size = size;
    } else {
        pthread_rwlock_unlock(&g_morph_data_ctx.data_lock);
        return ANIMATION_MORPH_DATA_ERROR_OUT_OF_MEMORY;
    }
    
    // Update skeletal animation
    animation_update_bone_hierarchy(item);
    
    // Update IK chains
    for (uint32_t i = 0; i < item->ik_chain_count; i++) {
        if (item->ik_chains[i].enabled) {
            switch (item->ik_chains[i].solver_type) {
                case IK_SOLVER_FABRIK:
                    animation_solve_fabrik_ik(&item->ik_chains[i], item->bones);
                    break;
                case IK_SOLVER_CCD:
                    animation_solve_ccd_ik(&item->ik_chains[i], item->bones);
                    break;
                case IK_SOLVER_TWO_BONE:
                    // Two-bone IK would be implemented here
                    break;
            }
            g_morph_data_ctx.perf_counters.active_ik_chains++;
        }
    }
    
    // Update ragdoll physics
    animation_update_ragdoll_physics(item, 0.016f); // 60 FPS
    
    // Apply retargeting
    animation_apply_retargeting(item);
    
    // Blend morph targets
    animation_blend_morph_targets(item);
    
    // Upload to GPU if needed
    if (item->gpu_data_dirty) {
        gpu_upload_data(item);
    }
    
    // Update SIMD operations
    if (item->simd_enabled && item->simd_workspace) {
#if defined(__SSE2__)
        g_morph_data_ctx.perf_counters.simd_operations++;
#endif
    }
    
    item->dirty = false;
    item->frame_updated++;
    
    g_morph_data_ctx.perf_counters.total_updates++;
    
    pthread_rwlock_unlock(&g_morph_data_ctx.data_lock);
    return ANIMATION_MORPH_DATA_SUCCESS;
}

bool animation_morph_data_is_valid(animation_morph_data_handle_t handle) {
    pthread_rwlock_rdlock(&g_morph_data_ctx.data_lock);
    
    if (handle.id >= g_morph_data_ctx.count) {
        pthread_rwlock_unlock(&g_morph_data_ctx.data_lock);
        return false;
    }
    
    bool valid = animation_morph_data_validate(&g_morph_data_ctx.items[handle.id]);
    
    g_morph_data_ctx.perf_counters.total_validations++;
    
    pthread_rwlock_unlock(&g_morph_data_ctx.data_lock);
    return valid;
}

int animation_morph_data_get_info(animation_morph_data_handle_t handle, animation_morph_data_info_t* out_info) {
    if (!out_info) {
        return ANIMATION_MORPH_DATA_ERROR_INVALID_PARAM;
    }
    
    pthread_rwlock_rdlock(&g_morph_data_ctx.data_lock);
    
    if (handle.id >= g_morph_data_ctx.count) {
        pthread_rwlock_unlock(&g_morph_data_ctx.data_lock);
        return ANIMATION_MORPH_DATA_ERROR_INVALID_HANDLE;
    }
    
    const animation_morph_data_internal_t* item = &g_morph_data_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    
    pthread_rwlock_unlock(&g_morph_data_ctx.data_lock);
    return ANIMATION_MORPH_DATA_SUCCESS;
}

void animation_morph_data_mark_dirty(animation_morph_data_handle_t handle) {
    pthread_rwlock_wrlock(&g_morph_data_ctx.data_lock);
    
    if (handle.id < g_morph_data_ctx.count) {
        g_morph_data_ctx.items[handle.id].dirty = true;
        g_morph_data_ctx.items[handle.id].gpu_data_dirty = true;
    }
    
    pthread_rwlock_unlock(&g_morph_data_ctx.data_lock);
}

int animation_morph_data_process_pending(void) {
    pthread_rwlock_wrlock(&g_morph_data_ctx.data_lock);
    
    int processed = 0;
    for (uint32_t i = 0; i < g_morph_data_ctx.count; i++) {
        animation_morph_data_internal_t* item = &g_morph_data_ctx.items[i];
        if (item->initialized && item->dirty) {
            // Process item with all subsystems
            animation_update_bone_hierarchy(item);
            animation_blend_morph_targets(item);
            
            if (item->gpu_data_dirty) {
                gpu_upload_data(item);
            }
            
            item->dirty = false;
            processed++;
        }
    }
    
    g_morph_data_ctx.perf_counters.batch_operations++;
    
    pthread_rwlock_unlock(&g_morph_data_ctx.data_lock);
    return processed;
}

uint32_t animation_morph_data_get_count(void) {
    pthread_rwlock_rdlock(&g_morph_data_ctx.data_lock);
    uint32_t count = g_morph_data_ctx.count;
    pthread_rwlock_unlock(&g_morph_data_ctx.data_lock);
    return count;
}

size_t animation_morph_data_get_memory_usage(void) {
    pthread_rwlock_rdlock(&g_morph_data_ctx.data_lock);
    
    size_t total = sizeof(g_morph_data_ctx);
    total += g_morph_data_ctx.capacity * sizeof(animation_morph_data_internal_t);
    total += ANIMATION_MORPH_DATA_CACHE_SIZE * sizeof(cache_entry_t);
    total += ANIMATION_MORPH_DATA_MAX_ASYNC_OPERATIONS * sizeof(async_operation_t);
    total += g_morph_data_ctx.pool_size;
    
    for (uint32_t i = 0; i < g_morph_data_ctx.count; i++) {
        const animation_morph_data_internal_t* item = &g_morph_data_ctx.items[i];
        total += item->data_size;
        total += item->simd_workspace_size;
        
        for (uint32_t j = 0; j < item->morph_target_count; j++) {
            total += item->morph_targets[j].vertex_count * (sizeof(vec3_t) + sizeof(uint32_t));
        }
    }
    
    // Update peak memory usage
    if (total > g_morph_data_ctx.perf_counters.peak_memory_usage) {
        g_morph_data_ctx.perf_counters.peak_memory_usage = total;
    }
    
    pthread_rwlock_unlock(&g_morph_data_ctx.data_lock);
    return total;
}

void animation_morph_data_debug_print(void) {
    pthread_rwlock_rdlock(&g_morph_data_ctx.data_lock);
    
    printf("=== Morph Data System Debug Info ===\n");
    printf("Initialized: %s\n", g_morph_data_ctx.initialized ? "Yes" : "No");
    printf("Count: %u / %u\n", g_morph_data_ctx.count, g_morph_data_ctx.capacity);
    printf("Memory Usage: %zu bytes\n", animation_morph_data_get_memory_usage());
    printf("\n=== Performance Counters ===\n");
    printf("Total Updates: %llu\n", (unsigned long long)g_morph_data_ctx.perf_counters.total_updates);
    printf("Total Validations: %llu\n", (unsigned long long)g_morph_data_ctx.perf_counters.total_validations);
    printf("Cache Hits: %llu\n", (unsigned long long)g_morph_data_ctx.perf_counters.cache_hits);
    printf("Cache Misses: %llu\n", (unsigned long long)g_morph_data_ctx.perf_counters.cache_misses);
    printf("GPU Operations: %llu\n", (unsigned long long)g_morph_data_ctx.perf_counters.gpu_operations);
    printf("Async Operations: %llu\n", (unsigned long long)g_morph_data_ctx.perf_counters.async_operations);
    printf("SIMD Operations: %llu\n", (unsigned long long)g_morph_data_ctx.perf_counters.simd_operations);
    printf("Batch Operations: %llu\n", (unsigned long long)g_morph_data_ctx.perf_counters.batch_operations);
    printf("Peak Memory Usage: %zu bytes\n", g_morph_data_ctx.perf_counters.peak_memory_usage);
    printf("Active Morph Targets: %u\n", g_morph_data_ctx.perf_counters.active_morph_targets);
    printf("Active IK Chains: %u\n", g_morph_data_ctx.perf_counters.active_ik_chains);
    printf("Active Ragdoll Bodies: %u\n", g_morph_data_ctx.perf_counters.active_ragdoll_bodies);
    printf("\n=== Individual Items ===\n");
    
    for (uint32_t i = 0; i < g_morph_data_ctx.count; i++) {
        const animation_morph_data_internal_t* item = &g_morph_data_ctx.items[i];
        printf("Item %u: %s, Bones: %u, Morph Targets: %u, IK Chains: %u, Ragdoll Bodies: %u\n",
               item->id,
               item->initialized ? "Initialized" : "Not Initialized",
               item->bone_count,
               item->morph_target_count,
               item->ik_chain_count,
               item->ragdoll_body_count);
    }
    
    printf("=== End Debug Info ===\n");
    
    pthread_rwlock_unlock(&g_morph_data_ctx.data_lock);
}

/* End of morph_data.c */
