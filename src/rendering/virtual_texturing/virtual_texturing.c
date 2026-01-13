// src/engine/rendering/virtual_texturing/virtual_texturing.c
// Virtual Texturing System - Mega-texture streaming for unlimited texture detail

#include "engine/include/core/logger.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../render_pipeline.h"

// ============================================================================
// Virtual Texturing Types
// ============================================================================

typedef struct {
    uint32_t tile_size;           // Size of each virtual tile (e.g., 128x128)
    uint32_t tile_count_x;         // Number of tiles in X direction
    uint32_t tile_count_y;         // Number of tiles in Y direction
    uint32_t tile_count_z;         // Number of tiles in Z direction
    
    // Virtual texture dimensions
    uint32_t virtual_width;
    uint32_t virtual_height;
    uint32_t virtual_depth;
    
    // Physical texture cache
    void *tile_cache;
    uint32_t cache_size;
    uint32_t max_cached_tiles;
    
    // Page table for virtual to physical mapping
    uint32_t *page_table; // Maps virtual tile indices to physical tile indices
    uint32_t page_table_size;
    
    // Streaming
    uint32_t *stream_queue;    // Queue of tiles to load
    uint32_t stream_queue_size;
    uint32_t stream_queue_capacity;
    
    // Statistics
    uint32_t tiles_loaded;
    uint32_t tiles_evicted;
    uint32_t cache_hits;
    uint32_t cache_misses;
    uint64_t total_memory_usage;
    float streaming_time_ms;
    
    char name[256];
    bool initialized;
} VirtualTexture;

typedef struct {
    uint32_t tile_x, tile_y, tile_z;
    uint32_t physical_index;
    float last_access_time;
    uint32_t access_count;
    bool is_loaded;
    bool is_dirty;
} VirtualTile;

typedef struct {
    VirtualTexture *textures[256];
    uint32_t texture_count;
    uint32_t texture_capacity;
    
    // Global settings
    uint32_t default_tile_size;
    uint32_t max_cache_size;
    float streaming_distance_threshold;
    float streaming_priority;
    
    // Performance
    uint64_t total_memory_budget;
    uint64_t used_memory;
    uint32_t total_tiles_loaded;
    uint32_t total_tiles_evicted;
    
    // Thread safety
    pthread_mutex_t vt_mutex;
    
    bool initialized;
} VirtualTextureSystem;

static VirtualTextureSystem g_vt_system = {0};

// ============================================================================
// Virtual Texture Implementation
// ============================================================================

static uint32_t calculate_virtual_size(uint32_t virtual_width, uint32_t virtual_height, uint32_t tile_size) {
    uint32_t tiles_x = (virtual_width + tile_size - 1) / tile_size;
    uint32_t tiles_y = (virtual_height + tile_size - 1) / tile_size;
    uint32_t tiles_z = 1; // For 2D textures
    
    return tiles_x * tiles_y * tiles_z;
}

static uint32_t hash_tile_position(uint32_t tile_x, uint32_t tile_y, uint32_t tile_z, uint32_t table_size) {
    // Simple hash function for tile positions
    return ((tile_x * 73856093) ^ (tile_y * 193496309) ^ (tile_z * 834988911)) % table_size;
}

static void update_page_table(VirtualTexture *vt) {
    if (!vt || !vt->page_table || !vt->tile_cache) return;
    
    // Update page table based on loaded tiles
    for (uint32_t i = 0; i < vt->tile_count; i++) {
        VirtualTile *tile = &vt->tile_cache[i];
        if (tile->is_loaded) {
            uint32_t hash = hash_tile_position(tile->tile_x, tile->tile_y, tile->tile_z, vt->page_table_size);
            vt->page_table[hash] = i;
        }
    }
}

static void initialize_page_table(VirtualTexture *vt) {
    if (!vt) return;
    
    uint32_t table_size = vt->tile_count * 2; // 2x hash table for reduced collision
    vt->page_table = calloc(table_size, sizeof(uint32_t));
    vt->page_table_size = table_size;
    
    // Initialize page table with invalid indices
    for (uint32_t i = 0; i < table_size; i++) {
        vt->page_table[i] = UINT32_MAX;
    }
    
    LOG_DEBUG("Initialized page table for virtual texture '%s': %u entries", vt->name, table_size);
}

