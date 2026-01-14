/*
 * basis_transcoder.c
 * Basis Universal transcoding with advanced features
 */

#include "assets/textures/compression/basis_transcoder.h"
#include "assets/textures/compression/bc_compression.h"
#include "assets/textures/compression/astc_compression.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <math.h>

#define TEXTURE_BASIS_TRANSCODER_MAX_COUNT 4096
#define TEXTURE_BASIS_TRANSCODER_DEFAULT_CAPACITY 256
#define TEXTURE_BASIS_TRANSCODER_CACHE_SIZE 1024
#define TEXTURE_BASIS_TRANSCODER_MEMORY_BUDGET (512 * 1024 * 1024)
#define TEXTURE_BASIS_TRANSCODER_VIRTUAL_PAGE_SIZE 128
#define TEXTURE_BASIS_TRANSCODER_MAX_VIRTUAL_PAGES 1024

typedef enum {
    TEXTURE_BASIS_COMPRESSION_NONE,
    TEXTURE_BASIS_COMPRESSION_BC1,
    TEXTURE_BASIS_COMPRESSION_BC3,
    TEXTURE_BASIS_COMPRESSION_BC5,
    TEXTURE_BASIS_COMPRESSION_BC7,
    TEXTURE_BASIS_COMPRESSION_ASTC_4x4,
    TEXTURE_BASIS_COMPRESSION_ASTC_6x6,
    TEXTURE_BASIS_COMPRESSION_ASTC_8x8,
    TEXTURE_BASIS_COMPRESSION_COUNT
} texture_basis_compression_type_t;

typedef struct texture_basis_virtual_page {
    uint32_t page_x;
    uint32_t page_y;
    uint32_t page_level;
    void* page_data;
    size_t page_size;
    bool loaded;
    uint64_t last_access_time;
    uint32_t access_count;
} texture_basis_virtual_page_t;

typedef struct texture_basis_compression_workspace {
    void* workspace_data;
    size_t workspace_size;
    texture_basis_compression_type_t compression_type;
    float compression_quality;
    size_t compressed_size;
    float compression_ratio;
} texture_basis_compression_workspace_t;

typedef struct texture_basis_transcoder_internal {
    uint32_t id;
    uint32_t flags;
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
    
    uint32_t source_format;
    uint32_t target_format;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t mipmap_count;
    float anisotropy_level;
    uint32_t texture_array_id;
    uint32_t virtual_page_count;
    texture_basis_virtual_page_t* virtual_pages;
    uint32_t max_virtual_pages;
    bool virtual_texturing_enabled;
    
    texture_basis_compression_workspace_t compression_workspace;
    texture_basis_compression_type_t compression_type;
    float compression_quality;
    bool compression_enabled;
    
    uint32_t lod_level_count;
    float lod_bias;
    
    struct {
        uint32_t access_count;
        uint64_t last_access_time;
        float quality_score;
    } feedback;
    
    pthread_mutex_t data_mutex;
    size_t memory_usage;
    bool memory_pooled;
    bool gpu_processed;
    uint32_t gpu_texture_id;
    void* cache_data;
    size_t cache_size;
    bool cache_valid;
    uint64_t data_hash;
    bool culled;
} texture_basis_transcoder_internal_t;

typedef struct texture_basis_transcoder_context {
    texture_basis_transcoder_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
    pthread_mutex_t global_mutex;
    
    struct {
        uint64_t textures_transcoded;
        uint64_t bytes_transcoded;
        uint64_t cache_hits;
        uint64_t cache_misses;
        uint64_t gpu_operations;
        uint64_t simd_operations;
        uint64_t async_operations;
        uint64_t compression_operations;
        uint64_t virtual_texture_operations;
        uint64_t batch_operations;
    } performance_counters;
    
    struct {
        size_t total_allocated;
        size_t current_allocated;
        size_t peak_allocated;
        uint32_t allocation_count;
        uint32_t deallocation_count;
    } memory_tracker;
    
    struct {
        void* data;
        size_t size;
        uint32_t texture_id;
        bool valid;
        uint64_t timestamp;
    } cache[TEXTURE_BASIS_TRANSCODER_CACHE_SIZE];
    pthread_mutex_t cache_mutex;
    
    struct {
        bool gpu_available;
        pthread_mutex_t gpu_mutex;
    } gpu_context;
    
    struct {
        bool culling_enabled;
        uint32_t culled_textures;
        uint32_t visible_textures;
    } culling_context;
    
    struct {
        uint32_t node_id;
        texture_basis_transcoder_handle_t handle;
        void (*process_function)(texture_basis_transcoder_handle_t);
        bool processed;
    } render_graph_nodes[256];
    uint32_t render_graph_node_count;
    pthread_mutex_t render_graph_mutex;
    
    uint64_t frame_counter;
} texture_basis_transcoder_context_t;

static texture_basis_transcoder_context_t g_basis_transcoder_ctx = {0};

