/*
 * morph_data_infrastructure.c
 * Morph target infrastructure (serialization, hot reload, GPU, etc.)
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
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
#include <sys/time.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <errno.h>
#include <fcntl.h>

/* External context reference */
extern animation_morph_context_t g_morph_ctx;

/* ============================================================================
 * SERIALIZATION
 * ============================================================================ */

typedef struct morph_data_header {
    uint32_t magic_number;
    uint32_t version;
    uint32_t vertex_count;
    uint32_t target_count;
    uint32_t bone_count;
    uint32_t flags;
    uint64_t checksum;
} morph_data_header_t;

static uint64_t calculate_checksum(const void* data, size_t size) {
    const uint8_t* bytes = (const uint8_t*)data;
    uint64_t checksum = 0;
    
    for (size_t i = 0; i < size; i++) {
        checksum = checksum * 31 + bytes[i];
    }
    
    return checksum;
}

int animation_morph_data_serialize(animation_morph_data_handle_t handle, const char* filename) {
    if (!filename) {
        return ANIMATION_MORPH_ERROR_INVALID_PARAMETER;
    }
    
    if (!g_morph_ctx.initialized || handle.id >= g_morph_ctx.count) {
        return ANIMATION_MORPH_ERROR_INVALID_HANDLE;
    }
    
    animation_morph_internal_t* item = &g_morph_ctx.items[handle.id];
    if (!item->initialized) {
        return ANIMATION_MORPH_ERROR_NOT_INITIALIZED;
    }
    
    pthread_mutex_lock(&item->mutex);
    
    FILE* file = fopen(filename, "wb");
    if (!file) {
        pthread_mutex_unlock(&item->mutex);
        return ANIMATION_MORPH_ERROR_SERIALIZATION_FAILED;
    }
    
    // Calculate total data size for checksum
    size_t total_size = sizeof(morph_data_header_t);
    total_size += item->vertex_count * sizeof(animation_morph_vertex_t);
    
    for (uint32_t i = 0; i < item->target_count; i++) {
        total_size += sizeof(uint32_t) + strlen(item->targets[i].name) + 1; // name
        total_size += sizeof(uint32_t); // vertex_count
        total_size += item->targets[i].vertex_count * sizeof(animation_morph_vertex_t);
        total_size += sizeof(uint32_t); // flags
        total_size += sizeof(float); // influence
        
        if (item->targets[i].weights) {
            total_size += item->targets[i].vertex_count * sizeof(float);
        }
    }
    
    // Allocate buffer for checksum calculation
    uint8_t* buffer = malloc(total_size);
    if (!buffer) {
        fclose(file);
        pthread_mutex_unlock(&item->mutex);
        return ANIMATION_MORPH_ERROR_OUT_OF_MEMORY;
    }
    
    uint8_t* ptr = buffer;
    
    // Write header (without checksum first)
    morph_data_header_t header = {0};
    header.magic_number = ANIMATION_MORPH_DATA_MAGIC_NUMBER;
    header.version = ANIMATION_MORPH_DATA_VERSION;
    header.vertex_count = item->vertex_count;
    header.target_count = item->target_count;
    header.bone_count = item->bone_count;
    header.flags = item->flags;
    
    memcpy(ptr, &header, sizeof(morph_data_header_t));
    ptr += sizeof(morph_data_header_t);
    
    // Write base vertices
    memcpy(ptr, item->base_vertices, item->vertex_count * sizeof(animation_morph_vertex_t));
    ptr += item->vertex_count * sizeof(animation_morph_vertex_t);
    
    // Write targets
    for (uint32_t i = 0; i < item->target_count; i++) {
        const animation_morph_target_t* target = &item->targets[i];
        
        // Write name
        uint32_t name_len = strlen(target->name) + 1;
        memcpy(ptr, &name_len, sizeof(uint32_t));
        ptr += sizeof(uint32_t);
        memcpy(ptr, target->name, name_len);
        ptr += name_len;
        
        // Write vertex count
        memcpy(ptr, &target->vertex_count, sizeof(uint32_t));
        ptr += sizeof(uint32_t);
        
        // Write vertices
        memcpy(ptr, target->vertices, target->vertex_count * sizeof(animation_morph_vertex_t));
        ptr += target->vertex_count * sizeof(animation_morph_vertex_t);
        
        // Write flags
        memcpy(ptr, &target->flags, sizeof(uint32_t));
        ptr += sizeof(uint32_t);
        
        // Write influence
        memcpy(ptr, &target->influence, sizeof(float));
        ptr += sizeof(float);
        
        // Write weights if present
        if (target->weights) {
            memcpy(ptr, target->weights, target->vertex_count * sizeof(float));
            ptr += target->vertex_count * sizeof(float);
        }
    }
    
    // Calculate checksum
    header.checksum = calculate_checksum(buffer, total_size);
    
    // Write header with checksum
    fseek(file, 0, SEEK_SET);
    if (fwrite(&header, sizeof(morph_data_header_t), 1, file) != 1) {
        free(buffer);
        fclose(file);
        pthread_mutex_unlock(&item->mutex);
        return ANIMATION_MORPH_ERROR_SERIALIZATION_FAILED;
    }
    
    // Write data
    if (fwrite(buffer + sizeof(morph_data_header_t), total_size - sizeof(morph_data_header_t), 1, file) != 1) {
        free(buffer);
        fclose(file);
        pthread_mutex_unlock(&item->mutex);
        return ANIMATION_MORPH_ERROR_SERIALIZATION_FAILED;
    }
    
    free(buffer);
    fclose(file);
    
    pthread_mutex_unlock(&item->mutex);
    return ANIMATION_MORPH_ERROR_NONE;
}

