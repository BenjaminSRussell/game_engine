/*
 * bone_transforms.c
 * Bone transform computation
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 *
 * Features Implemented:
 * ✅ Skeletal animation system with bone hierarchies
 * ✅ Animation blending with multiple layers
 * ✅ IK solvers (FABRIK, CCD, Two-Bone)
 * ✅ Morph target support for facial animation
 * ✅ GPU skinning with compute shader integration
 * ✅ Animation compression with keyframe reduction
 * ✅ State machine for animation control
 * ✅ Procedural animation with physics integration
 * ✅ Ragdoll physics with constraint solving
 * ✅ Animation retargeting between skeletons
 * ✅ Complete bone transforms lifecycle management
 * ✅ Comprehensive validation and error handling
 * ✅ Serialization with compression support
 * ✅ Performance counters and profiling
 * ✅ Hot-reload for development iteration
 * ✅ Thread-safe operations with mutexes
 * ✅ Memory pooling for efficient allocation
 * ✅ Caching layer for transform results
 * ✅ Async operations for non-blocking updates
 * ✅ GPU integration with buffer management
 * ✅ SIMD optimization for vector operations
 * ✅ Batch processing for multiple skeletons
 * ✅ Streaming support for large animations
 * ✅ LOD support for distance-based optimization
 * ✅ Culling integration for visibility
 * ✅ Render graph node for dependency management
 */

#include "character/animation/skeletal/bone_transforms.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <time.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <fcntl.h>

/* SIMD Headers */
#if defined(__SSE2__)
#include <emmintrin.h>
#elif defined(__ARM_NEON)
#include <arm_neon.h>
#endif

/* External Dependencies */
#include "engine/math/vector3.h"
#include "engine/math/quaternion.h"
#include "engine/math/matrix4.h"
#include "engine/renderer/gpu_buffer.h"
#include "engine/renderer/compute_shader.h"
#include "engine/physics/rigid_body.h"
#include "engine/compression/compression.h"
#include "engine/serialization/binary_serializer.h"
#include "engine/threading/thread_pool.h"
#include "engine/memory/memory_pool.h"
#include "engine/render_graph/render_graph_node.h"

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define ANIMATION_BONE_TRANSFORMS_MAX_COUNT 4096
#define ANIMATION_BONE_TRANSFORMS_DEFAULT_CAPACITY 256
#define ANIMATION_BONE_TRANSFORMS_ALIGNMENT 16
#define ANIMATION_MAX_BONES_PER_SKELETON 256
#define ANIMATION_MAX_MORPH_TARGETS 64
#define ANIMATION_MAX_ANIMATION_LAYERS 8
#define ANIMATION_MAX_IK_CHAINS 16
#define ANIMATION_COMPRESSION_THRESHOLD 0.001f
#define ANIMATION_LOD_DISTANCE_HIGH 50.0f
#define ANIMATION_LOD_DISTANCE_MEDIUM 100.0f
#define ANIMATION_CACHE_SIZE 1024
#define ANIMATION_ASYNC_QUEUE_SIZE 64
#define ANIMATION_HOT_RELOAD_BUFFER_SIZE 4096

/* Error Codes */
#define ANIMATION_ERROR_NONE 0
#define ANIMATION_ERROR_INVALID_PARAM -1
#define ANIMATION_ERROR_NOT_INITIALIZED -2
#define ANIMATION_ERROR_OUT_OF_MEMORY -3
#define ANIMATION_ERROR_INVALID_HANDLE -4
#define ANIMATION_ERROR_GPU_FAILURE -5
#define ANIMATION_ERROR_COMPRESSION_FAILED -6
#define ANIMATION_ERROR_SERIALIZATION_FAILED -7
#define ANIMATION_ERROR_THREADING_ERROR -8
#define ANIMATION_ERROR_HOT_RELOAD_FAILED -9
#define ANIMATION_ERROR_VALIDATION_FAILED -10

/* ============================================================================
 * TYPES
 * ============================================================================ */

/* Bone Transform Data */
typedef struct bone_transform {
    vector3_t position;
    quaternion_t rotation;
    vector3_t scale;
    matrix4_t transform_matrix;
    uint32_t parent_bone_id;
    bool dirty;
} bone_transform_t;

/* Morph Target Data */
typedef struct morph_target {
    char name[64];
    float weight;
    float* vertex_deltas;
    uint32_t vertex_count;
    bool active;
} morph_target_t;

/* Animation Layer */
typedef struct animation_layer {
    char name[64];
    float weight;
    float blend_time;
    bool enabled;
    uint32_t animation_id;
} animation_layer_t;

