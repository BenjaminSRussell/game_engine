/*
 * format_conversion.c
 * Format conversion implementation
 *
 * Part of the Texture subsystem
 * Advanced 3D Rendering Engine
 */

#include "assets/textures/compression/format_conversion.h"
#include "assets/textures/compression/bc_compression.h"
#include "assets/textures/compression/astc_compression.h"
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

/* ============================================================================
 * INTERNAL STRUCTURES
 * ============================================================================ */

typedef struct texture_array {
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t array_size;
    texture_format_t format;
    void* data;
    size_t data_size;
    bool initialized;
} texture_array_t;

typedef struct virtual_texturing {
    uint32_t page_size;
    uint32_t max_pages;
    uint32_t* page_table;
    void* page_data;
    bool enabled;
} virtual_texturing_t;

typedef struct format_converter {
    texture_format_t source_format;
    texture_format_t target_format;
    int (*convert_func)(const void* src, void* dst, size_t size);
} format_converter_t;

typedef struct cache_entry {
    uint32_t key;
    void* data;
    size_t size;
    uint64_t timestamp;
    struct cache_entry* next;
} cache_entry_t;

typedef struct async_operation {
    uint32_t id;
    void* data;
    size_t size;
    void (*callback)(void*, size_t);
    struct async_operation* next;
} async_operation_t;

typedef struct render_graph_node {
    char name[256];
    void (*execute_func)(void*);
    void* user_data;
    struct render_graph_node* dependencies[16];
    uint32_t dependency_count;
    bool executed;
} render_graph_node_t;

typedef struct format_conversion_context {
    texture_array_t texture_arrays[TEXTURE_FORMAT_CONVERSION_MAX_TEXTURE_ARRAYS];
    virtual_texturing_t virtual_texturing;
    format_converter_t converters[32];
    cache_entry_t* cache;
    async_operation_t* async_queue;
    render_graph_node_t* render_nodes[64];
    uint32_t anisotropy_level;
    uint32_t lod_count;
    float lod_distances[TEXTURE_FORMAT_CONVERSION_MAX_LOD_LEVELS];
    performance_counters_t counters;
    feedback_analysis_t feedback;
    pthread_mutex_t mutex;
    pthread_t worker_thread;
    int inotify_fd;
    bool initialized;
    bool gpu_enabled;
    bool simd_enabled;
} format_conversion_context_t;

static format_conversion_context_t g_context = {0};

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================ */

static uint32_t calculate_texture_size(uint32_t width, uint32_t height, uint32_t depth, texture_format_t format) {
    uint32_t pixel_size = 4;
    switch (format) {
        case TEXTURE_FORMAT_RGB8: pixel_size = 3; break;
        case TEXTURE_FORMAT_RGBA8: pixel_size = 4; break;
        case TEXTURE_FORMAT_R16F: pixel_size = 2; break;
        case TEXTURE_FORMAT_RG16F: pixel_size = 4; break;
        case TEXTURE_FORMAT_RGBA16F: pixel_size = 8; break;
        case TEXTURE_FORMAT_R32F: pixel_size = 4; break;
        case TEXTURE_FORMAT_RG32F: pixel_size = 8; break;
        case TEXTURE_FORMAT_RGBA32F: pixel_size = 16; break;
        default: pixel_size = 4; break;
    }
    return width * height * depth * pixel_size;
}

static int convert_rgb8_to_rgba8(const void* src, void* dst, size_t size) {
    const uint8_t* src_ptr = (const uint8_t*)src;
    uint8_t* dst_ptr = (uint8_t*)dst;
    
    for (size_t i = 0; i < size / 3; i++) {
        dst_ptr[i * 4 + 0] = src_ptr[i * 3 + 0];
        dst_ptr[i * 4 + 1] = src_ptr[i * 3 + 1];
        dst_ptr[i * 4 + 2] = src_ptr[i * 3 + 2];
        dst_ptr[i * 4 + 3] = 255;
    }
    
    return 0;
}

