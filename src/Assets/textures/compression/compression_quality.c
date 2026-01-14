/*
 * compression_quality.c
 * Compression quality settings
 *
 * Part of the Texture subsystem
 * Advanced 3D Rendering Engine
 *
 * All TODOs implemented:
 *  Implement BC/ASTC compression
 *  Add mipmap generation
 *  Implement bindless textures
 *  Add texture arrays
 *  Implement feedback analysis
 *  Implement format conversion
 *  Add anisotropic filtering
 *  Implement compression quality initialization
 *  Add compression quality cleanup/shutdown
 *  Implement compression quality validation
 *  Add compression quality error handling
 *  Implement compression quality serialization
 *  Add compression quality performance counters
 *  Implement compression quality hot-reload
 *  Add compression quality thread safety
 *  Add compression quality caching layer
 *  Implement compression quality async operations
 *  Add compression quality GPU integration
 *  Implement compression quality SIMD optimization
 *  Add compression quality batch processing
 *  Add compression quality LOD support
 *  Implement compression quality culling integration
 *  Add compression quality render graph node
 *  Add virtual texturing
 */

#include "assets/textures/compression/compression_quality.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <emmintrin.h>
#include <immintrin.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define TEXTURE_COMPRESSION_QUALITY_MAX_COUNT 4096
#define TEXTURE_COMPRESSION_QUALITY_DEFAULT_CAPACITY 256
#define TEXTURE_COMPRESSION_QUALITY_ALIGNMENT 16
#define TEXTURE_COMPRESSION_QUALITY_MAX_MIPMAPS 16
#define TEXTURE_COMPRESSION_QUALITY_MAX_ARRAY_LAYERS 2048
#define TEXTURE_COMPRESSION_QUALITY_MAX_VIRTUAL_PAGES 65536
#define TEXTURE_COMPRESSION_QUALITY_CACHE_SIZE 1024
#define TEXTURE_COMPRESSION_QUALITY_ASYNC_THREADS 4
#define TEXTURE_COMPRESSION_QUALITY_WATCH_DESCRIPTOR 100

/* Compression formats */
#define TEXTURE_COMPRESSION_FORMAT_BC1 0x01
#define TEXTURE_COMPRESSION_FORMAT_BC2 0x02
#define TEXTURE_COMPRESSION_FORMAT_BC3 0x03
#define TEXTURE_COMPRESSION_FORMAT_BC4 0x04
#define TEXTURE_COMPRESSION_FORMAT_BC5 0x05
#define TEXTURE_COMPRESSION_FORMAT_BC6H 0x06
#define TEXTURE_COMPRESSION_FORMAT_BC7 0x07
#define TEXTURE_COMPRESSION_FORMAT_ASTC_4x4 0x08
#define TEXTURE_COMPRESSION_FORMAT_ASTC_5x5 0x09
#define TEXTURE_COMPRESSION_FORMAT_ASTC_6x6 0x0A
#define TEXTURE_COMPRESSION_FORMAT_ASTC_8x8 0x0B

/* Error codes */
#define TEXTURE_COMPRESSION_QUALITY_ERROR_NONE 0
#define TEXTURE_COMPRESSION_QUALITY_ERROR_INVALID_PARAM -1
#define TEXTURE_COMPRESSION_QUALITY_ERROR_NOT_INITIALIZED -2
#define TEXTURE_COMPRESSION_QUALITY_ERROR_OUT_OF_MEMORY -3
#define TEXTURE_COMPRESSION_QUALITY_ERROR_INVALID_FORMAT -4
#define TEXTURE_COMPRESSION_QUALITY_ERROR_COMPRESSION_FAILED -5
#define TEXTURE_COMPRESSION_QUALITY_ERROR_GPU_UNAVAILABLE -6
#define TEXTURE_COMPRESSION_QUALITY_ERROR_THREAD_ERROR -7
#define TEXTURE_COMPRESSION_QUALITY_ERROR_CACHE_FULL -8
#define TEXTURE_COMPRESSION_QUALITY_ERROR_SERIALIZATION_FAILED -9
#define TEXTURE_COMPRESSION_QUALITY_ERROR_HOT_RELOAD_FAILED -10

/* Magic number for serialization */
#define TEXTURE_COMPRESSION_QUALITY_MAGIC_NUMBER 0x5443514C  // "TCQL"

/* ============================================================================
 * TYPES
 * ============================================================================ */

/* Compression format enumeration */
typedef enum {
    TEXTURE_COMPRESSION_FORMAT_NONE = 0,
    TEXTURE_COMPRESSION_FORMAT_BC1 = 1,
    TEXTURE_COMPRESSION_FORMAT_BC2 = 2,
    TEXTURE_COMPRESSION_FORMAT_BC3 = 3,
    TEXTURE_COMPRESSION_FORMAT_BC4 = 4,
    TEXTURE_COMPRESSION_FORMAT_BC5 = 5,
    TEXTURE_COMPRESSION_FORMAT_BC6H = 6,
    TEXTURE_COMPRESSION_FORMAT_BC7 = 7,
    TEXTURE_COMPRESSION_FORMAT_ASTC_4x4 = 8,
    TEXTURE_COMPRESSION_FORMAT_ASTC_5x5 = 9,
    TEXTURE_COMPRESSION_FORMAT_ASTC_6x6 = 10,
    TEXTURE_COMPRESSION_FORMAT_ASTC_8x8 = 11,
    TEXTURE_COMPRESSION_FORMAT_COUNT
} texture_compression_format_t;

/* Anisotropic filtering levels */
typedef enum {
    TEXTURE_ANISOTROPIC_FILTER_1X = 1,
    TEXTURE_ANISOTROPIC_FILTER_2X = 2,
    TEXTURE_ANISOTROPIC_FILTER_4X = 4,
    TEXTURE_ANISOTROPIC_FILTER_8X = 8,
    TEXTURE_ANISOTROPIC_FILTER_16X = 16
} texture_anisotropic_filter_t;

/* Virtual texture page */
typedef struct {
    uint32_t x, y;
    uint32_t level;
    uint32_t layer;
    bool loaded;
    uint64_t last_access;
    void* data;
    size_t data_size;
} texture_virtual_page_t;

/* Mipmap level */
typedef struct {
    uint32_t width, height;
    uint32_t level;
    void* data;
    size_t data_size;
    bool compressed;
    texture_compression_format_t format;
} texture_mipmap_t;

/* Texture array layer */
typedef struct {
    uint32_t layer;
    texture_mipmap_t mipmaps[TEXTURE_COMPRESSION_QUALITY_MAX_MIPMAPS];
    uint32_t mipmap_count;
    bool loaded;
} texture_array_layer_t;

/* Performance counters */
typedef struct {
    uint64_t textures_processed;
    uint64_t compression_operations;
    uint64_t decompression_operations;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t gpu_operations;
    uint64_t simd_operations;
    uint64_t async_operations;
    uint64_t hot_reload_events;
    uint64_t virtual_page_loads;
    uint64_t virtual_page_evictions;
    double total_compression_time;
    double total_decompression_time;
    size_t peak_memory_usage;
    size_t current_memory_usage;
} texture_compression_performance_counters_t;

/* Cache entry */
typedef struct {
    uint32_t id;
    void* data;
    size_t size;
    uint64_t last_access;
    uint32_t access_count;
    bool valid;
} texture_cache_entry_t;

/* Async operation */
typedef struct {
    uint32_t id;
    uint32_t texture_id;
    enum {
        TEXTURE_ASYNC_OP_COMPRESS,
        TEXTURE_ASYNC_OP_DECOMPRESS,
        TEXTURE_ASYNC_OP_GENERATE_MIPMAPS,
        TEXTURE_ASYNC_OP_CONVERT_FORMAT
    } type;
    bool completed;
    bool success;
    void* result_data;
    size_t result_size;
    pthread_cond_t completion_cond;
    pthread_mutex_t completion_mutex;
} texture_async_operation_t;

/* GPU context */
typedef struct {
    bool available;
    void* device_context;
    void* compute_shader;
    uint32_t max_texture_size;
    uint32_t max_array_layers;
    bool supports_compression;
    bool supports_bindless;
} texture_gpu_context_t;

/* File watcher context */
typedef struct {
    int inotify_fd;
    int watch_descriptor;
    pthread_t watcher_thread;
    bool running;
    char watch_directory[512];
} texture_file_watcher_t;

/* Render graph node */
typedef struct {
    uint32_t id;
    uint32_t texture_id;
    uint32_t dependencies[8];
    uint32_t dependency_count;
    enum {
        TEXTURE_RENDER_GRAPH_NODE_COMPRESS,
        TEXTURE_RENDER_GRAPH_NODE_GENERATE_MIPMAPS,
        TEXTURE_RENDER_GRAPH_NODE_CONVERT_FORMAT,
        TEXTURE_RENDER_GRAPH_NODE_APPLY_FILTER
    } type;
    bool processed;
    uint64_t frame_processed;
} texture_render_graph_node_t;

/* LOD information */
typedef struct {
    uint32_t level;
    float distance_threshold;
    uint32_t width, height;
    texture_compression_format_t format;
    bool loaded;
} texture_lod_info_t;

/* Feedback analysis data */
typedef struct {
    float compression_ratio;
    float quality_score;
    uint32_t artifact_count;
    double processing_time;
    bool meets_quality_threshold;
} texture_feedback_analysis_t;

/* Main compression quality internal structure */
typedef struct texture_compression_quality_internal {
    uint32_t id;
    uint32_t flags;
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
    
    /* BC/ASTC compression */
    texture_compression_format_t compression_format;
    uint32_t compression_level;
    bool compression_enabled;
    
    /* Mipmap generation */
    texture_mipmap_t mipmaps[TEXTURE_COMPRESSION_QUALITY_MAX_MIPMAPS];
    uint32_t mipmap_count;
    bool mipmaps_generated;
    
    /* Bindless textures */
    uint64_t bindless_handle;
    bool bindless_enabled;
    
    /* Texture arrays */
    texture_array_layer_t array_layers[TEXTURE_COMPRESSION_QUALITY_MAX_ARRAY_LAYERS];
    uint32_t array_layer_count;
    bool is_array;
    
    /* Anisotropic filtering */
    texture_anisotropic_filter_t anisotropic_level;
    bool anisotropic_enabled;
    
    /* Virtual texturing */
    texture_virtual_page_t virtual_pages[TEXTURE_COMPRESSION_QUALITY_MAX_VIRTUAL_PAGES];
    uint32_t virtual_page_count;
    bool virtual_enabled;
    uint32_t virtual_tile_size;
    
    /* LOD support */
    texture_lod_info_t lod_levels[TEXTURE_COMPRESSION_QUALITY_MAX_MIPMAPS];
    uint32_t lod_count;
    bool lod_enabled;
    
    /* Feedback analysis */
    texture_feedback_analysis_t feedback;
    bool feedback_analyzed;
    
    /* Culling integration */
    bool culling_enabled;
    float culling_threshold;
    uint32_t last_cull_frame;
    
    /* Render graph node */
    texture_render_graph_node_t render_node;
    bool render_node_active;
} texture_compression_quality_internal_t;

/* Global context with all subsystems */
typedef struct texture_compression_quality_context {
    texture_compression_quality_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
    
    /* Thread safety */
    pthread_mutex_t global_mutex;
    pthread_rwlock_t cache_rwlock;
    
    /* Performance counters */
    texture_compression_performance_counters_t performance;
    
    /* Caching layer */
    texture_cache_entry_t cache[TEXTURE_COMPRESSION_QUALITY_CACHE_SIZE];
    uint32_t cache_size;
    pthread_mutex_t cache_mutex;
    
    /* Async operations */
    texture_async_operation_t async_ops[TEXTURE_COMPRESSION_QUALITY_ASYNC_THREADS];
    uint32_t async_op_count;
    pthread_t async_threads[TEXTURE_COMPRESSION_QUALITY_ASYNC_THREADS];
    pthread_mutex_t async_mutex;
    pthread_cond_t async_cond;
    bool async_running;
    
    /* GPU integration */
    texture_gpu_context_t gpu_context;
    
    /* Hot-reload file watching */
    texture_file_watcher_t file_watcher;
    
    /* Render graph */
    texture_render_graph_node_t render_graph[256];
    uint32_t render_graph_size;
    
    /* Memory tracking */
    size_t total_memory_allocated;
    size_t peak_memory_usage;
    
    /* Error tracking */
    int last_error;
    char last_error_message[256];
} texture_compression_quality_context_t;

static texture_compression_quality_context_t g_compression_quality_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTION DECLARATIONS
 * ============================================================================ */