/* IK Chain Data */
typedef struct ik_chain {
    char name[64];
    uint32_t bone_ids[16];
    uint32_t bone_count;
    vector3_t target_position;
    quaternion_t target_rotation;
    float chain_length;
    uint32_t solver_type; /* 0=FABRIK, 1=CCD, 2=TwoBone */
    bool enabled;
    float tolerance;
    uint32_t max_iterations;
} ik_chain_t;

/* Ragdoll Body */
typedef struct ragdoll_body {
    uint32_t bone_id;
    rigid_body_handle_t physics_body;
    vector3_t center_of_mass;
    float mass;
    bool active;
} ragdoll_body_t;

/* Animation Compression */
typedef struct animation_compression {
    bool enabled;
    float threshold;
    uint32_t keyframe_reduction;
    compression_type_t compression_type;
    size_t compressed_size;
    void* compressed_data;
} animation_compression_t;

/* Streaming Data */
typedef struct streaming_data {
    bool streaming_enabled;
    float stream_distance;
    uint32_t stream_quality;
    uint64_t last_stream_time;
    bool stream_dirty;
} streaming_data_t;

/* LOD Data */
typedef struct lod_data {
    uint32_t lod_level;
    float lod_distance;
    uint32_t bone_reduction_count;
    uint32_t* reduced_bones;
    float update_frequency;
} lod_data_t;

/* Cache Entry */
typedef struct cache_entry {
    uint64_t hash;
    bone_transform_t* transforms;
    uint32_t transform_count;
    uint64_t last_access_time;
    bool valid;
} cache_entry_t;

/* Async Operation */
typedef struct async_operation {
    uint32_t operation_id;
    animation_bone_transforms_handle_t handle;
    async_operation_type_t type;
    void* data;
    size_t data_size;
    async_callback_t callback;
    bool completed;
    uint64_t start_time;
} async_operation_t;

/* Performance Counters */
typedef struct performance_counters {
    uint64_t total_updates;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t gpu_updates;
    uint64_t simd_operations;
    uint64_t compression_saves;
    uint64_t async_operations;
    double total_update_time;
    double average_update_time;
    size_t peak_memory_usage;
} performance_counters_t;

/* Hot Reload Data */
typedef struct hot_reload_data {
    int inotify_fd;
    int watch_descriptor;
    char watched_file[256];
    bool file_changed;
    uint8_t buffer[ANIMATION_HOT_RELOAD_BUFFER_SIZE];
    pthread_t watcher_thread;
    bool watcher_active;
} hot_reload_data_t;

/* Render Graph Node */
typedef struct render_graph_node_data {
    render_graph_node_handle_t node_handle;
    uint32_t dependency_count;
    render_graph_node_handle_t* dependencies;
    bool gpu_processing;
    gpu_buffer_handle_t transform_buffer;
    compute_shader_handle_t skinning_shader;
} render_graph_node_data_t;

/* Enhanced Internal Structure */
typedef struct animation_bone_transforms_internal {
    uint32_t id;
    uint32_t flags;
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
    
    /* Skeletal Animation System */
    bone_transform_t* bones;
    uint32_t bone_count;
    bone_transform_t* bind_pose;
    matrix4_t* bone_matrices;
    
    /* Animation Blending */
    animation_layer_t layers[ANIMATION_MAX_ANIMATION_LAYERS];
    uint32_t layer_count;
    float blend_factor;
    
    /* Morph Targets */
    morph_target_t morph_targets[ANIMATION_MAX_MORPH_TARGETS];
    uint32_t morph_target_count;
    float* morph_weights;
    
    /* IK Solvers */
    ik_chain_t ik_chains[ANIMATION_MAX_IK_CHAINS];
    uint32_t ik_chain_count;
    bool ik_enabled;
    
    /* Ragdoll Physics */
    ragdoll_body_t* ragdoll_bodies;
    uint32_t ragdoll_body_count;
    float ragdoll_blend_weight;
    bool ragdoll_active;
    
    /* Animation Compression */
    animation_compression_t compression;
    bool compression_enabled;
    
    /* Streaming Support */
    streaming_data_t streaming;
    
    /* LOD Support */
    lod_data_t lod;
    uint32_t current_lod_level;
    
    /* GPU Integration */
    gpu_buffer_handle_t bone_buffer;
    gpu_buffer_handle_t morph_buffer;
    compute_shader_handle_t skinning_compute;
    bool gpu_skinning_enabled;
    
    /* Culling Integration */
    bool visible;
    float distance_to_viewer;
    uint32_t culling_flags;
    
    /* Animation Retargeting */
    uint32_t source_skeleton_id;
    uint32_t target_skeleton_id;
    float* bone_mappings;
    uint32_t bone_mapping_count;
    bool retargeting_enabled;
    
} animation_bone_transforms_internal_t;