int animation_morph_data_deserialize(animation_morph_data_handle_t* out_handle, const char* filename) {
    if (!filename || !out_handle) {
        return ANIMATION_MORPH_ERROR_INVALID_PARAMETER;
    }
    
    if (!g_morph_ctx.initialized) {
        return ANIMATION_MORPH_ERROR_NOT_INITIALIZED;
    }
    
    FILE* file = fopen(filename, "rb");
    if (!file) {
        return ANIMATION_MORPH_ERROR_SERIALIZATION_FAILED;
    }
    
    // Read header
    morph_data_header_t header;
    if (fread(&header, sizeof(morph_data_header_t), 1, file) != 1) {
        fclose(file);
        return ANIMATION_MORPH_ERROR_SERIALIZATION_FAILED;
    }
    
    // Validate header
    if (header.magic_number != ANIMATION_MORPH_DATA_MAGIC_NUMBER || 
        header.version != ANIMATION_MORPH_DATA_VERSION) {
        fclose(file);
        return ANIMATION_MORPH_ERROR_SERIALIZATION_FAILED;
    }
    
    // Create morph data
    animation_morph_data_desc_t desc = {0};
    desc.vertex_count = header.vertex_count;
    desc.target_count = header.target_count;
    desc.bone_count = header.bone_count;
    desc.flags = header.flags;
    
    int result = animation_morph_data_create(out_handle, &desc);
    if (result != ANIMATION_MORPH_ERROR_NONE) {
        fclose(file);
        return result;
    }
    
    animation_morph_internal_t* item = &g_morph_ctx.items[out_handle->id];
    
    pthread_mutex_lock(&item->mutex);
    
    // Read base vertices
    if (fread(item->base_vertices, sizeof(animation_morph_vertex_t), item->vertex_count, file) != item->vertex_count) {
        pthread_mutex_unlock(&item->mutex);
        fclose(file);
        return ANIMATION_MORPH_ERROR_SERIALIZATION_FAILED;
    }
    
    // Read targets
    for (uint32_t i = 0; i < item->target_count; i++) {
        animation_morph_target_t* target = &item->targets[i];
        
        // Read name
        uint32_t name_len;
        if (fread(&name_len, sizeof(uint32_t), 1, file) != 1) {
            pthread_mutex_unlock(&item->mutex);
            fclose(file);
            return ANIMATION_MORPH_ERROR_SERIALIZATION_FAILED;
        }
        
        if (fread(target->name, 1, name_len, file) != name_len) {
            pthread_mutex_unlock(&item->mutex);
            fclose(file);
            return ANIMATION_MORPH_ERROR_SERIALIZATION_FAILED;
        }
        target->name[name_len - 1] = '\0'; // Ensure null termination
        
        // Read vertex count
        if (fread(&target->vertex_count, sizeof(uint32_t), 1, file) != 1) {
            pthread_mutex_unlock(&item->mutex);
            fclose(file);
            return ANIMATION_MORPH_ERROR_SERIALIZATION_FAILED;
        }
        
        // Allocate and read vertices
        target->vertices = malloc(target->vertex_count * sizeof(animation_morph_vertex_t));
        if (!target->vertices) {
            pthread_mutex_unlock(&item->mutex);
            fclose(file);
            return ANIMATION_MORPH_ERROR_OUT_OF_MEMORY;
        }
        
        if (fread(target->vertices, sizeof(animation_morph_vertex_t), target->vertex_count, file) != target->vertex_count) {
            pthread_mutex_unlock(&item->mutex);
            fclose(file);
            return ANIMATION_MORPH_ERROR_SERIALIZATION_FAILED;
        }
        
        // Read flags
        if (fread(&target->flags, sizeof(uint32_t), 1, file) != 1) {
            pthread_mutex_unlock(&item->mutex);
            fclose(file);
            return ANIMATION_MORPH_ERROR_SERIALIZATION_FAILED;
        }
        
        // Read influence
        if (fread(&target->influence, sizeof(float), 1, file) != 1) {
            pthread_mutex_unlock(&item->mutex);
            fclose(file);
            return ANIMATION_MORPH_ERROR_SERIALIZATION_FAILED;
        }
        
        // Read weights if present (check if we have more data)
        long current_pos = ftell(file);
        fseek(file, 0, SEEK_END);
        long end_pos = ftell(file);
        fseek(file, current_pos, SEEK_SET);
        
        if (end_pos - current_pos >= (long)(target->vertex_count * sizeof(float))) {
            target->weights = malloc(target->vertex_count * sizeof(float));
            if (!target->weights) {
                pthread_mutex_unlock(&item->mutex);
                fclose(file);
                return ANIMATION_MORPH_ERROR_OUT_OF_MEMORY;
            }
            
            if (fread(target->weights, sizeof(float), target->vertex_count, file) != target->vertex_count) {
                pthread_mutex_unlock(&item->mutex);
                fclose(file);
                return ANIMATION_MORPH_ERROR_SERIALIZATION_FAILED;
            }
        }
    }
    
    item->dirty = true;
    item->frame_updated = get_current_time_ms();
    
    pthread_mutex_unlock(&item->mutex);
    fclose(file);
    
    return ANIMATION_MORPH_ERROR_NONE;
}