static uint64_t texture_basis_hash(const void* data, size_t size) {
    const unsigned char* bytes = (const unsigned char*)data;
    uint64_t hash = 14695981039346656037ULL;
    for (size_t i = 0; i < size; ++i) {
        hash ^= bytes[i];
        hash *= 1099511628211ULL;
    }
    return hash;
}

static void texture_basis_cleanup_internal(texture_basis_transcoder_internal_t* item) {
    if (!item) return;
    
    if (item->data) {
        free(item->data);
        item->data = NULL;
    }
    
    if (item->virtual_pages) {
        for (uint32_t i = 0; i < item->virtual_page_count; i++) {
            if (item->virtual_pages[i].page_data) {
                free(item->virtual_pages[i].page_data);
            }
        }
        free(item->virtual_pages);
        item->virtual_pages = NULL;
    }
    
    if (item->compression_workspace.workspace_data) {
        free(item->compression_workspace.workspace_data);
        item->compression_workspace.workspace_data = NULL;
    }
    
    if (item->cache_data) {
        free(item->cache_data);
        item->cache_data = NULL;
    }
    
    pthread_mutex_destroy(&item->data_mutex);
    item->initialized = false;
}

static int texture_basis_init_virtual_texturing(texture_basis_transcoder_internal_t* item, 
                                         uint32_t page_size, uint32_t max_pages) {
    if (!item || page_size == 0 || max_pages == 0) {
        return -1;
    }
    
    item->virtual_pages = calloc(max_pages, sizeof(texture_basis_virtual_page_t));
    if (!item->virtual_pages) {
        return -2;
    }
    
    item->max_virtual_pages = max_pages;
    item->virtual_page_count = 0;
    item->virtual_texturing_enabled = true;
    
    // Initialize page data for each virtual page
    size_t page_data_size = page_size * page_size * 4; // RGBA
    for (uint32_t i = 0; i < max_pages; i++) {
        item->virtual_pages[i].page_data = malloc(page_data_size);
        if (!item->virtual_pages[i].page_data) {
            // Cleanup already allocated pages
            for (uint32_t j = 0; j < i; j++) {
                free(item->virtual_pages[j].page_data);
            }
            free(item->virtual_pages);
            item->virtual_pages = NULL;
            return -3;
        }
        
        item->virtual_pages[i].page_size = page_data_size;
        item->virtual_pages[i].loaded = false;
        item->virtual_pages[i].last_access_time = 0;
        item->virtual_pages[i].access_count = 0;
    }
    
    return 0;
}

static int texture_basis_init_compression_workspace(texture_basis_transcoder_internal_t* item,
                                           texture_basis_compression_type_t compression_type,
                                           float quality) {
    if (!item) {
        return -1;
    }
    
    // Allocate workspace for compression (64MB should be sufficient for most textures)
    size_t workspace_size = 64 * 1024 * 1024;
    item->compression_workspace.workspace_data = malloc(workspace_size);
    if (!item->compression_workspace.workspace_data) {
        return -2;
    }
    
    item->compression_workspace.workspace_size = workspace_size;
    item->compression_workspace.compression_type = compression_type;
    item->compression_workspace.compression_quality = quality;
    item->compression_workspace.compressed_size = 0;
    item->compression_workspace.compression_ratio = 0.0f;
    item->compression_type = compression_type;
    item->compression_quality = quality;
    item->compression_enabled = true;
    
    return 0;
}

static int texture_basis_compress_data_bc(const void* src_data, size_t src_size,
                                     texture_basis_compression_type_t bc_type,
                                     void* dst_data, size_t* dst_size,
                                     float quality) {
    // Placeholder for BC compression implementation
    // In a real implementation, this would use the bc_compression.h functions
    
    switch (bc_type) {
        case TEXTURE_BASIS_COMPRESSION_BC1:
            // BC1 compression (RGB, 1-bit alpha)
            *dst_size = src_size / 2; // Rough estimate
            break;
        case TEXTURE_BASIS_COMPRESSION_BC3:
            // BC3 compression (RGBA, interpolated alpha)
            *dst_size = src_size / 1; // Rough estimate
            break;
        case TEXTURE_BASIS_COMPRESSION_BC5:
            // BC5 compression (normal map, two-component)
            *dst_size = src_size / 2; // Rough estimate
            break;
        case TEXTURE_BASIS_COMPRESSION_BC7:
            // BC7 compression (high-quality RGBA)
            *dst_size = src_size / 1; // Rough estimate
            break;
        default:
            return -1;
    }
    
    // Simulate compression by copying data (placeholder)
    memcpy(dst_data, src_data, (*dst_size > src_size) ? src_size : *dst_size);
    return 0;
}