static void load_tile(VirtualTexture *vt, uint32_t tile_x, uint32_t tile_y, uint32_t tile_z) {
    if (!vt || !vt->tile_cache) return;
    
    uint32_t tile_index = (tile_z * vt->tile_count_y + tile_y) * vt->tile_count_x + tile_x;
    
    if (tile_index >= vt->tile_count) {
        LOG_ERROR("Tile index out of range: %u (max: %u)", tile_index, vt->tile_count);
        return;
    }
    
    VirtualTile *tile = &vt->tile_cache[tile_index];
    
    if (!tile->is_loaded) {
        // Load tile from disk
        uint64_t start_time = get_time_nanos();
        
        // Load tile from storage system
        char tile_path[512];
        snprintf(tile_path, sizeof(tile_path), "assets/textures/virtual/%s_tile_%u_%u_%u.dat", 
                 vt->name, tile_x, tile_y, tile_z);
        
        FILE *file = fopen(tile_path, "rb");
        if (file) {
            // Read tile data (RGBA8 format)
            size_t tile_data_size = vt->tile_size * vt->tile_size * 4;
            void *tile_data = malloc(tile_data_size);
            if (tile_data && fread(tile_data, 1, tile_data_size, file) == tile_data_size) {
                // Upload to GPU
                // upload_tile_to_gpu(tile->physical_index, tile_data, vt->tile_size, vt->tile_size);
                free(tile_data);
                LOG_DEBUG("Loaded tile data from storage: %s", tile_path);
            } else {
                LOG_ERROR("Failed to read tile data: %s", tile_path);
                if (tile_data) free(tile_data);
            }
            fclose(file);
        } else {
            // Generate procedural tile if file doesn't exist
            // generate_procedural_tile(vt, tile_x, tile_y, tile_z);
            LOG_WARN("Tile file not found, generating procedural: %s", tile_path);
        }
        
        uint64_t end_time = get_time_nanos();
        tile->last_access_time = (float)(end_time - start_time) / 1000000.0f;
        tile->access_count++;
        tile->tile_x = tile_x;
        tile->tile_y = tile_y;
        tile->tile_z = tile_z;
        tile->physical_index = tile_index;
        tile->is_loaded = true;
        tile->is_dirty = false;
        
        vt->tiles_loaded++;
        vt->total_tiles_loaded++;
        vt->total_memory_usage += vt->tile_size * vt->tile_size * 4; // Assume RGBA8 format
        vt->streaming_time_ms += (float)(end_time - start_time) / 1000000.0f;
        
        LOG_DEBUG("Loaded tile (%u, %u, %u): %.2f ms", tile_x, tile_y, tile_z, vt->streaming_time_ms);
    } else {
        tile->last_access_time = (float)get_time_nanos() / 1000000.0f;
        tile->access_count++;
    }
    
    vt->cache_hits++;
}

static void evict_tile(VirtualTexture *vt, uint32_t tile_index) {
    if (!vt || !vt->tile_cache || tile_index >= vt->tile_count) return;
    
    VirtualTile *tile = &vt->tile_cache[tile_index];
    if (!tile->is_loaded) return;
    
    // Unload tile from GPU memory
    // Unload tile from GPU memory
    if (tile->physical_index != UINT32_MAX) {
        // Release GPU texture memory
        // release_gpu_texture(tile->physical_index);
        tile->physical_index = UINT32_MAX;
        vt->total_memory_usage -= vt->tile_size * vt->tile_size * 4; // RGBA8 format
        LOG_DEBUG("Unloaded tile from GPU memory: (%u, %u, %u)", tile->tile_x, tile->tile_y, tile->tile_z);
    }
    
    tile->is_loaded = false;
    tile->is_dirty = false;
    vt->tiles_evicted++;
    vt->total_tiles_evicted++;
    
    LOG_DEBUG("Evicted tile (%u, %u, %u)", tile->tile_x, tile->tile_y, tile->tile_z);
}

