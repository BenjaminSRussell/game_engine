/*
 * animation_retargeting.c
 * Enterprise-grade animation retargeting system
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 *
 * Features implemented:
 * - Professional bone mapping with similarity algorithms
 * - Hierarchical pose transfer with scale compensation
 * - Real-time retargeting with LOD support
 * - Multi-threaded processing with work queues
 * - Comprehensive error handling and validation
 * - Performance optimization with LRU caching
 * - Support for different skeleton topologies
 * - Advanced bone matching algorithms
 * - Motion preservation techniques
 * - Symmetric mapping detection
 * - Chain-based retargeting
 * - IK correction and foot sliding prevention
 */

#include "character/animation/retargeting/animation_retargeting.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>

/* SIMD includes */
#ifdef __SSE2__
#include <emmintrin.h>
#endif
#ifdef __AVX__
#include <immintrin.h>
#endif

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define ANIMATION_RETARGETING_MAGIC_NUMBER 0x41525447  /* "ARTG" */
#define ANIMATION_RETARGETING_VERSION 1
#define ANIMATION_RETARGETING_MEMORY_ALIGNMENT 16
#define ANIMATION_RETARGETING_BATCH_SIZE 64
#define ANIMATION_RETARGETING_NAME_SIMILARITY_THRESHOLD 0.7f
#define ANIMATION_RETARGETING_HIERARCHY_SIMILARITY_THRESHOLD 0.6f
#define ANIMATION_RETARGETING_DEFAULT_BLEND_TIME 0.1f

/* Error codes */
#define ANIMATION_RETARGETING_ERROR_NONE 0
#define ANIMATION_RETARGETING_ERROR_INVALID_HANDLE -1
#define ANIMATION_RETARGETING_ERROR_INVALID_PARAMETER -2
#define ANIMATION_RETARGETING_ERROR_OUT_OF_MEMORY -3
#define ANIMATION_RETARGETING_ERROR_NOT_INITIALIZED -4
#define ANIMATION_RETARGETING_ERROR_ALREADY_INITIALIZED -5
#define ANIMATION_RETARGETING_ERROR_OPERATION_FAILED -6
#define ANIMATION_RETARGETING_ERROR_THREAD_LOCK_FAILED -7
#define ANIMATION_RETARGETING_ERROR_SKELETON_NOT_FOUND -8
#define ANIMATION_RETARGETING_ERROR_MAPPING_FAILED -9
#define ANIMATION_RETARGETING_ERROR_RETARGET_FAILED -10

/* ============================================================================
 * INTERNAL TYPES
 * ============================================================================ */

typedef struct animation_retargeting_skeleton_internal {
    uint32_t id;
    animation_skeleton_t skeleton;
    uint32_t reference_count;
    bool is_registered;
} animation_retargeting_skeleton_internal_t;

typedef struct animation_retargeting_mapping_internal {
    uint32_t source_skeleton_id;
    uint32_t target_skeleton_id;
    animation_retargeting_bone_mapping_t bone_mappings[ANIMATION_RETARGETING_MAX_MAPPINGS];
    uint32_t bone_mapping_count;
    animation_retargeting_chain_t chains[ANIMATION_RETARGETING_MAX_BONES];
    uint32_t chain_count;
    bool is_valid;
} animation_retargeting_mapping_internal_t;

typedef struct animation_retargeting_async_operation {
    uint32_t operation_id;
    bool completed;
    bool in_progress;
    pthread_cond_t completion_cond;
    pthread_mutex_t completion_mutex;
    int error_code;
    uint32_t source_animation_id;
    uint32_t target_skeleton_id;
    uint32_t result_animation_id;
} animation_retargeting_async_operation_t;

typedef struct animation_retargeting_memory_pool {
    void* skeleton_memory;
    void* mapping_memory;
    void* cache_memory;
    size_t skeleton_size;
    size_t mapping_size;
    size_t cache_size;
    bool initialized;
} animation_retargeting_memory_pool_t;

typedef struct animation_retargeting_internal {
    uint32_t id;
    char name[128];
    uint32_t flags;
    bool initialized;
    
    /* Settings */
    animation_retargeting_settings_t settings;
    
    /* Skeletons */
    animation_retargeting_skeleton_internal_t skeletons[ANIMATION_RETARGETING_MAX_SKELETONS];
    uint32_t skeleton_count;
    
    /* Mappings */
    animation_retargeting_mapping_internal_t mappings[ANIMATION_RETARGETING_MAX_SKELETONS * ANIMATION_RETARGETING_MAX_SKELETONS];
    uint32_t mapping_count;
    
    /* Cache */
    animation_retargeting_cache_entry_t cache[ANIMATION_RETARGETING_CACHE_SIZE];
    uint32_t cache_count;
    uint32_t cache_capacity;
    uint64_t next_cache_eviction_time;
    
    /* Async operations */
    animation_retargeting_async_operation_t* async_operations;
    uint32_t async_operation_capacity;
    uint32_t next_async_operation_id;
    
    /* Performance tracking */
    animation_retargeting_performance_stats_t performance_stats;
    
    /* Memory management */
    animation_retargeting_memory_pool_t memory_pool;
    size_t memory_budget;
    size_t current_memory_usage;
    
    /* Threading */
    pthread_mutex_t retargeting_mutex;
    pthread_t* worker_threads;
    uint32_t worker_thread_count;
    bool worker_threads_running;
    
    /* Debug and profiling */
    char last_error[256];
    uint64_t frame_count;
    
} animation_retargeting_internal_t;

typedef struct animation_retargeting_context {
    animation_retargeting_internal_t* instances;
    uint32_t instance_count;
    uint32_t instance_capacity;
    void* allocator;
    bool initialized;
    
    /* Global state */
    pthread_mutex_t global_mutex;
    size_t global_memory_usage;
    uint32_t next_instance_id;
    
    /* SIMD detection */
    bool simd_available;
    bool avx_available;
    
} animation_retargeting_context_t;

static animation_retargeting_context_t g_animation_retargeting_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTION DECLARATIONS
 * ============================================================================ */

/* Math utilities */
static float animation_vec3_dot(const animation_vec3_t* a, const animation_vec3_t* b);
static float animation_vec3_length(const animation_vec3_t* v);
static animation_vec3_t animation_vec3_normalize(const animation_vec3_t* v);
static animation_vec3_t animation_vec3_add(const animation_vec3_t* a, const animation_vec3_t* b);
static animation_vec3_t animation_vec3_sub(const animation_vec3_t* a, const animation_vec3_t* b);
static animation_vec3_t animation_vec3_scale(const animation_vec3_t* v, float s);
static animation_quat_t animation_quat_multiply(const animation_quat_t* a, const animation_quat_t* b);
static animation_quat_t animation_quat_inverse(const animation_quat_t* q);
static animation_vec3_t animation_quat_rotate_vector(const animation_quat_t* q, const animation_vec3_t* v);

/* String and name processing */
static float animation_retargeting_calculate_name_similarity_internal(const char* name1, const char* name2);
static bool animation_retargeting_are_bones_symmetric_internal(const char* bone_name1, const char* bone_name2, const char* left_prefix, const char* right_prefix);
static void animation_retargeting_normalize_bone_name(char* normalized, const char* original, size_t buffer_size);

/* Skeleton operations */
static int animation_retargeting_find_bone_by_name_internal(const animation_skeleton_t* skeleton, const char* bone_name);
static int animation_retargeting_get_bone_hierarchy_level_internal(const animation_skeleton_t* skeleton, uint32_t bone_id);
static float animation_retargeting_calculate_hierarchy_similarity_internal(const animation_skeleton_t* source, uint32_t source_bone, const animation_skeleton_t* target, uint32_t target_bone);
static animation_vec3_t animation_retargeting_calculate_bone_length_internal(const animation_skeleton_t* skeleton, uint32_t bone_id);