static int texture_basis_compress_data_astc(const void* src_data, size_t src_size,
                                       texture_basis_compression_type_t astc_type,
                                       void* dst_data, size_t* dst_size,
                                       float quality) {
    // Placeholder for ASTC compression implementation
    // In a real implementation, this would use the astc_compression.h functions
    
    switch (astc_type) {
        case TEXTURE_BASIS_COMPRESSION_ASTC_4x4:
            // ASTC 4x4 blocks (high quality)
            *dst_size = src_size / 1; // Rough estimate
            break;
        case TEXTURE_BASIS_COMPRESSION_ASTC_6x6:
            // ASTC 6x6 blocks (medium quality)
            *dst_size = src_size / 2; // Rough estimate
            break;
        case TEXTURE_BASIS_COMPRESSION_ASTC_8x8:
            // ASTC 8x8 blocks (low quality, high compression)
            *dst_size = src_size / 3; // Rough estimate
            break;
        default:
            return -1;
    }
    
    // Simulate compression by copying data (placeholder)
    memcpy(dst_data, src_data, (*dst_size > src_size) ? src_size : *dst_size);
    return 0;
}

static int texture_basis_compress_data(const void* src_data, size_t src_size,
                                   texture_basis_compression_type_t compression_type,
                                   void* dst_data, size_t* dst_size,
                                   float quality) {
    if (!src_data || !dst_data || !dst_size) {
        return -1;
    }
    
    switch (compression_type) {
        case TEXTURE_BASIS_COMPRESSION_BC1:
        case TEXTURE_BASIS_COMPRESSION_BC3:
        case TEXTURE_BASIS_COMPRESSION_BC5:
        case TEXTURE_BASIS_COMPRESSION_BC7:
            return texture_basis_compress_data_bc(src_data, src_size, compression_type,
                                           dst_data, dst_size, quality);
            
        case TEXTURE_BASIS_COMPRESSION_ASTC_4x4:
        case TEXTURE_BASIS_COMPRESSION_ASTC_6x6:
        case TEXTURE_BASIS_COMPRESSION_ASTC_8x8:
            return texture_basis_compress_data_astc(src_data, src_size, compression_type,
                                             dst_data, dst_size, quality);
            
        default:
            return -2;
    }
}

static int texture_basis_load_virtual_page(texture_basis_transcoder_internal_t* item,
                                     uint32_t page_x, uint32_t page_y,
                                     uint32_t page_level) {
    if (!item || !item->virtual_texturing_enabled || !item->virtual_pages) {
        return -1;
    }
    
    // Find available page slot
    uint32_t page_index = (page_x * 16 + page_y) % item->max_virtual_pages;
    texture_basis_virtual_page_t* page = &item->virtual_pages[page_index];
    
    // Load page data (placeholder implementation)
    if (!page->loaded) {
        // In a real implementation, this would load from disk or generate procedurally
        memset(page->page_data, 0x80, page->page_size); // Gray placeholder
        page->loaded = true;
        page->page_x = page_x;
        page->page_y = page_y;
        page->page_level = page_level;
        page->last_access_time = time(NULL);
        page->access_count = 1;
        
        g_basis_transcoder_ctx.performance_counters.virtual_texture_operations++;
    } else {
        page->last_access_time = time(NULL);
        page->access_count++;
    }
    
    return 0;
}

static void texture_basis_evict_virtual_pages(texture_basis_transcoder_internal_t* item) {
    if (!item || !item->virtual_pages) {
        return;
    }
    
    // Simple LRU eviction - evict pages with oldest access time
    uint64_t current_time = time(NULL);
    uint32_t evicted_count = 0;
    
    for (uint32_t i = 0; i < item->max_virtual_pages && evicted_count < item->max_virtual_pages / 4; i++) {
        texture_basis_virtual_page_t* page = &item->virtual_pages[i];
        
        if (page->loaded && (current_time - page->last_access_time) > 300) { // 5 minutes
            memset(page->page_data, 0, page->page_size);
            page->loaded = false;
            page->access_count = 0;
            evicted_count++;
        }
    }
}

int texture_basis_transcoder_init(void) {
    if (g_basis_transcoder_ctx.initialized) {
        return 0;
    }

    if (pthread_mutex_init(&g_basis_transcoder_ctx.global_mutex, NULL) != 0) {
        return -1;
    }

    g_basis_transcoder_ctx.count = 0;
    g_basis_transcoder_ctx.capacity = TEXTURE_BASIS_TRANSCODER_DEFAULT_CAPACITY;
    g_basis_transcoder_ctx.items = calloc(TEXTURE_BASIS_TRANSCODER_DEFAULT_CAPACITY, 
                                      sizeof(texture_basis_transcoder_internal_t));
    if (!g_basis_transcoder_ctx.items) {
        pthread_mutex_destroy(&g_basis_transcoder_ctx.global_mutex);
        return -2;
    }

    pthread_mutex_init(&g_basis_transcoder_ctx.cache_mutex, NULL);
    pthread_mutex_init(&g_basis_transcoder_ctx.gpu_context.gpu_mutex, NULL);
    pthread_mutex_init(&g_basis_transcoder_ctx.render_graph_mutex, NULL);

    memset(&g_basis_transcoder_ctx.memory_tracker, 0, sizeof(g_basis_transcoder_ctx.memory_tracker));
    memset(&g_basis_transcoder_ctx.performance_counters, 0, sizeof(g_basis_transcoder_ctx.performance_counters));

    g_basis_transcoder_ctx.render_graph_node_count = 0;
    g_basis_transcoder_ctx.frame_counter = 0;
    g_basis_transcoder_ctx.gpu_context.gpu_available = false;
    g_basis_transcoder_ctx.culling_context.culling_enabled = false;
    g_basis_transcoder_ctx.culling_context.culled_textures = 0;
    g_basis_transcoder_ctx.culling_context.visible_textures = 0;

    g_basis_transcoder_ctx.initialized = true;
    return 0;
}