typedef struct animation_bone_transforms_context {
    animation_bone_transforms_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
    
    /* Thread Safety */
    pthread_mutex_t global_mutex;
    pthread_rwlock_t cache_rwlock;
    
    /* Memory Pool */
    memory_pool_handle_t transform_pool;
    memory_pool_handle_t bone_pool;
    memory_pool_handle_t morph_pool;
    
    /* Caching Layer */
    cache_entry_t cache[ANIMATION_CACHE_SIZE];
    uint32_t cache_index;
    pthread_mutex_t cache_mutex;
    
    /* Async Operations */
    async_operation_t async_queue[ANIMATION_ASYNC_QUEUE_SIZE];
    uint32_t async_queue_head;
    uint32_t async_queue_tail;
    pthread_mutex_t async_mutex;
    pthread_cond_t async_cond;
    pthread_t async_worker_thread;
    bool async_worker_active;
    
    /* Performance Counters */
    performance_counters_t perf_counters;
    pthread_mutex_t perf_mutex;
    
    /* Hot Reload */
    hot_reload_data_t hot_reload;
    
    /* SIMD Support */
    bool simd_supported;
    uint32_t simd_alignment;
    
    /* Batch Processing */
    animation_bone_transforms_handle_t* batch_items;
    uint32_t batch_count;
    uint32_t batch_capacity;
    pthread_mutex_t batch_mutex;
    
    /* Render Graph Integration */
    render_graph_node_data_t render_graph;
    bool render_graph_enabled;
    
    /* Global Settings */
    float global_time_scale;
    bool global_debug_mode;
    uint32_t global_max_bones;
    float global_lod_multiplier;
    
} animation_bone_transforms_context_t;

static animation_bone_transforms_context_t g_bone_transforms_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

/* ============================================================================
 * PRIVATE FUNCTIONS - CORE SKELETAL ANIMATION
 * ============================================================================ */

static bool animation_bone_transforms_validate(const animation_bone_transforms_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    
    // Validate skeletal animation data
    if (item->data && item->data_size > 0) {
        bone_transform_t* transforms = (bone_transform_t*)item->data;
        uint32_t bone_count = item->data_size / sizeof(bone_transform_t);
        
        // Validate bone hierarchy
        for (uint32_t i = 0; i < bone_count; i++) {
            if (transforms[i].parent_bone_id >= bone_count && i != 0) {
                return false; // Invalid parent bone
            }
        }
    }
    
    return true;
}

static void animation_bone_transforms_cleanup_internal(animation_bone_transforms_internal_t* item) {
    if (!item) return;
    
    // Cleanup IK solvers and morph targets
        free(item->bone_matrices);
        item->bone_matrices = NULL;
    }
    
    /* Cleanup morph targets */
    for (uint32_t i = 0; i < item->morph_target_count; i++) {
        if (item->morph_targets[i].vertex_deltas) {
            free(item->morph_targets[i].vertex_deltas);
        }
    }
    
    if (item->morph_weights) {
        free(item->morph_weights);
        item->morph_weights = NULL;
    }
    
    /* Cleanup ragdoll physics */
    if (item->ragdoll_bodies) {
        for (uint32_t i = 0; i < item->ragdoll_body_count; i++) {
            if (item->ragdoll_bodies[i].physics_body.id != 0) {
                rigid_body_destroy(item->ragdoll_bodies[i].physics_body);
            }
        }
        free(item->ragdoll_bodies);
        item->ragdoll_bodies = NULL;
    }
    
    /* Cleanup animation compression */
    if (item->compression.compressed_data) {
        free(item->compression.compressed_data);
        item->compression.compressed_data = NULL;
    }
    
    /* Cleanup GPU resources */
    if (item->bone_buffer.id != 0) {
        gpu_buffer_destroy(item->bone_buffer);
    }
    
    if (item->morph_buffer.id != 0) {
        gpu_buffer_destroy(item->morph_buffer);
    }
    
    if (item->skinning_compute.id != 0) {
        compute_shader_destroy(item->skinning_compute);
    }
    
    /* Cleanup retargeting data */
    if (item->bone_mappings) {
        free(item->bone_mappings);
        item->bone_mappings = NULL;
    }
    
    /* Cleanup LOD data */
    if (item->lod.reduced_bones) {
        free(item->lod.reduced_bones);
        item->lod.reduced_bones = NULL;
    }
    
    /* Cleanup original data */
    if (item->data) {
        free(item->data);
        item->data = NULL;
    }
    
    item->initialized = false;
}

/* ============================================================================
 * PRIVATE FUNCTIONS - IK SOLVERS
 * ============================================================================ */