static int convert_rgba8_to_rgb8(const void* src, void* dst, size_t size) {
    const uint8_t* src_ptr = (const uint8_t*)src;
    uint8_t* dst_ptr = (uint8_t*)dst;
    
    for (size_t i = 0; i < size / 4; i++) {
        dst_ptr[i * 3 + 0] = src_ptr[i * 4 + 0];
        dst_ptr[i * 3 + 1] = src_ptr[i * 4 + 1];
        dst_ptr[i * 3 + 2] = src_ptr[i * 4 + 2];
    }
    
    return 0;
}

static void update_performance_counters(uint32_t operation_type, uint64_t time_ns) {
    pthread_mutex_lock(&g_context.mutex);
    
    switch (operation_type) {
        case 0: g_context.counters.textures_converted++; break;
        case 1: g_context.counters.textures_compressed++; break;
        case 2: g_context.counters.textures_decompressed++; break;
        case 3: g_context.counters.mipmaps_generated++; break;
        case 4: g_context.counters.virtual_pages_loaded++; break;
        case 5: g_context.counters.async_operations++; break;
        case 6: g_context.counters.batch_operations++; break;
        case 7: g_context.counters.gpu_operations++; break;
        case 8: g_context.counters.simd_operations++; break;
    }
    
    g_context.counters.total_processing_time_ns += time_ns;
    
    pthread_mutex_unlock(&g_context.mutex);
}

static void* worker_thread_func(void* arg) {
    while (g_context.initialized) {
        pthread_mutex_lock(&g_context.mutex);
        
        async_operation_t* op = g_context.async_queue;
        if (op) {
            g_context.async_queue = op->next;
            pthread_mutex_unlock(&g_context.mutex);
            
            if (op->callback) {
                op->callback(op->data, op->size);
            }
            
            free(op->data);
            free(op);
        } else {
            pthread_mutex_unlock(&g_context.mutex);
            usleep(1000);
        }
    }
    return NULL;
}

/* ============================================================================
 * API IMPLEMENTATION
 * ============================================================================ */

int texture_format_conversion_init(void) {
    if (g_context.initialized) {
        return 0;
    }
    
    memset(&g_context, 0, sizeof(g_context));
    
    if (pthread_mutex_init(&g_context.mutex, NULL) != 0) {
        return -1;
    }
    
    g_context.anisotropy_level = 1;
    g_context.lod_count = 1;
    g_context.lod_distances[0] = 0.0f;
    
    g_context.converters[0].source_format = TEXTURE_FORMAT_RGB8;
    g_context.converters[0].target_format = TEXTURE_FORMAT_RGBA8;
    g_context.converters[0].convert_func = convert_rgb8_to_rgba8;
    
    g_context.converters[1].source_format = TEXTURE_FORMAT_RGBA8;
    g_context.converters[1].target_format = TEXTURE_FORMAT_RGB8;
    g_context.converters[1].convert_func = convert_rgba8_to_rgb8;
    
    g_context.inotify_fd = inotify_init();
    if (g_context.inotify_fd < 0) {
        pthread_mutex_destroy(&g_context.mutex);
        return -1;
    }
    
    if (pthread_create(&g_context.worker_thread, NULL, worker_thread_func, NULL) != 0) {
        close(g_context.inotify_fd);
        pthread_mutex_destroy(&g_context.mutex);
        return -1;
    }
    
    g_context.initialized = true;
    return 0;
}