/* ============================================================================
 * PERFORMANCE COUNTERS
 * ============================================================================ */

int animation_morph_data_get_stats(animation_morph_data_handle_t handle, animation_morph_stats_t* out_stats) {
    if (!out_stats) {
        return ANIMATION_MORPH_ERROR_INVALID_PARAMETER;
    }
    
    if (!g_morph_ctx.initialized || handle.id >= g_morph_ctx.count) {
        return ANIMATION_MORPH_ERROR_INVALID_HANDLE;
    }
    
    animation_morph_internal_t* item = &g_morph_ctx.items[handle.id];
    if (!item->initialized) {
        return ANIMATION_MORPH_ERROR_NOT_INITIALIZED;
    }
    
    pthread_mutex_lock(&item->mutex);
    *out_stats = item->stats;
    pthread_mutex_unlock(&item->mutex);
    
    return ANIMATION_MORPH_ERROR_NONE;
}

void animation_morph_data_reset_stats(animation_morph_data_handle_t handle) {
    if (!g_morph_ctx.initialized || handle.id >= g_morph_ctx.count) {
        return;
    }
    
    animation_morph_internal_t* item = &g_morph_ctx.items[handle.id];
    if (!item->initialized) {
        return;
    }
    
    pthread_mutex_lock(&item->mutex);
    
    memset(&item->stats, 0, sizeof(animation_morph_stats_t));
    item->stats.total_vertices = item->vertex_count;
    item->stats.total_targets = item->target_count;
    item->stats.total_bones = item->bone_count;
    
    pthread_mutex_unlock(&item->mutex);
}

/* ============================================================================
 * HOT RELOAD
 * ============================================================================ */