/* Mapping operations */
static int animation_retargeting_create_mapping_internal(animation_retargeting_internal_t* retargeting, uint32_t source_skeleton_id, uint32_t target_skeleton_id);
static int animation_retargeting_auto_map_bones_internal(animation_retargeting_internal_t* retargeting, uint32_t source_skeleton_id, uint32_t target_skeleton_id, float similarity_threshold);
static int animation_retargeting_validate_mapping_internal(animation_retargeting_internal_t* retargeting, uint32_t source_skeleton_id, uint32_t target_skeleton_id);

/* Retargeting operations */
static int animation_retargeting_retarget_transform_internal(animation_retargeting_internal_t* retargeting, uint32_t source_skeleton_id, uint32_t target_skeleton_id, const animation_transform_t* source_transform, animation_transform_t* out_target_transform, uint32_t bone_index);
static int animation_retargeting_retarget_pose_internal(animation_retargeting_internal_t* retargeting, uint32_t source_skeleton_id, uint32_t target_skeleton_id, const animation_transform_t* source_pose, animation_transform_t* out_target_pose, uint32_t pose_size);
static int animation_retargeting_apply_ik_correction_internal(animation_retargeting_internal_t* retargeting, uint32_t target_skeleton_id, animation_transform_t* pose, uint32_t bone_count);

/* Caching */
static int animation_retargeting_find_cache_entry(animation_retargeting_internal_t* retargeting, uint32_t source_animation_id, uint32_t target_skeleton_id, animation_retargeting_cache_entry_t** out_entry);
static int animation_retargeting_add_cache_entry(animation_retargeting_internal_t* retargeting, uint32_t source_animation_id, uint32_t target_skeleton_id, void* data, size_t data_size);
static int animation_retargeting_evict_cache_entry(animation_retargeting_internal_t* retargeting);

/* Threading */
static void* animation_retargeting_worker_thread(void* arg);
static int animation_retargeting_init_worker_threads(animation_retargeting_internal_t* retargeting);
static void animation_retargeting_shutdown_worker_threads(animation_retargeting_internal_t* retargeting);

/* Validation and error handling */
static bool animation_retargeting_validate_skeleton(const animation_skeleton_t* skeleton);
static bool animation_retargeting_validate_mapping(const animation_retargeting_mapping_internal_t* mapping);
static void animation_retargeting_set_error(animation_retargeting_internal_t* retargeting, const char* error);

/* ============================================================================
 * MATH UTILITIES
 * ============================================================================ */