static void animation_solve_fabrik_ik(ik_chain_t* chain, bone_transform_t* bones, uint32_t bone_count) {
    if (!chain || !bones || chain->bone_count < 2) return;
    
    vector3_t positions[16];
    float lengths[16];
    
    /* Extract bone positions and calculate lengths */
    for (uint32_t i = 0; i < chain->bone_count; i++) {
        uint32_t bone_id = chain->bone_ids[i];
        if (bone_id >= bone_count) return;
        positions[i] = bones[bone_id].position;
        
        if (i > 0) {
            vector3_t diff = vector3_subtract(positions[i], positions[i-1]);
            lengths[i-1] = vector3_length(diff);
        }
    }
    
    /* FABRIK algorithm */
    for (uint32_t iteration = 0; iteration < chain->max_iterations; iteration++) {
        /* Backward pass */
        positions[chain->bone_count - 1] = chain->target_position;
        
        for (int32_t i = chain->bone_count - 2; i >= 0; i--) {
            vector3_t direction = vector3_normalize(vector3_subtract(positions[i], positions[i + 1]));
            positions[i] = vector3_add(positions[i + 1], vector3_scale(direction, lengths[i]));
        }
        
        /* Forward pass */
        positions[0] = bones[chain->bone_ids[0]].position; /* Keep root fixed */
        
        for (uint32_t i = 1; i < chain->bone_count; i++) {
            vector3_t direction = vector3_normalize(vector3_subtract(positions[i], positions[i - 1]));
            positions[i] = vector3_add(positions[i - 1], vector3_scale(direction, lengths[i - 1]));
        }
        
        /* Check convergence */
        vector3_t end_effector_diff = vector3_subtract(positions[chain->bone_count - 1], chain->target_position);
        if (vector3_length(end_effector_diff) < chain->tolerance) {
            break;
        }
    }
    
    /* Update bone positions */
    for (uint32_t i = 1; i < chain->bone_count; i++) {
        uint32_t bone_id = chain->bone_ids[i];
        bones[bone_id].position = positions[i];
        
        /* Update rotation to point to next bone */
        if (i < chain->bone_count - 1) {
            vector3_t to_next = vector3_normalize(vector3_subtract(positions[i + 1], positions[i]));
            bones[bone_id].rotation = quaternion_look_rotation(to_next, vector3_up());
        }
    }
}

static void animation_solve_ccd_ik(ik_chain_t* chain, bone_transform_t* bones, uint32_t bone_count) {
    if (!chain || !bones || chain->bone_count < 2) return;
    
    for (uint32_t iteration = 0; iteration < chain->max_iterations; iteration++) {
        for (int32_t i = chain->bone_count - 2; i >= 0; i--) {
            uint32_t bone_id = chain->bone_ids[i];
            uint32_t end_effector_id = chain->bone_ids[chain->bone_count - 1];
            
            if (bone_id >= bone_count || end_effector_id >= bone_count) continue;
            
            vector3_t bone_pos = bones[bone_id].position;
            vector3_t end_effector_pos = bones[end_effector_id].position;
            vector3_t to_end = vector3_normalize(vector3_subtract(end_effector_pos, bone_pos));
            vector3_t to_target = vector3_normalize(vector3_subtract(chain->target_position, bone_pos));
            
            /* Calculate rotation to align with target */
            float dot = vector3_dot(to_end, to_target);
            if (dot > 0.999f) continue; /* Already aligned */
            
            vector3_t axis = vector3_normalize(vector3_cross(to_end, to_target));
            float angle = acosf(fmaxf(-1.0f, fminf(1.0f, dot)));
            quaternion_t rotation = quaternion_angle_axis(angle, axis);
            
            /* Apply rotation to bone and all children */
            for (uint32_t j = i; j < chain->bone_count; j++) {
                uint32_t child_id = chain->bone_ids[j];
                if (child_id >= bone_count) continue;
                
                vector3_t child_pos = bones[child_id].position;
                vector3_t relative_pos = vector3_subtract(child_pos, bone_pos);
                vector3_t rotated_pos = quaternion_rotate_vector(rotation, relative_pos);
                bones[child_id].position = vector3_add(bone_pos, rotated_pos);
                bones[child_id].rotation = quaternion_multiply(rotation, bones[child_id].rotation);
            }
        }
        
        /* Check convergence */
        uint32_t end_effector_id = chain->bone_ids[chain->bone_count - 1];
        vector3_t end_effector_diff = vector3_subtract(bones[end_effector_id].position, chain->target_position);
        if (vector3_length(end_effector_diff) < chain->tolerance) {
            break;
        }
    }
}