static void* hot_reload_thread(void* arg) {
    animation_morph_data_handle_t handle = *(animation_morph_data_handle_t*)arg;
    free(arg);
    
    if (!g_morph_ctx.initialized || handle.id >= g_morph_ctx.count) {
        return NULL;
    }
    
    animation_morph_internal_t* item = &g_morph_ctx.items[handle.id];
    
    char buffer[4096];
    while (item->hot_reload_fd >= 0) {
        int length = read(item->hot_reload_fd, buffer, sizeof(buffer));
        if (length > 0) {
            struct inotify_event* event = (struct inotify_event*)buffer;
            
            if (event->mask & IN_MODIFY) {
                pthread_mutex_lock(&item->mutex);
                item->hot_reload_pending = true;
                item->dirty = true;
                item->frame_updated = get_current_time_ms();
                pthread_mutex_unlock(&item->mutex);
            }
        }
        
        usleep(100000); // 100ms sleep
    }
    
    return NULL;
}

int animation_morph_data_enable_hot_reload(animation_morph_data_handle_t handle, const char* filename) {
    if (!filename) {
        return ANIMATION_MORPH_ERROR_INVALID_PARAMETER;
    }
    
    if (!g_morph_ctx.initialized || handle.id >= g_morph_ctx.count) {
        return ANIMATION_MORPH_ERROR_INVALID_HANDLE;
    }
    
    animation_morph_internal_t* item = &g_morph_ctx.items[handle.id];
    if (!item->initialized) {
        return ANIMATION_MORPH_ERROR_NOT_INITIALIZED;
    }
    
    pthread_mutex_lock(&item->mutex);
    
    // Store filename
    strncpy(item->hot_reload_filename, filename, sizeof(item->hot_reload_filename) - 1);
    item->hot_reload_filename[sizeof(item->hot_reload_filename) - 1] = '\0';
    
    // Initialize inotify
    item->hot_reload_fd = inotify_init();
    if (item->hot_reload_fd < 0) {
        pthread_mutex_unlock(&item->mutex);
        return ANIMATION_MORPH_ERROR_THREAD_ERROR;
    }
    
    // Add watch
    item->hot_reload_wd = inotify_add_watch(item->hot_reload_fd, filename, IN_MODIFY);
    if (item->hot_reload_wd < 0) {
        close(item->hot_reload_fd);
        item->hot_reload_fd = -1;
        pthread_mutex_unlock(&item->mutex);
        return ANIMATION_MORPH_ERROR_THREAD_ERROR;
    }
    
    // Start hot reload thread
    animation_morph_data_handle_t* handle_copy = malloc(sizeof(animation_morph_data_handle_t));
    if (!handle_copy) {
        inotify_rm_watch(item->hot_reload_fd, item->hot_reload_wd);
        close(item->hot_reload_fd);
        item->hot_reload_fd = -1;
        pthread_mutex_unlock(&item->mutex);
        return ANIMATION_MORPH_ERROR_OUT_OF_MEMORY;
    }
    
    *handle_copy = handle;
    
    pthread_t thread;
    if (pthread_create(&thread, NULL, hot_reload_thread, handle_copy) != 0) {
        free(handle_copy);
        inotify_rm_watch(item->hot_reload_fd, item->hot_reload_wd);
        close(item->hot_reload_fd);
        item->hot_reload_fd = -1;
        pthread_mutex_unlock(&item->mutex);
        return ANIMATION_MORPH_ERROR_THREAD_ERROR;
    }
    
    pthread_detach(thread);
    
    item->hot_reload_pending = false;
    
    pthread_mutex_unlock(&item->mutex);
    return ANIMATION_MORPH_ERROR_NONE;
}

void animation_morph_data_disable_hot_reload(animation_morph_data_handle_t handle) {
    if (!g_morph_ctx.initialized || handle.id >= g_morph_ctx.count) {
        return;
    }
    
    animation_morph_internal_t* item = &g_morph_ctx.items[handle.id];
    if (!item->initialized) {
        return;
    }
    
    pthread_mutex_lock(&item->mutex);
    
    if (item->hot_reload_fd >= 0) {
        inotify_rm_watch(item->hot_reload_fd, item->hot_reload_wd);
        close(item->hot_reload_fd);
        item->hot_reload_fd = -1;
    }
    
    item->hot_reload_pending = false;
    
    pthread_mutex_unlock(&item->mutex);
}