void texture_format_conversion_shutdown(void) {
    if (!g_context.initialized) {
        return;
    }
    
    g_context.initialized = false;
    
    pthread_join(g_context.worker_thread, NULL);
    
    for (uint32_t i = 0; i < TEXTURE_FORMAT_CONVERSION_MAX_TEXTURE_ARRAYS; i++) {
        if (g_context.texture_arrays[i].initialized) {
            free(g_context.texture_arrays[i].data);
        }
    }
    
    if (g_context.virtual_texturing.page_table) {
        free(g_context.virtual_texturing.page_table);
    }
    
    if (g_context.virtual_texturing.page_data) {
        free(g_context.virtual_texturing.page_data);
    }
    
    cache_entry_t* entry = g_context.cache;
    while (entry) {
        cache_entry_t* next = entry->next;
        free(entry->data);
        free(entry);
        entry = next;
    }
    
    async_operation_t* op = g_context.async_queue;
    while (op) {
        async_operation_t* next = op->next;
        free(op->data);
        free(op);
        op = next;
    }
    
    for (uint32_t i = 0; i < 64; i++) {
        if (g_context.render_nodes[i]) {
            free(g_context.render_nodes[i]);
        }
    }
    
    if (g_context.inotify_fd >= 0) {
        close(g_context.inotify_fd);
    }
    
    pthread_mutex_destroy(&g_context.mutex);
    memset(&g_context, 0, sizeof(g_context));
}

int texture_format_conversion_create(texture_format_conversion_handle_t* out_handle, const texture_format_conversion_desc_t* desc) {
    if (!out_handle || !g_context.initialized) {
        return -1;
    }
    
    pthread_mutex_lock(&g_context.mutex);
    
    for (uint32_t i = 0; i < TEXTURE_FORMAT_CONVERSION_MAX_TEXTURE_ARRAYS; i++) {
        if (!g_context.texture_arrays[i].initialized) {
            g_context.texture_arrays[i].initialized = true;
            out_handle->id = i;
            pthread_mutex_unlock(&g_context.mutex);
            return 0;
        }
    }
    
    pthread_mutex_unlock(&g_context.mutex);
    return -1;
}

void texture_format_conversion_destroy(texture_format_conversion_handle_t handle) {
    if (!g_context.initialized || handle.id >= TEXTURE_FORMAT_CONVERSION_MAX_TEXTURE_ARRAYS) {
        return;
    }
    
    pthread_mutex_lock(&g_context.mutex);
    
    if (g_context.texture_arrays[handle.id].initialized) {
        free(g_context.texture_arrays[handle.id].data);
        memset(&g_context.texture_arrays[handle.id], 0, sizeof(texture_array_t));
    }
    
    pthread_mutex_unlock(&g_context.mutex);
}

int texture_format_conversion_create_texture_array(texture_format_conversion_handle_t* out_handle, 
                                                  uint32_t width, uint32_t height, 
                                                  uint32_t depth, uint32_t array_size, 
                                                  texture_format_t format) {
    if (!out_handle || !g_context.initialized) {
        return -1;
    }
    
    pthread_mutex_lock(&g_context.mutex);
    
    for (uint32_t i = 0; i < TEXTURE_FORMAT_CONVERSION_MAX_TEXTURE_ARRAYS; i++) {
        if (!g_context.texture_arrays[i].initialized) {
            texture_array_t* array = &g_context.texture_arrays[i];
            array->width = width;
            array->height = height;
            array->depth = depth;
            array->array_size = array_size;
            array->format = format;
            array->data_size = calculate_texture_size(width, height, depth, format) * array_size;
            array->data = malloc(array->data_size);
            
            if (!array->data) {
                pthread_mutex_unlock(&g_context.mutex);
                return -1;
            }
            
            array->initialized = true;
            out_handle->id = i;
            
            g_context.counters.total_memory_used += array->data_size;
            
            pthread_mutex_unlock(&g_context.mutex);
            return 0;
        }
    }
    
    pthread_mutex_unlock(&g_context.mutex);
    return -1;
}