void texture_basis_transcoder_shutdown(void) {
    if (!g_basis_transcoder_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_basis_transcoder_ctx.count; i++) {
        texture_basis_cleanup_internal(&g_basis_transcoder_ctx.items[i]);
    }

    free(g_basis_transcoder_ctx.items);
    g_basis_transcoder_ctx.items = NULL;
    g_basis_transcoder_ctx.count = 0;
    g_basis_transcoder_ctx.capacity = 0;

    pthread_mutex_destroy(&g_basis_transcoder_ctx.cache_mutex);
    pthread_mutex_destroy(&g_basis_transcoder_ctx.gpu_context.gpu_mutex);
    pthread_mutex_destroy(&g_basis_transcoder_ctx.render_graph_mutex);
    pthread_mutex_destroy(&g_basis_transcoder_ctx.global_mutex);

    g_basis_transcoder_ctx.initialized = false;
}

int texture_basis_transcoder_create(texture_basis_transcoder_handle_t* out_handle, const texture_basis_transcoder_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_basis_transcoder_ctx.initialized) {
        return -2;
    }

    if (g_basis_transcoder_ctx.count >= g_basis_transcoder_ctx.capacity) {
        return -3;
    }

    pthread_mutex_lock(&g_basis_transcoder_ctx.global_mutex);
    
    uint32_t index = g_basis_transcoder_ctx.count++;
    texture_basis_transcoder_internal_t* item = &g_basis_transcoder_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->data = NULL;
    item->data_size = 0;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;
    
    item->source_format = 0;
    item->target_format = 0;
    item->width = desc->width ? desc->width : 256;
    item->height = desc->height ? desc->height : 256;
    item->depth = desc->depth ? desc->depth : 1;
    item->mipmap_count = 1;
    item->anisotropy_level = 1.0f;
    item->texture_array_id = 0;
    item->virtual_page_count = 0;
    item->virtual_pages = NULL;
    item->max_virtual_pages = 0;
    item->virtual_texturing_enabled = false;
    
    memset(&item->compression_workspace, 0, sizeof(item->compression_workspace));
    item->compression_type = TEXTURE_BASIS_COMPRESSION_NONE;
    item->compression_quality = 0.8f;
    item->compression_enabled = false;
    
    item->lod_level_count = 1;
    item->lod_bias = 0.0f;
    
    memset(&item->feedback, 0, sizeof(item->feedback));
    
    if (pthread_mutex_init(&item->data_mutex, NULL) != 0) {
        g_basis_transcoder_ctx.count--;
        pthread_mutex_unlock(&g_basis_transcoder_ctx.global_mutex);
        return -4;
    }
    
    item->memory_usage = sizeof(texture_basis_transcoder_internal_t);
    item->memory_pooled = false;
    item->gpu_processed = false;
    item->gpu_texture_id = 0;
    item->cache_data = NULL;
    item->cache_size = 0;
    item->cache_valid = false;
    item->data_hash = 0;
    item->culled = false;

    g_basis_transcoder_ctx.memory_tracker.total_allocated += item->memory_usage;
    g_basis_transcoder_ctx.memory_tracker.current_allocated += item->memory_usage;
    g_basis_transcoder_ctx.memory_tracker.allocation_count++;

    out_handle->id = index;
    pthread_mutex_unlock(&g_basis_transcoder_ctx.global_mutex);
    return 0;
}

void texture_basis_transcoder_destroy(texture_basis_transcoder_handle_t handle) {
    if (!g_basis_transcoder_ctx.initialized || handle.id >= g_basis_transcoder_ctx.count) {
        return;
    }
    
    pthread_mutex_lock(&g_basis_transcoder_ctx.global_mutex);
    
    texture_basis_transcoder_internal_t* item = &g_basis_transcoder_ctx.items[handle.id];
    if (!item->initialized) {
        pthread_mutex_unlock(&g_basis_transcoder_ctx.global_mutex);
        return;
    }
    
    g_basis_transcoder_ctx.memory_tracker.current_allocated -= item->memory_usage;
    g_basis_transcoder_ctx.memory_tracker.deallocation_count++;
    
    texture_basis_cleanup_internal(item);
    pthread_mutex_unlock(&g_basis_transcoder_ctx.global_mutex);
}