bool animation_morph_data_has_pending_reload(animation_morph_data_handle_t handle) {
    if (!g_morph_ctx.initialized || handle.id >= g_morph_ctx.count) {
        return false;
    }
    
    animation_morph_internal_t* item = &g_morph_ctx.items[handle.id];
    if (!item->initialized) {
        return false;
    }
    
    pthread_mutex_lock(&item->mutex);
    bool pending = item->hot_reload_pending;
    pthread_mutex_unlock(&item->mutex);
    
    return pending;
}

/* ============================================================================
 * THREAD SAFETY
 * ============================================================================ */

int animation_morph_data_lock(animation_morph_data_handle_t handle) {
    if (!g_morph_ctx.initialized || handle.id >= g_morph_ctx.count) {
        return ANIMATION_MORPH_ERROR_INVALID_HANDLE;
    }
    
    animation_morph_internal_t* item = &g_morph_ctx.items[handle.id];
    if (!item->initialized) {
        return ANIMATION_MORPH_ERROR_NOT_INITIALIZED;
    }
    
    if (pthread_mutex_lock(&item->mutex) != 0) {
        return ANIMATION_MORPH_ERROR_THREAD_ERROR;
    }
    
    item->locked = true;
    return ANIMATION_MORPH_ERROR_NONE;
}

int animation_morph_data_unlock(animation_morph_data_handle_t handle) {
    if (!g_morph_ctx.initialized || handle.id >= g_morph_ctx.count) {
        return ANIMATION_MORPH_ERROR_INVALID_HANDLE;
    }
    
    animation_morph_internal_t* item = &g_morph_ctx.items[handle.id];
    if (!item->initialized) {
        return ANIMATION_MORPH_ERROR_NOT_INITIALIZED;
    }
    
    if (pthread_mutex_unlock(&item->mutex) != 0) {
        return ANIMATION_MORPH_ERROR_THREAD_ERROR;
    }
    
    item->locked = false;
    return ANIMATION_MORPH_ERROR_NONE;
}

int animation_morph_data_try_lock(animation_morph_data_handle_t handle) {
    if (!g_morph_ctx.initialized || handle.id >= g_morph_ctx.count) {
        return ANIMATION_MORPH_ERROR_INVALID_HANDLE;
    }
    
    animation_morph_internal_t* item = &g_morph_ctx.items[handle.id];
    if (!item->initialized) {
        return ANIMATION_MORPH_ERROR_NOT_INITIALIZED;
    }
    
    int result = pthread_mutex_trylock(&item->mutex);
    if (result == 0) {
        item->locked = true;
        return ANIMATION_MORPH_ERROR_NONE;
    } else if (result == EBUSY) {
        return -1; // Lock is busy
    } else {
        return ANIMATION_MORPH_ERROR_THREAD_ERROR;
    }
}

/* ============================================================================
 * MEMORY POOLING
 * ============================================================================ */

int animation_morph_data_init_memory_pool(size_t pool_size) {
    if (!g_morph_ctx.initialized) {
        return ANIMATION_MORPH_ERROR_NOT_INITIALIZED;
    }
    
    return memory_pool_init(&g_morph_ctx.memory_pool, pool_size);
}

void animation_morph_data_cleanup_memory_pool(void) {
    if (!g_morph_ctx.initialized) {
        return;
    }
    
    memory_pool_cleanup(&g_morph_ctx.memory_pool);
}

void* animation_morph_data_pool_alloc(size_t size) {
    if (!g_morph_ctx.initialized) {
        return NULL;
    }
    
    return memory_pool_alloc(&g_morph_ctx.memory_pool, size);
}

void animation_morph_data_pool_free(void* ptr) {
    if (!g_morph_ctx.initialized) {
        return;
    }
    
    memory_pool_free(&g_morph_ctx.memory_pool, ptr);
}

/* ============================================================================
 * CACHING LAYER
 * ============================================================================ */

int animation_morph_data_init_cache(uint32_t max_entries) {
    if (!g_morph_ctx.initialized) {
        return ANIMATION_MORPH_ERROR_NOT_INITIALIZED;
    }
    
    // Cleanup existing cache
    if (g_morph_ctx.cache) {
        cache_cleanup(g_morph_ctx.cache, g_morph_ctx.cache_max_entries);
    }
    
    int result = cache_init(&g_morph_ctx.cache, max_entries);
    if (result == ANIMATION_MORPH_ERROR_NONE) {
        g_morph_ctx.cache_max_entries = max_entries;
    }
    
    return result;
}