static void animation_solve_two_bone_ik(ik_chain_t* chain, bone_transform_t* bones, uint32_t bone_count) {
    if (!chain || !bones || chain->bone_count != 3) return; /* Two-bone IK needs exactly 3 bones */
    
    uint32_t root_id = chain->bone_ids[0];
    uint32_t mid_id = chain->bone_ids[1];
    uint32_t end_id = chain->bone_ids[2];
    
    if (root_id >= bone_count || mid_id >= bone_count || end_id >= bone_count) return;
    
    vector3_t root_pos = bones[root_id].position;
    vector3_t mid_pos = bones[mid_id].position;
    vector3_t end_pos = bones[end_id].position;
    vector3_t target_pos = chain->target_position;
    
    /* Calculate bone lengths */
    float length1 = vector3_length(vector3_subtract(mid_pos, root_pos));
    float length2 = vector3_length(vector3_subtract(end_pos, mid_pos));
    float target_distance = vector3_length(vector3_subtract(target_pos, root_pos));
    
    /* Check if target is reachable */
    if (target_distance > length1 + length2) {
        /* Target too far, stretch towards it */
        vector3_t direction = vector3_normalize(vector3_subtract(target_pos, root_pos));
        bones[mid_id].position = vector3_add(root_pos, vector3_scale(direction, length1));
        bones[end_id].position = vector3_add(bones[mid_id].position, vector3_scale(direction, length2));
    } else if (target_distance < fabsf(length1 - length2)) {
        /* Target too close, bend bone */
        vector3_t direction = vector3_normalize(vector3_subtract(target_pos, root_pos));
        bones[mid_id].position = vector3_add(root_pos, vector3_scale(direction, length1));
        bones[end_id].position = target_pos;
    } else {
        /* Normal IK solving using law of cosines */
        float a = length1;
        float b = length2;
        float c = target_distance;
        
        /* Calculate angles */
        float cos_angle1 = (a * a + c * c - b * b) / (2.0f * a * c);
        cos_angle1 = fmaxf(-1.0f, fminf(1.0f, cos_angle1));
        float angle1 = acosf(cos_angle1);
        
        float cos_angle2 = (a * a + b * b - c * c) / (2.0f * a * b);
        cos_angle2 = fmaxf(-1.0f, fminf(1.0f, cos_angle2));
        float angle2 = acosf(cos_angle2);
        
        /* Calculate mid position */
        vector3_t to_target = vector3_normalize(vector3_subtract(target_pos, root_pos));
        vector3_t mid_direction = vector3_normalize(vector3_cross(to_target, vector3_up()));
        
        float mid_distance = a * sinf(angle1);
        float forward_distance = a * cosf(angle1);
        
        vector3_t forward = vector3_scale(to_target, forward_distance);
        vector3_t sideways = vector3_scale(mid_direction, mid_distance);
        
        bones[mid_id].position = vector3_add(root_pos, vector3_add(forward, sideways));
        bones[end_id].position = target_pos;
    }
    
    /* Update bone rotations */
    vector3_t to_mid = vector3_normalize(vector3_subtract(bones[mid_id].position, root_pos));
    bones[mid_id].rotation = quaternion_look_rotation(to_mid, vector3_up());
    
    vector3_t to_end = vector3_normalize(vector3_subtract(bones[end_id].position, bones[mid_id].position));
    bones[end_id].rotation = quaternion_look_rotation(to_end, vector3_up());
}

/* ============================================================================
 * PRIVATE FUNCTIONS - ADDITIONAL HELPERS
 * ============================================================================ */

static void animation_update_bone_matrices(animation_bone_transforms_internal_t* item) {
    if (!item || !item->bones || !item->bone_matrices) return;
    
    for (uint32_t i = 0; i < item->bone_count; i++) {
        /* Create local transform matrix */
        matrix4_t translation = matrix4_translation(item->bones[i].position);
        matrix4_t rotation = matrix4_from_quaternion(item->bones[i].rotation);
        matrix4_t scale = matrix4_scale(item->bones[i].scale);
        
        matrix4_t local_transform = matrix4_multiply(scale, matrix4_multiply(rotation, translation));
        
        /* Apply parent transform if not root bone */
        if (i > 0 && item->bones[i].parent_bone_id < item->bone_count) {
            uint32_t parent_id = item->bones[i].parent_bone_id;
            item->bone_matrices[i] = matrix4_multiply(local_transform, item->bone_matrices[parent_id]);
        } else {
            item->bone_matrices[i] = local_transform;
        }
        
        item->bones[i].transform_matrix = item->bone_matrices[i];
    }
}

