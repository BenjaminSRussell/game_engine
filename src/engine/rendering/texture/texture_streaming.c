// src/engine/rendering/texture/texture_streaming.c
// Texture Streaming System - Dynamic texture loading and unloading

#include <core/logger.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "../render_pipeline.h"

// ============================================================================
// Texture Streaming Types
// ============================================================================

typedef enum {
    TEXTURE_STATUS_UNLOADED,
    TEXTURE_STATUS_LOADING,
    TEXTURE_STATUS_LOADED,
    TEXTURE_STATUS_ERROR,
    TEXTURE_STATUS_UNLOADING
} TextureStatus;

typedef enum {
    TEXTURE_PRIORITY_LOW = 0,
    TEXTURE_PRIORITY_MEDIUM = 1,
    TEXTURE_PRIORITY_HIGH = 2,
    TEXTURE_PRIORITY_CRITICAL = 3
} TexturePriority;

typedef struct {
    char file_path[512];
    char name[256];
    void *texture_handle;
    TextureStatus status;
    TexturePriority priority;
    
    // Memory information
    uint32_t width, height, depth;
    uint32_t format;
    uint64_t memory_size;
    
    // Streaming information
    float distance_to_camera;
    float last_access_time;
    uint32_t access_count;
    bool is_persistent;
    
    // Async loading
    pthread_t load_thread;
    bool async_load_in_progress;
    void *temp_data; // Loaded data before GPU upload
    
    // Reference counting
    uint32_t ref_count;
} TextureEntry;

typedef struct {
    TextureEntry *textures;
    uint32_t texture_count;
    uint32_t texture_capacity;
    
    // Memory management
    uint64_t total_memory_budget;
    uint64_t used_memory;
    uint64_t peak_memory;
    
    // Streaming parameters
    float unload_distance_threshold;
    float load_distance_threshold;
    uint32_t max_concurrent_loads;
    uint32_t current_loads;
    
    // Priority queue for loading/unloading
    TextureEntry *load_queue[1024];
    uint32_t load_queue_size;
    
    TextureEntry *unload_queue[1024];
    uint32_t unload_queue_size;
    
    // Thread management
    pthread_mutex_t stream_mutex;
    pthread_cond_t stream_condition;
    pthread_t stream_thread;
    bool streaming_active;
    
    // Statistics
    uint32_t textures_loaded;
    uint32_t textures_unloaded;
    uint32_t streaming_errors;
    uint64_t total_bytes_streamed;
    
    bool initialized;
} TextureStreamingSystem;

static TextureStreamingSystem g_streaming_system = {0};

// ============================================================================
// Texture Loading Utilities
// ============================================================================

static uint64_t calculate_texture_memory_size(uint32_t width, uint32_t height, 
                                              uint32_t depth, uint32_t format) {
    // Simplified memory calculation
    uint32_t bytes_per_pixel = 4; // Assume RGBA8
    switch (format) {
        case 1: bytes_per_pixel = 1; break;  // R8
        case 2: bytes_per_pixel = 2; break;  // RG8
        case 3: bytes_per_pixel = 3; break;  // RGB8
        case 4: bytes_per_pixel = 4; break;  // RGBA8
        case 6: bytes_per_pixel = 6; break;  // RGB16F
        case 8: bytes_per_pixel = 8; break;  // RGBA16F
        default: bytes_per_pixel = 4; break;
    }
    
    return (uint64_t)width * height * depth * bytes_per_pixel;
}

static void* load_texture_from_file(const char *file_path, uint32_t *width, uint32_t *height, 
                                   uint32_t *format) {
    // TODO: Implement actual texture loading using stb_image or similar
    // This is a placeholder that would load PNG/JPEG/etc.
    
    LOG_DEBUG("Loading texture from file: %s", file_path);
    
    // Simulate loading
    *width = 256;
    *height = 256;
    *format = 4; // RGBA8
    
    size_t data_size = (*width) * (*height) * (*format);
    void *data = malloc(data_size);
    if (data) {
        // Fill with dummy data (checkerboard pattern)
        uint8_t *pixels = (uint8_t*)data;
        for (uint32_t y = 0; y < *height; y++) {
            for (uint32_t x = 0; x < *width; x++) {
                uint32_t index = (y * *width + x) * 4;
                uint8_t value = ((x / 16) + (y / 16)) % 2 ? 255 : 0;
                pixels[index + 0] = value;     // R
                pixels[index + 1] = value;     // G
                pixels[index + 2] = value;     // B
                pixels[index + 3] = 255;       // A
            }
        }
    }
    
    return data;
}