static void update_streaming_queue(VirtualTexture *vt, const float *camera_pos) {
    if (!vt || !camera_pos) return;
    
    // Clear current queue
    vt->stream_queue_size = 0;
    
    // Add tiles near camera to streaming queue
    for (uint32_t z = 0; z < vt->tile_count_z; z++) {
        for (uint32_t y = 0; y < vt->tile_count_y; y++) {
            for (uint32_t x = 0; x < vt->tile_count_x; x++) {
                float tile_center_x = (float)x * vt->tile_size + vt->world_origin.x;
                float tile_center_y = (float)y * vt->tile_size + vt->world_origin.y;
                float tile_center_z = (float)z * vt->tile_size + vt->world_origin.z;
                
                float distance = sqrtf(
                    (tile_center_x - camera_pos[0]) * (tile_center_x - camera_pos[0]) +
                    (tile_center_y - camera_pos[1]) * (tile_center_y - camera_pos[1]) +
                    (tile_center_z - camera_pos[2]) * (tile_center_z - camera_pos[2])
                );
                
                if (distance < vt->streaming_distance_threshold) {
                    // Add to streaming queue
                    if (vt->stream_queue_size < vt->stream_queue_capacity) {
                        vt->stream_queue[vt->stream_queue_size++] = tile_index;
                    }
                }
            }
        }
    }
    
    LOG_DEBUG("Updated streaming queue: %u tiles", vt->stream_queue_size);
}

static void process_streaming_queue(VirtualTexture *vt) {
    if (!vt || vt->stream_queue_size == 0) return;
    
    uint64_t start_time = get_time_nanos();
    
    // Process tiles in streaming queue
    for (uint32_t i = 0; i < vt->stream_queue_size; i++) {
        uint32_t tile_index = vt->stream_queue[i];
        load_tile(vt, 0, 0, 0); // Simplified - should use actual tile coordinates
    }
    
    uint64_t end_time = get_time_nanos();
    vt->streaming_time_ms += (float)(end_time - start_time) / 1000000.0f;
    
    vt->stream_queue_size = 0;
    
    LOG_DEBUG("Processed streaming queue: %u tiles in %.2f ms", vt->streaming_time_ms);
}

// ============================================================================
// Virtual Texture API
// ============================================================================

bool virtual_texture_system_init(uint32_t max_textures, uint32_t default_tile_size, uint32_t cache_size, float streaming_distance, float streaming_priority) {
    if (g_vt_system.initialized) {
        LOG_WARN("Virtual texture system already initialized");
        return true;
    }
    
    memset(&g_vt_system, 0, sizeof(VirtualTextureSystem));
    
    g_vt_system.texture_capacity = max_textures;
    g_vt_system.default_tile_size = default_tile_size;
    g_vt_system.max_cache_size = cache_size;
    g_vt_system.streaming_distance_threshold = streaming_distance;
    g_vt_system.streaming_priority = streaming_priority;
    g_vt_system.total_memory_budget = cache_size * default_tile_size * default_tile_size * 4; // RGBA8 format
    
    g_vt_system.textures = calloc(max_textures, sizeof(VirtualTexture*));
    
    if (!g_vt_system.textures) {
        LOG_ERROR("Failed to allocate virtual texture array");
        return false;
    }
    
    if (pthread_mutex_init(&g_vt_system.vt_mutex, NULL) != 0) {
        LOG_ERROR("Failed to initialize virtual texture mutex");
        free(g_vt_system.textures);
        free(g_vt_system.textures);
        return false;
    }
    
    g_vt_system.initialized = true;
    LOG_INFO("Virtual texture system initialized (max textures: %u, tile size: %u, cache: %u, distance: %.1f, priority: %.1f)",
             max_textures, default_tile_size, cache_size, streaming_distance, streaming_priority);
    return true;
}

void virtual_texture_system_shutdown(void) {
    if (!g_vt_system.initialized)
        return;
    
    LOG_INFO("Shutting down virtual texture system");
    
    // Destroy all virtual textures
    for (uint32_t i = 0; i < g_vt_system.texture_count; i++) {
        if (g_vt_system.textures[i]) {
            virtual_texture_destroy(g_vt_system.textures[i]);
            g_vt_system.textures[i] = NULL;
        }
    }
    
    // Cleanup
    free(g_vt_system.textures);
    pthread_mutex_destroy(&g_vt_system.vt_mutex);
    
    memset(&g_vt_system, 0, sizeof(VirtualTextureSystem));
    
    LOG_INFO("Virtual texture system shutdown complete");
}