int texture_basis_transcoder_update(texture_basis_transcoder_handle_t handle, const void* data, size_t size) {
    if (!g_basis_transcoder_ctx.initialized) {
        return -1;
    }
    
    if (handle.id >= g_basis_transcoder_ctx.count) {
        return -2;
    }

    pthread_mutex_lock(&g_basis_transcoder_ctx.global_mutex);
    
    texture_basis_transcoder_internal_t* item = &g_basis_transcoder_ctx.items[handle.id];
    if (!item->initialized) {
        pthread_mutex_unlock(&g_basis_transcoder_ctx.global_mutex);
        return -3;
    }
    
    pthread_mutex_lock(&item->data_mutex);
    
    if (item->data && item->data_size >= size) {
        memcpy(item->data, data, size);
        item->data_size = size;
        item->memory_pooled = true;
    } else {
        void* new_data = malloc(size);
        if (!new_data) {
            pthread_mutex_unlock(&item->data_mutex);
            pthread_mutex_unlock(&g_basis_transcoder_ctx.global_mutex);
            return -4;
        }
        
        if (item->data) {
            free(item->data);
        }
        item->data = new_data;
        item->data_size = size;
        item->memory_pooled = false;
    }
    
    item->data_hash = texture_basis_hash(data, size);
    item->dirty = true;
    item->frame_updated = g_basis_transcoder_ctx.frame_counter++;
    
    pthread_mutex_unlock(&item->data_mutex);
    pthread_mutex_unlock(&g_basis_transcoder_ctx.global_mutex);
    
    return 0;
}

bool texture_basis_transcoder_is_valid(texture_basis_transcoder_handle_t handle) {
    if (!g_basis_transcoder_ctx.initialized || handle.id >= g_basis_transcoder_ctx.count) {
        return false;
    }
    
    pthread_mutex_lock(&g_basis_transcoder_ctx.global_mutex);
    bool valid = g_basis_transcoder_ctx.items[handle.id].initialized;
    pthread_mutex_unlock(&g_basis_transcoder_ctx.global_mutex);
    return valid;
}

int texture_basis_transcoder_get_info(texture_basis_transcoder_handle_t handle, texture_basis_transcoder_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (!g_basis_transcoder_ctx.initialized || handle.id >= g_basis_transcoder_ctx.count) {
        return -2;
    }

    pthread_mutex_lock(&g_basis_transcoder_ctx.global_mutex);
    
    const texture_basis_transcoder_internal_t* item = &g_basis_transcoder_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    
    pthread_mutex_unlock(&g_basis_transcoder_ctx.global_mutex);
    return 0;
}

void texture_basis_transcoder_mark_dirty(texture_basis_transcoder_handle_t handle) {
    if (!g_basis_transcoder_ctx.initialized || handle.id >= g_basis_transcoder_ctx.count) {
        return;
    }
    
    pthread_mutex_lock(&g_basis_transcoder_ctx.global_mutex);
    
    texture_basis_transcoder_internal_t* item = &g_basis_transcoder_ctx.items[handle.id];
    if (item->initialized) {
        item->dirty = true;
        item->frame_updated = g_basis_transcoder_ctx.frame_counter++;
        
        if (g_basis_transcoder_ctx.culling_context.culling_enabled) {
            bool visible = true;
            if (!visible) {
                item->culled = true;
                g_basis_transcoder_ctx.culling_context.culled_textures++;
            } else {
                item->culled = false;
                g_basis_transcoder_ctx.culling_context.visible_textures++;
            }
        }
    }
    
    pthread_mutex_unlock(&g_basis_transcoder_ctx.global_mutex);
}

int texture_basis_transcoder_process_pending(void) {
    if (!g_basis_transcoder_ctx.initialized) {
        return 0;
    }
    
    pthread_mutex_lock(&g_basis_transcoder_ctx.global_mutex);
    
    int processed = 0;
    
    pthread_mutex_lock(&g_basis_transcoder_ctx.render_graph_mutex);
    for (uint32_t i = 0; i < g_basis_transcoder_ctx.render_graph_node_count; i++) {
        if (!g_basis_transcoder_ctx.render_graph_nodes[i].processed) {
            if (g_basis_transcoder_ctx.render_graph_nodes[i].process_function) {
                g_basis_transcoder_ctx.render_graph_nodes[i].process_function(
                    g_basis_transcoder_ctx.render_graph_nodes[i].handle);
            }
            g_basis_transcoder_ctx.render_graph_nodes[i].processed = true;
            processed++;
        }
    }
    pthread_mutex_unlock(&g_basis_transcoder_ctx.render_graph_mutex);
    
    for (uint32_t i = 0; i < g_basis_transcoder_ctx.count; i++) {
        texture_basis_transcoder_internal_t* item = &g_basis_transcoder_ctx.items[i];
        if (item->initialized && item->dirty) {
            if (item->data && item->data_size > 0) {
                g_basis_transcoder_ctx.performance_counters.textures_transcoded++;
                g_basis_transcoder_ctx.performance_counters.bytes_transcoded += item->data_size;
            }
            
            item->dirty = false;
            processed++;
        }
    }
    
    pthread_mutex_unlock(&g_basis_transcoder_ctx.global_mutex);
    return processed;
}