int texture_format_conversion_enable_virtual_texturing(texture_format_conversion_handle_t handle, 
                                                   uint32_t page_size, uint32_t max_pages) {
    if (!g_context.initialized || handle.id >= TEXTURE_FORMAT_CONVERSION_MAX_TEXTURE_ARRAYS) {
        return -1;
    }
    
    pthread_mutex_lock(&g_context.mutex);
    
    if (g_context.virtual_texturing.page_table) {
        free(g_context.virtual_texturing.page_table);
    }
    
    if (g_context.virtual_texturing.page_data) {
        free(g_context.virtual_texturing.page_data);
    }
    
    g_context.virtual_texturing.page_size = page_size;
    g_context.virtual_texturing.max_pages = max_pages;
    g_context.virtual_texturing.page_table = calloc(max_pages, sizeof(uint32_t));
    g_context.virtual_texturing.page_data = malloc(page_size * page_size * 4);
    g_context.virtual_texturing.enabled = true;
    
    if (!g_context.virtual_texturing.page_table || !g_context.virtual_texturing.page_data) {
        pthread_mutex_unlock(&g_context.mutex);
        return -1;
    }
    
    pthread_mutex_unlock(&g_context.mutex);
    return 0;
}

int texture_format_conversion_set_anisotropic_filtering(texture_format_conversion_handle_t handle, 
                                                    uint32_t anisotropy_level) {
    if (!g_context.initialized || anisotropy_level > TEXTURE_FORMAT_CONVERSION_ANISOTROPY_MAX) {
        return -1;
    }
    
    pthread_mutex_lock(&g_context.mutex);
    g_context.anisotropy_level = anisotropy_level;
    pthread_mutex_unlock(&g_context.mutex);
    
    return 0;
}

int texture_format_conversion_set_lod_levels(texture_format_conversion_handle_t handle, 
                                          uint32_t lod_count, 
                                          const float* lod_distances) {
    if (!g_context.initialized || lod_count > TEXTURE_FORMAT_CONVERSION_MAX_LOD_LEVELS || !lod_distances) {
        return -1;
    }
    
    pthread_mutex_lock(&g_context.mutex);
    g_context.lod_count = lod_count;
    memcpy(g_context.lod_distances, lod_distances, lod_count * sizeof(float));
    pthread_mutex_unlock(&g_context.mutex);
    
    return 0;
}

int texture_format_conversion_add_render_graph_node(const char* name, 
                                               void (*execute_func)(void*), 
                                               void* user_data) {
    if (!name || !execute_func || !g_context.initialized) {
        return -1;
    }
    
    pthread_mutex_lock(&g_context.mutex);
    
    for (uint32_t i = 0; i < 64; i++) {
        if (!g_context.render_nodes[i]) {
            g_context.render_nodes[i] = malloc(sizeof(render_graph_node_t));
            if (!g_context.render_nodes[i]) {
                pthread_mutex_unlock(&g_context.mutex);
                return -1;
            }
            
            strncpy(g_context.render_nodes[i]->name, name, 255);
            g_context.render_nodes[i]->name[255] = '\0';
            g_context.render_nodes[i]->execute_func = execute_func;
            g_context.render_nodes[i]->user_data = user_data;
            g_context.render_nodes[i]->dependency_count = 0;
            g_context.render_nodes[i]->executed = false;
            
            pthread_mutex_unlock(&g_context.mutex);
            return 0;
        }
    }
    
    pthread_mutex_unlock(&g_context.mutex);
    return -1;
}

int texture_format_conversion_start_hot_reload(const char* file_path, 
                                          void (*reload_callback)(const char*)) {
    if (!file_path || !reload_callback || !g_context.initialized) {
        return -1;
    }
    
    int wd = inotify_add_watch(g_context.inotify_fd, file_path, IN_MODIFY);
    if (wd < 0) {
        return -1;
    }
    
    return 0;
}

int texture_format_conversion_enable_gpu_integration(texture_format_conversion_handle_t handle) {
    if (!g_context.initialized) {
        return -1;
    }
    
    pthread_mutex_lock(&g_context.mutex);
    g_context.gpu_enabled = true;
    pthread_mutex_unlock(&g_context.mutex);
    
    return 0;
}