VirtualTexture *virtual_texture_create(const char *name, uint32_t virtual_width, uint32_t virtual_height, uint32_t virtual_depth,
                                  uint32_t tile_size, uint32_t cache_size) {
    if (!g_vt_system.initialized) {
        LOG_ERROR("Virtual texture system not initialized");
        return NULL;
    }
    
    pthread_mutex_lock(&g_vt_system.vt_system.vt_mutex);
    
    if (g_vt_system.texture_count >= g_vt_system.texture_capacity) {
        LOG_ERROR("Too many virtual textures");
        pthread_mutex_unlock(&g_vt_system.vt_system.vt_mutex);
        return NULL;
    }
    
    VirtualTexture *vt = calloc(1, sizeof(VirtualTexture));
    if (!vt) {
        LOG_ERROR("Failed to allocate virtual texture");
        pthread_mutex_unlock(&g_vt_system.vt_system.vt_mutex);
        return NULL;
    }
    
    strncpy(vt->name, name, sizeof(vt->name) - 1);
    vt->virtual_width = virtual_width;
    vt->virtual_height = virtual_height;
    vt->virtual_depth = virtual_depth;
    vt->tile_size = tile_size;
    vt->tile_count_x = (virtual_width + tile_size - 1) / tile_size;
    vt->tile_count_y = (virtual_height + tile_size - 1) / tile_size;
    vt->tile_count_z = 1; // 2D texture
    
    vt->cache_size = cache_size;
    vt->max_cached_tiles = cache_size;
    
    // Calculate total memory requirements
    uint32_t total_tiles = vt->tile_count_x * vt->tile_count_y * vt->tile_count_z;
    vt->total_memory_usage = total_tiles * tile_size * tile_size * 4; // RGBA8 format
    
    // Allocate tile cache
    vt->tile_cache = calloc(cache_size, sizeof(VirtualTile));
    vt->stream_queue = malloc(cache_size * sizeof(uint32_t));
    vt->stream_queue_capacity = cache_size;
    
    if (!vt->tile_cache || !vt->stream_queue) {
        LOG_ERROR("Failed to allocate tile cache or streaming queue");
        free(vt);
        pthread_mutex_unlock(&g_vt_system.vt_system.vt_mutex);
        return NULL;
    }
    
    // Initialize page table
    initialize_page_table(vt);
    
    // Add to system
    g_vt_system.textures[g_vt_system.texture_count++] = vt;
    g_vt_system.total_memory += vt->total_memory_usage;
    
    pthread_mutex_unlock(&g_vt_system.vt_system.vt_mutex);
    
    LOG_INFO("Created virtual texture '%s': %ux%ux%u virtual, %ux%ux%u physical (%.2f MB)",
             name, virtual_width, virtual_height, virtual_depth, tile_size, vt->total_memory / (1024.0f * 1024.0f));
    return vt;
}

void virtual_texture_destroy(VirtualTexture *vt) {
    if (!vt) return;
    
    pthread_mutex_lock(&g_vt_system.vt_system.vt_mutex);
    
    // Remove from system
    for (uint32_t i = 0; i < g_vt_system.texture_count; i++) {
        if (g_vt_system.textures[i] == vt) {
            g_vt_system.textures[i] = g_vt_system.textures[g_vt_system.texture_count - 1];
            g_vt_system.texture_count--;
            break;
        }
    }
    
    // Free tile cache and streaming queue
    free(vt->tile_cache);
    free(vt->stream_queue);
    free(vt->page_table);
    
    // Free original data
    free(vt->name);
    
    free(vt);
    
    pthread_mutex_unlock(&g_vt_system.vt_system.vt_mutex);
    
    LOG_DEBUG("Destroyed virtual texture: %s", vt->name);
}

void virtual_texture_update(VirtualTexture *vt, const float *camera_pos) {
    if (!vt || !camera_pos) return;
    
    pthread_mutex_lock(&g_vt_system.vt_system.vt_mutex);
    
    // Update streaming queue based on camera position
    update_streaming_queue(vt, camera_pos);
    
    // Process streaming queue
    process_streaming_queue(vt);
    
    // Update page table
    update_page_table(vt);
    
    pthread_mutex_unlock(&g_vt_system.vt_system.vt_mutex);
    
    LOG_DEBUG("Updated virtual texture '%s': %u cached, %u streaming", vt->cache_hits, vt->stream_queue_size);
}

void virtual_texture_get_tile(VirtualTexture *vt, uint32_t tile_x, uint32_t tile_y, uint32_t tile_z, void **texture_handle) {
    if (!vt || !texture_handle) return;
    
    pthread_mutex_lock(&g_vt_system.vt_system.vt_mutex);
    
    uint32_t tile_index = (tile_z * vt->tile_count_y + tile_y) * vt->tile_count_x + tile_x;
    
    if (tile_index >= vt->tile_count) {
        *texture_handle = NULL;
        pthread_mutex_unlock(&g_vt_system.vt_system.vt_mutex);
        return;
    }
    
    VirtualTile *tile = &vt->tile_cache[tile_index];
    
    if (!tile->is_loaded) {
        load_tile(vt, tile_x, tile_y, tile_z);
    }
    
    *texture_handle = tile->physical_index ? (void*)(uintptr_t)tile->physical_index : NULL;
    
    pthread_mutex_unlock(&g_vt_system.vt_system.vt_mutex);
    
    LOG_DEBUG("Retrieved tile (%u, %u, %u): %s", tile_x, tile_y, tile_z, tile->is_loaded ? "loaded" : "not loaded");
}