static void* upload_texture_to_gpu(void *data, uint32_t width, uint32_t height, uint32_t format) {
    // TODO: Implement actual GPU texture upload using Metal/Vulkan backend
    // This is a placeholder that would create a GPU texture
    
    LOG_DEBUG("Uploading texture to GPU: %ux%u, format %u", width, height, format);
    
    // Return a fake texture handle for now
    return (void*)(uintptr_t)(rand() % 10000 + 1);
}

static void destroy_gpu_texture(void *texture_handle) {
    // TODO: Implement actual GPU texture destruction
    LOG_DEBUG("Destroying GPU texture: %p", texture_handle);
}

// ============================================================================
// Async Loading Functions
// ============================================================================

static void* texture_load_thread_func(void *arg) {
    TextureEntry *entry = (TextureEntry*)arg;
    
    LOG_DEBUG("Starting async load for texture: %s", entry->name);
    
    // Load from file
    uint32_t width, height, format;
    entry->temp_data = load_texture_from_file(entry->file_path, &width, &height, &format);
    
    if (!entry->temp_data) {
        entry->status = TEXTURE_STATUS_ERROR;
        g_streaming_system.streaming_errors++;
        LOG_ERROR("Failed to load texture file: %s", entry->file_path);
        return NULL;
    }
    
    // Update texture info
    entry->width = width;
    entry->height = height;
    entry->format = format;
    entry->memory_size = calculate_texture_memory_size(width, height, 1, format);
    
    // Upload to GPU (this might need to happen on main thread)
    entry->texture_handle = upload_texture_to_gpu(entry->temp_data, width, height, format);
    
    if (!entry->texture_handle) {
        entry->status = TEXTURE_STATUS_ERROR;
        g_streaming_system.streaming_errors++;
        free(entry->temp_data);
        entry->temp_data = NULL;
        LOG_ERROR("Failed to upload texture to GPU: %s", entry->name);
        return NULL;
    }
    
    // Clean up temporary data
    free(entry->temp_data);
    entry->temp_data = NULL;
    
    // Update status
    pthread_mutex_lock(&g_streaming_system.stream_mutex);
    entry->status = TEXTURE_STATUS_LOADED;
    g_streaming_system.used_memory += entry->memory_size;
    g_streaming_system.textures_loaded++;
    g_streaming_system.total_bytes_streamed += entry->memory_size;
    g_streaming_system.current_loads--;
    entry->async_load_in_progress = false;
    pthread_mutex_unlock(&g_streaming_system.stream_mutex);
    
    LOG_DEBUG("Texture loaded successfully: %s (%.2f MB)", entry->name, 
             entry->memory_size / (1024.0f * 1024.0f));
    
    return NULL;
}

// ============================================================================
// Streaming Management
// ============================================================================

static void add_to_load_queue(TextureEntry *entry) {
    if (g_streaming_system.load_queue_size >= 1024) {
        LOG_WARN("Load queue is full");
        return;
    }
    
    g_streaming_system.load_queue[g_streaming_system.load_queue_size++] = entry;
}

static void add_to_unload_queue(TextureEntry *entry) {
    if (g_streaming_system.unload_queue_size >= 1024) {
        LOG_WARN("Unload queue is full");
        return;
    }
    
    g_streaming_system.unload_queue[g_streaming_system.unload_queue_size++] = entry;
}

static void process_load_queue(void) {
    while (g_streaming_system.load_queue_size > 0 && 
           g_streaming_system.current_loads < g_streaming_system.max_concurrent_loads) {
        
        // Find highest priority texture
        int best_index = -1;
        TexturePriority best_priority = TEXTURE_PRIORITY_LOW;
        
        for (uint32_t i = 0; i < g_streaming_system.load_queue_size; i++) {
            TextureEntry *entry = g_streaming_system.load_queue[i];
            if (entry->priority > best_priority) {
                best_priority = entry->priority;
                best_index = i;
            }
        }
        
        if (best_index == -1) break;
        
        TextureEntry *entry = g_streaming_system.load_queue[best_index];
        
        // Remove from queue
        g_streaming_system.load_queue[best_index] = g_streaming_system.load_queue[--g_streaming_system.load_queue_size];
        
        // Check if we have enough memory
        if (g_streaming_system.used_memory + entry->memory_size > g_streaming_system.total_memory_budget) {
            // Need to unload some textures first
            add_to_unload_queue(entry);
            continue;
        }
        
        // Start async load
        entry->status = TEXTURE_STATUS_LOADING;
        entry->async_load_in_progress = true;
        g_streaming_system.current_loads++;
        
        if (pthread_create(&entry->load_thread, NULL, texture_load_thread_func, entry) != 0) {
            LOG_ERROR("Failed to create texture loading thread");
            entry->status = TEXTURE_STATUS_ERROR;
            entry->async_load_in_progress = false;
            g_streaming_system.current_loads--;
        } else {
            pthread_detach(entry->load_thread);
        }
    }
}