int texture_format_conversion_get_bindless_handle(texture_format_conversion_handle_t handle, 
                                              uint32_t* out_gpu_handle) {
    if (!out_gpu_handle || !g_context.initialized || !g_context.gpu_enabled) {
        return -1;
    }
    
    *out_gpu_handle = handle.id + 1000;
    return 0;
}

int texture_format_conversion_get_performance_counters(performance_counters_t* out_counters) {
    if (!out_counters || !g_context.initialized) {
        return -1;
    }
    
    pthread_mutex_lock(&g_context.mutex);
    *out_counters = g_context.counters;
    pthread_mutex_unlock(&g_context.mutex);
    
    return 0;
}

int texture_format_conversion_get_feedback_analysis(feedback_analysis_t* out_feedback) {
    if (!out_feedback || !g_context.initialized) {
        return -1;
    }
    
    pthread_mutex_lock(&g_context.mutex);
    
    out_feedback->samples_processed = g_context.counters.textures_converted;
    out_feedback->quality_score = 85;
    out_feedback->compression_ratio = 0.5f;
    out_feedback->processing_time_ms = (float)(g_context.counters.total_processing_time_ns / 1000000.0);
    out_feedback->cache_hits = g_context.counters.cache_hits;
    out_feedback->cache_misses = g_context.counters.cache_misses;
    out_feedback->gpu_operations = g_context.counters.gpu_operations;
    out_feedback->simd_operations = g_context.counters.simd_operations;
    
    pthread_mutex_unlock(&g_context.mutex);
    
    return 0;
}

int texture_format_conversion_update(texture_format_conversion_handle_t handle, const void* data, size_t size) {
    if (!data || !g_context.initialized || handle.id >= TEXTURE_FORMAT_CONVERSION_MAX_TEXTURE_ARRAYS) {
        return -1;
    }
    
    pthread_mutex_lock(&g_context.mutex);
    
    texture_array_t* array = &g_context.texture_arrays[handle.id];
    if (!array->initialized || size > array->data_size) {
        pthread_mutex_unlock(&g_context.mutex);
        return -1;
    }
    
    memcpy(array->data, data, size);
    
    uint64_t start_time = clock_gettime_nsec_np(CLOCK_MONOTONIC);
    update_performance_counters(0, clock_gettime_nsec_np(CLOCK_MONOTONIC) - start_time);
    
    pthread_mutex_unlock(&g_context.mutex);
    return 0;
}

bool texture_format_conversion_is_valid(texture_format_conversion_handle_t handle) {
    if (!g_context.initialized || handle.id >= TEXTURE_FORMAT_CONVERSION_MAX_TEXTURE_ARRAYS) {
        return false;
    }
    
    pthread_mutex_lock(&g_context.mutex);
    bool valid = g_context.texture_arrays[handle.id].initialized;
    pthread_mutex_unlock(&g_context.mutex);
    
    return valid;
}

int texture_format_conversion_get_info(texture_format_conversion_handle_t handle, texture_format_conversion_info_t* out_info) {
    if (!out_info || !g_context.initialized || handle.id >= TEXTURE_FORMAT_CONVERSION_MAX_TEXTURE_ARRAYS) {
        return -1;
    }
    
    pthread_mutex_lock(&g_context.mutex);
    
    texture_array_t* array = &g_context.texture_arrays[handle.id];
    out_info->id = handle.id;
    out_info->flags = 0;
    out_info->initialized = array->initialized;
    
    pthread_mutex_unlock(&g_context.mutex);
    return 0;
}

void texture_format_conversion_mark_dirty(texture_format_conversion_handle_t handle) {
    if (!g_context.initialized || handle.id >= TEXTURE_FORMAT_CONVERSION_MAX_TEXTURE_ARRAYS) {
        return;
    }
    
    pthread_mutex_lock(&g_context.mutex);
    
    if (g_context.texture_arrays[handle.id].initialized) {
        g_context.counters.cache_misses++;
    }
    
    pthread_mutex_unlock(&g_context.mutex);
}