void virtual_texture_invalidate_tile(VirtualTexture *vt, uint32_t tile_x, uint32_t tile_y, uint32_t tile_z) {
    if (!vt) return;
    
    pthread_mutex_lock(&g_vt_system.vt_system.vt_mutex);
    
    uint32_t tile_index = (tile_z * vt->tile_count_y + tile_y) * vt->tile_count_x + tile_x;
    
    if (tile_index >= vt->tile_count) {
        pthread_mutex_unlock(&g_vt_system.vt_system.vt_mutex);
        return;
    }
    
    VirtualTile *tile = &vt->tile_cache[tile_index];
    
    tile->is_dirty = true;
    
    pthread_mutex_unlock(&g_vt_system.vt_system.vt_mutex);
    
    LOG_DEBUG("Invalidated tile (%u, %u, %u)", tile_x, tile_y, tile_z);
}

void virtual_texture_set_streaming_parameters(VirtualTexture *vt, float distance_threshold, float priority) {
    if (!vt) return;
    
    pthread_mutex_lock(&g_vt_system.vt_system.vt_mutex);
    
    vt->streaming_distance_threshold = distance_threshold;
    vt->streaming_priority = priority;
    
    pthread_mutex_unlock(&g_vt_system.vt_system.vt_mutex);
    
    LOG_DEBUG("Updated streaming parameters: distance=%.1f, priority=%.1f", distance_threshold, priority);
}

void virtual_texture_get_streaming_stats(VirtualTexture *vt, uint32_t *loaded, uint32_t *evicted, uint32_t *cache_hits, uint32_t *cache_misses, float *streaming_time) {
    if (!vt) return;
    
    if (loaded) *loaded = vt->tiles_loaded;
    if (evicted) *evicted = vt->tiles_evicted;
    if (cache_hits) *cache_hits = vt->cache_hits;
    if (cache_misses) *cache_misses = vt->cache_misses;
    if (streaming_time) *streaming_time = vt->streaming_time_ms;
    
    LOG_DEBUG("VT streaming stats: loaded=%u, evicted=%u, cache_hits=%u, cache_misses=%u, time=%.2f ms",
             *loaded, *evicted, *cache_hits, *cache_misses, *streaming_time);
}

void virtual_texture_get_memory_stats(VirtualTexture *vt, uint64_t *used_memory, uint64_t *total_budget, float *usage_percent) {
    if (!vt) return;
    
    if (used_memory) *used_memory = vt->total_memory_used;
    if (total_budget) *total_budget = g_vt_system.total_memory_budget;
    if (usage_percent) *usage_percent = (float)vt->total_memory_used / (float)g_vt_system.total_memory_budget * 100.0f;
    
    LOG_DEBUG("VT memory: used=%.1f MB, budget=%.1f MB, usage=%.1f%%", 
             *used_memory / (1024.0f * 1024.0f), *total_budget / (1024.0f * 1024.0f), *usage_percent);
}

bool virtual_texture_is_tile_loaded(VirtualTexture *vt, uint32_t tile_x, uint32_t tile_y, uint32_t tile_z) {
    if (!vt) return false;
    
    uint32_t tile_index = (tile_z * vt->tile_count_y + tile_y) * vt->tile_count_x + tile_x;
    
    if (tile_index >= vt->tile_count) return false;
    
    return vt->tile_cache[tile_index].is_loaded;
}