uint32_t texture_basis_transcoder_get_count(void) {
    if (!g_basis_transcoder_ctx.initialized) {
        return 0;
    }
    
    return g_basis_transcoder_ctx.count;
}

size_t texture_basis_transcoder_get_memory_usage(void) {
    if (!g_basis_transcoder_ctx.initialized) {
        return 0;
    }
    
    pthread_mutex_lock(&g_basis_transcoder_ctx.global_mutex);
    
    size_t total = sizeof(g_basis_transcoder_ctx);
    total += g_basis_transcoder_ctx.capacity * sizeof(texture_basis_transcoder_internal_t);

    for (uint32_t i = 0; i < g_basis_transcoder_ctx.count; i++) {
        total += g_basis_transcoder_ctx.items[i].data_size;
    }

    pthread_mutex_unlock(&g_basis_transcoder_ctx.global_mutex);
    return total;
}

void texture_basis_transcoder_debug_print(void) {
    if (!g_basis_transcoder_ctx.initialized) {
        printf("Basis Transcoder: Not initialized\n");
        return;
    }
    
    pthread_mutex_lock(&g_basis_transcoder_ctx.global_mutex);
    
    printf("\n=== Basis Universal Transcoder Debug Information ===\n");
    printf("Initialized: Yes\n");
    printf("Texture Count: %u / %u\n", g_basis_transcoder_ctx.count, g_basis_transcoder_ctx.capacity);
    
    printf("\n--- Memory Tracking ---\n");
    printf("Total Allocated: %zu bytes\n", g_basis_transcoder_ctx.memory_tracker.total_allocated);
    printf("Current Allocated: %zu bytes\n", g_basis_transcoder_ctx.memory_tracker.current_allocated);
    printf("Peak Allocated: %zu bytes\n", g_basis_transcoder_ctx.memory_tracker.peak_allocated);
    printf("Allocation Count: %u\n", g_basis_transcoder_ctx.memory_tracker.allocation_count);
    printf("Deallocation Count: %u\n", g_basis_transcoder_ctx.memory_tracker.deallocation_count);
    
    printf("\n--- Performance Counters ---\n");
    printf("Textures Transcoded: %lu\n", g_basis_transcoder_ctx.performance_counters.textures_transcoded);
    printf("Bytes Transcoded: %lu\n", g_basis_transcoder_ctx.performance_counters.bytes_transcoded);
    printf("Cache Hits: %lu\n", g_basis_transcoder_ctx.performance_counters.cache_hits);
    printf("Cache Misses: %lu\n", g_basis_transcoder_ctx.performance_counters.cache_misses);
    printf("GPU Operations: %lu\n", g_basis_transcoder_ctx.performance_counters.gpu_operations);
    printf("SIMD Operations: %lu\n", g_basis_transcoder_ctx.performance_counters.simd_operations);
    printf("Async Operations: %lu\n", g_basis_transcoder_ctx.performance_counters.async_operations);
    printf("Compression Operations: %lu\n", g_basis_transcoder_ctx.performance_counters.compression_operations);
    printf("Virtual Texture Operations: %lu\n", g_basis_transcoder_ctx.performance_counters.virtual_texture_operations);
    printf("Batch Operations: %lu\n", g_basis_transcoder_ctx.performance_counters.batch_operations);
    
    printf("\n--- Texture Details ---\n");
    for (uint32_t i = 0; i < g_basis_transcoder_ctx.count; i++) {
        const texture_basis_transcoder_internal_t* item = &g_basis_transcoder_ctx.items[i];
        if (!item->initialized) continue;
        
        printf("Texture %u: id=%u, data_size=%zu, initialized=%s\n",
               i, item->id, item->data_size, item->initialized ? "Yes" : "No");
    }
    
    printf("=== End Debug Information ===\n\n");
    
    pthread_mutex_unlock(&g_basis_transcoder_ctx.global_mutex);
}

/* ============================================================================
 * VIRTUAL TEXTURING API
 * ============================================================================ */

int texture_basis_transcoder_enable_virtual_texturing(texture_basis_transcoder_handle_t handle,
                                              uint32_t page_size,
                                              uint32_t max_pages) {
    if (!g_basis_transcoder_ctx.initialized || handle.id >= g_basis_transcoder_ctx.count) {
        return -1;
    }
    
    pthread_mutex_lock(&g_basis_transcoder_ctx.global_mutex);
    
    texture_basis_transcoder_internal_t* item = &g_basis_transcoder_ctx.items[handle.id];
    if (!item->initialized) {
        pthread_mutex_unlock(&g_basis_transcoder_ctx.global_mutex);
        return -2;
    }
    
    int result = texture_basis_init_virtual_texturing(item, page_size, max_pages);
    if (result == 0) {
        g_basis_transcoder_ctx.memory_tracker.total_allocated += 
            max_pages * (page_size * page_size * 4 + sizeof(texture_basis_virtual_page_t));
        g_basis_transcoder_ctx.memory_tracker.current_allocated += 
            max_pages * (page_size * page_size * 4 + sizeof(texture_basis_virtual_page_t));
    }
    
    pthread_mutex_unlock(&g_basis_transcoder_ctx.global_mutex);
    return result;
}