void animation_morph_data_cleanup_cache(void) {
    if (!g_morph_ctx.initialized) {
        return;
    }
    
    if (g_morph_ctx.cache) {
        cache_cleanup(g_morph_ctx.cache, g_morph_ctx.cache_max_entries);
        g_morph_ctx.cache = NULL;
    }
}

int animation_morph_data_cache_get(animation_morph_data_handle_t handle, const char* key, void** out_data) {
    if (!key || !out_data) {
        return ANIMATION_MORPH_ERROR_INVALID_PARAMETER;
    }
    
    if (!g_morph_ctx.initialized || handle.id >= g_morph_ctx.count) {
        return ANIMATION_MORPH_ERROR_INVALID_HANDLE;
    }
    
    if (!g_morph_ctx.cache) {
        return ANIMATION_MORPH_ERROR_NOT_INITIALIZED;
    }
    
    int result = cache_get(g_morph_ctx.cache, g_morph_ctx.cache_max_entries, key, out_data);
    
    // Update statistics
    if (result == ANIMATION_MORPH_ERROR_NONE) {
        animation_morph_internal_t* item = &g_morph_ctx.items[handle.id];
        pthread_mutex_lock(&item->mutex);
        item->stats.cache_hits++;
        pthread_mutex_unlock(&item->mutex);
    } else {
        animation_morph_internal_t* item = &g_morph_ctx.items[handle.id];
        pthread_mutex_lock(&item->mutex);
        item->stats.cache_misses++;
        pthread_mutex_unlock(&item->mutex);
    }
    
    return result;
}

int animation_morph_data_cache_set(animation_morph_data_handle_t handle, const char* key, const void* data, size_t size) {
    if (!key || !data) {
        return ANIMATION_MORPH_ERROR_INVALID_PARAMETER;
    }
    
    if (!g_morph_ctx.initialized || handle.id >= g_morph_ctx.count) {
        return ANIMATION_MORPH_ERROR_INVALID_HANDLE;
    }
    
    if (!g_morph_ctx.cache) {
        return ANIMATION_MORPH_ERROR_NOT_INITIALIZED;
    }
    
    return cache_set(g_morph_ctx.cache, g_morph_ctx.cache_max_entries, key, data, size);
}

/* ============================================================================
 * ASYNC OPERATIONS
 * ============================================================================ */

int animation_morph_data_async_update(animation_morph_data_handle_t handle, const void* data, size_t size, 
                                      animation_morph_async_callback_t callback, void* user_data) {
    if (!data) {
        return ANIMATION_MORPH_ERROR_INVALID_PARAMETER;
    }
    
    if (!g_morph_ctx.initialized || handle.id >= g_morph_ctx.count) {
        return ANIMATION_MORPH_ERROR_INVALID_HANDLE;
    }
    
    animation_morph_internal_t* item = &g_morph_ctx.items[handle.id];
    if (!item->initialized) {
        return ANIMATION_MORPH_ERROR_NOT_INITIALIZED;
    }
    
    // Queue async operation
    int result = async_queue_operation(handle, data, size, callback, user_data);
    
    if (result == ANIMATION_MORPH_ERROR_NONE) {
        pthread_mutex_lock(&item->mutex);
        item->stats.async_operations_pending++;
        pthread_mutex_unlock(&item->mutex);
    }
    
    return result;
}

int animation_morph_data_process_async_queue(void) {
    if (!g_morph_ctx.initialized) {
        return ANIMATION_MORPH_ERROR_NOT_INITIALIZED;
    }
    
    // This is handled by the async worker thread
    // Just return the number of pending operations
    pthread_mutex_lock(&g_morph_ctx.async_mutex);
    uint32_t pending = (g_morph_ctx.async_queue_tail - g_morph_ctx.async_queue_head) % ANIMATION_MORPH_ASYNC_QUEUE_SIZE;
    pthread_mutex_unlock(&g_morph_ctx.async_mutex);
    
    return (int)pending;
}

/* End of morph_data_infrastructure.c */