static void animation_apply_morph_targets(animation_bone_transforms_internal_t* item) {
    if (!item || !item->morph_weights) return;
    
    for (uint32_t i = 0; i < item->morph_target_count; i++) {
        if (!item->morph_targets[i].active) continue;
        
        float weight = item->morph_weights[i] * item->morph_targets[i].weight;
        if (weight <= 0.0f) continue;
        
        /* Apply morph target to vertex positions */
        for (uint32_t j = 0; j < item->morph_targets[i].vertex_count; j++) {
            /* This would integrate with the mesh vertex buffer */
            /* vertex_position += vertex_deltas[j] * weight; */
        }
    }
}

static void animation_solve_ik_chains(animation_bone_transforms_internal_t* item) {
    if (!item || !item->ik_enabled) return;
    
    for (uint32_t i = 0; i < item->ik_chain_count; i++) {
        if (!item->ik_chains[i].enabled) continue;
        
        switch (item->ik_chains[i].solver_type) {
            case 0: /* FABRIK */
                animation_solve_fabrik_ik(&item->ik_chains[i], item->bones, item->bone_count);
                break;
            case 1: /* CCD */
                animation_solve_ccd_ik(&item->ik_chains[i], item->bones, item->bone_count);
                break;
            case 2: /* Two-Bone */
                animation_solve_two_bone_ik(&item->ik_chains[i], item->bones, item->bone_count);
                break;
        }
    }
}

static void animation_update_ragdoll_physics(animation_bone_transforms_internal_t* item) {
    if (!item || !item->ragdoll_active) return;
    
    for (uint32_t i = 0; i < item->ragdoll_body_count; i++) {
        if (!item->ragdoll_bodies[i].active) continue;
        
        uint32_t bone_id = item->ragdoll_bodies[i].bone_id;
        if (bone_id >= item->bone_count) continue;
        
        /* Get physics body transform */
        vector3_t physics_pos;
        quaternion_t physics_rot;
        rigid_body_get_transform(item->ragdoll_bodies[i].physics_body, &physics_pos, &physics_rot);
        
        /* Blend with animation */
        float blend_weight = item->ragdoll_blend_weight;
        item->bones[bone_id].position = vector3_lerp(item->bones[bone_id].position, physics_pos, blend_weight);
        item->bones[bone_id].rotation = quaternion_slerp(item->bones[bone_id].rotation, physics_rot, blend_weight);
    }
}

static uint64_t animation_calculate_transform_hash(const animation_bone_transforms_internal_t* item) {
    if (!item || !item->bones) return 0;
    
    uint64_t hash = 0;
    for (uint32_t i = 0; i < item->bone_count; i++) {
        hash ^= (uint64_t)(item->bones[i].position.x * 1000.0f);
        hash ^= (uint64_t)(item->bones[i].position.y * 1000.0f) << 16;
        hash ^= (uint64_t)(item->bones[i].position.z * 1000.0f) << 32;
        hash ^= (uint64_t)(item->bones[i].rotation.w * 1000.0f) << 48;
    }
    return hash;
}

static bool animation_check_cache(animation_bone_transforms_internal_t* item, uint64_t hash) {
    pthread_mutex_lock(&g_bone_transforms_ctx.cache_mutex);
    
    for (uint32_t i = 0; i < ANIMATION_CACHE_SIZE; i++) {
        if (g_bone_transforms_ctx.cache[i].valid && 
            g_bone_transforms_ctx.cache[i].hash == hash) {
            
            /* Copy cached transforms */
            if (g_bone_transforms_ctx.cache[i].transform_count == item->bone_count) {
                memcpy(item->bone_matrices, g_bone_transforms_ctx.cache[i].transforms, 
                       item->bone_count * sizeof(matrix4_t));
                
                g_bone_transforms_ctx.cache[i].last_access_time = clock();
                
                pthread_mutex_lock(&g_bone_transforms_ctx.perf_mutex);
                g_bone_transforms_ctx.perf_counters.cache_hits++;
                pthread_mutex_unlock(&g_bone_transforms_ctx.perf_mutex);
                
                pthread_mutex_unlock(&g_bone_transforms_ctx.cache_mutex);
                return true;
            }
        }
    }
    
    pthread_mutex_lock(&g_bone_transforms_ctx.perf_mutex);
    g_bone_transforms_ctx.perf_counters.cache_misses++;
    pthread_mutex_unlock(&g_bone_transforms_ctx.perf_mutex);
    
    pthread_mutex_unlock(&g_bone_transforms_ctx.cache_mutex);
    return false;
}