static void process_unload_queue(void) {
    while (g_streaming_system.unload_queue_size > 0) {
        TextureEntry *entry = g_streaming_system.unload_queue[0];
        
        // Remove from queue
        for (uint32_t i = 0; i < g_streaming_system.unload_queue_size - 1; i++) {
            g_streaming_system.unload_queue[i] = g_streaming_system.unload_queue[i + 1];
        }
        g_streaming_system.unload_queue_size--;
        
        // Don't unload persistent or referenced textures
        if (entry->is_persistent || entry->ref_count > 0) {
            continue;
        }
        
        // Unload texture
        if (entry->texture_handle) {
            destroy_gpu_texture(entry->texture_handle);
            entry->texture_handle = NULL;
            g_streaming_system.used_memory -= entry->memory_size;
            g_streaming_system.textures_unloaded++;
            
            LOG_DEBUG("Texture unloaded: %s", entry->name);
        }
        
        entry->status = TEXTURE_STATUS_UNLOADED;
    }
}

static void* streaming_thread_func(void *arg) {
    (void)arg;
    
    LOG_INFO("Texture streaming thread started");
    
    while (g_streaming_system.streaming_active) {
        pthread_mutex_lock(&g_streaming_system.stream_mutex);
        
        process_load_queue();
        process_unload_queue();
        
        pthread_mutex_unlock(&g_streaming_system.stream_mutex);
        
        // Sleep for a bit to avoid busy waiting
        usleep(16000); // ~60 FPS
    }
    
    LOG_INFO("Texture streaming thread stopped");
    return NULL;
}

// ============================================================================
// Texture Streaming API
// ============================================================================

bool texture_streaming_init(uint64_t memory_budget_mb) {
    if (g_streaming_system.initialized) {
        LOG_WARN("Texture streaming system already initialized");
        return true;
    }
    
    memset(&g_streaming_system, 0, sizeof(TextureStreamingSystem));
    
    g_streaming_system.texture_capacity = 4096;
    g_streaming_system.textures = calloc(g_streaming_system.texture_capacity, sizeof(TextureEntry));
    
    if (!g_streaming_system.textures) {
        LOG_ERROR("Failed to allocate texture entries");
        return false;
    }
    
    g_streaming_system.total_memory_budget = memory_budget_mb * 1024 * 1024;
    g_streaming_system.unload_distance_threshold = 100.0f;
    g_streaming_system.load_distance_threshold = 50.0f;
    g_streaming_system.max_concurrent_loads = 4;
    
    // Initialize threading
    if (pthread_mutex_init(&g_streaming_system.stream_mutex, NULL) != 0) {
        LOG_ERROR("Failed to initialize streaming mutex");
        free(g_streaming_system.textures);
        return false;
    }
    
    if (pthread_cond_init(&g_streaming_system.stream_condition, NULL) != 0) {
        LOG_ERROR("Failed to initialize streaming condition");
        pthread_mutex_destroy(&g_streaming_system.stream_mutex);
        free(g_streaming_system.textures);
        return false;
    }
    
    g_streaming_system.streaming_active = true;
    
    if (pthread_create(&g_streaming_system.stream_thread, NULL, streaming_thread_func, NULL) != 0) {
        LOG_ERROR("Failed to create streaming thread");
        pthread_mutex_destroy(&g_streaming_system.stream_mutex);
        pthread_cond_destroy(&g_streaming_system.stream_condition);
        free(g_streaming_system.textures);
        return false;
    }
    
    g_streaming_system.initialized = true;
    LOG_INFO("Texture streaming system initialized (budget: %.1f MB)", memory_budget_mb);
    return true;
}

void texture_streaming_shutdown(void) {
    if (!g_streaming_system.initialized)
        return;
    
    // Stop streaming thread
    g_streaming_system.streaming_active = false;
    pthread_join(g_streaming_system.stream_thread, NULL);
    
    // Unload all textures
    for (uint32_t i = 0; i < g_streaming_system.texture_count; i++) {
        TextureEntry *entry = &g_streaming_system.textures[i];
        if (entry->texture_handle) {
            destroy_gpu_texture(entry->texture_handle);
        }
        if (entry->temp_data) {
            free(entry->temp_data);
        }
    }
    
    // Cleanup
    pthread_mutex_destroy(&g_streaming_system.stream_mutex);
    pthread_cond_destroy(&g_streaming_system.stream_condition);
    free(g_streaming_system.textures);
    
    memset(&g_streaming_system, 0, sizeof(TextureStreamingSystem));
    
    LOG_INFO("Texture streaming system shutdown");
}