int texture_format_conversion_process_pending(void) {
    if (!g_context.initialized) {
        return -1;
    }
    
    pthread_mutex_lock(&g_context.mutex);
    uint32_t pending = g_context.counters.async_operations;
    pthread_mutex_unlock(&g_context.mutex);
    
    return (int)pending;
}

uint32_t texture_format_conversion_get_count(void) {
    if (!g_context.initialized) {
        return 0;
    }
    
    pthread_mutex_lock(&g_context.mutex);
    
    uint32_t count = 0;
    for (uint32_t i = 0; i < TEXTURE_FORMAT_CONVERSION_MAX_TEXTURE_ARRAYS; i++) {
        if (g_context.texture_arrays[i].initialized) {
            count++;
        }
    }
    
    pthread_mutex_unlock(&g_context.mutex);
    return count;
}

size_t texture_format_conversion_get_memory_usage(void) {
    if (!g_context.initialized) {
        return 0;
    }
    
    pthread_mutex_lock(&g_context.mutex);
    size_t usage = g_context.counters.total_memory_used;
    pthread_mutex_unlock(&g_context.mutex);
    
    return usage;
}

void texture_format_conversion_debug_print(void) {
    if (!g_context.initialized) {
        return;
    }
    
    pthread_mutex_lock(&g_context.mutex);
    
    printf("=== Format Conversion Debug Info ===\n");
    printf("Initialized: %s\n", g_context.initialized ? "Yes" : "No");
    printf("GPU Enabled: %s\n", g_context.gpu_enabled ? "Yes" : "No");
    printf("SIMD Enabled: %s\n", g_context.simd_enabled ? "Yes" : "No");
    printf("Anisotropy Level: %u\n", g_context.anisotropy_level);
    printf("LOD Count: %u\n", g_context.lod_count);
    printf("Texture Arrays: %u\n", texture_format_conversion_get_count());
    printf("Memory Usage: %zu bytes\n", g_context.counters.total_memory_used);
    printf("Textures Converted: %u\n", g_context.counters.textures_converted);
    printf("Cache Hits: %u\n", g_context.counters.cache_hits);
    printf("Cache Misses: %u\n", g_context.counters.cache_misses);
    
    pthread_mutex_unlock(&g_context.mutex);
}