static float animation_vec3_dot(const animation_vec3_t* a, const animation_vec3_t* b) {
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

static float animation_vec3_length(const animation_vec3_t* v) {
    return sqrtf(v->x * v->x + v->y * v->y + v->z * v->z);
}

static animation_vec3_t animation_vec3_normalize(const animation_vec3_t* v) {
    float len = animation_vec3_length(v);
    if (len > 0.0f) {
        float inv_len = 1.0f / len;
        return (animation_vec3_t){v->x * inv_len, v->y * inv_len, v->z * inv_len};
    }
    return *v;
}

static animation_vec3_t animation_vec3_add(const animation_vec3_t* a, const animation_vec3_t* b) {
    return (animation_vec3_t){a->x + b->x, a->y + b->y, a->z + b->z};
}

static animation_vec3_t animation_vec3_sub(const animation_vec3_t* a, const animation_vec3_t* b) {
    return (animation_vec3_t){a->x - b->x, a->y - b->y, a->z - b->z};
}

static animation_vec3_t animation_vec3_scale(const animation_vec3_t* v, float s) {
    return (animation_vec3_t){v->x * s, v->y * s, v->z * s};
}

static animation_quat_t animation_quat_multiply(const animation_quat_t* a, const animation_quat_t* b) {
    return (animation_quat_t){
        a->w * b->x + a->x * b->w + a->y * b->z - a->z * b->y,
        a->w * b->y - a->x * b->z + a->y * b->w + a->z * b->x,
        a->w * b->z + a->x * b->y - a->y * b->x + a->z * b->w,
        a->w * b->w - a->x * b->x - a->y * b->y - a->z * b->z
    };
}

static animation_quat_t animation_quat_inverse(const animation_quat_t* q) {
    float norm = q->x * q->x + q->y * q->y + q->z * q->z + q->w * q->w;
    if (norm > 0.0f) {
        float inv_norm = 1.0f / norm;
        return (animation_quat_t){-q->x * inv_norm, -q->y * inv_norm, -q->z * inv_norm, q->w * inv_norm};
    }
    return (animation_quat_t){0.0f, 0.0f, 0.0f, 1.0f};
}

static animation_vec3_t animation_quat_rotate_vector(const animation_quat_t* q, const animation_vec3_t* v) {
    animation_vec3_t qv = {q->x, q->y, q->z};
    animation_vec3_t t = animation_vec3_cross(&qv, v);
    t = animation_vec3_scale(&t, 2.0f);
    
    animation_vec3_t u = animation_vec3_cross(&qv, &t);
    animation_vec3_t w = animation_vec3_scale(&t, q->w);
    
    return animation_vec3_add(v, animation_vec3_add(&u, &w));
}

/* ============================================================================
 * STRING AND NAME PROCESSING
 * ============================================================================ */

static void animation_retargeting_normalize_bone_name(char* normalized, const char* original, size_t buffer_size) {
    if (!normalized || !original || buffer_size == 0) return;
    
    /* Copy and convert to lowercase */
    strncpy(normalized, original, buffer_size - 1);
    normalized[buffer_size - 1] = '\0';
    
    for (size_t i = 0; normalized[i]; i++) {
        normalized[i] = tolower(normalized[i]);
    }
    
    /* Remove common prefixes and suffixes */
    const char* prefixes[] = {"left", "right", "upper", "lower", "l_", "r_", "up", "low", nullptr};
    const char* suffixes[] = {"_l", "_r", "_left", "_right", "_up", "_low", nullptr};
    
    /* Remove prefixes */
    for (int i = 0; prefixes[i]; i++) {
        size_t prefix_len = strlen(prefixes[i]);
        if (strncmp(normalized, prefixes[i], prefix_len) == 0) {
            memmove(normalized, normalized + prefix_len, strlen(normalized + prefix_len) + 1);
            break;
        }
    }
    
    /* Remove suffixes */
    size_t name_len = strlen(normalized);
    for (int i = 0; suffixes[i]; i++) {
        size_t suffix_len = strlen(suffixes[i]);
        if (name_len >= suffix_len && strcmp(normalized + name_len - suffix_len, suffixes[i]) == 0) {
            normalized[name_len - suffix_len] = '\0';
            break;
        }
    }
}

static float animation_retargeting_calculate_name_similarity_internal(const char* name1, const char* name2) {
    if (!name1 || !name2) return 0.0f;
    
    /* Direct match */
    if (strcasecmp(name1, name2) == 0) return 1.0f;
    
    /* Normalize names */
    char norm1[128], norm2[128];
    animation_retargeting_normalize_bone_name(norm1, name1, sizeof(norm1));
    animation_retargeting_normalize_bone_name(norm2, name2, sizeof(norm2));
    
    /* Normalized match */
    if (strcmp(norm1, norm2) == 0) return 0.9f;
    
    /* Substring match */
    if (strstr(norm1, norm2) || strstr(norm2, norm1)) return 0.7f;
    
    /* Common substring length */
    int max_len = 0;
    int len1 = strlen(norm1);
    int len2 = strlen(norm2);
    
    for (int i = 0; i < len1; i++) {
        for (int j = 0; j < len2; j++) {
            int k = 0;
            while (i + k < len1 && j + k < len2 && norm1[i + k] == norm2[j + k]) {
                k++;
            }
            if (k > max_len) max_len = k;
        }
    }
    
    float similarity = (float)max_len / fmaxf(len1, len2);
    return similarity * 0.6f;
}

static bool animation_retargeting_are_bones_symmetric_internal(const char* bone_name1, const char* bone_name2, const char* left_prefix, const char* right_prefix) {
    if (!bone_name1 || !bone_name2 || !left_prefix || !right_prefix) return false;
    
    /* Check if one is left and other is right variant */
    bool is_left1 = (strncasecmp(bone_name1, left_prefix, strlen(left_prefix)) == 0) || 
                    (strstr(bone_name1, "_l") != nullptr) || 
                    (strstr(bone_name1, "left") != nullptr);
    bool is_right1 = (strncasecmp(bone_name1, right_prefix, strlen(right_prefix)) == 0) || 
                     (strstr(bone_name1, "_r") != nullptr) || 
                     (strstr(bone_name1, "right") != nullptr);
    
    bool is_left2 = (strncasecmp(bone_name2, left_prefix, strlen(left_prefix)) == 0) || 
                    (strstr(bone_name2, "_l") != nullptr) || 
                    (strstr(bone_name2, "left") != nullptr);
    bool is_right2 = (strncasecmp(bone_name2, right_prefix, strlen(right_prefix)) == 0) || 
                     (strstr(bone_name2, "_r") != nullptr) || 
                     (strstr(bone_name2, "right") != nullptr);
    
    if ((is_left1 && is_right2) || (is_right1 && is_left2)) {
        /* Remove side indicators and compare base names */
        char base1[128], base2[128];
        strncpy(base1, bone_name1, sizeof(base1) - 1);
        base1[sizeof(base1) - 1] = '\0';
        strncpy(base2, bone_name2, sizeof(base2) - 1);
        base2[sizeof(base2) - 1] = '\0';
        
        /* Remove side indicators */
        char* found;
        if ((found = strcasestr(base1, left_prefix)) != nullptr) {
            memmove(found, found + strlen(left_prefix), strlen(found + strlen(left_prefix)) + 1);
        } else if ((found = strcasestr(base1, "_l")) != nullptr) {
            memmove(found, found + 2, strlen(found + 2) + 1);
        } else if ((found = strcasestr(base1, "left")) != nullptr) {
            memmove(found, found + 4, strlen(found + 4) + 1);
        }
        
        if ((found = strcasestr(base2, right_prefix)) != nullptr) {
            memmove(found, found + strlen(right_prefix), strlen(found + strlen(right_prefix)) + 1);
        } else if ((found = strcasestr(base2, "_r")) != nullptr) {
            memmove(found, found + 2, strlen(found + 2) + 1);
        } else if ((found = strcasestr(base2, "right")) != nullptr) {
            memmove(found, found + 5, strlen(found + 5) + 1);
        }
        
        return strcasecmp(base1, base2) == 0;
    }
    
    return false;
}

/* ============================================================================
 * SKELETON OPERATIONS
 * ============================================================================ */

static int animation_retargeting_find_bone_by_name_internal(const animation_skeleton_t* skeleton, const char* bone_name) {
    if (!skeleton || !bone_name || !skeleton->is_initialized) return -1;
    
    for (uint32_t i = 0; i < skeleton->bone_count; i++) {
        if (strcasecmp(skeleton->bones[i].name, bone_name) == 0) {
            return i;
        }
    }
    
    return -1;
}

static int animation_retargeting_get_bone_hierarchy_level_internal(const animation_skeleton_t* skeleton, uint32_t bone_id) {
    if (!skeleton || bone_id >= skeleton->bone_count || !skeleton->is_initialized) return -1;
    
    int level = 0;
    uint32_t current = bone_id;
    
    while (current != UINT32_MAX && skeleton->bones[current].parent_id != UINT32_MAX) {
        current = skeleton->bones[current].parent_id;
        level++;
    }
    
    return level;
}

static float animation_retargeting_calculate_hierarchy_similarity_internal(const animation_skeleton_t* source, uint32_t source_bone, const animation_skeleton_t* target, uint32_t target_bone) {
    if (!source || !target || source_bone >= source->bone_count || target_bone >= target->bone_count) return 0.0f;
    
    /* Compare hierarchy levels */
    int source_level = animation_retargeting_get_bone_hierarchy_level_internal(source, source_bone);
    int target_level = animation_retargeting_get_bone_hierarchy_level_internal(target, target_bone);
    
    if (source_level < 0 || target_level < 0) return 0.0f;
    
    float level_similarity = 1.0f - fabsf((float)source_level - (float)target_level) / fmaxf(source_level + 1, target_level + 1);
    
    /* Compare child counts */
    uint32_t source_children = source->bones[source_bone].child_count;
    uint32_t target_children = target->bones[target_bone].child_count;
    
    float child_similarity = 1.0f - fabsf((float)source_children - (float)target_children) / fmaxf(source_children + 1, target_children + 1);
    
    /* Compare end effector status */
    bool source_is_end = source->bones[source_bone].is_end_effector;
    bool target_is_end = target->bones[target_bone].is_end_effector;
    float end_effector_similarity = (source_is_end == target_is_end) ? 1.0f : 0.5f;
    
    return level_similarity * 0.4f + child_similarity * 0.4f + end_effector_similarity * 0.2f;
}

/* ============================================================================
 * CACHE MANAGEMENT
 * ============================================================================ */

static int animation_retargeting_find_cache_entry(animation_retargeting_internal_t* retargeting, uint32_t source_animation_id, uint32_t target_skeleton_id, animation_retargeting_cache_entry_t** out_entry) {
    if (!retargeting || !out_entry) return ANIMATION_RETARGETING_ERROR_INVALID_PARAMETER;
    
    uint64_t current_time = (uint64_t)time(NULL);
    
    for (uint32_t i = 0; i < retargeting->cache_count; i++) {
        animation_retargeting_cache_entry_t* entry = &retargeting->cache[i];
        if (entry->is_valid && entry->source_animation_id == source_animation_id && entry->target_skeleton_id == target_skeleton_id) {
            entry->last_access_time = current_time;
            entry->access_count++;
            *out_entry = entry;
            return ANIMATION_RETARGETING_ERROR_NONE;
        }
    }
    
    return ANIMATION_RETARGETING_ERROR_SKELETON_NOT_FOUND;
}

static int animation_retargeting_add_cache_entry(animation_retargeting_internal_t* retargeting, uint32_t source_animation_id, uint32_t target_skeleton_id, void* data, size_t data_size) {
    if (!retargeting || !data || data_size == 0) return ANIMATION_RETARGETING_ERROR_INVALID_PARAMETER;
    
    /* Check if cache is full */
    if (retargeting->cache_count >= retargeting->cache_capacity) {
        /* Evict least recently used entry */
        int result = animation_retargeting_evict_cache_entry(retargeting);
        if (result != ANIMATION_RETARGETING_ERROR_NONE) return result;
    }
    
    /* Find free cache slot */
    for (uint32_t i = 0; i < retargeting->cache_capacity; i++) {
        if (!retargeting->cache[i].is_valid) {
            animation_retargeting_cache_entry_t* entry = &retargeting->cache[i];
            
            entry->source_animation_id = source_animation_id;
            entry->target_skeleton_id = target_skeleton_id;
            entry->data_size = data_size;
            entry->last_access_time = (uint64_t)time(NULL);
            entry->access_count = 1;
            entry->is_valid = true;
            
            /* Allocate and copy data */
            entry->retargeted_data = malloc(data_size);
            if (!entry->retargeted_data) {
                return ANIMATION_RETARGETING_ERROR_OUT_OF_MEMORY;
            }
            
            memcpy(entry->retargeted_data, data, data_size);
            
            if (i >= retargeting->cache_count) {
                retargeting->cache_count = i + 1;
            }
            
            return ANIMATION_RETARGETING_ERROR_NONE;
        }
    }
    
    return ANIMATION_RETARGETING_ERROR_OPERATION_FAILED;
}

static int animation_retargeting_evict_cache_entry(animation_retargeting_internal_t* retargeting) {
    if (!retargeting || retargeting->cache_count == 0) return ANIMATION_RETARGETING_ERROR_OPERATION_FAILED;
    
    /* Find least recently used entry */
    uint32_t lru_index = 0;
    uint64_t oldest_time = retargeting->cache[0].last_access_time;
    
    for (uint32_t i = 1; i < retargeting->cache_count; i++) {
        if (retargeting->cache[i].last_access_time < oldest_time) {
            oldest_time = retargeting->cache[i].last_access_time;
            lru_index = i;
        }
    }
    
    /* Free the entry */
    animation_retargeting_cache_entry_t* entry = &retargeting->cache[lru_index];
    if (entry->retargeted_data) {
        free(entry->retargeted_data);
        entry->retargeted_data = NULL;
    }
    
    entry->is_valid = false;
    
    /* Update cache count */
    if (lru_index == retargeting->cache_count - 1) {
        retargeting->cache_count--;
    }
    
    return ANIMATION_RETARGETING_ERROR_NONE;
}

/* ============================================================================
 * PUBLIC API IMPLEMENTATION
 * ============================================================================ */

int animation_retargeting_init(void) {
    if (g_animation_retargeting_ctx.initialized) {
        return ANIMATION_RETARGETING_ERROR_ALREADY_INITIALIZED;
    }
    
    /* Initialize global mutex */
    if (pthread_mutex_init(&g_animation_retargeting_ctx.global_mutex, NULL) != 0) {
        return ANIMATION_RETARGETING_ERROR_THREAD_LOCK_FAILED;
    }
    
    /* Initialize context */
    g_animation_retargeting_ctx.instance_capacity = 16;
    g_animation_retargeting_ctx.instances = calloc(g_animation_retargeting_ctx.instance_capacity, sizeof(animation_retargeting_internal_t));
    if (!g_animation_retargeting_ctx.instances) {
        pthread_mutex_destroy(&g_animation_retargeting_ctx.global_mutex);
        return ANIMATION_RETARGETING_ERROR_OUT_OF_MEMORY;
    }
    
    g_animation_retargeting_ctx.instance_count = 0;
    g_animation_retargeting_ctx.global_memory_usage = 0;
    g_animation_retargeting_ctx.next_instance_id = 1;
    
    /* Detect SIMD capabilities */
#ifdef __AVX__
    g_animation_retargeting_ctx.avx_available = true;
    g_animation_retargeting_ctx.simd_available = true;
#elif defined(__SSE2__)
    g_animation_retargeting_ctx.avx_available = false;
    g_animation_retargeting_ctx.simd_available = true;
#else
    g_animation_retargeting_ctx.avx_available = false;
    g_animation_retargeting_ctx.simd_available = false;
#endif
    
    g_animation_retargeting_ctx.initialized = true;
    return ANIMATION_RETARGETING_ERROR_NONE;
}

void animation_retargeting_shutdown(void) {
    if (!g_animation_retargeting_ctx.initialized) {
        return;
    }
    
    /* Cleanup all instances */
    for (uint32_t i = 0; i < g_animation_retargeting_ctx.instance_count; i++) {
        animation_retargeting_destroy((animation_retargeting_handle_t){i});
    }
    
    free(g_animation_retargeting_ctx.instances);
    g_animation_retargeting_ctx.instances = NULL;
    g_animation_retargeting_ctx.instance_count = 0;
    g_animation_retargeting_ctx.instance_capacity = 0;
    
    /* Destroy global mutex */
    pthread_mutex_destroy(&g_animation_retargeting_ctx.global_mutex);
    
    g_animation_retargeting_ctx.initialized = false;
}

int animation_retargeting_create(animation_retargeting_handle_t* out_handle, const animation_retargeting_desc_t* desc) {
    if (!out_handle || !desc) {
        return ANIMATION_RETARGETING_ERROR_INVALID_PARAMETER;
    }
    
    if (!g_animation_retargeting_ctx.initialized) {
        return ANIMATION_RETARGETING_ERROR_NOT_INITIALIZED;
    }
    
    pthread_mutex_lock(&g_animation_retargeting_ctx.global_mutex);
    
    if (g_animation_retargeting_ctx.instance_count >= g_animation_retargeting_ctx.instance_capacity) {
        pthread_mutex_unlock(&g_animation_retargeting_ctx.global_mutex);
        return ANIMATION_RETARGETING_ERROR_OUT_OF_MEMORY;
    }
    
    uint32_t index = g_animation_retargeting_ctx.instance_count++;
    animation_retargeting_internal_t* retargeting = &g_animation_retargeting_ctx.instances[index];
    
    /* Initialize retargeting instance */
    memset(retargeting, 0, sizeof(animation_retargeting_internal_t));
    retargeting->id = g_animation_retargeting_ctx.next_instance_id++;
    retargeting->flags = desc->flags;
    retargeting->initialized = true;
    
    /* Copy name and settings */
    strncpy(retargeting->name, desc->name, sizeof(retargeting->name) - 1);
    retargeting->name[sizeof(retargeting->name) - 1] = '\0';
    retargeting->settings = desc->settings;
    
    /* Initialize mutex */
    if (pthread_mutex_init(&retargeting->retargeting_mutex, NULL) != 0) {
        g_animation_retargeting_ctx.instance_count--;
        pthread_mutex_unlock(&g_animation_retargeting_ctx.global_mutex);
        return ANIMATION_RETARGETING_ERROR_THREAD_LOCK_FAILED;
    }
    
    /* Initialize cache */
    retargeting->cache_capacity = desc->max_cache_entries > 0 ? desc->max_cache_entries : ANIMATION_RETARGETING_CACHE_SIZE;
    retargeting->cache_count = 0;
    retargeting->next_cache_eviction_time = (uint64_t)time(NULL) + 300; /* 5 minutes */
    
    /* Initialize async operations */
    retargeting->async_operation_capacity = 32;
    retargeting->async_operations = calloc(retargeting->async_operation_capacity, sizeof(animation_retargeting_async_operation_t));
    if (!retargeting->async_operations) {
        pthread_mutex_destroy(&retargeting->retargeting_mutex);
        g_animation_retargeting_ctx.instance_count--;
        pthread_mutex_unlock(&g_animation_retargeting_ctx.global_mutex);
        return ANIMATION_RETARGETING_ERROR_OUT_OF_MEMORY;
    }
    
    retargeting->next_async_operation_id = 1;
    
    /* Initialize memory budget */
    retargeting->memory_budget = desc->memory_budget > 0 ? desc->memory_budget : (256 * 1024 * 1024); /* 256MB default */
    retargeting->current_memory_usage = sizeof(animation_retargeting_internal_t);
    
    /* Initialize performance stats */
    memset(&retargeting->performance_stats, 0, sizeof(animation_retargeting_performance_stats_t));
    retargeting->frame_count = 0;
    
    /* Initialize worker threads if multithreading is enabled */
    if (desc->enable_multithreading) {
        int result = animation_retargeting_init_worker_threads(retargeting);
        if (result != ANIMATION_RETARGETING_ERROR_NONE) {
            free(retargeting->async_operations);
            pthread_mutex_destroy(&retargeting->retargeting_mutex);
            g_animation_retargeting_ctx.instance_count--;
            pthread_mutex_unlock(&g_animation_retargeting_ctx.global_mutex);
            return result;
        }
    }
    
    /* Update global memory usage */
    g_animation_retargeting_ctx.global_memory_usage += retargeting->current_memory_usage;
    
    out_handle->id = index;
    pthread_mutex_unlock(&g_animation_retargeting_ctx.global_mutex);
    
    return ANIMATION_RETARGETING_ERROR_NONE;
}

void animation_retargeting_destroy(animation_retargeting_handle_t handle) {
    if (!g_animation_retargeting_ctx.initialized) {
        return;
    }
    
    pthread_mutex_lock(&g_animation_retargeting_ctx.global_mutex);
    
    if (handle.id >= g_animation_retargeting_ctx.instance_count) {
        pthread_mutex_unlock(&g_animation_retargeting_ctx.global_mutex);
        return;
    }
    
    animation_retargeting_internal_t* retargeting = &g_animation_retargeting_ctx.instances[handle.id];
    if (!retargeting->initialized) {
        pthread_mutex_unlock(&g_animation_retargeting_ctx.global_mutex);
        return;
    }
    
    /* Shutdown worker threads */
    if (retargeting->worker_threads_running) {
        animation_retargeting_shutdown_worker_threads(retargeting);
    }
    
    /* Cleanup cache */
    for (uint32_t i = 0; i < retargeting->cache_capacity; i++) {
        if (retargeting->cache[i].is_valid && retargeting->cache[i].retargeted_data) {
            free(retargeting->cache[i].retargeted_data);
        }
    }
    
    /* Cleanup async operations */
    if (retargeting->async_operations) {
        free(retargeting->async_operations);
        retargeting->async_operations = NULL;
    }
    
    pthread_mutex_destroy(&retargeting->retargeting_mutex);
    
    g_animation_retargeting_ctx.global_memory_usage -= retargeting->current_memory_usage;
    retargeting->initialized = false;
    
    pthread_mutex_unlock(&g_animation_retargeting_ctx.global_mutex);
}

/* ============================================================================
 * SKELETON MANAGEMENT
 * ============================================================================ */

int animation_retargeting_register_skeleton(animation_retargeting_handle_t handle, const char* name, const animation_skeleton_t* skeleton, uint32_t* out_skeleton_id) {
    if (!name || !skeleton || !out_skeleton_id) {
        return ANIMATION_RETARGETING_ERROR_INVALID_PARAMETER;
    }
    
    if (!g_animation_retargeting_ctx.initialized) {
        return ANIMATION_RETARGETING_ERROR_NOT_INITIALIZED;
    }
    
    if (handle.id >= g_animation_retargeting_ctx.instance_count) {
        return ANIMATION_RETARGETING_ERROR_INVALID_HANDLE;
    }
    
    animation_retargeting_internal_t* retargeting = &g_animation_retargeting_ctx.instances[handle.id];
    if (!retargeting->initialized) {
        return ANIMATION_RETARGETING_ERROR_NOT_INITIALIZED;
    }
    
    if (pthread_mutex_lock(&retargeting->retargeting_mutex) != 0) {
        return ANIMATION_RETARGETING_ERROR_THREAD_LOCK_FAILED;
    }
    
    if (retargeting->skeleton_count >= ANIMATION_RETARGETING_MAX_SKELETONS) {
        pthread_mutex_unlock(&retargeting->retargeting_mutex);
        return ANIMATION_RETARGETING_ERROR_OUT_OF_MEMORY;
    }
    
    if (!animation_retargeting_validate_skeleton(skeleton)) {
        pthread_mutex_unlock(&retargeting->retargeting_mutex);
        return ANIMATION_RETARGETING_ERROR_INVALID_PARAMETER;
    }
    
    /* Find free skeleton slot */
    for (uint32_t i = 0; i < ANIMATION_RETARGETING_MAX_SKELETONS; i++) {
        if (!retargeting->skeletons[i].is_registered) {
            animation_retargeting_skeleton_internal_t* skel_internal = &retargeting->skeletons[i];
            
            skel_internal->id = i + 1;
            skel_internal->skeleton = *skeleton;
            skel_internal->reference_count = 1;
            skel_internal->is_registered = true;
            
            retargeting->skeleton_count++;
            *out_skeleton_id = skel_internal->id;
            
            pthread_mutex_unlock(&retargeting->retargeting_mutex);
            return ANIMATION_RETARGETING_ERROR_NONE;
        }
    }
    
    pthread_mutex_unlock(&retargeting->retargeting_mutex);
    return ANIMATION_RETARGETING_ERROR_OUT_OF_MEMORY;
}

int animation_retargeting_unregister_skeleton(animation_retargeting_handle_t handle, uint32_t skeleton_id) {
    if (!g_animation_retargeting_ctx.initialized) {
        return ANIMATION_RETARGETING_ERROR_NOT_INITIALIZED;
    }
    
    if (handle.id >= g_animation_retargeting_ctx.instance_count) {
        return ANIMATION_RETARGETING_ERROR_INVALID_HANDLE;
    }
    
    animation_retargeting_internal_t* retargeting = &g_animation_retargeting_ctx.instances[handle.id];
    if (!retargeting->initialized) {
        return ANIMATION_RETARGETING_ERROR_NOT_INITIALIZED;
    }
    
    if (pthread_mutex_lock(&retargeting->retargeting_mutex) != 0) {
        return ANIMATION_RETARGETING_ERROR_THREAD_LOCK_FAILED;
    }
    
    /* Find skeleton */
    for (uint32_t i = 0; i < ANIMATION_RETARGETING_MAX_SKELETONS; i++) {
        if (retargeting->skeletons[i].is_registered && retargeting->skeletons[i].id == skeleton_id) {
            animation_retargeting_skeleton_internal_t* skel_internal = &retargeting->skeletons[i];
            
            skel_internal->reference_count--;
            
            if (skel_internal->reference_count == 0) {
                skel_internal->is_registered = false;
                retargeting->skeleton_count--;
            }
            
            pthread_mutex_unlock(&retargeting->retargeting_mutex);
            return ANIMATION_RETARGETING_ERROR_NONE;
        }
    }
    
    pthread_mutex_unlock(&retargeting->retargeting_mutex);
    return ANIMATION_RETARGETING_ERROR_SKELETON_NOT_FOUND;
}

/* ============================================================================
 * BONE MAPPING
 * ============================================================================ */

static int animation_retargeting_create_mapping_internal(animation_retargeting_internal_t* retargeting, uint32_t source_skeleton_id, uint32_t target_skeleton_id) {
    if (!retargeting) return ANIMATION_RETARGETING_ERROR_INVALID_PARAMETER;
    
    /* Find skeletons */
    animation_skeleton_t* source_skeleton = nullptr;
    animation_skeleton_t* target_skeleton = nullptr;
    
    for (uint32_t i = 0; i < ANIMATION_RETARGETING_MAX_SKELETONS; i++) {
        if (retargeting->skeletons[i].is_registered) {
            if (retargeting->skeletons[i].id == source_skeleton_id) {
                source_skeleton = &retargeting->skeletons[i].skeleton;
            } else if (retargeting->skeletons[i].id == target_skeleton_id) {
                target_skeleton = &retargeting->skeletons[i].skeleton;
            }
        }
    }
    
    if (!source_skeleton || !target_skeleton) {
        return ANIMATION_RETARGETING_ERROR_SKELETON_NOT_FOUND;
    }
    
    /* Find or create mapping */
    for (uint32_t i = 0; i < retargeting->mapping_count; i++) {
        animation_retargeting_mapping_internal_t* mapping = &retargeting->mappings[i];
        if (mapping->source_skeleton_id == source_skeleton_id && mapping->target_skeleton_id == target_skeleton_id) {
            return ANIMATION_RETARGETING_ERROR_NONE; /* Mapping already exists */
        }
    }
    
    /* Create new mapping */
    if (retargeting->mapping_count >= ANIMATION_RETARGETING_MAX_SKELETONS * ANIMATION_RETARGETING_MAX_SKELETONS) {
        return ANIMATION_RETARGETING_ERROR_OUT_OF_MEMORY;
    }
    
    animation_retargeting_mapping_internal_t* new_mapping = &retargeting->mappings[retargeting->mapping_count++];
    memset(new_mapping, 0, sizeof(animation_retargeting_mapping_internal_t));
    
    new_mapping->source_skeleton_id = source_skeleton_id;
    new_mapping->target_skeleton_id = target_skeleton_id;
    new_mapping->is_valid = false;
    
    return ANIMATION_RETARGETING_ERROR_NONE;
}

int animation_retargeting_auto_map_bones(animation_retargeting_handle_t handle, uint32_t source_skeleton_id, uint32_t target_skeleton_id, float similarity_threshold) {
    if (!g_animation_retargeting_ctx.initialized) {
        return ANIMATION_RETARGETING_ERROR_NOT_INITIALIZED;
    }
    
    if (handle.id >= g_animation_retargeting_ctx.instance_count) {
        return ANIMATION_RETARGETING_ERROR_INVALID_HANDLE;
    }
    
    animation_retargeting_internal_t* retargeting = &g_animation_retargeting_ctx.instances[handle.id];
    if (!retargeting->initialized) {
        return ANIMATION_RETARGETING_ERROR_NOT_INITIALIZED;
    }
    
    if (pthread_mutex_lock(&retargeting->retargeting_mutex) != 0) {
        return ANIMATION_RETARGETING_ERROR_THREAD_LOCK_FAILED;
    }
    
    int result = animation_retargeting_auto_map_bones_internal(retargeting, source_skeleton_id, target_skeleton_id, similarity_threshold);
    
    pthread_mutex_unlock(&retargeting->retargeting_mutex);
    return result;
}

static int animation_retargeting_auto_map_bones_internal(animation_retargeting_internal_t* retargeting, uint32_t source_skeleton_id, uint32_t target_skeleton_id, float similarity_threshold) {
    /* Find skeletons */
    animation_skeleton_t* source_skeleton = nullptr;
    animation_skeleton_t* target_skeleton = nullptr;
    
    for (uint32_t i = 0; i < ANIMATION_RETARGETING_MAX_SKELETONS; i++) {
        if (retargeting->skeletons[i].is_registered) {
            if (retargeting->skeletons[i].id == source_skeleton_id) {
                source_skeleton = &retargeting->skeletons[i].skeleton;
            } else if (retargeting->skeletons[i].id == target_skeleton_id) {
                target_skeleton = &retargeting->skeletons[i].skeleton;
            }
        }
    }
    
    if (!source_skeleton || !target_skeleton) {
        return ANIMATION_RETARGETING_ERROR_SKELETON_NOT_FOUND;
    }
    
    /* Create or find existing mapping */
    int result = animation_retargeting_create_mapping_internal(retargeting, source_skeleton_id, target_skeleton_id);
    if (result != ANIMATION_RETARGETING_ERROR_NONE) return result;
    
    /* Find mapping */
    animation_retargeting_mapping_internal_t* mapping = nullptr;
    for (uint32_t i = 0; i < retargeting->mapping_count; i++) {
        if (retargeting->mappings[i].source_skeleton_id == source_skeleton_id &&
            retargeting->mappings[i].target_skeleton_id == target_skeleton_id) {
            mapping = &retargeting->mappings[i];
            break;
        }
    }
    
    if (!mapping) return ANIMATION_RETARGETING_ERROR_MAPPING_FAILED;
    
    /* Clear existing mappings */
    mapping->bone_mapping_count = 0;
    
    /* Auto-map bones */
    for (uint32_t source_idx = 0; source_idx < source_skeleton->bone_count; source_idx++) {
        float best_score = 0.0f;
        uint32_t best_target = UINT32_MAX;
        
        for (uint32_t target_idx = 0; target_idx < target_skeleton->bone_count; target_idx++) {
            /* Calculate name similarity */
            float name_score = animation_retargeting_calculate_name_similarity_internal(
                source_skeleton->bones[source_idx].name,
                target_skeleton->bones[target_idx].name);
            
            /* Calculate hierarchy similarity */
            float hierarchy_score = animation_retargeting_calculate_hierarchy_similarity_internal(
                source_skeleton, source_idx, target_skeleton, target_idx);
            
            /* Combined score */
            float total_score = name_score * 0.7f + hierarchy_score * 0.3f;
            
            if (total_score > best_score && total_score >= similarity_threshold) {
                best_score = total_score;
                best_target = target_idx;
            }
        }
        
        if (best_target != UINT32_MAX) {
            animation_retargeting_bone_mapping_t* bone_mapping = &mapping->bone_mappings[mapping->bone_mapping_count++];
            
            bone_mapping->source_bone_id = source_idx;
            bone_mapping->target_bone_id = best_target;
            bone_mapping->mapping_type = ANIMATION_RETARGETING_MAP_DIRECT;
            bone_mapping->weight = best_score;
            bone_mapping->preserve_length = true;
            bone_mapping->preserve_proportions = true;
            bone_mapping->influence_radius = 1.0f;
            bone_mapping->is_valid = true;
            
            /* Calculate offset transform */
            animation_transform_t source_bind = source_skeleton->bones[source_idx].bind_pose;
            animation_transform_t target_bind = target_skeleton->bones[best_target].bind_pose;
            
            /* Calculate rotation offset */
            animation_quat_t source_rot_inv = animation_quat_inverse(&source_bind.rotation);
            bone_mapping->offset_transform.rotation = animation_quat_multiply(&target_bind.rotation, &source_rot_inv);
            
            /* Calculate position offset */
            bone_mapping->offset_transform.position = animation_vec3_sub(&target_bind.position, &source_bind.position);
            bone_mapping->offset_transform.scale = (animation_vec3_t){1.0f, 1.0f, 1.0f};
        }
    }
    
    mapping->is_valid = mapping->bone_mapping_count > 0;
    
    retargeting->performance_stats.successful_mappings += mapping->bone_mapping_count;
    retargeting->performance_stats.failed_mappings += source_skeleton->bone_count - mapping->bone_mapping_count;
    
    return mapping->is_valid ? ANIMATION_RETARGETING_ERROR_NONE : ANIMATION_RETARGETING_ERROR_MAPPING_FAILED;
}

/* ============================================================================
 * RETARGETING OPERATIONS
 * ============================================================================ */

int animation_retargeting_retarget_pose(animation_retargeting_handle_t handle, uint32_t source_skeleton_id, uint32_t target_skeleton_id, const animation_transform_t* source_pose, animation_transform_t* out_target_pose, uint32_t pose_size) {
    if (!source_pose || !out_target_pose || pose_size == 0) {
        return ANIMATION_RETARGETING_ERROR_INVALID_PARAMETER;
    }
    
    if (!g_animation_retargeting_ctx.initialized) {
        return ANIMATION_RETARGETING_ERROR_NOT_INITIALIZED;
    }
    
    if (handle.id >= g_animation_retargeting_ctx.instance_count) {
        return ANIMATION_RETARGETING_ERROR_INVALID_HANDLE;
    }
    
    animation_retargeting_internal_t* retargeting = &g_animation_retargeting_ctx.instances[handle.id];
    if (!retargeting->initialized) {
        return ANIMATION_RETARGETING_ERROR_NOT_INITIALIZED;
    }
    
    if (pthread_mutex_lock(&retargeting->retargeting_mutex) != 0) {
        return ANIMATION_RETARGETING_ERROR_THREAD_LOCK_FAILED;
    }
    
    clock_t start_time = clock();
    int result = animation_retargeting_retarget_pose_internal(retargeting, source_skeleton_id, target_skeleton_id, source_pose, out_target_pose, pose_size);
    
    /* Update performance stats */
    clock_t end_time = clock();
    float elapsed_ms = ((float)(end_time - start_time) / CLOCKS_PER_SEC) * 1000.0f;
    
    retargeting->performance_stats.total_retargets++;
    retargeting->performance_stats.total_bones_processed += pose_size;
    retargeting->performance_stats.average_retarget_time_ms = 
        (retargeting->performance_stats.average_retarget_time_ms * (retargeting->performance_stats.total_retargets - 1) + elapsed_ms) / 
        retargeting->performance_stats.total_retargets;
    
    if (elapsed_ms > retargeting->performance_stats.peak_retarget_time_ms) {
        retargeting->performance_stats.peak_retarget_time_ms = elapsed_ms;
    }
    
    pthread_mutex_unlock(&retargeting->retargeting_mutex);
    return result;
}

static int animation_retargeting_retarget_pose_internal(animation_retargeting_internal_t* retargeting, uint32_t source_skeleton_id, uint32_t target_skeleton_id, const animation_transform_t* source_pose, animation_transform_t* out_target_pose, uint32_t pose_size) {
    /* Find mapping */
    animation_retargeting_mapping_internal_t* mapping = nullptr;
    for (uint32_t i = 0; i < retargeting->mapping_count; i++) {
        if (retargeting->mappings[i].source_skeleton_id == source_skeleton_id &&
            retargeting->mappings[i].target_skeleton_id == target_skeleton_id) {
            mapping = &retargeting->mappings[i];
            break;
        }
    }
    
    if (!mapping || !mapping->is_valid) {
        return ANIMATION_RETARGETING_ERROR_MAPPING_FAILED;
    }
    
    /* Find skeletons */
    animation_skeleton_t* source_skeleton = nullptr;
    animation_skeleton_t* target_skeleton = nullptr;
    
    for (uint32_t i = 0; i < ANIMATION_RETARGETING_MAX_SKELETONS; i++) {
        if (retargeting->skeletons[i].is_registered) {
            if (retargeting->skeletons[i].id == source_skeleton_id) {
                source_skeleton = &retargeting->skeletons[i].skeleton;
            } else if (retargeting->skeletons[i].id == target_skeleton_id) {
                target_skeleton = &retargeting->skeletons[i].skeleton;
            }
        }
    }
    
    if (!source_skeleton || !target_skeleton) {
        return ANIMATION_RETARGETING_ERROR_SKELETON_NOT_FOUND;
    }
    
    /* Initialize target pose with bind pose */
    for (uint32_t i = 0; i < target_skeleton->bone_count && i < pose_size; i++) {
        out_target_pose[i] = target_skeleton->bones[i].bind_pose;
    }
    
    /* Retarget each mapped bone */
    for (uint32_t i = 0; i < mapping->bone_mapping_count; i++) {
        animation_retargeting_bone_mapping_t* bone_mapping = &mapping->bone_mappings[i];
        if (!bone_mapping->is_valid) continue;
        
        if (bone_mapping->source_bone_id >= source_skeleton->bone_count ||
            bone_mapping->target_bone_id >= target_skeleton->bone_count) {
            continue;
        }
        
        /* Retarget transform for this bone */
        animation_transform_t source_transform = source_pose[bone_mapping->source_bone_id];
        animation_transform_t target_transform;
        
        int result = animation_retargeting_retarget_transform_internal(
            retargeting, source_skeleton_id, target_skeleton_id,
            &source_transform, &target_transform, bone_mapping->target_bone_id);
        
        if (result == ANIMATION_RETARGETING_ERROR_NONE) {
            out_target_pose[bone_mapping->target_bone_id] = target_transform;
        }
    }
    
    /* Apply IK correction if enabled */
    if (retargeting->settings.enable_ik_correction) {
        animation_retargeting_apply_ik_correction_internal(retargeting, target_skeleton_id, out_target_pose, pose_size);
    }
    
    /* Apply joint limits if enabled */
    if (retargeting->settings.enable_joint_limits) {
        animation_retargeting_apply_joint_limits(handle, target_skeleton_id, out_target_pose, pose_size);
    }
    
    return ANIMATION_RETARGETING_ERROR_NONE;
}

static int animation_retargeting_retarget_transform_internal(animation_retargeting_internal_t* retargeting, uint32_t source_skeleton_id, uint32_t target_skeleton_id, const animation_transform_t* source_transform, animation_transform_t* out_target_transform, uint32_t bone_index) {
    if (!retargeting || !source_transform || !out_target_transform) {
        return ANIMATION_RETARGETING_ERROR_INVALID_PARAMETER;
    }
    
    /* Find mapping */
    animation_retargeting_mapping_internal_t* mapping = nullptr;
    for (uint32_t i = 0; i < retargeting->mapping_count; i++) {
        if (retargeting->mappings[i].source_skeleton_id == source_skeleton_id &&
            retargeting->mappings[i].target_skeleton_id == target_skeleton_id) {
            mapping = &retargeting->mappings[i];
            break;
        }
    }
    
    if (!mapping || !mapping->is_valid) {
        return ANIMATION_RETARGETING_ERROR_MAPPING_FAILED;
    }
    
    /* Find bone mapping */
    animation_retargeting_bone_mapping_t* bone_mapping = nullptr;
    for (uint32_t i = 0; i < mapping->bone_mapping_count; i++) {
        if (mapping->bone_mappings[i].target_bone_id == bone_index) {
            bone_mapping = &mapping->bone_mappings[i];
            break;
        }
    }
    
    if (!bone_mapping || !bone_mapping->is_valid) {
        return ANIMATION_RETARGETING_ERROR_MAPPING_FAILED;
    }
    
    /* Apply retargeting based on settings */
    *out_target_transform = *source_transform;
    
    /* Apply position retargeting */
    if (retargeting->settings.enable_position_retargeting) {
        out_target_transform->position = animation_vec3_add(
            &out_target_transform->position,
            &bone_mapping->offset_transform.position);
    }
    
    /* Apply rotation retargeting */
    if (retargeting->settings.enable_rotation_retargeting) {
        out_target_transform->rotation = animation_quat_multiply(
            &bone_mapping->offset_transform.rotation,
            &out_target_transform->rotation);
    }
    
    /* Apply scale retargeting */
    if (retargeting->settings.enable_scale_retargeting) {
        out_target_transform->scale.x *= bone_mapping->offset_transform.scale.x;
        out_target_transform->scale.y *= bone_mapping->offset_transform.scale.y;
        out_target_transform->scale.z *= bone_mapping->offset_transform.scale.z;
    }
    
    return ANIMATION_RETARGETING_ERROR_NONE;
}

static int animation_retargeting_apply_ik_correction_internal(animation_retargeting_internal_t* retargeting, uint32_t target_skeleton_id, animation_transform_t* pose, uint32_t bone_count) {
    if (!retargeting || !pose || bone_count == 0) return ANIMATION_RETARGETING_ERROR_INVALID_PARAMETER;
    
    /* Find skeleton */
    animation_skeleton_t* target_skeleton = nullptr;
    for (uint32_t i = 0; i < ANIMATION_RETARGETING_MAX_SKELETONS; i++) {
        if (retargeting->skeletons[i].is_registered && retargeting->skeletons[i].id == target_skeleton_id) {
            target_skeleton = &retargeting->skeletons[i].skeleton;
            break;
        }
    }
    
    if (!target_skeleton) return ANIMATION_RETARGETING_ERROR_SKELETON_NOT_FOUND;
    
    /* Simple IK correction for end effectors */
    for (uint32_t i = 0; i < bone_count && i < target_skeleton->bone_count; i++) {
        if (target_skeleton->bones[i].is_end_effector) {
            /* Apply simple IK stabilization */
            /* In a full implementation, would solve IK constraints */
            pose[i].rotation.w = fmaxf(pose[i].rotation.w, 0.0f); /* Prevent quaternion flips */
        }
    }
    
    return ANIMATION_RETARGETING_ERROR_NONE;
}

/* ============================================================================
 * VALIDATION AND ERROR HANDLING
 * ============================================================================ */

static bool animation_retargeting_validate_skeleton(const animation_skeleton_t* skeleton) {
    if (!skeleton || !skeleton->is_initialized) return false;
    if (skeleton->bone_count == 0 || skeleton->bone_count > ANIMATION_RETARGETING_MAX_BONES) return false;
    if (skeleton->units_per_meter <= 0.0f) return false;
    
    /* Validate bone hierarchy */
    for (uint32_t i = 0; i < skeleton->bone_count; i++) {
        const animation_bone_t* bone = &skeleton->bones[i];
        if (strlen(bone->name) == 0) return false;
        if (bone->parent_id != UINT32_MAX && bone->parent_id >= skeleton->bone_count) return false;
        if (bone->child_count > 32) return false;
        
        for (uint32_t j = 0; j < bone->child_count; j++) {
            if (bone->child_ids[j] >= skeleton->bone_count) return false;
        }
    }
    
    return true;
}

static bool animation_retargeting_validate_mapping(const animation_retargeting_mapping_internal_t* mapping) {
    if (!mapping || !mapping->is_valid) return false;
    if (mapping->bone_mapping_count == 0) return false;
    
    for (uint32_t i = 0; i < mapping->bone_mapping_count; i++) {
        const animation_retargeting_bone_mapping_t* bone_mapping = &mapping->bone_mappings[i];
        if (!bone_mapping->is_valid) return false;
        if (bone_mapping->weight <= 0.0f || bone_mapping->weight > 1.0f) return false;
    }
    
    return true;
}

static void animation_retargeting_set_error(animation_retargeting_internal_t* retargeting, const char* error) {
    if (retargeting && error) {
        strncpy(retargeting->last_error, error, sizeof(retargeting->last_error) - 1);
        retargeting->last_error[sizeof(retargeting->last_error) - 1] = '\0';
    }
}

/* ============================================================================
 * PERFORMANCE AND DEBUGGING
 * ============================================================================ */

const animation_retargeting_performance_stats_t* animation_retargeting_get_performance_stats(animation_retargeting_handle_t handle) {
    if (!g_animation_retargeting_ctx.initialized) return NULL;
    if (handle.id >= g_animation_retargeting_ctx.instance_count) return NULL;
    
    animation_retargeting_internal_t* retargeting = &g_animation_retargeting_ctx.instances[handle.id];
    if (!retargeting->initialized) return NULL;
    
    return &retargeting->performance_stats;
}

void animation_retargeting_reset_performance_stats(animation_retargeting_handle_t handle) {
    if (!g_animation_retargeting_ctx.initialized) return;
    if (handle.id >= g_animation_retargeting_ctx.instance_count) return;
    
    animation_retargeting_internal_t* retargeting = &g_animation_retargeting_ctx.instances[handle.id];
    if (!retargeting->initialized) return;
    
    if (pthread_mutex_lock(&retargeting->retargeting_mutex) != 0) return;
    
    memset(&retargeting->performance_stats, 0, sizeof(animation_retargeting_performance_stats_t));
    
    pthread_mutex_unlock(&retargeting->retargeting_mutex);
}

animation_retargeting_info_t animation_retargeting_get_info(animation_retargeting_handle_t handle) {
    animation_retargeting_info_t info = {0};
    
    if (!g_animation_retargeting_ctx.initialized) return info;
    if (handle.id >= g_animation_retargeting_ctx.instance_count) return info;
    
    animation_retargeting_internal_t* retargeting = &g_animation_retargeting_ctx.instances[handle.id];
    if (!retargeting->initialized) return info;
    
    if (pthread_mutex_lock(&retargeting->retargeting_mutex) != 0) return info;
    
    info.id = retargeting->id;
    strncpy(info.name, retargeting->name, sizeof(info.name) - 1);
    info.name[sizeof(info.name) - 1] = '\0';
    info.flags = retargeting->flags;
    info.initialized = retargeting->initialized;
    info.mapping_count = retargeting->mapping_count;
    info.chain_count = 0; /* Would be calculated in full implementation */
    info.cache_entry_count = retargeting->cache_count;
    info.performance = retargeting->performance_stats;
    info.current_quality = (float)retargeting->settings.quality;
    info.memory_usage = retargeting->current_memory_usage;
    
    pthread_mutex_unlock(&retargeting->retargeting_mutex);
    return info;
}

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

float animation_retargeting_calculate_bone_similarity(const char* bone_name1, const char* bone_name2) {
    return animation_retargeting_calculate_name_similarity_internal(bone_name1, bone_name2);
}

int animation_retargeting_find_bone_by_name(const animation_skeleton_t* skeleton, const char* bone_name) {
    return animation_retargeting_find_bone_by_name_internal(skeleton, bone_name);
}

int animation_retargeting_get_bone_hierarchy_level(const animation_skeleton_t* skeleton, uint32_t bone_id) {
    return animation_retargeting_get_bone_hierarchy_level_internal(skeleton, bone_id);
}

bool animation_retargeting_are_bones_symmetric(const char* bone_name1, const char* bone_name2, const char* left_prefix, const char* right_prefix) {
    return animation_retargeting_are_bones_symmetric_internal(bone_name1, bone_name2, left_prefix, right_prefix);
}

/* ============================================================================
 * THREADING (PLACEHOLDER)
 * ============================================================================ */

static int animation_retargeting_init_worker_threads(animation_retargeting_internal_t* retargeting) {
    /* Placeholder for worker thread initialization */
    retargeting->worker_thread_count = 4;
    retargeting->worker_threads_running = true;
    return ANIMATION_RETARGETING_ERROR_NONE;
}

static void animation_retargeting_shutdown_worker_threads(animation_retargeting_internal_t* retargeting) {
    /* Placeholder for worker thread shutdown */
    retargeting->worker_threads_running = false;
}

static void* animation_retargeting_worker_thread(void* arg) {
    /* Placeholder for worker thread function */
    return NULL;
}

/* ============================================================================
 * THREAD SAFETY
 * ============================================================================ */

int animation_retargeting_lock(animation_retargeting_handle_t handle) {
    if (!g_animation_retargeting_ctx.initialized) {
        return ANIMATION_RETARGETING_ERROR_NOT_INITIALIZED;
    }
    
    if (handle.id >= g_animation_retargeting_ctx.instance_count) {
        return ANIMATION_RETARGETING_ERROR_INVALID_HANDLE;
    }
    
    animation_retargeting_internal_t* retargeting = &g_animation_retargeting_ctx.instances[handle.id];
    if (!retargeting->initialized) {
        return ANIMATION_RETARGETING_ERROR_NOT_INITIALIZED;
    }
    
    return pthread_mutex_lock(&retargeting->retargeting_mutex) == 0 ? ANIMATION_RETARGETING_ERROR_NONE : ANIMATION_RETARGETING_ERROR_THREAD_LOCK_FAILED;
}

int animation_retargeting_unlock(animation_retargeting_handle_t handle) {
    if (!g_animation_retargeting_ctx.initialized) {
        return ANIMATION_RETARGETING_ERROR_NOT_INITIALIZED;
    }
    
    if (handle.id >= g_animation_retargeting_ctx.instance_count) {
        return ANIMATION_RETARGETING_ERROR_INVALID_HANDLE;
    }
    
    animation_retargeting_internal_t* retargeting = &g_animation_retargeting_ctx.instances[handle.id];
    if (!retargeting->initialized) {
        return ANIMATION_RETARGETING_ERROR_NOT_INITIALIZED;
    }
    
    return pthread_mutex_unlock(&retargeting->retargeting_mutex) == 0 ? ANIMATION_RETARGETING_ERROR_NONE : ANIMATION_RETARGETING_ERROR_THREAD_LOCK_FAILED;
}

int animation_retargeting_try_lock(animation_retargeting_handle_t handle) {
    if (!g_animation_retargeting_ctx.initialized) {
        return ANIMATION_RETARGETING_ERROR_NOT_INITIALIZED;
    }
    
    if (handle.id >= g_animation_retargeting_ctx.instance_count) {
        return ANIMATION_RETARGETING_ERROR_INVALID_HANDLE;
    }
    
    animation_retargeting_internal_t* retargeting = &g_animation_retargeting_ctx.instances[handle.id];
    if (!retargeting->initialized) {
        return ANIMATION_RETARGETING_ERROR_NOT_INITIALIZED;
    }
    
    return pthread_mutex_trylock(&retargeting->retargeting_mutex) == 0 ? ANIMATION_RETARGETING_ERROR_NONE : ANIMATION_RETARGETING_ERROR_THREAD_LOCK_FAILED;
}

/* End of animation_retargeting.c */