uint32_t texture_streaming_add_texture(const char *name, const char *file_path, 
                                      TexturePriority priority, bool persistent) {
    if (!g_streaming_system.initialized || !name || !file_path) {
        LOG_ERROR("Streaming system not initialized or invalid parameters");
        return 0;
    }
    
    if (g_streaming_system.texture_count >= g_streaming_system.texture_capacity) {
        LOG_ERROR("Too many textures in streaming system");
        return 0;
    }
    
    TextureEntry *entry = &g_streaming_system.textures[g_streaming_system.texture_count++];
    strncpy(entry->name, name, sizeof(entry->name) - 1);
    strncpy(entry->file_path, file_path, sizeof(entry->file_path) - 1);
    entry->priority = priority;
    entry->is_persistent = persistent;
    entry->status = TEXTURE_STATUS_UNLOADED;
    entry->last_access_time = 0.0f;
    entry->access_count = 0;
    entry->ref_count = 0;
    
    LOG_DEBUG("Added texture to streaming system: %s (%s)", name, file_path);
    return g_streaming_system.texture_count;
}

void texture_streaming_update_distance(uint32_t texture_id, float distance) {
    if (!g_streaming_system.initialized || texture_id == 0 || 
        texture_id > g_streaming_system.texture_count) {
        return;
    }
    
    pthread_mutex_lock(&g_streaming_system.stream_mutex);
    
    TextureEntry *entry = &g_streaming_system.textures[texture_id - 1];
    entry->distance_to_camera = distance;
    entry->last_access_time = (float)clock() / CLOCKS_PER_SEC;
    entry->access_count++;
    
    // Check if we need to load or unload this texture
    if (entry->status == TEXTURE_STATUS_UNLOADED && distance < g_streaming_system.load_distance_threshold) {
        add_to_load_queue(entry);
    } else if (entry->status == TEXTURE_STATUS_LOADED && distance > g_streaming_system.unload_distance_threshold) {
        add_to_unload_queue(entry);
    }
    
    pthread_mutex_unlock(&g_streaming_system.stream_mutex);
}

void* texture_streaming_get_texture(uint32_t texture_id) {
    if (!g_streaming_system.initialized || texture_id == 0 || 
        texture_id > g_streaming_system.texture_count) {
        return NULL;
    }
    
    TextureEntry *entry = &g_streaming_system.textures[texture_id - 1];
    
    if (entry->status == TEXTURE_STATUS_LOADED) {
        entry->ref_count++;
        return entry->texture_handle;
    }
    
    // Request load if not loaded
    if (entry->status == TEXTURE_STATUS_UNLOADED) {
        pthread_mutex_lock(&g_streaming_system.stream_mutex);
        add_to_load_queue(entry);
        pthread_mutex_unlock(&g_streaming_system.stream_mutex);
    }
    
    return NULL;
}

void texture_streaming_release_texture(uint32_t texture_id) {
    if (!g_streaming_system.initialized || texture_id == 0 || 
        texture_id > g_streaming_system.texture_count) {
        return;
    }
    
    TextureEntry *entry = &g_streaming_system.textures[texture_id - 1];
    if (entry->ref_count > 0) {
        entry->ref_count--;
    }
}

void texture_streaming_get_stats(uint64_t *used_memory, uint64_t *total_budget,
                               uint32_t *loaded_count, uint32_t *total_count) {
    if (!g_streaming_system.initialized) {
        if (used_memory) *used_memory = 0;
        if (total_budget) *total_budget = 0;
        if (loaded_count) *loaded_count = 0;
        if (total_count) *total_count = 0;
        return;
    }
    
    pthread_mutex_lock(&g_streaming_system.stream_mutex);
    
    if (used_memory) *used_memory = g_streaming_system.used_memory;
    if (total_budget) *total_budget = g_streaming_system.total_memory_budget;
    
    uint32_t loaded = 0;
    for (uint32_t i = 0; i < g_streaming_system.texture_count; i++) {
        if (g_streaming_system.textures[i].status == TEXTURE_STATUS_LOADED) {
            loaded++;
        }
    }
    
    if (loaded_count) *loaded_count = loaded;
    if (total_count) *total_count = g_streaming_system.texture_count;
    
    pthread_mutex_unlock(&g_streaming_system.stream_mutex);
}