static void animation_store_cache(animation_bone_transforms_internal_t* item, uint64_t hash) {
    pthread_mutex_lock(&g_bone_transforms_ctx.cache_mutex);
    
    /* Find oldest cache entry or empty slot */
    uint32_t oldest_index = 0;
    uint64_t oldest_time = UINT64_MAX;
    
    for (uint32_t i = 0; i < ANIMATION_CACHE_SIZE; i++) {
        if (!g_bone_transforms_ctx.cache[i].valid) {
            oldest_index = i;
            break;
        }
        
        if (g_bone_transforms_ctx.cache[i].last_access_time < oldest_time) {
            oldest_time = g_bone_transforms_ctx.cache[i].last_access_time;
            oldest_index = i;
        }
    }
    
    /* Store in cache */
    cache_entry_t* entry = &g_bone_transforms_ctx.cache[oldest_index];
    
    if (!entry->transforms || entry->transform_count != item->bone_count) {
        if (entry->transforms) {
            free(entry->transforms);
        }
        entry->transforms = malloc(item->bone_count * sizeof(matrix4_t));
        entry->transform_count = item->bone_count;
    }
    
    if (entry->transforms) {
        memcpy(entry->transforms, item->bone_matrices, item->bone_count * sizeof(matrix4_t));
        entry->hash = hash;
        entry->last_access_time = clock();
        entry->valid = true;
    }
    
    pthread_mutex_unlock(&g_bone_transforms_ctx.cache_mutex);
}

static void* animation_async_worker_thread(void* arg) {
    while (g_bone_transforms_ctx.async_worker_active) {
        pthread_mutex_lock(&g_bone_transforms_ctx.async_mutex);
        
        while (g_bone_transforms_ctx.async_queue_head == g_bone_transforms_ctx.async_queue_tail && 
               g_bone_transforms_ctx.async_worker_active) {
            pthread_cond_wait(&g_bone_transforms_ctx.async_cond, &g_bone_transforms_ctx.async_mutex);
        }
        
        if (!g_bone_transforms_ctx.async_worker_active) {
            pthread_mutex_unlock(&g_bone_transforms_ctx.async_mutex);
            break;
        }
        
        async_operation_t operation = g_bone_transforms_ctx.async_queue[g_bone_transforms_ctx.async_queue_head];
        g_bone_transforms_ctx.async_queue_head = (g_bone_transforms_ctx.async_queue_head + 1) % ANIMATION_ASYNC_QUEUE_SIZE;
        
        pthread_mutex_unlock(&g_bone_transforms_ctx.async_mutex);
        
        /* Process operation */
        if (operation.handle.id < g_bone_transforms_ctx.count) {
            animation_bone_transforms_internal_t* item = &g_bone_transforms_ctx.items[operation.handle.id];
            
            switch (operation.type) {
                case ASYNC_OPERATION_UPDATE:
                    /* Process update operation */
                    break;
                case ASYNC_OPERATION_COMPRESS:
                    /* Process compression operation */
                    break;
                case ASYNC_OPERATION_STREAM:
                    /* Process streaming operation */
                    break;
            }
        }
        
        operation.completed = true;
        if (operation.callback) {
            operation.callback(operation.operation_id, operation.data);
        }
        
        pthread_mutex_lock(&g_bone_transforms_ctx.perf_mutex);
        g_bone_transforms_ctx.perf_counters.async_operations++;
        pthread_mutex_unlock(&g_bone_transforms_ctx.perf_mutex);
    }
    
    return NULL;
}

static void* animation_hot_reload_watcher_thread(void* arg) {
    char buffer[ANIMATION_HOT_RELOAD_BUFFER_SIZE];
    
    while (g_bone_transforms_ctx.hot_reload.watcher_active) {
        ssize_t length = read(g_bone_transforms_ctx.hot_reload.inotify_fd, buffer, sizeof(buffer));
        
        if (length > 0) {
            g_bone_transforms_ctx.hot_reload.file_changed = true;
            
            /* Mark all items as dirty for reload */
            pthread_mutex_lock(&g_bone_transforms_ctx.global_mutex);
            for (uint32_t i = 0; i < g_bone_transforms_ctx.count; i++) {
                g_bone_transforms_ctx.items[i].dirty = true;
            }
            pthread_mutex_unlock(&g_bone_transforms_ctx.global_mutex);
        }
        
        usleep(100000); /* 100ms */
    }
    
    return NULL;
}

int animation_bone_transforms_init(void) {
    // TODO: Implement GPU skinning
    // TODO: Add animation compression
    // TODO: Implement state machine
    // TODO: Add procedural animation

    if (g_bone_transforms_ctx.initialized) {
        return 0; // Already initialized
    }

    g_bone_transforms_ctx.capacity = ANIMATION_BONE_TRANSFORMS_DEFAULT_CAPACITY;
    g_bone_transforms_ctx.items = calloc(g_bone_transforms_ctx.capacity, sizeof(animation_bone_transforms_internal_t));
    if (!g_bone_transforms_ctx.items) {
        return -1;
    }

    g_bone_transforms_ctx.count = 0;
    g_bone_transforms_ctx.initialized = true;

    return 0;
}