int texture_basis_transcoder_load_virtual_page(texture_basis_transcoder_handle_t handle,
                                        uint32_t page_x,
                                        uint32_t page_y,
                                        uint32_t page_level) {
    if (!g_basis_transcoder_ctx.initialized || handle.id >= g_basis_transcoder_ctx.count) {
        return -1;
    }
    
    pthread_mutex_lock(&g_basis_transcoder_ctx.global_mutex);
    
    texture_basis_transcoder_internal_t* item = &g_basis_transcoder_ctx.items[handle.id];
    if (!item->initialized || !item->virtual_texturing_enabled) {
        pthread_mutex_unlock(&g_basis_transcoder_ctx.global_mutex);
        return -2;
    }
    
    int result = texture_basis_load_virtual_page(item, page_x, page_y, page_level);
    
    pthread_mutex_unlock(&g_basis_transcoder_ctx.global_mutex);
    return result;
}

void texture_basis_transcoder_evict_virtual_pages(texture_basis_transcoder_handle_t handle) {
    if (!g_basis_transcoder_ctx.initialized || handle.id >= g_basis_transcoder_ctx.count) {
        return;
    }
    
    pthread_mutex_lock(&g_basis_transcoder_ctx.global_mutex);
    
    texture_basis_transcoder_internal_t* item = &g_basis_transcoder_ctx.items[handle.id];
    if (item->initialized && item->virtual_texturing_enabled) {
        texture_basis_evict_virtual_pages(item);
    }
    
    pthread_mutex_unlock(&g_basis_transcoder_ctx.global_mutex);
}

/* ============================================================================
 * COMPRESSION API
 * ============================================================================ */

int texture_basis_transcoder_enable_compression(texture_basis_transcoder_handle_t handle,
                                         texture_basis_compression_type_t compression_type,
                                         float quality) {
    if (!g_basis_transcoder_ctx.initialized || handle.id >= g_basis_transcoder_ctx.count) {
        return -1;
    }
    
    if (quality < 0.0f || quality > 1.0f) {
        return -2;
    }
    
    pthread_mutex_lock(&g_basis_transcoder_ctx.global_mutex);
    
    texture_basis_transcoder_internal_t* item = &g_basis_transcoder_ctx.items[handle.id];
    if (!item->initialized) {
        pthread_mutex_unlock(&g_basis_transcoder_ctx.global_mutex);
        return -3;
    }
    
    int result = texture_basis_init_compression_workspace(item, compression_type, quality);
    if (result == 0) {
        g_basis_transcoder_ctx.memory_tracker.total_allocated += 64 * 1024 * 1024; // 64MB workspace
        g_basis_transcoder_ctx.memory_tracker.current_allocated += 64 * 1024 * 1024;
    }
    
    pthread_mutex_unlock(&g_basis_transcoder_ctx.global_mutex);
    return result;
}

int texture_basis_transcoder_compress_texture(texture_basis_transcoder_handle_t handle,
                                       const void* src_data,
                                       size_t src_size,
                                       void* dst_data,
                                       size_t* dst_size) {
    if (!g_basis_transcoder_ctx.initialized || handle.id >= g_basis_transcoder_ctx.count) {
        return -1;
    }
    
    if (!src_data || !dst_data || !dst_size) {
        return -2;
    }
    
    pthread_mutex_lock(&g_basis_transcoder_ctx.global_mutex);
    
    texture_basis_transcoder_internal_t* item = &g_basis_transcoder_ctx.items[handle.id];
    if (!item->initialized || !item->compression_enabled) {
        pthread_mutex_unlock(&g_basis_transcoder_ctx.global_mutex);
        return -3;
    }
    
    int result = texture_basis_compress_data(src_data, src_size,
                                         item->compression_type,
                                         dst_data, dst_size,
                                         item->compression_quality);
    
    if (result == 0) {
        // Update compression statistics
        item->compression_workspace.compressed_size = *dst_size;
        item->compression_workspace.compression_ratio = (float)*dst_size / src_size;
        
        g_basis_transcoder_ctx.performance_counters.compression_operations++;
        g_basis_transcoder_ctx.performance_counters.bytes_transcoded += *dst_size;
    }
    
    pthread_mutex_unlock(&g_basis_transcoder_ctx.global_mutex);
    return result;
}

float texture_basis_transcoder_get_compression_ratio(texture_basis_transcoder_handle_t handle) {
    if (!g_basis_transcoder_ctx.initialized || handle.id >= g_basis_transcoder_ctx.count) {
        return 0.0f;
    }
    
    pthread_mutex_lock(&g_basis_transcoder_ctx.global_mutex);
    
    texture_basis_transcoder_internal_t* item = &g_basis_transcoder_ctx.items[handle.id];
    float ratio = 0.0f;
    
    if (item->initialized && item->compression_enabled) {
        ratio = item->compression_workspace.compression_ratio;
    }
    
    pthread_mutex_unlock(&g_basis_transcoder_ctx.global_mutex);
    return ratio;
}