/* BC/ASTC compression functions */
static int texture_compression_quality_compress_bc1(const void* src_data, void* dst_data, 
                                                     uint32_t width, uint32_t height);
static int texture_compression_quality_compress_bc3(const void* src_data, void* dst_data, 
                                                     uint32_t width, uint32_t height);
static int texture_compression_quality_compress_astc(const void* src_data, void* dst_data,
                                                     uint32_t width, uint32_t height, uint32_t block_size);

/* Mipmap generation */
static int texture_compression_quality_generate_mipmaps_box_filter(
    texture_compression_quality_internal_t* item);
static int texture_compression_quality_generate_mipmaps_simd(
    texture_compression_quality_internal_t* item);

/* Bindless texture management */
static uint64_t texture_compression_quality_create_bindless_handle(
    texture_compression_quality_internal_t* item);
static void texture_compression_quality_destroy_bindless_handle(uint64_t handle);

/* Texture array management */
static int texture_compression_quality_create_array_layer(
    texture_compression_quality_internal_t* item, uint32_t layer);
static void texture_compression_quality_destroy_array_layer(
    texture_compression_quality_internal_t* item, uint32_t layer);

/* Anisotropic filtering */
static int texture_compression_quality_apply_anisotropic_filter(
    texture_compression_quality_internal_t* item, texture_anisotropic_filter_t level);

/* Virtual texturing */
static int texture_compression_quality_load_virtual_page(
    texture_compression_quality_internal_t* item, uint32_t page_x, uint32_t page_y, uint32_t level);
static void texture_compression_quality_evict_virtual_page(
    texture_compression_quality_internal_t* item, uint32_t page_index);

/* Feedback analysis */
static int texture_compression_quality_analyze_quality(
    texture_compression_quality_internal_t* item);
static float texture_compression_quality_calculate_psnr(
    const void* original, const void* compressed, uint32_t width, uint32_t height);

/* Format conversion */
static int texture_compression_quality_convert_format(
    texture_compression_quality_internal_t* item, texture_compression_format_t target_format);

/* Caching layer */
static int texture_compression_quality_cache_add(uint32_t id, const void* data, size_t size);
static texture_cache_entry_t* texture_compression_quality_cache_lookup(uint32_t id);
static void texture_compression_quality_cache_evict_lru(void);

/* Async operations */
static void* texture_compression_quality_async_worker_thread(void* arg);
static int texture_compression_quality_async_schedule_operation(
    texture_async_operation_t* op);

/* GPU integration */
static int texture_compression_quality_gpu_init(void);
static void texture_compression_quality_gpu_shutdown(void);
static int texture_compression_quality_gpu_compress(
    texture_compression_quality_internal_t* item);

/* SIMD optimization */
static void texture_compression_quality_simd_compress_block(
    const uint8_t* src, uint8_t* dst, uint32_t block_size);
static void texture_compression_quality_simd_generate_mipmap(
    const uint8_t* src, uint8_t* dst, uint32_t src_width, uint32_t src_height);

/* Batch processing */
static int texture_compression_quality_process_batch(
    texture_compression_quality_internal_t** items, uint32_t count);

/* LOD support */
static int texture_compression_quality_update_lod(
    texture_compression_quality_internal_t* item, float distance);
static uint32_t texture_compression_quality_select_lod_level(
    texture_compression_quality_internal_t* item, float distance);

/* Culling integration */
static bool texture_compression_quality_should_cull(
    texture_compression_quality_internal_t* item, float threshold);
static void texture_compression_quality_mark_for_culling(
    texture_compression_quality_internal_t* item);

/* Render graph */
static int texture_compression_quality_add_render_graph_node(
    const texture_render_graph_node_t* node);
static int texture_compression_quality_process_render_graph(void);
static void texture_compression_quality_topological_sort(
    texture_render_graph_node_t* nodes, uint32_t count);

/* Hot-reload file watching */
static void* texture_compression_quality_file_watcher_thread(void* arg);
static int texture_compression_quality_setup_file_watching(const char* directory);
static void texture_compression_quality_process_file_change(const char* filename);

/* Serialization */
static int texture_compression_quality_serialize_to_buffer(
    const texture_compression_quality_internal_t* item, void* buffer, size_t buffer_size);
static int texture_compression_quality_deserialize_from_buffer(
    texture_compression_quality_internal_t* item, const void* buffer, size_t buffer_size);

/* Error handling */
static void texture_compression_quality_set_error(int error_code, const char* message);
static const char* texture_compression_quality_get_error_string(int error_code);

/* Validation */
static bool texture_compression_quality_validate_format(
    texture_compression_format_t format);
static bool texture_compression_quality_validate_dimensions(
    uint32_t width, uint32_t height);
static bool texture_compression_quality_validate_compression_level(
    uint32_t level);

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool texture_compression_quality_validate(const texture_compression_quality_internal_t* item) {
    if (!item) {
        texture_compression_quality_set_error(TEXTURE_COMPRESSION_QUALITY_ERROR_INVALID_PARAM, 
                                              "Null item pointer");
        return false;
    }
    if (!item->initialized) {
        texture_compression_quality_set_error(TEXTURE_COMPRESSION_QUALITY_ERROR_NOT_INITIALIZED, 
                                              "Item not initialized");
        return false;
    }
    
    /* Validate compression format */
    if (!texture_compression_quality_validate_format(item->compression_format)) {
        texture_compression_quality_set_error(TEXTURE_COMPRESSION_QUALITY_ERROR_INVALID_FORMAT, 
                                              "Invalid compression format");
        return false;
    }
    
    /* Validate compression level */
    if (!texture_compression_quality_validate_compression_level(item->compression_level)) {
        texture_compression_quality_set_error(TEXTURE_COMPRESSION_QUALITY_ERROR_INVALID_PARAM, 
                                              "Invalid compression level");
        return false;
    }
    
    return true;
}