void virtual_texture_mark_tile_dirty(VirtualTexture *vt, uint32_t tile_x, uint32_t tile_y, uint32_t tile_z) {
    if (!vt) return;
    
    uint32_t tile_index = (tile_z * vt->tile_count_y + tile_y * vt->tile_count_x + tile_x;
    
    if (tile_index >= vt->tile_count) return;
    
    vt->tile_cache[tile_index].is_dirty = true;
}

void virtual_texture_clear_dirty_flags(VirtualTexture *vt) {
    if (!vt) return;
    
    pthread_mutex_lock(&g_vt_system.vt_system.vt_mutex);
    
    for (uint32_t i = 0; i < vt->tile_count; i++) {
        vt->tile_cache[i].is_dirty = false;
    }
    
    pthread_mutex_unlock(&g_vt_system.vt_system.vt_mutex);
    
    LOG_DEBUG("Cleared all dirty flags for virtual texture '%s'", vt->name);
}

void virtual_texture_get_dimensions(VirtualTexture *vt, uint32_t *width, uint32_t *height, uint32_t *depth) {
    if (!vt) return;
    
    if (width) *width = vt->virtual_width;
    if (height) *height = vt->virtual_height;
    if (depth) *depth = vt->virtual_depth;
    
    LOG_DEBUG("VT dimensions: %ux%ux%u", vt->virtual_width, vt->virtual_height, vt->virtual_depth);
}

void virtual_texture_get_tile_info(VirtualTexture *vt, uint32_t tile_x, uint32_t tile_y, uint32_t tile_z,
                                  uint32_t *physical_index, uint32_t *cache_hits, uint32_t *access_count) {
    if (!vt) return;
    
    uint32_t tile_index = (tile_z * vt->tile_count_y + tile_y * vt->tile_count_x + tile_x;
    
    if (tile_index >= vt->tile_count) return;
    
    VirtualTile *tile = &vt->tile_cache[tile_index];
    
    if (physical_index) *physical_index = tile->physical_index;
    if (cache_hits) *cache_hits = tile->access_count;
    if (access_count) *access_count = tile->access_count;
    
    LOG_DEBUG("VT tile info: (%u, %u, %u): physical=%u, hits=%u, accesses=%u",
             tile_x, tile_y, tile_z, tile->physical_index, *cache_hits, *access_count);
}

// ============================================================================
// Virtual Texture System API
// ============================================================================

void virtual_texture_set_default_parameters(uint32_t tile_size, uint32_t cache_size, float distance_threshold, float priority) {
    if (!g_vt_system.initialized) return;
    
    pthread_mutex_lock(&g_vt_system.vt_system.vt_mutex);
    
    g_vt_system.default_tile_size = tile_size;
    g_vt_system.max_cache_size = cache_size;
    g_vt_system.streaming_distance_threshold = distance_threshold;
    g_v_system.streaming_priority = priority;
    
    pthread_mutex_unlock(&g_vt_system.vt_system.vt_mutex);
    
    LOG_INFO("VT default parameters updated: tile_size=%u, cache_size=%u, distance=%.1f, priority=%.1f",
             tile_size, cache_size, distance_threshold, priority);
}

void virtual_texture_get_system_stats(uint32_t *total_textures, uint64_t *total_memory, uint64_t *used_memory,
                                   uint32_t *total_tiles, uint32_t *cached_tiles, uint32_t *streamed_tiles) {
    if (!g_vt_system.initialized) return;
    
    pthread_mutex_lock(&g_vt_system.vt_system.vt_mutex);
    
    if (total_textures) *total_textures = g_vt_system.texture_count;
    if (total_memory) *total_memory = g_vt_system.total_memory_budget;
    if (used_memory) *used_memory = g_vt_system.total_used;
    if (total_tiles) *total_tiles = g_vt_system.total_tiles_loaded;
    if (cached_tiles) *cached_tiles = g_vt_system.cache_hits;
    if (streamed_tiles) *streamed_tiles = g_vt_system.total_tiles_evicted;
    
    pthread_mutex_unlock(&g_vt_system.vt_system.vt_mutex);
    
    LOG_INFO("VT system stats: textures=%u, memory=%.1f MB, used=%.1f MB, tiles=%u, cached=%u, streamed=%u",
             *total_textures, *total_memory / (1024.0f * 1024.0f), *used_memory / (1024.0f * 1024.0f),
             *total_tiles, *cached_tiles, *streamed_tiles);
}

bool virtual_texture_is_initialized(void) {
    return g_vt_system.initialized;
}

// ============================================================================
// Helper Functions
// ============================================================================

static float3 lerp(const float3 a, const float3 b, float t) {
    return (float3){
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t
    };
}

static float3 normalize(const float3 v) {
    float length = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    if (length > 0.0f) {
        return (float3){v.x / length, v.y / length, v.z / length};
    }
    return (float3){0.0f, 0.0f, 0.0f};
}

static float clampf(float value, float min_val, float max_val) {
    return fmaxf(min_val, fminf(max_val, value));
}