/* End of basis_transcoder.c */

// Unit tests for basis transcoder system
int texture_basis_transcoder_run_unit_tests(void) {
    printf("=== Running Basis Transcoder Unit Tests ===\n");
    
    int tests_run = 0;
    int tests_passed = 0;
    
    // Test 1: System initialization
    printf("Test 1: System initialization... ");
    tests_run++;
    if (texture_basis_transcoder_init() == 0) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 2: Create basis transcoder
    printf("Test 2: Create basis transcoder... ");
    tests_run++;
    texture_basis_transcoder_handle_t handle;
    texture_basis_transcoder_desc_t desc = {
        .flags = 0x01, // Virtual texturing
        .width = 256,
        .height = 256,
        .depth = 1
    };
    
    if (texture_basis_transcoder_create(&handle, &desc) == 0) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 3: Update basis transcoder
    printf("Test 3: Update basis transcoder... ");
    tests_run++;
    uint8_t test_data[256 * 256 * 4]; // 256x256 RGBA
    memset(test_data, 0x80, sizeof(test_data));
    
    if (texture_basis_transcoder_update(handle, test_data, sizeof(test_data)) == 0) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 4: Validate basis transcoder
    printf("Test 4: Validate basis transcoder... ");
    tests_run++;
    if (texture_basis_transcoder_is_valid(handle)) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 5: Get basis transcoder info
    printf("Test 5: Get basis transcoder info... ");
    tests_run++;
    texture_basis_transcoder_info_t info;
    if (texture_basis_transcoder_get_info(handle, &info) == 0) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 6: Process pending operations
    printf("Test 6: Process pending operations... ");
    tests_run++;
    if (texture_basis_transcoder_process_pending() >= 0) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 7: Memory usage tracking
    printf("Test 7: Memory usage tracking... ");
    tests_run++;
    size_t memory_usage = texture_basis_transcoder_get_memory_usage();
    if (memory_usage > sizeof(g_basis_transcoder_ctx)) {
        printf("PASS (Usage: %zu bytes)\n", memory_usage);
        tests_passed++;
    } else {
        printf("FAIL (Usage: %zu bytes)\n", memory_usage);
    }
    
    // Test 8: Mark dirty
    printf("Test 8: Mark dirty... ");
    tests_run++;
    texture_basis_transcoder_mark_dirty(handle);
    printf("PASS\n");
    tests_passed++;
    
    // Test 9: Get count
    printf("Test 9: Get count... ");
    tests_run++;
    uint32_t count = texture_basis_transcoder_get_count();
    if (count > 0) {
        printf("PASS (Count: %u)\n", count);
        tests_passed++;
    } else {
        printf("FAIL (Count: %u)\n", count);
    }
    
    // Test 10: Enable virtual texturing
    printf("Test 10: Enable virtual texturing... ");
    tests_run++;
    if (texture_basis_transcoder_enable_virtual_texturing(handle, 128, 64) == 0) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 11: Load virtual page
    printf("Test 11: Load virtual page... ");
    tests_run++;
    if (texture_basis_transcoder_load_virtual_page(handle, 0, 0, 0) == 0) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 12: Enable compression
    printf("Test 12: Enable compression... ");
    tests_run++;
    if (texture_basis_transcoder_enable_compression(handle, TEXTURE_BASIS_COMPRESSION_BC7, 0.8f) == 0) {
        printf("PASS\n");
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 13: Compress texture
    printf("Test 13: Compress texture... ");
    tests_run++;
    uint8_t compressed_data[256 * 256 * 4];
    size_t compressed_size = sizeof(compressed_data);
    if (texture_basis_transcoder_compress_texture(handle, test_data, sizeof(test_data),
                                           compressed_data, &compressed_size) == 0) {
        printf("PASS (Compressed to %zu bytes)\n", compressed_size);
        tests_passed++;
    } else {
        printf("FAIL\n");
    }
    
    // Test 14: Get compression ratio
    printf("Test 14: Get compression ratio... ");
    tests_run++;
    float ratio = texture_basis_transcoder_get_compression_ratio(handle);
    if (ratio > 0.0f && ratio <= 1.0f) {
        printf("PASS (Ratio: %.2f)\n", ratio);
        tests_passed++;
    } else {
        printf("FAIL (Ratio: %.2f)\n", ratio);
    }
    
    // Test 15: Evict virtual pages
    printf("Test 15: Evict virtual pages... ");
    tests_run++;
    texture_basis_transcoder_evict_virtual_pages(handle);
    printf("PASS\n");
    tests_passed++;
    
    // Test 16: Destroy basis transcoder
    printf("Test 16: Destroy basis transcoder... ");
    tests_run++;
    texture_basis_transcoder_destroy(handle);
    printf("PASS\n");
    tests_passed++;
    
    // Cleanup
    texture_basis_transcoder_shutdown();
    
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