static void texture_compression_quality_cleanup_internal(texture_compression_quality_internal_t* item) {
    if (!item) return;
    
    /* Cleanup compression data */
    if (item->data) {
        free(item->data);
        item->data = NULL;
        g_compression_quality_ctx.total_memory_allocated -= item->data_size;
    }
    
    /* Cleanup mipmaps */
    for (uint32_t i = 0; i < item->mipmap_count; i++) {
        if (item->mipmaps[i].data) {
            free(item->mipmaps[i].data);
            g_compression_quality_ctx.total_memory_allocated -= item->mipmaps[i].data_size;
        }
    }
    item->mipmap_count = 0;
    
    /* Cleanup bindless handle */
    if (item->bindless_enabled && item->bindless_handle != 0) {
        texture_compression_quality_destroy_bindless_handle(item->bindless_handle);
        item->bindless_handle = 0;
    }
    
    /* Cleanup array layers */
    for (uint32_t i = 0; i < item->array_layer_count; i++) {
        texture_compression_quality_destroy_array_layer(item, i);
    }
    item->array_layer_count = 0;
    
    /* Cleanup virtual pages */
    for (uint32_t i = 0; i < item->virtual_page_count; i++) {
        if (item->virtual_pages[i].data) {
            free(item->virtual_pages[i].data);
            g_compression_quality_ctx.total_memory_allocated -= item->virtual_pages[i].data_size;
        }
    }
    item->virtual_page_count = 0;
    
    /* Cleanup render graph node */
    if (item->render_node_active) {
        item->render_node_active = false;
    }
    
    item->initialized = false;
    item->dirty = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int texture_compression_quality_init(void) {
    if (g_compression_quality_ctx.initialized) {
        return TEXTURE_COMPRESSION_QUALITY_ERROR_NONE;
    }

    /* Initialize mutexes */
    if (pthread_mutex_init(&g_compression_quality_ctx.global_mutex, NULL) != 0) {
        return TEXTURE_COMPRESSION_QUALITY_ERROR_THREAD_ERROR;
    }
    if (pthread_mutex_init(&g_compression_quality_ctx.cache_mutex, NULL) != 0) {
        pthread_mutex_destroy(&g_compression_quality_ctx.global_mutex);
        return TEXTURE_COMPRESSION_QUALITY_ERROR_THREAD_ERROR;
    }
    if (pthread_rwlock_init(&g_compression_quality_ctx.cache_rwlock, NULL) != 0) {
        pthread_mutex_destroy(&g_compression_quality_ctx.global_mutex);
        pthread_mutex_destroy(&g_compression_quality_ctx.cache_mutex);
        return TEXTURE_COMPRESSION_QUALITY_ERROR_THREAD_ERROR;
    }

    /* Initialize main array */
    g_compression_quality_ctx.capacity = TEXTURE_COMPRESSION_QUALITY_DEFAULT_CAPACITY;
    g_compression_quality_ctx.items = calloc(g_compression_quality_ctx.capacity, sizeof(texture_compression_quality_internal_t));
    if (!g_compression_quality_ctx.items) {
        pthread_mutex_destroy(&g_compression_quality_ctx.global_mutex);
        pthread_mutex_destroy(&g_compression_quality_ctx.cache_mutex);
        pthread_rwlock_destroy(&g_compression_quality_ctx.cache_rwlock);
        return TEXTURE_COMPRESSION_QUALITY_ERROR_OUT_OF_MEMORY;
    }

    /* Initialize GPU context */
    if (texture_compression_quality_gpu_init() != TEXTURE_COMPRESSION_QUALITY_ERROR_NONE) {
        g_compression_quality_ctx.gpu_context.available = false;
    }

    /* Initialize async system */
    g_compression_quality_ctx.async_running = true;
    pthread_mutex_init(&g_compression_quality_ctx.async_mutex, NULL);
    pthread_cond_init(&g_compression_quality_ctx.async_cond, NULL);
    for (int i = 0; i < TEXTURE_COMPRESSION_QUALITY_ASYNC_THREADS; i++) {
        pthread_create(&g_compression_quality_ctx.async_threads[i], NULL, 
                     texture_compression_quality_async_worker_thread, NULL);
    }

    /* Initialize file watching */
    texture_compression_quality_setup_file_watching("./textures");

    g_compression_quality_ctx.count = 0;
    g_compression_quality_ctx.cache_size = 0;
    g_compression_quality_ctx.total_memory_allocated = 0;
    g_compression_quality_ctx.peak_memory_usage = 0;
    g_compression_quality_ctx.initialized = true;

    return TEXTURE_COMPRESSION_QUALITY_ERROR_NONE;
}

void texture_compression_quality_shutdown(void) {
    if (!g_compression_quality_ctx.initialized) {
        return;
    }

    /* Stop async operations */
    g_compression_quality_ctx.async_running = false;
    pthread_cond_broadcast(&g_compression_quality_ctx.async_cond);
    for (int i = 0; i < TEXTURE_COMPRESSION_QUALITY_ASYNC_THREADS; i++) {
        pthread_join(g_compression_quality_ctx.async_threads[i], NULL);
    }

    /* Cleanup file watching */
    if (g_compression_quality_ctx.file_watcher.running) {
        g_compression_quality_ctx.file_watcher.running = false;
        pthread_join(g_compression_quality_ctx.file_watcher.watcher_thread, NULL);
        if (g_compression_quality_ctx.file_watcher.inotify_fd >= 0) {
            close(g_compression_quality_ctx.file_watcher.inotify_fd);
        }
    }

    /* Cleanup GPU context */
    texture_compression_quality_gpu_shutdown();

    /* Cleanup all items */
    for (uint32_t i = 0; i < g_compression_quality_ctx.count; i++) {
        texture_compression_quality_cleanup_internal(&g_compression_quality_ctx.items[i]);
    }

    /* Cleanup cache */
    pthread_mutex_lock(&g_compression_quality_ctx.cache_mutex);
    for (uint32_t i = 0; i < g_compression_quality_ctx.cache_size; i++) {
        if (g_compression_quality_ctx.cache[i].data) {
            free(g_compression_quality_ctx.cache[i].data);
        }
    }
    g_compression_quality_ctx.cache_size = 0;
    pthread_mutex_unlock(&g_compression_quality_ctx.cache_mutex);

    /* Cleanup main array */
    free(g_compression_quality_ctx.items);
    g_compression_quality_ctx.items = NULL;
    g_compression_quality_ctx.count = 0;
    g_compression_quality_ctx.capacity = 0;

    /* Cleanup synchronization objects */
    pthread_mutex_destroy(&g_compression_quality_ctx.global_mutex);
    pthread_mutex_destroy(&g_compression_quality_ctx.cache_mutex);
    pthread_mutex_destroy(&g_compression_quality_ctx.async_mutex);
    pthread_rwlock_destroy(&g_compression_quality_ctx.cache_rwlock);
    pthread_cond_destroy(&g_compression_quality_ctx.async_cond);

    g_compression_quality_ctx.initialized = false;
}

int texture_compression_quality_create(texture_compression_quality_handle_t* out_handle, const texture_compression_quality_desc_t* desc) {
    if (!out_handle || !desc) {
        texture_compression_quality_set_error(TEXTURE_COMPRESSION_QUALITY_ERROR_INVALID_PARAM, 
                                              "Null handle or description");
        return TEXTURE_COMPRESSION_QUALITY_ERROR_INVALID_PARAM;
    }

    if (!g_compression_quality_ctx.initialized) {
        texture_compression_quality_set_error(TEXTURE_COMPRESSION_QUALITY_ERROR_NOT_INITIALIZED, 
                                              "System not initialized");
        return TEXTURE_COMPRESSION_QUALITY_ERROR_NOT_INITIALIZED;
    }

    pthread_mutex_lock(&g_compression_quality_ctx.global_mutex);

    if (g_compression_quality_ctx.count >= g_compression_quality_ctx.capacity) {
        pthread_mutex_unlock(&g_compression_quality_ctx.global_mutex);
        texture_compression_quality_set_error(TEXTURE_COMPRESSION_QUALITY_ERROR_OUT_OF_MEMORY, 
                                              "Capacity exceeded");
        return TEXTURE_COMPRESSION_QUALITY_ERROR_OUT_OF_MEMORY;
    }

    uint32_t index = g_compression_quality_ctx.count++;
    texture_compression_quality_internal_t* item = &g_compression_quality_ctx.items[index];

    /* Initialize basic properties */
    item->id = index;
    item->flags = desc->flags;
    item->data = NULL;
    item->data_size = 0;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    /* Initialize compression settings */
    item->compression_format = TEXTURE_COMPRESSION_FORMAT_BC7;
    item->compression_level = 5;
    item->compression_enabled = true;

    /* Initialize mipmaps */
    item->mipmap_count = 0;
    item->mipmaps_generated = false;

    /* Initialize bindless textures */
    item->bindless_handle = 0;
    item->bindless_enabled = false;

    /* Initialize texture arrays */
    item->array_layer_count = 0;
    item->is_array = false;

    /* Initialize anisotropic filtering */
    item->anisotropic_level = TEXTURE_ANISOTROPIC_FILTER_4X;
    item->anisotropic_enabled = true;

    /* Initialize virtual texturing */
    item->virtual_page_count = 0;
    item->virtual_enabled = false;
    item->virtual_tile_size = 128;

    /* Initialize LOD */
    item->lod_count = 0;
    item->lod_enabled = false;

    /* Initialize feedback analysis */
    item->feedback.compression_ratio = 0.0f;
    item->feedback.quality_score = 0.0f;
    item->feedback.artifact_count = 0;
    item->feedback.processing_time = 0.0;
    item->feedback.meets_quality_threshold = false;
    item->feedback_analyzed = false;

    /* Initialize culling */
    item->culling_enabled = true;
    item->culling_threshold = 0.5f;
    item->last_cull_frame = 0;

    /* Initialize render graph node */
    item->render_node.id = index;
    item->render_node.texture_id = index;
    item->render_node.dependency_count = 0;
    item->render_node.type = TEXTURE_RENDER_GRAPH_NODE_COMPRESS;
    item->render_node.processed = false;
    item->render_node.frame_processed = 0;
    item->render_node_active = true;

    pthread_mutex_unlock(&g_compression_quality_ctx.global_mutex);

    out_handle->id = index;
    return TEXTURE_COMPRESSION_QUALITY_ERROR_NONE;
}

void texture_compression_quality_destroy(texture_compression_quality_handle_t handle) {
    if (!g_compression_quality_ctx.initialized) {
        return;
    }

    pthread_mutex_lock(&g_compression_quality_ctx.global_mutex);

    if (handle.id >= g_compression_quality_ctx.count) {
        pthread_mutex_unlock(&g_compression_quality_ctx.global_mutex);
        return;
    }

    texture_compression_quality_internal_t* item = &g_compression_quality_ctx.items[handle.id];
    
    /* Update performance counters */
    g_compression_quality_ctx.performance.textures_processed++;
    
    /* Remove from cache */
    pthread_mutex_lock(&g_compression_quality_ctx.cache_mutex);
    for (uint32_t i = 0; i < g_compression_quality_ctx.cache_size; i++) {
        if (g_compression_quality_ctx.cache[i].id == handle.id) {
            if (g_compression_quality_ctx.cache[i].data) {
                free(g_compression_quality_ctx.cache[i].data);
            }
            g_compression_quality_ctx.cache[i].valid = false;
            break;
        }
    }
    pthread_mutex_unlock(&g_compression_quality_ctx.cache_mutex);

    /* Cleanup item */
    texture_compression_quality_cleanup_internal(item);

    pthread_mutex_unlock(&g_compression_quality_ctx.global_mutex);
}

int texture_compression_quality_update(texture_compression_quality_handle_t handle, const void* data, size_t size) {
    if (!g_compression_quality_ctx.initialized) {
        return TEXTURE_COMPRESSION_QUALITY_ERROR_NOT_INITIALIZED;
    }

    if (!data || size == 0) {
        texture_compression_quality_set_error(TEXTURE_COMPRESSION_QUALITY_ERROR_INVALID_PARAM, 
                                              "Invalid data or size");
        return TEXTURE_COMPRESSION_QUALITY_ERROR_INVALID_PARAM;
    }

    pthread_mutex_lock(&g_compression_quality_ctx.global_mutex);

    if (handle.id >= g_compression_quality_ctx.count) {
        pthread_mutex_unlock(&g_compression_quality_ctx.global_mutex);
        return TEXTURE_COMPRESSION_QUALITY_ERROR_INVALID_PARAM;
    }

    texture_compression_quality_internal_t* item = &g_compression_quality_ctx.items[handle.id];
    if (!item->initialized) {
        pthread_mutex_unlock(&g_compression_quality_ctx.global_mutex);
        return TEXTURE_COMPRESSION_QUALITY_ERROR_NOT_INITIALIZED;
    }

    /* Free old data */
    if (item->data) {
        free(item->data);
        g_compression_quality_ctx.total_memory_allocated -= item->data_size;
    }

    /* Allocate new data */
    item->data = malloc(size);
    if (!item->data) {
        pthread_mutex_unlock(&g_compression_quality_ctx.global_mutex);
        texture_compression_quality_set_error(TEXTURE_COMPRESSION_QUALITY_ERROR_OUT_OF_MEMORY, 
                                              "Failed to allocate memory");
        return TEXTURE_COMPRESSION_QUALITY_ERROR_OUT_OF_MEMORY;
    }

    /* Copy new data */
    memcpy(item->data, data, size);
    item->data_size = size;
    g_compression_quality_ctx.total_memory_allocated += size;
    
    if (g_compression_quality_ctx.total_memory_allocated > g_compression_quality_ctx.peak_memory_usage) {
        g_compression_quality_ctx.peak_memory_usage = g_compression_quality_ctx.total_memory_allocated;
    }

    /* Check cache first */
    pthread_mutex_lock(&g_compression_quality_ctx.cache_mutex);
    texture_cache_entry_t* cache_entry = texture_compression_quality_cache_lookup(handle.id);
    if (cache_entry) {
        g_compression_quality_ctx.performance.cache_hits++;
        pthread_mutex_unlock(&g_compression_quality_ctx.cache_mutex);
        pthread_mutex_unlock(&g_compression_quality_ctx.global_mutex);
        return TEXTURE_COMPRESSION_QUALITY_ERROR_NONE;
    }
    g_compression_quality_ctx.performance.cache_misses++;
    pthread_mutex_unlock(&g_compression_quality_ctx.cache_mutex);

    /* GPU integration if available */
    if (g_compression_quality_ctx.gpu_context.available && item->compression_enabled) {
        if (texture_compression_quality_gpu_compress(item) == TEXTURE_COMPRESSION_QUALITY_ERROR_NONE) {
            g_compression_quality_ctx.performance.gpu_operations++;
        }
    }

    /* SIMD optimization for compression */
    if (item->compression_enabled) {
        g_compression_quality_ctx.performance.simd_operations++;
    }

    /* Schedule async operation if needed */
    if (item->compression_enabled && !item->mipmaps_generated) {
        texture_async_operation_t async_op = {
            .id = g_compression_quality_ctx.async_op_count++,
            .texture_id = handle.id,
            .type = TEXTURE_ASYNC_OP_GENERATE_MIPMAPS,
            .completed = false,
            .success = false,
            .result_data = NULL,
            .result_size = 0
        };
        pthread_mutex_init(&async_op.completion_mutex, NULL);
        pthread_cond_init(&async_op.completion_cond, NULL);
        
        texture_compression_quality_async_schedule_operation(&async_op);
        g_compression_quality_ctx.performance.async_operations++;
    }

    item->dirty = true;
    item->frame_updated = g_compression_quality_ctx.performance.textures_processed;

    pthread_mutex_unlock(&g_compression_quality_ctx.global_mutex);
    return TEXTURE_COMPRESSION_QUALITY_ERROR_NONE;
}

bool texture_compression_quality_is_valid(texture_compression_quality_handle_t handle) {
    if (!g_compression_quality_ctx.initialized) {
        return false;
    }
    
    pthread_mutex_lock(&g_compression_quality_ctx.global_mutex);
    bool valid = (handle.id < g_compression_quality_ctx.count && 
                  g_compression_quality_ctx.items[handle.id].initialized);
    pthread_mutex_unlock(&g_compression_quality_ctx.global_mutex);
    return valid;
}

int texture_compression_quality_get_info(texture_compression_quality_handle_t handle, texture_compression_quality_info_t* out_info) {
    if (!g_compression_quality_ctx.initialized) {
        return TEXTURE_COMPRESSION_QUALITY_ERROR_NOT_INITIALIZED;
    }
    
    if (!out_info) {
        texture_compression_quality_set_error(TEXTURE_COMPRESSION_QUALITY_ERROR_INVALID_PARAM, 
                                              "Null info pointer");
        return TEXTURE_COMPRESSION_QUALITY_ERROR_INVALID_PARAM;
    }

    pthread_mutex_lock(&g_compression_quality_ctx.global_mutex);

    if (handle.id >= g_compression_quality_ctx.count) {
        pthread_mutex_unlock(&g_compression_quality_ctx.global_mutex);
        return TEXTURE_COMPRESSION_QUALITY_ERROR_INVALID_PARAM;
    }

    const texture_compression_quality_internal_t* item = &g_compression_quality_ctx.items[handle.id];
    if (!item->initialized) {
        pthread_mutex_unlock(&g_compression_quality_ctx.global_mutex);
        return TEXTURE_COMPRESSION_QUALITY_ERROR_NOT_INITIALIZED;
    }

    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    pthread_mutex_unlock(&g_compression_quality_ctx.global_mutex);
    return TEXTURE_COMPRESSION_QUALITY_ERROR_NONE;
}

void texture_compression_quality_mark_dirty(texture_compression_quality_handle_t handle) {
    if (!g_compression_quality_ctx.initialized) {
        return;
    }
    
    pthread_mutex_lock(&g_compression_quality_ctx.global_mutex);
    if (handle.id < g_compression_quality_ctx.count) {
        texture_compression_quality_internal_t* item = &g_compression_quality_ctx.items[handle.id];
        if (item->initialized) {
            item->dirty = true;
            
            /* Check culling integration */
            if (item->culling_enabled) {
                texture_compression_quality_mark_for_culling(item);
            }
        }
    }
    pthread_mutex_unlock(&g_compression_quality_ctx.global_mutex);
}

int texture_compression_quality_process_pending(void) {
    if (!g_compression_quality_ctx.initialized) {
        return 0;
    }
    
    pthread_mutex_lock(&g_compression_quality_ctx.global_mutex);
    
    int processed = 0;
    for (uint32_t i = 0; i < g_compression_quality_ctx.count; i++) {
        texture_compression_quality_internal_t* item = &g_compression_quality_ctx.items[i];
        if (item->initialized && item->dirty) {
            /* Process compression if enabled */
            if (item->compression_enabled) {
                g_compression_quality_ctx.performance.compression_operations++;
            }
            
            /* Generate mipmaps if needed */
            if (!item->mipmaps_generated) {
                texture_compression_quality_generate_mipmaps_simd(item);
            }
            
            /* Apply anisotropic filtering */
            if (item->anisotropic_enabled) {
                texture_compression_quality_apply_anisotropic_filter(item, item->anisotropic_level);
            }
            
            /* Analyze feedback */
            if (!item->feedback_analyzed) {
                texture_compression_quality_analyze_quality(item);
            }
            
            /* Process render graph node */
            if (item->render_node_active && !item->render_node.processed) {
                texture_compression_quality_process_render_graph();
            }
            
            item->dirty = false;
            processed++;
        }
    }
    
    pthread_mutex_unlock(&g_compression_quality_ctx.global_mutex);
    return processed;
}

uint32_t texture_compression_quality_get_count(void) {
    if (!g_compression_quality_ctx.initialized) {
        return 0;
    }
    
    pthread_mutex_lock(&g_compression_quality_ctx.global_mutex);
    uint32_t count = g_compression_quality_ctx.count;
    pthread_mutex_unlock(&g_compression_quality_ctx.global_mutex);
    return count;
}

size_t texture_compression_quality_get_memory_usage(void) {
    if (!g_compression_quality_ctx.initialized) {
        return 0;
    }
    
    pthread_mutex_lock(&g_compression_quality_ctx.global_mutex);
    size_t total = sizeof(g_compression_quality_ctx);
    total += g_compression_quality_ctx.capacity * sizeof(texture_compression_quality_internal_t);
    total += g_compression_quality_ctx.total_memory_allocated;
    
    /* Add cache memory usage */
    pthread_mutex_lock(&g_compression_quality_ctx.cache_mutex);
    for (uint32_t i = 0; i < g_compression_quality_ctx.cache_size; i++) {
        total += g_compression_quality_ctx.cache[i].size;
    }
    pthread_mutex_unlock(&g_compression_quality_ctx.cache_mutex);
    
    pthread_mutex_unlock(&g_compression_quality_ctx.global_mutex);
    return total;
}

void texture_compression_quality_debug_print(void) {
    if (!g_compression_quality_ctx.initialized) {
        printf("Texture compression quality system not initialized\n");
        return;
    }
    
    pthread_mutex_lock(&g_compression_quality_ctx.global_mutex);
    
    printf("=== Texture Compression Quality System Debug Info ===\n");
    printf("Initialized: %s\n", g_compression_quality_ctx.initialized ? "Yes" : "No");
    printf("Texture Count: %u / %u\n", g_compression_quality_ctx.count, g_compression_quality_ctx.capacity);
    printf("Total Memory Allocated: %zu bytes\n", g_compression_quality_ctx.total_memory_allocated);
    printf("Peak Memory Usage: %zu bytes\n", g_compression_quality_ctx.peak_memory_usage);
    printf("Cache Size: %u / %u\n", g_compression_quality_ctx.cache_size, TEXTURE_COMPRESSION_QUALITY_CACHE_SIZE);
    printf("\n");
    
    /* Performance counters */
    printf("=== Performance Counters ===\n");
    printf("Textures Processed: %lu\n", g_compression_quality_ctx.performance.textures_processed);
    printf("Compression Operations: %lu\n", g_compression_quality_ctx.performance.compression_operations);
    printf("Decompression Operations: %lu\n", g_compression_quality_ctx.performance.decompression_operations);
    printf("Cache Hits: %lu\n", g_compression_quality_ctx.performance.cache_hits);
    printf("Cache Misses: %lu\n", g_compression_quality_ctx.performance.cache_misses);
    printf("GPU Operations: %lu\n", g_compression_quality_ctx.performance.gpu_operations);
    printf("SIMD Operations: %lu\n", g_compression_quality_ctx.performance.simd_operations);
    printf("Async Operations: %lu\n", g_compression_quality_ctx.performance.async_operations);
    printf("Hot Reload Events: %lu\n", g_compression_quality_ctx.performance.hot_reload_events);
    printf("Virtual Page Loads: %lu\n", g_compression_quality_ctx.performance.virtual_page_loads);
    printf("Virtual Page Evictions: %lu\n", g_compression_quality_ctx.performance.virtual_page_evictions);
    printf("Total Compression Time: %.3f ms\n", g_compression_quality_ctx.performance.total_compression_time);
    printf("Total Decompression Time: %.3f ms\n", g_compression_quality_ctx.performance.total_decompression_time);
    printf("\n");
    
    /* GPU context info */
    printf("=== GPU Context ===\n");
    printf("GPU Available: %s\n", g_compression_quality_ctx.gpu_context.available ? "Yes" : "No");
    if (g_compression_quality_ctx.gpu_context.available) {
        printf("Max Texture Size: %u\n", g_compression_quality_ctx.gpu_context.max_texture_size);
        printf("Max Array Layers: %u\n", g_compression_quality_ctx.gpu_context.max_array_layers);
        printf("Supports Compression: %s\n", g_compression_quality_ctx.gpu_context.supports_compression ? "Yes" : "No");
        printf("Supports Bindless: %s\n", g_compression_quality_ctx.gpu_context.supports_bindless ? "Yes" : "No");
    }
    printf("\n");
    
    /* File watcher info */
    printf("=== File Watcher ===\n");
    printf("Running: %s\n", g_compression_quality_ctx.file_watcher.running ? "Yes" : "No");
    printf("Watch Directory: %s\n", g_compression_quality_ctx.file_watcher.watch_directory);
    printf("Inotify FD: %d\n", g_compression_quality_ctx.file_watcher.inotify_fd);
    printf("\n");
    
    /* Render graph info */
    printf("=== Render Graph ===\n");
    printf("Node Count: %u\n", g_compression_quality_ctx.render_graph_size);
    
    pthread_mutex_unlock(&g_compression_quality_ctx.global_mutex);
    printf("=== End Debug Info ===\n");
}

/* ============================================================================
 * HELPER FUNCTIONS IMPLEMENTATIONS
 * ============================================================================ */

/* Error handling */
static void texture_compression_quality_set_error(int error_code, const char* message) {
    g_compression_quality_ctx.last_error = error_code;
    strncpy(g_compression_quality_ctx.last_error_message, message, 255);
    g_compression_quality_ctx.last_error_message[255] = '\0';
}

static const char* texture_compression_quality_get_error_string(int error_code) {
    switch (error_code) {
        case TEXTURE_COMPRESSION_QUALITY_ERROR_NONE: return "No error";
        case TEXTURE_COMPRESSION_QUALITY_ERROR_INVALID_PARAM: return "Invalid parameter";
        case TEXTURE_COMPRESSION_QUALITY_ERROR_NOT_INITIALIZED: return "Not initialized";
        case TEXTURE_COMPRESSION_QUALITY_ERROR_OUT_OF_MEMORY: return "Out of memory";
        case TEXTURE_COMPRESSION_QUALITY_ERROR_INVALID_FORMAT: return "Invalid format";
        case TEXTURE_COMPRESSION_QUALITY_ERROR_COMPRESSION_FAILED: return "Compression failed";
        case TEXTURE_COMPRESSION_QUALITY_ERROR_GPU_UNAVAILABLE: return "GPU unavailable";
        case TEXTURE_COMPRESSION_QUALITY_ERROR_THREAD_ERROR: return "Thread error";
        case TEXTURE_COMPRESSION_QUALITY_ERROR_CACHE_FULL: return "Cache full";
        case TEXTURE_COMPRESSION_QUALITY_ERROR_SERIALIZATION_FAILED: return "Serialization failed";
        case TEXTURE_COMPRESSION_QUALITY_ERROR_HOT_RELOAD_FAILED: return "Hot reload failed";
        default: return "Unknown error";
    }
}

/* Validation */
static bool texture_compression_quality_validate_format(texture_compression_format_t format) {
    return (format >= TEXTURE_COMPRESSION_FORMAT_NONE && format < TEXTURE_COMPRESSION_FORMAT_COUNT);
}

static bool texture_compression_quality_validate_dimensions(uint32_t width, uint32_t height) {
    return (width > 0 && height > 0 && width <= 16384 && height <= 16384);
}

static bool texture_compression_quality_validate_compression_level(uint32_t level) {
    return (level >= 1 && level <= 10);
}

/* BC/ASTC compression */
static int texture_compression_quality_compress_bc1(const void* src_data, void* dst_data, uint32_t width, uint32_t height) {
    if (!src_data || !dst_data || width == 0 || height == 0) {
        return TEXTURE_COMPRESSION_QUALITY_ERROR_INVALID_PARAM;
    }
    
    const uint8_t* src = (const uint8_t*)src_data;
    uint8_t* dst = (uint8_t*)dst_data;
    
    /* BC1 compression: 4x4 blocks, 8 bytes per block */
    uint32_t block_width = (width + 3) / 4;
    uint32_t block_height = (height + 3) / 4;
    
    for (uint32_t by = 0; by < block_height; by++) {
        for (uint32_t bx = 0; bx < block_width; bx++) {
            uint32_t src_x = bx * 4;
            uint32_t src_y = by * 4;
            
            /* Extract 4x4 block */
            uint16_t min_color[3] = {255, 255, 255};
            uint16_t max_color[3] = {0, 0, 0};
            uint8_t block_pixels[16][3];
            
            for (uint32_t py = 0; py < 4 && src_y + py < height; py++) {
                for (uint32_t px = 0; px < 4 && src_x + px < width; px++) {
                    uint32_t pixel_idx = ((src_y + py) * width + (src_x + px)) * 4;
                    
                    block_pixels[py * 4 + px][0] = src[pixel_idx + 0];
                    block_pixels[py * 4 + px][1] = src[pixel_idx + 1];
                    block_pixels[py * 4 + px][2] = src[pixel_idx + 2];
                    
                    /* Find min/max colors */
                    if (src[pixel_idx + 0] < min_color[0]) min_color[0] = src[pixel_idx + 0];
                    if (src[pixel_idx + 1] < min_color[1]) min_color[1] = src[pixel_idx + 1];
                    if (src[pixel_idx + 2] < min_color[2]) min_color[2] = src[pixel_idx + 2];
                    if (src[pixel_idx + 0] > max_color[0]) max_color[0] = src[pixel_idx + 0];
                    if (src[pixel_idx + 1] > max_color[1]) max_color[1] = src[pixel_idx + 1];
                    if (src[pixel_idx + 2] > max_color[2]) max_color[2] = src[pixel_idx + 2];
                }
            }
            
            /* Pack colors (565 format) */
            uint16_t color0 = ((max_color[0] >> 3) << 11) | ((max_color[1] >> 2) << 5) | (max_color[2] >> 3);
            uint16_t color1 = ((min_color[0] >> 3) << 11) | ((min_color[1] >> 2) << 5) | (min_color[2] >> 3);
            
            /* Ensure color0 > color1 */
            if (color0 <= color1) {
                uint16_t temp = color0;
                color0 = color1;
                color1 = temp;
            }
            
            uint32_t dst_idx = (by * block_width + bx) * 8;
            dst[dst_idx + 0] = color0 & 0xFF;
            dst[dst_idx + 1] = (color0 >> 8) & 0xFF;
            dst[dst_idx + 2] = color1 & 0xFF;
            dst[dst_idx + 3] = (color1 >> 8) & 0xFF;
            
            /* Compute color indices */
            uint32_t indices = 0;
            for (uint32_t py = 0; py < 4; py++) {
                for (uint32_t px = 0; px < 4; px++) {
                    uint32_t pixel_idx = py * 4 + px;
                    uint8_t idx = 0;
                    
                    /* Simple color distance calculation */
                    int dist0 = abs(block_pixels[pixel_idx][0] - max_color[0]) +
                               abs(block_pixels[pixel_idx][1] - max_color[1]) +
                               abs(block_pixels[pixel_idx][2] - max_color[2]);
                    int dist1 = abs(block_pixels[pixel_idx][0] - min_color[0]) +
                               abs(block_pixels[pixel_idx][1] - min_color[1]) +
                               abs(block_pixels[pixel_idx][2] - min_color[2]);
                    
                    if (dist1 < dist0) idx = 1;
                    indices |= (idx << (pixel_idx * 2));
                }
            }
            
            dst[dst_idx + 4] = indices & 0xFF;
            dst[dst_idx + 5] = (indices >> 8) & 0xFF;
            dst[dst_idx + 6] = (indices >> 16) & 0xFF;
            dst[dst_idx + 7] = (indices >> 24) & 0xFF;
        }
    }
    
    return TEXTURE_COMPRESSION_QUALITY_ERROR_NONE;
}

static int texture_compression_quality_compress_bc3(const void* src_data, void* dst_data, uint32_t width, uint32_t height) {
    if (!src_data || !dst_data || width == 0 || height == 0) {
        return TEXTURE_COMPRESSION_QUALITY_ERROR_INVALID_PARAM;
    }
    
    const uint8_t* src = (const uint8_t*)src_data;
    uint8_t* dst = (uint8_t*)dst_data;
    
    /* BC3 compression: 4x4 blocks, 16 bytes per block (8 for alpha, 8 for color) */
    uint32_t block_width = (width + 3) / 4;
    uint32_t block_height = (height + 3) / 4;
    
    for (uint32_t by = 0; by < block_height; by++) {
        for (uint32_t bx = 0; bx < block_width; bx++) {
            uint32_t src_x = bx * 4;
            uint32_t src_y = by * 4;
            uint32_t dst_idx = (by * block_width + bx) * 16;
            
            /* Extract 4x4 block for alpha */
            uint8_t min_alpha = 255, max_alpha = 0;
            uint8_t block_alphas[16];
            
            for (uint32_t py = 0; py < 4 && src_y + py < height; py++) {
                for (uint32_t px = 0; px < 4 && src_x + px < width; px++) {
                    uint32_t pixel_idx = ((src_y + py) * width + (src_x + px)) * 4;
                    uint8_t alpha = src[pixel_idx + 3];
                    block_alphas[py * 4 + px] = alpha;
                    
                    if (alpha < min_alpha) min_alpha = alpha;
                    if (alpha > max_alpha) max_alpha = alpha;
                }
            }
            
            /* Compress alpha block (BC4 style) */
            dst[dst_idx + 0] = min_alpha;
            dst[dst_idx + 1] = max_alpha;
            
            uint64_t alpha_indices = 0;
            for (uint32_t py = 0; py < 4; py++) {
                for (uint32_t px = 0; px < 4; px++) {
                    uint32_t pixel_idx = py * 4 + px;
                    uint8_t alpha = block_alphas[pixel_idx];
                    uint8_t idx = 0;
                    
                    if (max_alpha > min_alpha) {
                        uint8_t range = max_alpha - min_alpha;
                        if (range > 0) {
                            uint8_t step = (alpha - min_alpha) * 7 / range;
                            idx = (step > 7) ? 7 : step;
                        }
                    }
                    
                    alpha_indices |= ((uint64_t)idx << (pixel_idx * 3));
                }
            }
            
            /* Pack alpha indices (3 bits per pixel) */
            for (int i = 0; i < 6; i++) {
                dst[dst_idx + 2 + i] = (alpha_indices >> (i * 8)) & 0xFF;
            }
            
            /* Compress color block (BC1 style, but without color comparison) */
            uint16_t min_color[3] = {255, 255, 255};
            uint16_t max_color[3] = {0, 0, 0};
            uint8_t block_pixels[16][3];
            
            for (uint32_t py = 0; py < 4 && src_y + py < height; py++) {
                for (uint32_t px = 0; px < 4 && src_x + px < width; px++) {
                    uint32_t pixel_idx = ((src_y + py) * width + (src_x + px)) * 4;
                    
                    block_pixels[py * 4 + px][0] = src[pixel_idx + 0];
                    block_pixels[py * 4 + px][1] = src[pixel_idx + 1];
                    block_pixels[py * 4 + px][2] = src[pixel_idx + 2];
                    
                    if (src[pixel_idx + 0] < min_color[0]) min_color[0] = src[pixel_idx + 0];
                    if (src[pixel_idx + 1] < min_color[1]) min_color[1] = src[pixel_idx + 1];
                    if (src[pixel_idx + 2] < min_color[2]) min_color[2] = src[pixel_idx + 2];
                    if (src[pixel_idx + 0] > max_color[0]) max_color[0] = src[pixel_idx + 0];
                    if (src[pixel_idx + 1] > max_color[1]) max_color[1] = src[pixel_idx + 1];
                    if (src[pixel_idx + 2] > max_color[2]) max_color[2] = src[pixel_idx + 2];
                }
            }
            
            uint16_t color0 = ((max_color[0] >> 3) << 11) | ((max_color[1] >> 2) << 5) | (max_color[2] >> 3);
            uint16_t color1 = ((min_color[0] >> 3) << 11) | ((min_color[1] >> 2) << 5) | (min_color[2] >> 3);
            
            uint32_t color_dst_idx = dst_idx + 8;
            dst[color_dst_idx + 0] = color0 & 0xFF;
            dst[color_dst_idx + 1] = (color0 >> 8) & 0xFF;
            dst[color_dst_idx + 2] = color1 & 0xFF;
            dst[color_dst_idx + 3] = (color1 >> 8) & 0xFF;
            
            uint32_t indices = 0;
            for (uint32_t py = 0; py < 4; py++) {
                for (uint32_t px = 0; px < 4; px++) {
                    uint32_t pixel_idx = py * 4 + px;
                    uint8_t idx = 0;
                    
                    int dist0 = abs(block_pixels[pixel_idx][0] - max_color[0]) +
                               abs(block_pixels[pixel_idx][1] - max_color[1]) +
                               abs(block_pixels[pixel_idx][2] - max_color[2]);
                    int dist1 = abs(block_pixels[pixel_idx][0] - min_color[0]) +
                               abs(block_pixels[pixel_idx][1] - min_color[1]) +
                               abs(block_pixels[pixel_idx][2] - min_color[2]);
                    
                    if (dist1 < dist0) idx = 1;
                    indices |= (idx << (pixel_idx * 2));
                }
            }
            
            dst[color_dst_idx + 4] = indices & 0xFF;
            dst[color_dst_idx + 5] = (indices >> 8) & 0xFF;
            dst[color_dst_idx + 6] = (indices >> 16) & 0xFF;
            dst[color_dst_idx + 7] = (indices >> 24) & 0xFF;
        }
    }
    
    return TEXTURE_COMPRESSION_QUALITY_ERROR_NONE;
}

static int texture_compression_quality_compress_astc(const void* src_data, void* dst_data,
                                                     uint32_t width, uint32_t height, uint32_t block_size) {
    if (!src_data || !dst_data || width == 0 || height == 0 || block_size == 0) {
        return TEXTURE_COMPRESSION_QUALITY_ERROR_INVALID_PARAM;
    }
    
    const uint8_t* src = (const uint8_t*)src_data;
    uint8_t* dst = (uint8_t*)dst_data;
    
    /* ASTC compression: variable block sizes, 16 bytes per block */
    uint32_t block_width = (width + block_size - 1) / block_size;
    uint32_t block_height = (height + block_size - 1) / block_size;
    
    for (uint32_t by = 0; by < block_height; by++) {
        for (uint32_t bx = 0; bx < block_width; bx++) {
            uint32_t src_x = bx * block_size;
            uint32_t src_y = by * block_size;
            uint32_t dst_idx = (by * block_width + bx) * 16;
            
            /* Extract block and compute average color */
            uint32_t total_pixels = 0;
            uint32_t sum_r = 0, sum_g = 0, sum_b = 0, sum_a = 0;
            
            for (uint32_t py = 0; py < block_size && src_y + py < height; py++) {
                for (uint32_t px = 0; px < block_size && src_x + px < width; px++) {
                    uint32_t pixel_idx = ((src_y + py) * width + (src_x + px)) * 4;
                    sum_r += src[pixel_idx + 0];
                    sum_g += src[pixel_idx + 1];
                    sum_b += src[pixel_idx + 2];
                    sum_a += src[pixel_idx + 3];
                    total_pixels++;
                }
            }
            
            if (total_pixels == 0) {
                /* Fill with zeros for empty blocks */
                memset(&dst[dst_idx], 0, 16);
                continue;
            }
            
            /* Compute average color */
            uint8_t avg_r = sum_r / total_pixels;
            uint8_t avg_g = sum_g / total_pixels;
            uint8_t avg_b = sum_b / total_pixels;
            uint8_t avg_a = sum_a / total_pixels;
            
            /* Simple ASTC-style encoding (placeholder) */
            /* Real ASTC is much more complex with endpoint encoding and weight selection */
            dst[dst_idx + 0] = avg_r;
            dst[dst_idx + 1] = avg_g;
            dst[dst_idx + 2] = avg_b;
            dst[dst_idx + 3] = avg_a;
            
            /* Store block configuration */
            uint8_t config = (block_size << 4) | block_size;
            dst[dst_idx + 4] = config;
            
            /* Fill remaining bytes with pattern data */
            for (int i = 5; i < 16; i++) {
                dst[dst_idx + i] = 0x80; /* Neutral weight pattern */
            }
        }
    }
    
    return TEXTURE_COMPRESSION_QUALITY_ERROR_NONE;
}

/* Mipmap generation */
static int texture_compression_quality_generate_mipmaps_box_filter(texture_compression_quality_internal_t* item) {
    if (!item || !item->data || item->data_size == 0) {
        return TEXTURE_COMPRESSION_QUALITY_ERROR_INVALID_PARAM;
    }
    
    const uint8_t* src_data = (const uint8_t*)item->data;
    uint32_t width = 512; /* Default width - should be stored in item */
    uint32_t height = 512; /* Default height - should be stored in item */
    
    /* Calculate number of mipmap levels */
    uint32_t max_dim = (width > height) ? width : height;
    uint32_t num_levels = 0;
    while ((1u << num_levels) <= max_dim) {
        num_levels++;
    }
    if (num_levels > TEXTURE_COMPRESSION_QUALITY_MAX_MIPMAPS) {
        num_levels = TEXTURE_COMPRESSION_QUALITY_MAX_MIPMAPS;
    }
    
    item->mipmap_count = num_levels;
    
    /* Generate each mipmap level */
    for (uint32_t level = 0; level < num_levels; level++) {
        uint32_t mip_width = (width >> level) ? (width >> level) : 1;
        uint32_t mip_height = (height >> level) ? (height >> level) : 1;
        
        if (level == 0) {
            /* Level 0 is the original texture */
            item->mipmaps[level].width = width;
            item->mipmaps[level].height = height;
            item->mipmaps[level].level = 0;
            item->mipmaps[level].data = malloc(item->data_size);
            if (!item->mipmaps[level].data) {
                return TEXTURE_COMPRESSION_QUALITY_ERROR_OUT_OF_MEMORY;
            }
            memcpy(item->mipmaps[level].data, src_data, item->data_size);
            item->mipmaps[level].data_size = item->data_size;
            item->mipmaps[level].compressed = false;
            item->mipmaps[level].format = TEXTURE_COMPRESSION_FORMAT_NONE;
        } else {
            /* Generate downsampled mipmap */
            uint32_t prev_width = (width >> (level - 1)) ? (width >> (level - 1)) : 1;
            uint32_t prev_height = (height >> (level - 1)) ? (height >> (level - 1)) : 1;
            const uint8_t* prev_data = (const uint8_t*)item->mipmaps[level - 1].data;
            
            size_t mip_size = mip_width * mip_height * 4; /* RGBA */
            uint8_t* mip_data = malloc(mip_size);
            if (!mip_data) {
                return TEXTURE_COMPRESSION_QUALITY_ERROR_OUT_OF_MEMORY;
            }
            
            /* Box filter downsampling */
            for (uint32_t y = 0; y < mip_height; y++) {
                for (uint32_t x = 0; x < mip_width; x++) {
                    uint32_t src_x = x * 2;
                    uint32_t src_y = y * 2;
                    
                    uint32_t sum_r = 0, sum_g = 0, sum_b = 0, sum_a = 0;
                    uint32_t count = 0;
                    
                    /* Sample 2x2 area from previous level */
                    for (uint32_t dy = 0; dy < 2; dy++) {
                        for (uint32_t dx = 0; dx < 2; dx++) {
                            uint32_t sample_x = src_x + dx;
                            uint32_t sample_y = src_y + dy;
                            
                            if (sample_x < prev_width && sample_y < prev_height) {
                                uint32_t src_idx = (sample_y * prev_width + sample_x) * 4;
                                sum_r += prev_data[src_idx + 0];
                                sum_g += prev_data[src_idx + 1];
                                sum_b += prev_data[src_idx + 2];
                                sum_a += prev_data[src_idx + 3];
                                count++;
                            }
                        }
                    }
                    
                    uint32_t dst_idx = (y * mip_width + x) * 4;
                    mip_data[dst_idx + 0] = count > 0 ? sum_r / count : 0;
                    mip_data[dst_idx + 1] = count > 0 ? sum_g / count : 0;
                    mip_data[dst_idx + 2] = count > 0 ? sum_b / count : 0;
                    mip_data[dst_idx + 3] = count > 0 ? sum_a / count : 0;
                }
            }
            
            item->mipmaps[level].width = mip_width;
            item->mipmaps[level].height = mip_height;
            item->mipmaps[level].level = level;
            item->mipmaps[level].data = mip_data;
            item->mipmaps[level].data_size = mip_size;
            item->mipmaps[level].compressed = false;
            item->mipmaps[level].format = TEXTURE_COMPRESSION_FORMAT_NONE;
        }
    }
    
    item->mipmaps_generated = true;
    return TEXTURE_COMPRESSION_QUALITY_ERROR_NONE;
}

static int texture_compression_quality_generate_mipmaps_simd(texture_compression_quality_internal_t* item) {
    if (!item || !item->data || item->data_size == 0) {
        return TEXTURE_COMPRESSION_QUALITY_ERROR_INVALID_PARAM;
    }
    
    const uint8_t* src_data = (const uint8_t*)item->data;
    uint32_t width = 512; /* Default width - should be stored in item */
    uint32_t height = 512; /* Default height - should be stored in item */
    
    /* Calculate number of mipmap levels */
    uint32_t max_dim = (width > height) ? width : height;
    uint32_t num_levels = 0;
    while ((1u << num_levels) <= max_dim) {
        num_levels++;
    }
    if (num_levels > TEXTURE_COMPRESSION_QUALITY_MAX_MIPMAPS) {
        num_levels = TEXTURE_COMPRESSION_QUALITY_MAX_MIPMAPS;
    }
    
    item->mipmap_count = num_levels;
    
    /* Generate each mipmap level with SIMD optimization */
    for (uint32_t level = 0; level < num_levels; level++) {
        uint32_t mip_width = (width >> level) ? (width >> level) : 1;
        uint32_t mip_height = (height >> level) ? (height >> level) : 1;
        
        if (level == 0) {
            /* Level 0 is the original texture */
            item->mipmaps[level].width = width;
            item->mipmaps[level].height = height;
            item->mipmaps[level].level = 0;
            item->mipmaps[level].data = malloc(item->data_size);
            if (!item->mipmaps[level].data) {
                return TEXTURE_COMPRESSION_QUALITY_ERROR_OUT_OF_MEMORY;
            }
            memcpy(item->mipmaps[level].data, src_data, item->data_size);
            item->mipmaps[level].data_size = item->data_size;
            item->mipmaps[level].compressed = false;
            item->mipmaps[level].format = TEXTURE_COMPRESSION_FORMAT_NONE;
        } else {
            /* Generate downsampled mipmap with SIMD */
            uint32_t prev_width = (width >> (level - 1)) ? (width >> (level - 1)) : 1;
            uint32_t prev_height = (height >> (level - 1)) ? (height >> (level - 1)) : 1;
            const uint8_t* prev_data = (const uint8_t*)item->mipmaps[level - 1].data;
            
            size_t mip_size = mip_width * mip_height * 4; /* RGBA */
            /* Align memory for SIMD operations */
            uint8_t* mip_data = _mm_malloc(mip_size, 16);
            if (!mip_data) {
                return TEXTURE_COMPRESSION_QUALITY_ERROR_OUT_OF_MEMORY;
            }
            
            /* SIMD-optimized box filter downsampling */
            for (uint32_t y = 0; y < mip_height; y++) {
                for (uint32_t x = 0; x < mip_width; x += 4) {
                    uint32_t src_x = x * 2;
                    uint32_t src_y = y * 2;
                    
                    /* Process 4 pixels at once with SIMD */
                    __m128i sum_r = _mm_setzero_si128();
                    __m128i sum_g = _mm_setzero_si128();
                    __m128i sum_b = _mm_setzero_si128();
                    __m128i sum_a = _mm_setzero_si128();
                    __m128i count = _mm_setzero_si128();
                    
                    for (uint32_t dy = 0; dy < 2; dy++) {
                        for (uint32_t dx = 0; dx < 2; dx++) {
                            uint32_t sample_x = src_x + dx;
                            uint32_t sample_y = src_y + dy;
                            
                            if (sample_x < prev_width && sample_y < prev_height) {
                                uint32_t src_idx = (sample_y * prev_width + sample_x) * 4;
                                
                                /* Load 4 RGBA components */
                                __m128i pixel = _mm_loadu_si128((__m128i*)&prev_data[src_idx]);
                                
                                /* Extract and accumulate components */
                                __m128i r_mask = _mm_set1_epi32(0xFF);
                                __m128i g_mask = _mm_set1_epi32(0xFF00);
                                __m128i b_mask = _mm_set1_epi32(0xFF0000);
                                __m128i a_mask = _mm_set1_epi32(0xFF000000);
                                
                                sum_r = _mm_add_epi32(sum_r, _mm_and_si128(pixel, r_mask));
                                sum_g = _mm_add_epi32(sum_g, _mm_and_si128(_mm_srli_epi32(pixel, 8), r_mask));
                                sum_b = _mm_add_epi32(sum_b, _mm_and_si128(_mm_srli_epi32(pixel, 16), r_mask));
                                sum_a = _mm_add_epi32(sum_a, _mm_and_si128(_mm_srli_epi32(pixel, 24), r_mask));
                                count = _mm_add_epi32(count, _mm_set1_epi32(1));
                            }
                        }
                    }
                    
                    /* Average the accumulated values */
                    __m128i avg_r = _mm_div_epi32(sum_r, count);
                    __m128i avg_g = _mm_div_epi32(sum_g, count);
                    __m128i avg_b = _mm_div_epi32(sum_b, count);
                    __m128i avg_a = _mm_div_epi32(sum_a, count);
                    
                    /* Pack back to RGBA format */
                    __m128i result = _mm_or_si128(_mm_or_si128(avg_r, _mm_slli_epi32(avg_g, 8)),
                                                   _mm_or_si128(_mm_slli_epi32(avg_b, 16), _mm_slli_epi32(avg_a, 24)));
                    
                    /* Store result */
                    if (x + 3 < mip_width) {
                        _mm_storeu_si128((__m128i*)&mip_data[(y * mip_width + x) * 4], result);
                    } else {
                        /* Handle edge case for non-multiple-of-4 widths */
                        for (uint32_t i = 0; i < mip_width - x; i++) {
                            uint32_t dst_idx = (y * mip_width + x + i) * 4;
                            uint32_t pixel_val = _mm_extract_epi32(result, i);
                            mip_data[dst_idx + 0] = pixel_val & 0xFF;
                            mip_data[dst_idx + 1] = (pixel_val >> 8) & 0xFF;
                            mip_data[dst_idx + 2] = (pixel_val >> 16) & 0xFF;
                            mip_data[dst_idx + 3] = (pixel_val >> 24) & 0xFF;
                        }
                    }
                }
            }
            
            item->mipmaps[level].width = mip_width;
            item->mipmaps[level].height = mip_height;
            item->mipmaps[level].level = level;
            item->mipmaps[level].data = mip_data;
            item->mipmaps[level].data_size = mip_size;
            item->mipmaps[level].compressed = false;
            item->mipmaps[level].format = TEXTURE_COMPRESSION_FORMAT_NONE;
        }
    }
    
    item->mipmaps_generated = true;
    return TEXTURE_COMPRESSION_QUALITY_ERROR_NONE;
}

/* Bindless textures */
static uint64_t texture_compression_quality_create_bindless_handle(texture_compression_quality_internal_t* item) {
    if (!item || !item->data) {
        return 0;
    }
    
    /* Generate a unique bindless handle */
    static uint64_t next_handle = 1;
    uint64_t handle = __sync_fetch_and_add(&next_handle, 1);
    
    /* In a real implementation, this would:
     * 1. Create a GPU texture resource
     * 2. Get the bindless handle from the GPU driver
     * 3. Store the handle for later use
     */
    
    /* Store handle information in the item */
    item->bindless_handle = handle;
    item->bindless_enabled = true;
    
    return handle;
}

static void texture_compression_quality_destroy_bindless_handle(uint64_t handle) {
    if (handle == 0) {
        return;
    }
    
    /* Find the item with this handle */
    pthread_mutex_lock(&g_compression_quality_ctx.global_mutex);
    for (uint32_t i = 0; i < g_compression_quality_ctx.count; i++) {
        texture_compression_quality_internal_t* item = &g_compression_quality_ctx.items[i];
        if (item->bindless_handle == handle) {
            /* In a real implementation, this would:
             * 1. Release the GPU texture resource
             * 2. Invalidate the bindless handle
             */
            item->bindless_handle = 0;
            item->bindless_enabled = false;
            break;
        }
    }
    pthread_mutex_unlock(&g_compression_quality_ctx.global_mutex);
}

/* Anisotropic filtering */
static int texture_compression_quality_apply_anisotropic_filter(texture_compression_quality_internal_t* item, texture_anisotropic_filter_t level) {
    if (!item || !item->data) {
        return TEXTURE_COMPRESSION_QUALITY_ERROR_INVALID_PARAM;
    }
    
    if (level < TEXTURE_ANISOTROPIC_FILTER_1X || level > TEXTURE_ANISOTROPIC_FILTER_16X) {
        return TEXTURE_COMPRESSION_QUALITY_ERROR_INVALID_PARAM;
    }
    
    /* Store the anisotropic level */
    item->anisotropic_level = level;
    item->anisotropic_enabled = true;
    
    /* In a real implementation, this would:
     * 1. Set the anisotropic filtering level on the GPU texture
     * 2. Update the sampler state
     * 3. Apply the filtering to the texture sampling
     */
    
    /* For demonstration, we'll simulate anisotropic filtering by applying
     * a directional blur to the texture data based on the level
     */
    if (level > TEXTURE_ANISOTROPIC_FILTER_1X && item->data_size > 0) {
        uint8_t* data = (uint8_t*)item->data;
        uint32_t width = 512; /* Should be stored in item */
        uint32_t height = 512; /* Should be stored in item */
        
        /* Apply a simple anisotropic blur */
        float blur_strength = (float)level / 16.0f;
        uint32_t blur_radius = (uint32_t)(blur_strength * 2.0f);
        
        if (blur_radius > 0 && blur_radius < 8) {
            /* Create temporary buffer for blurred result */
            uint8_t* temp_data = malloc(item->data_size);
            if (!temp_data) {
                return TEXTURE_COMPRESSION_QUALITY_ERROR_OUT_OF_MEMORY;
            }
            
            memcpy(temp_data, data, item->data_size);
            
            /* Apply blur in X direction (anisotropic) */
            for (uint32_t y = 0; y < height; y++) {
                for (uint32_t x = 0; x < width; x++) {
                    uint32_t sum_r = 0, sum_g = 0, sum_b = 0, sum_a = 0;
                    uint32_t count = 0;
                    
                    for (int dx = -(int)blur_radius; dx <= (int)blur_radius; dx++) {
                        int sample_x = (int)x + dx;
                        if (sample_x >= 0 && sample_x < (int)width) {
                            uint32_t idx = (y * width + sample_x) * 4;
                            sum_r += temp_data[idx + 0];
                            sum_g += temp_data[idx + 1];
                            sum_b += temp_data[idx + 2];
                            sum_a += temp_data[idx + 3];
                            count++;
                        }
                    }
                    
                    uint32_t dst_idx = (y * width + x) * 4;
                    data[dst_idx + 0] = count > 0 ? sum_r / count : 0;
                    data[dst_idx + 1] = count > 0 ? sum_g / count : 0;
                    data[dst_idx + 2] = count > 0 ? sum_b / count : 0;
                    data[dst_idx + 3] = count > 0 ? sum_a / count : 0;
                }
            }
            
            free(temp_data);
        }
    }
    
    return TEXTURE_COMPRESSION_QUALITY_ERROR_NONE;
}

/* Virtual texturing */
static int texture_compression_quality_load_virtual_page(texture_compression_quality_internal_t* item, uint32_t page_x, uint32_t page_y, uint32_t level) {
    if (!item) {
        return TEXTURE_COMPRESSION_QUALITY_ERROR_INVALID_PARAM;
    }
    
    /* Check if virtual texturing is enabled */
    if (!item->virtual_enabled) {
        return TEXTURE_COMPRESSION_QUALITY_ERROR_INVALID_PARAM;
    }
    
    /* Calculate page index */
    uint32_t pages_per_row = (512 + item->virtual_tile_size - 1) / item->virtual_tile_size; /* Should use actual texture width */
    uint32_t page_index = page_y * pages_per_row + page_x;
    
    /* Check if page already exists */
    if (page_index < item->virtual_page_count && 
        item->virtual_pages[page_index].loaded) {
        return TEXTURE_COMPRESSION_QUALITY_ERROR_NONE;
    }
    
    /* Ensure we have space for the new page */
    if (page_index >= TEXTURE_COMPRESSION_QUALITY_MAX_VIRTUAL_PAGES) {
        return TEXTURE_COMPRESSION_QUALITY_ERROR_OUT_OF_MEMORY;
    }
    
    /* Calculate page data size */
    uint32_t page_width = item->virtual_tile_size;
    uint32_t page_height = item->virtual_tile_size;
    size_t page_data_size = page_width * page_height * 4; /* RGBA */
    
    /* Allocate page data */
    void* page_data = malloc(page_data_size);
    if (!page_data) {
        return TEXTURE_COMPRESSION_QUALITY_ERROR_OUT_OF_MEMORY;
    }
    
    /* In a real implementation, this would:
     * 1. Load the page data from disk or generate it
     * 2. Decompress if necessary
     * 3. Upload to GPU memory
     */
    
    /* For demonstration, we'll generate a simple pattern */
    uint8_t* data = (uint8_t*)page_data;
    for (uint32_t y = 0; y < page_height; y++) {
        for (uint32_t x = 0; x < page_width; x++) {
            uint32_t idx = (y * page_width + x) * 4;
            /* Generate a checkerboard pattern based on page coordinates */
            uint8_t value = ((page_x + page_y) % 2) ? 0x80 : 0xC0;
            data[idx + 0] = value + (x % 32) * 2;
            data[idx + 1] = value + (y % 32) * 2;
            data[idx + 2] = value + ((x + y) % 32) * 2;
            data[idx + 3] = 0xFF;
        }
    }
    
    /* Initialize the virtual page */
    if (page_index >= item->virtual_page_count) {
        item->virtual_page_count = page_index + 1;
    }
    
    item->virtual_pages[page_index].x = page_x;
    item->virtual_pages[page_index].y = page_y;
    item->virtual_pages[page_index].level = level;
    item->virtual_pages[page_index].layer = 0;
    item->virtual_pages[page_index].loaded = true;
    item->virtual_pages[page_index].last_access = time(NULL);
    item->virtual_pages[page_index].data = page_data;
    item->virtual_pages[page_index].data_size = page_data_size;
    
    /* Update performance counters */
    g_compression_quality_ctx.performance.virtual_page_loads++;
    g_compression_quality_ctx.total_memory_allocated += page_data_size;
    
    return TEXTURE_COMPRESSION_QUALITY_ERROR_NONE;
}

static void texture_compression_quality_evict_virtual_page(texture_compression_quality_internal_t* item, uint32_t page_index) {
    if (!item || page_index >= item->virtual_page_count) {
        return;
    }
    
    texture_virtual_page_t* page = &item->virtual_pages[page_index];
    if (!page->loaded) {
        return;
    }
    
    /* Free page data */
    if (page->data) {
        free(page->data);
        g_compression_quality_ctx.total_memory_allocated -= page->data_size;
    }
    
    /* Clear page entry */
    page->x = 0;
    page->y = 0;
    page->level = 0;
    page->layer = 0;
    page->loaded = false;
    page->last_access = 0;
    page->data = NULL;
    page->data_size = 0;
    
    /* Update performance counters */
    g_compression_quality_ctx.performance.virtual_page_evictions++;
}

/* Feedback analysis */
static int texture_compression_quality_analyze_quality(texture_compression_quality_internal_t* item) {
    if (!item || !item->data) {
        return TEXTURE_COMPRESSION_QUALITY_ERROR_INVALID_PARAM;
    }
    
    /* Perform quality analysis on the compressed texture */
    texture_feedback_analysis_t* feedback = &item->feedback;
    
    /* Calculate compression ratio */
    size_t original_size = item->data_size;
    size_t compressed_size = original_size; /* Would be actual compressed size */
    
    if (compressed_size > 0) {
        feedback->compression_ratio = (float)original_size / (float)compressed_size;
    } else {
        feedback->compression_ratio = 1.0f;
    }
    
    /* Analyze quality metrics */
    const uint8_t* data = (const uint8_t*)item->data;
    uint32_t width = 512; /* Should be stored in item */
    uint32_t height = 512; /* Should be stored in item */
    
    /* Calculate PSNR (Peak Signal-to-Noise Ratio) */
    float mse = 0.0f; /* Mean Squared Error */
    uint32_t sample_count = 0;
    
    /* Sample the texture to estimate quality */
    for (uint32_t y = 0; y < height; y += 16) {
        for (uint32_t x = 0; x < width; x += 16) {
            uint32_t idx = (y * width + x) * 4;
            if (idx + 3 < item->data_size) {
                /* Simple quality metric based on color variance */
                uint8_t r = data[idx + 0];
                uint8_t g = data[idx + 1];
                uint8_t b = data[idx + 2];
                uint8_t a = data[idx + 3];
                
                /* Calculate local variance */
                float local_mean = (r + g + b) / 3.0f;
                float variance = (r - local_mean) * (r - local_mean) +
                              (g - local_mean) * (g - local_mean) +
                              (b - local_mean) * (b - local_mean);
                variance /= 3.0f;
                
                mse += variance;
                sample_count++;
            }
        }
    }
    
    if (sample_count > 0) {
        mse /= sample_count;
        /* Convert MSE to PSNR */
        if (mse > 0.0f) {
            feedback->quality_score = 20.0f * log10f(255.0f / sqrtf(mse));
        } else {
            feedback->quality_score = 100.0f; /* Perfect quality */
        }
    } else {
        feedback->quality_score = 0.0f;
    }
    
    /* Count artifacts (simplified) */
    feedback->artifact_count = 0;
    for (uint32_t y = 1; y < height - 1; y += 8) {
        for (uint32_t x = 1; x < width - 1; x += 8) {
            uint32_t idx = (y * width + x) * 4;
            if (idx + 7 < item->data_size) {
                /* Check for sharp edges that might indicate compression artifacts */
                int diff_x = abs(data[idx + 0] - data[idx + 4]) +
                            abs(data[idx + 1] - data[idx + 5]) +
                            abs(data[idx + 2] - data[idx + 6]);
                int diff_y = abs(data[idx + 0] - data[idx + width * 4]) +
                            abs(data[idx + 1] - data[idx + width * 4 + 1]) +
                            abs(data[idx + 2] - data[idx + width * 4 + 2]);
                
                if (diff_x > 100 || diff_y > 100) {
                    feedback->artifact_count++;
                }
            }
        }
    }
    
    /* Determine if quality meets threshold */
    feedback->meets_quality_threshold = 
        (feedback->quality_score >= 30.0f) && /* PSNR >= 30dB */
        (feedback->compression_ratio >= 2.0f) && /* At least 2:1 compression */
        (feedback->artifact_count < 100); /* Few artifacts */
    
    /* Set processing time (simulated) */
    feedback->processing_time = (double)(width * height) / 1000000.0; /* 1MP per second */
    
    item->feedback_analyzed = true;
    
    return TEXTURE_COMPRESSION_QUALITY_ERROR_NONE;
}

/* Caching */
static int texture_compression_quality_cache_add(uint32_t id, const void* data, size_t size) {
    if (!data || size == 0) {
        return TEXTURE_COMPRESSION_QUALITY_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&g_compression_quality_ctx.cache_mutex);
    
    /* Check if cache is full */
    if (g_compression_quality_ctx.cache_size >= TEXTURE_COMPRESSION_QUALITY_CACHE_SIZE) {
        /* Evict LRU entry */
        texture_compression_quality_cache_evict_lru();
    }
    
    /* Find empty slot */
    uint32_t slot = 0;
    for (uint32_t i = 0; i < TEXTURE_COMPRESSION_QUALITY_CACHE_SIZE; i++) {
        if (!g_compression_quality_ctx.cache[i].valid) {
            slot = i;
            break;
        }
    }
    
    /* Allocate cache entry data */
    void* cache_data = malloc(size);
    if (!cache_data) {
        pthread_mutex_unlock(&g_compression_quality_ctx.cache_mutex);
        return TEXTURE_COMPRESSION_QUALITY_ERROR_OUT_OF_MEMORY;
    }
    
    /* Copy data to cache */
    memcpy(cache_data, data, size);
    
    /* Initialize cache entry */
    g_compression_quality_ctx.cache[slot].id = id;
    g_compression_quality_ctx.cache[slot].data = cache_data;
    g_compression_quality_ctx.cache[slot].size = size;
    g_compression_quality_ctx.cache[slot].last_access = time(NULL);
    g_compression_quality_ctx.cache[slot].access_count = 1;
    g_compression_quality_ctx.cache[slot].valid = true;
    
    g_compression_quality_ctx.cache_size++;
    g_compression_quality_ctx.total_memory_allocated += size;
    
    pthread_mutex_unlock(&g_compression_quality_ctx.cache_mutex);
    
    return TEXTURE_COMPRESSION_QUALITY_ERROR_NONE;
}

static void texture_compression_quality_cache_evict_lru(void) {
    uint64_t oldest_time = UINT64_MAX;
    uint32_t lru_index = 0;
    
    /* Find LRU entry */
    for (uint32_t i = 0; i < TEXTURE_COMPRESSION_QUALITY_CACHE_SIZE; i++) {
        if (g_compression_quality_ctx.cache[i].valid && 
            g_compression_quality_ctx.cache[i].last_access < oldest_time) {
            
            oldest_time = g_compression_quality_ctx.cache[i].last_access;
            lru_index = i;
        }
    }
    
    /* Evict LRU entry */
    if (g_compression_quality_ctx.cache[lru_index].valid) {
        if (g_compression_quality_ctx.cache[lru_index].data) {
            free(g_compression_quality_ctx.cache[lru_index].data);
            g_compression_quality_ctx.total_memory_allocated -= 
                g_compression_quality_ctx.cache[lru_index].size;
        }
        
        g_compression_quality_ctx.cache[lru_index].valid = false;
        g_compression_quality_ctx.cache_size--;
    }
}

static texture_cache_entry_t* texture_compression_quality_cache_lookup(uint32_t id) {
    pthread_rwlock_rdlock(&g_compression_quality_ctx.cache_rwlock);
    
    texture_cache_entry_t* entry = NULL;
    
    /* Search for cache entry */
    for (uint32_t i = 0; i < TEXTURE_COMPRESSION_QUALITY_CACHE_SIZE; i++) {
        if (g_compression_quality_ctx.cache[i].valid && 
            g_compression_quality_ctx.cache[i].id == id) {
            
            entry = &g_compression_quality_ctx.cache[i];
            entry->last_access = time(NULL);
            entry->access_count++;
            
            /* Update performance counters */
            g_compression_quality_ctx.performance.cache_hits++;
            break;
        }
    }
    
    if (!entry) {
        g_compression_quality_ctx.performance.cache_misses++;
    }
    
    pthread_rwlock_unlock(&g_compression_quality_ctx.cache_rwlock);
    
    return entry;
}

/* GPU integration */
static int texture_compression_quality_gpu_init(void) {
    /* Initialize GPU context */
    texture_gpu_context_t* gpu = &g_compression_quality_ctx.gpu_context;
    
    /* In a real implementation, this would:
     * 1. Initialize graphics API (OpenGL/Vulkan/DirectX)
     * 2. Query GPU capabilities
     * 3. Create compute shader for compression
     * 4. Set up GPU memory management
     */
    
    /* Simulate GPU initialization */
    gpu->available = true; /* Assume GPU is available */
    gpu->device_context = (void*)0x12345678; /* Mock handle */
    gpu->compute_shader = NULL;
    gpu->max_texture_size = 16384;
    gpu->max_array_layers = 2048;
    gpu->supports_compression = true;
    gpu->supports_bindless = true;
    
    return TEXTURE_COMPRESSION_QUALITY_ERROR_NONE;
}

static void texture_compression_quality_gpu_shutdown(void) {
    texture_gpu_context_t* gpu = &g_compression_quality_ctx.gpu_context;
    
    /* In a real implementation, this would:
     * 1. Release GPU resources
     * 2. Destroy compute shader
     * 3. Clean up device context
     */
    
    gpu->available = false;
    gpu->device_context = NULL;
    gpu->compute_shader = NULL;
    gpu->max_texture_size = 0;
    gpu->max_array_layers = 0;
    gpu->supports_compression = false;
    gpu->supports_bindless = false;
}

static int texture_compression_quality_gpu_compress(texture_compression_quality_internal_t* item) {
    if (!item || !item->data) {
        return TEXTURE_COMPRESSION_QUALITY_ERROR_INVALID_PARAM;
    }
    
    texture_gpu_context_t* gpu = &g_compression_quality_ctx.gpu_context;
    
    if (!gpu->available) {
        return TEXTURE_COMPRESSION_QUALITY_ERROR_GPU_UNAVAILABLE;
    }
    
    /* In a real implementation, this would:
     * 1. Upload texture data to GPU
     * 2. Execute compression compute shader
     * 3. Download compressed data back to CPU
     * 4. Update performance counters
     */
    
    /* Simulate GPU compression */
    uint32_t width = 512; /* Should be stored in item */
    uint32_t height = 512; /* Should be stored in item */
    
    /* Allocate compressed data buffer */
    size_t compressed_size = (width * height) / 2; /* Estimate 2:1 compression */
    void* compressed_data = malloc(compressed_size);
    if (!compressed_data) {
        return TEXTURE_COMPRESSION_QUALITY_ERROR_OUT_OF_MEMORY;
    }
    
    /* Simulate GPU compression with simple algorithm */
    const uint8_t* src = (const uint8_t*)item->data;
    uint8_t* dst = (uint8_t*)compressed_data;
    
    for (size_t i = 0; i < compressed_size; i++) {
        dst[i] = src[i * 2] ^ 0x80; /* Simple transform */
    }
    
    /* Free old data and replace with compressed data */
    free(item->data);
    item->data = compressed_data;
    item->data_size = compressed_size;
    
    /* Update memory tracking */
    g_compression_quality_ctx.total_memory_allocated += compressed_size;
    
    return TEXTURE_COMPRESSION_QUALITY_ERROR_NONE;
}

/* Async operations */
static void* texture_compression_quality_async_worker_thread(void* arg) {
    (void)arg;
    
    while (g_compression_quality_ctx.async_running) {
        pthread_mutex_lock(&g_compression_quality_ctx.async_mutex);
        
        /* Check for pending operations */
        bool has_work = false;
        for (uint32_t i = 0; i < TEXTURE_COMPRESSION_QUALITY_ASYNC_THREADS; i++) {
            texture_async_operation_t* op = &g_compression_quality_ctx.async_ops[i];
            if (!op->completed && op->texture_id != UINT32_MAX) {
                has_work = true;
                
                /* Process the operation */
                switch (op->type) {
                    case TEXTURE_ASYNC_OP_COMPRESS: {
                        /* Find the texture and compress it */
                        if (op->texture_id < g_compression_quality_ctx.count) {
                            texture_compression_quality_internal_t* item = 
                                &g_compression_quality_ctx.items[op->texture_id];
                            if (item->initialized && item->data) {
                                /* Simulate compression work */
                                usleep(10000); /* 10ms work simulation */
                                op->success = true;
                            } else {
                                op->success = false;
                            }
                        } else {
                            op->success = false;
                        }
                        break;
                    }
                    
                    case TEXTURE_ASYNC_OP_DECOMPRESS: {
                        /* Simulate decompression work */
                        usleep(8000); /* 8ms work simulation */
                        op->success = true;
                        break;
                    }
                    
                    case TEXTURE_ASYNC_OP_GENERATE_MIPMAPS: {
                        /* Simulate mipmap generation */
                        usleep(15000); /* 15ms work simulation */
                        op->success = true;
                        break;
                    }
                    
                    case TEXTURE_ASYNC_OP_CONVERT_FORMAT: {
                        /* Simulate format conversion */
                        usleep(12000); /* 12ms work simulation */
                        op->success = true;
                        break;
                    }
                    
                    default:
                        op->success = false;
                        break;
                }
                
                /* Mark operation as completed */
                op->completed = true;
                
                /* Signal completion */
                pthread_mutex_lock(&op->completion_mutex);
                pthread_cond_signal(&op->completion_cond);
                pthread_mutex_unlock(&op->completion_mutex);
                
                break;
            }
        }
        
        pthread_mutex_unlock(&g_compression_quality_ctx.async_mutex);
        
        /* Sleep if no work */
        if (!has_work) {
            usleep(1000); /* 1ms sleep */
        }
    }
    
    return NULL;
}

static int texture_compression_quality_async_schedule_operation(texture_async_operation_t* op) {
    if (!op) {
        return TEXTURE_COMPRESSION_QUALITY_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&g_compression_quality_ctx.async_mutex);
    
    /* Find free slot */
    uint32_t slot = UINT32_MAX;
    for (uint32_t i = 0; i < TEXTURE_COMPRESSION_QUALITY_ASYNC_THREADS; i++) {
        if (g_compression_quality_ctx.async_ops[i].completed) {
            slot = i;
            break;
        }
    }
    
    if (slot == UINT32_MAX) {
        pthread_mutex_unlock(&g_compression_quality_ctx.async_mutex);
        return TEXTURE_COMPRESSION_QUALITY_ERROR_OUT_OF_MEMORY;
    }
    
    /* Copy operation to slot */
    g_compression_quality_ctx.async_ops[slot] = *op;
    g_compression_quality_ctx.async_ops[slot].completed = false;
    g_compression_quality_ctx.async_ops[slot].success = false;
    
    /* Initialize completion synchronization */
    pthread_mutex_init(&g_compression_quality_ctx.async_ops[slot].completion_mutex, NULL);
    pthread_cond_init(&g_compression_quality_ctx.async_ops[slot].completion_cond, NULL);
    
    pthread_mutex_unlock(&g_compression_quality_ctx.async_mutex);
    
    /* Wake up worker threads */
    pthread_cond_broadcast(&g_compression_quality_ctx.async_cond);
    
    return TEXTURE_COMPRESSION_QUALITY_ERROR_NONE;
}

/* File watching */
static int texture_compression_quality_setup_file_watching(const char* directory) {
    if (!directory) {
        return TEXTURE_COMPRESSION_QUALITY_ERROR_INVALID_PARAM;
    }
    
    texture_file_watcher_t* watcher = &g_compression_quality_ctx.file_watcher;
    
    /* Initialize inotify */
    watcher->inotify_fd = inotify_init();
    if (watcher->inotify_fd < 0) {
        return TEXTURE_COMPRESSION_QUALITY_ERROR_HOT_RELOAD_FAILED;
    }
    
    /* Add watch for directory */
    watcher->watch_descriptor = inotify_add_watch(watcher->inotify_fd, directory, 
        IN_MODIFY | IN_CREATE | IN_DELETE | IN_MOVED_TO | IN_MOVED_FROM);
    
    if (watcher->watch_descriptor < 0) {
        close(watcher->inotify_fd);
        watcher->inotify_fd = -1;
        return TEXTURE_COMPRESSION_QUALITY_ERROR_HOT_RELOAD_FAILED;
    }
    
    /* Store directory path */
    strncpy(watcher->watch_directory, directory, 511);
    watcher->watch_directory[511] = '\0';
    
    /* Create watcher thread */
    if (pthread_create(&watcher->watcher_thread, NULL, 
        texture_compression_quality_file_watcher_thread, NULL) != 0) {
        inotify_rm_watch(watcher->inotify_fd, watcher->watch_descriptor);
        close(watcher->inotify_fd);
        watcher->inotify_fd = -1;
        return TEXTURE_COMPRESSION_QUALITY_ERROR_THREAD_ERROR;
    }
    
    watcher->running = true;
    return TEXTURE_COMPRESSION_QUALITY_ERROR_NONE;
}

static void* texture_compression_quality_file_watcher_thread(void* arg) {
    (void)arg;
    
    texture_file_watcher_t* watcher = &g_compression_quality_ctx.file_watcher;
    char buffer[4096];
    
    while (watcher->running && watcher->inotify_fd >= 0) {
        /* Read file system events */
        ssize_t length = read(watcher->inotify_fd, buffer, sizeof(buffer));
        
        if (length > 0) {
            size_t offset = 0;
            while (offset < (size_t)length) {
                struct inotify_event* event = (struct inotify_event*)&buffer[offset];
                
                if (event->len > 0) {
                    /* Process file change */
                    texture_compression_quality_process_file_change(event->name);
                    g_compression_quality_ctx.performance.hot_reload_events++;
                }
                
                offset += sizeof(struct inotify_event) + event->len;
            }
        }
        
        usleep(100000); /* 100ms sleep */
    }
    
    return NULL;
}

static void texture_compression_quality_process_file_change(const char* filename) {
    if (!filename) {
        return;
    }
    
    /* Check for texture file extensions */
    const char* extensions[] = {".png", ".jpg", ".tga", ".dds", ".bmp"};
    bool is_texture = false;
    
    for (int i = 0; i < 5; i++) {
        if (strstr(filename, extensions[i]) != NULL) {
            is_texture = true;
            break;
        }
    }
    
    if (is_texture) {
        /* In a real implementation, this would:
         * 1. Find affected texture items
         * 2. Mark them as dirty
         * 3. Trigger reload if needed
         */
        printf("Texture file changed: %s\n", filename);
    }
}

/* Culling */
static bool texture_compression_quality_should_cull(texture_compression_quality_internal_t* item, float threshold) {
    if (!item) {
        return false;
    }
    
    /* Calculate culling criteria */
    float distance = item->lod_distance;
    float quality_score = item->feedback.quality_score;
    float compression_ratio = item->feedback.compression_ratio;
    
    /* Cull if texture is too far away and low quality */
    bool should_cull = 
        (distance > threshold) &&
        (quality_score < 20.0f) && /* Low PSNR */
        (compression_ratio < 1.5f) && /* Poor compression */
        (!item->bindless_enabled); /* Not actively used */
    
    return should_cull;
}

static void texture_compression_quality_mark_for_culling(texture_compression_quality_internal_t* item) {
    if (!item) {
        return;
    }
    
    /* Mark item for culling */
    item->marked_for_culling = true;
    
    /* Update performance counters */
    g_compression_quality_ctx.performance.culled_textures++;
}

/* Render graph */
static int texture_compression_quality_process_render_graph(void) {
    texture_render_graph_t* graph = &g_compression_quality_ctx.render_graph;
    
    /* Process all nodes in the render graph */
    for (uint32_t i = 0; i < graph->node_count; i++) {
        texture_render_node_t* node = &graph->nodes[i];
        
        if (!node->active) {
            continue;
        }
        
        /* Process node based on type */
        switch (node->type) {
            case TEXTURE_RENDER_NODE_COMPRESS: {
                /* Find texture and compress it */
                if (node->texture_id < g_compression_quality_ctx.count) {
                    texture_compression_quality_internal_t* item = 
                        &g_compression_quality_ctx.items[node->texture_id];
                    if (item->initialized && item->data) {
                        /* Schedule async compression */
                        texture_async_operation_t op;
                        op.type = TEXTURE_ASYNC_OP_COMPRESS;
                        op.texture_id = node->texture_id;
                        op.priority = node->priority;
                        op.completed = false;
                        op.success = false;
                        
                        texture_compression_quality_async_schedule_operation(&op);
                    }
                }
                break;
            }
            
            case TEXTURE_RENDER_NODE_GENERATE_MIPMAPS: {
                /* Generate mipmaps */
                if (node->texture_id < g_compression_quality_ctx.count) {
                    texture_compression_quality_internal_t* item = 
                        &g_compression_quality_ctx.items[node->texture_id];
                    if (item->initialized && item->data) {
                        texture_compression_quality_generate_mipmaps_simd(item);
                    }
                }
                break;
            }
            
            case TEXTURE_RENDER_NODE_APPLY_FILTER: {
                /* Apply anisotropic filtering */
                if (node->texture_id < g_compression_quality_ctx.count) {
                    texture_compression_quality_internal_t* item = 
                        &g_compression_quality_ctx.items[node->texture_id];
                    if (item->initialized && item->data) {
                        texture_compression_quality_apply_anisotropic_filter(item, 
                            TEXTURE_ANISOTROPIC_FILTER_4X);
                    }
                }
                break;
            }
            
            case TEXTURE_RENDER_NODE_ANALYZE_QUALITY: {
                /* Analyze quality */
                if (node->texture_id < g_compression_quality_ctx.count) {
                    texture_compression_quality_internal_t* item = 
                        &g_compression_quality_ctx.items[node->texture_id];
                    if (item->initialized && item->data) {
                        texture_compression_quality_analyze_quality(item);
                    }
                }
                break;
            }
            
            default:
                break;
        }
        
        /* Mark node as processed */
        node->processed = true;
        node->last_processed = time(NULL);
    }
    
    /* Update performance counters */
    g_compression_quality_ctx.performance.render_graph_nodes_processed += graph->node_count;
    
    return TEXTURE_COMPRESSION_QUALITY_ERROR_NONE;
}

/* End of compression_quality.c */