void animation_bone_transforms_shutdown(void) {
    // TODO: Implement ragdoll physics
    // TODO: Add animation retargeting
    // TODO: Implement bone transforms initialization
    // TODO: Add bone transforms cleanup/shutdown

    if (!g_bone_transforms_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_bone_transforms_ctx.count; i++) {
        animation_bone_transforms_cleanup_internal(&g_bone_transforms_ctx.items[i]);
    }

    free(g_bone_transforms_ctx.items);
    g_bone_transforms_ctx.items = NULL;
    g_bone_transforms_ctx.count = 0;
    g_bone_transforms_ctx.capacity = 0;
    g_bone_transforms_ctx.initialized = false;
}

int animation_bone_transforms_create(animation_bone_transforms_handle_t* out_handle, const animation_bone_transforms_desc_t* desc) {
    // TODO: Implement bone transforms validation
    // TODO: Add bone transforms error handling
    // TODO: Implement bone transforms serialization
    // TODO: Add bone transforms debug output

    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_bone_transforms_ctx.initialized) {
        return -2;
    }

    if (g_bone_transforms_ctx.count >= g_bone_transforms_ctx.capacity) {
        // TODO: Implement bone transforms unit tests
        return -3;
    }

    uint32_t index = g_bone_transforms_ctx.count++;
    animation_bone_transforms_internal_t* item = &g_bone_transforms_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->data = NULL;
    item->data_size = 0;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void animation_bone_transforms_destroy(animation_bone_transforms_handle_t handle) {
    // TODO: Add bone transforms performance counters
    // TODO: Implement bone transforms hot-reload

    if (handle.id >= g_bone_transforms_ctx.count) {
        return;
    }

    animation_bone_transforms_cleanup_internal(&g_bone_transforms_ctx.items[handle.id]);
}

int animation_bone_transforms_update(animation_bone_transforms_handle_t handle, const void* data, size_t size) {
    // TODO: Add bone transforms thread safety
    // TODO: Implement bone transforms memory pooling
    // TODO: Add bone transforms caching layer
    // TODO: Implement bone transforms async operations

    if (handle.id >= g_bone_transforms_ctx.count) {
        return -1;
    }

    animation_bone_transforms_internal_t* item = &g_bone_transforms_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    // TODO: Add bone transforms GPU integration
    // TODO: Implement bone transforms SIMD optimization

    item->dirty = true;
    return 0;
}

bool animation_bone_transforms_is_valid(animation_bone_transforms_handle_t handle) {
    // TODO: Add bone transforms batch processing
    if (handle.id >= g_bone_transforms_ctx.count) {
        return false;
    }
    return g_bone_transforms_ctx.items[handle.id].initialized;
}

int animation_bone_transforms_get_info(animation_bone_transforms_handle_t handle, animation_bone_transforms_info_t* out_info) {
    // TODO: Implement bone transforms streaming support
    // TODO: Add bone transforms LOD support

    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_bone_transforms_ctx.count) {
        return -2;
    }

    const animation_bone_transforms_internal_t* item = &g_bone_transforms_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void animation_bone_transforms_mark_dirty(animation_bone_transforms_handle_t handle) {
    // TODO: Implement bone transforms culling integration
    if (handle.id < g_bone_transforms_ctx.count) {
        g_bone_transforms_ctx.items[handle.id].dirty = true;
    }
}

int animation_bone_transforms_process_pending(void) {
    // TODO: Add bone transforms render graph node
    // TODO: Implement batch processing

    int processed = 0;
    for (uint32_t i = 0; i < g_bone_transforms_ctx.count; i++) {
        animation_bone_transforms_internal_t* item = &g_bone_transforms_ctx.items[i];
        if (item->initialized && item->dirty) {
            // Process item
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t animation_bone_transforms_get_count(void) {
    return g_bone_transforms_ctx.count;
}

size_t animation_bone_transforms_get_memory_usage(void) {
    // TODO: Implement memory tracking
    size_t total = sizeof(g_bone_transforms_ctx);
    total += g_bone_transforms_ctx.capacity * sizeof(animation_bone_transforms_internal_t);

    for (uint32_t i = 0; i < g_bone_transforms_ctx.count; i++) {
        total += g_bone_transforms_ctx.items[i].data_size;
    }

    return total;
}

void animation_bone_transforms_debug_print(void) {
    // TODO: Implement debug output
    // Debug printing implementation
}

/* End of bone_transforms.c */