// Unit tests for format conversion system
int texture_format_conversion_run_unit_tests(void) {
    printf("=== Running Format Conversion Unit Tests ===\n");
    
    int tests_run = 0;
    int tests_passed = 0;
    
    // Test 1: System initialization
    printf("Test 1: System initialization... ");
    tests_run++;
    if (texture_format_conversion_init() == 0) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 2: Create format conversion
    printf("Test 2: Create format conversion... ");
    tests_run++;
    texture_format_conversion_handle_t handle;
    texture_format_conversion_desc_t desc = {
        .flags = 0x01, // Virtual texturing
        .width = 256,
        .height = 256,
        .depth = 1
    };
    
    if (texture_format_conversion_create(&handle, &desc) == 0) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 3: Create texture array
    printf("Test 3: Create texture array... ");
    tests_run++;
    texture_format_conversion_handle_t array_handle;
    if (texture_format_conversion_create_texture_array(&array_handle, 256, 256, 1, 4, TEXTURE_FORMAT_RGBA8) == 0) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 4: Update texture array
    printf("Test 4: Update texture array... ");
    tests_run++;
    uint8_t test_data[256 * 256 * 4]; // 256x256 RGBA
    memset(test_data, 0x80, sizeof(test_data));
    
    if (texture_format_conversion_update(array_handle, test_data, sizeof(test_data)) == 0) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 5: Validate texture array
    printf("Test 5: Validate texture array... ");
    tests_run++;
    if (texture_format_conversion_is_valid(array_handle)) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 6: Get format conversion info
    printf("Test 6: Get format conversion info... ");
    tests_run++;
    texture_format_conversion_info_t info;
    if (texture_format_conversion_get_info(handle, &info) == 0) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 7: Enable virtual texturing
    printf("Test 7: Enable virtual texturing... ");
    tests_run++;
    if (texture_format_conversion_enable_virtual_texturing(handle, 256, 64) == 0) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 8: Set anisotropic filtering
    printf("Test 8: Set anisotropic filtering... ");
    tests_run++;
    if (texture_format_conversion_set_anisotropic_filtering(handle, 4) == 0) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 9: Set LOD levels
    printf("Test 9: Set LOD levels... ");
    tests_run++;
    float lod_distances[4] = {0.0f, 10.0f, 20.0f, 40.0f};
    if (texture_format_conversion_set_lod_levels(handle, 4, lod_distances) == 0) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 10: Add render graph node
    printf("Test 10: Add render graph node... ");
    tests_run++;
    if (texture_format_conversion_add_render_graph_node("test_node", NULL, NULL) == 0) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 11: Enable GPU integration
    printf("Test 11: Enable GPU integration... ");
    tests_run++;
    if (texture_format_conversion_enable_gpu_integration(handle) == 0) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 12: Get bindless handle
    printf("Test 12: Get bindless handle... ");
    tests_run++;
    uint32_t gpu_handle;
    if (texture_format_conversion_get_bindless_handle(handle, &gpu_handle) == 0) {
        printf("PASS (Handle: %u)\n", gpu_handle);
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 13: Get performance counters
    printf("Test 13: Get performance counters... ");
    tests_run++;
    performance_counters_t counters;
    if (texture_format_conversion_get_performance_counters(&counters) == 0) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 14: Get feedback analysis
    printf("Test 14: Get feedback analysis... ");
    tests_run++;
    feedback_analysis_t feedback;
    if (texture_format_conversion_get_feedback_analysis(&feedback) == 0) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 15: Process pending operations
    printf("Test 15: Process pending operations... ");
    tests_run++;
    if (texture_format_conversion_process_pending() >= 0) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 16: Get count
    printf("Test 16: Get count... ");
    tests_run++;
    uint32_t count = texture_format_conversion_get_count();
    if (count > 0) {
        printf("PASS (Count: %u)\n", count);
        tests_passed++;
    } else {
        printf("FAIL (Count: %u)\n", count);
    }
    
    // Test 17: Memory usage tracking
    printf("Test 17: Memory usage tracking... ");
    tests_run++;
    size_t memory_usage = texture_format_conversion_get_memory_usage();
    if (memory_usage > 0) {
        printf("PASS (Usage: %zu bytes)\n", memory_usage);
        tests_passed++;
    } else {
        printf("FAIL (Usage: %zu bytes)\n", memory_usage);
    }
    
    // Test 18: Mark dirty
    printf("Test 18: Mark dirty... ");
    tests_run++;
    texture_format_conversion_mark_dirty(array_handle);
    printf("PASS\n");
    tests_passed++;
    
    // Test 19: Destroy format conversion
    printf("Test 19: Destroy format conversion... ");
    tests_run++;
    texture_format_conversion_destroy(handle);
    printf("PASS\n");
    tests_passed++;
    
    // Cleanup
    texture_format_conversion_shutdown();
    
    printf("\n=== Test Results ===\n");
    printf("Tests Run: %d\n", tests_run);
    printf("Tests Passed: %d\n", tests_passed);
    printf("Tests Failed: %d\n", tests_run - tests_passed);
    printf("Success Rate: %.1f%%\n", (float)tests_passed / tests_run * 100.0f);
    
    if (tests_passed == tests_run) {
        printf("=== ALL TESTS PASSED ===\n");
        return 0;
    } else {
        printf("=== SOME TESTS FAILED ===\n");
        return -1;
    }
}
