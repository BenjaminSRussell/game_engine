/*
 * Helper functions implementation for io_export_manager_01
 * This file contains all the supporting functions for the main manager
 */

#include "assets/io/export/manager_01.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>

// Resource pooling implementation
static int io_export_resource_pool_init(io_export_resource_pool_t* pool, size_t resource_size) {
    if (!pool || resource_size == 0) return -1;
    
    pool->resource_size = resource_size;
    pool->next_free = 0;
    pthread_mutex_init(&pool->pool_mutex, NULL);
    
    // Initialize all resources as unused
    for (uint32_t i = 0; i < IO_EXPORT_MANAGER_01_RESOURCE_POOL_SIZE; i++) {
        pool->resources[i] = NULL;
        pool->in_use[i] = false;
    }
    
    return 0;
}

static void* io_export_resource_pool_alloc(io_export_resource_pool_t* pool) {
    if (!pool) return NULL;
    
    pthread_mutex_lock(&pool->pool_mutex);
    
    // Find first free slot
    for (uint32_t i = 0; i < IO_EXPORT_MANAGER_01_RESOURCE_POOL_SIZE; i++) {
        uint32_t index = (pool->next_free + i) % IO_EXPORT_MANAGER_01_RESOURCE_POOL_SIZE;
        if (!pool->in_use[index]) {
            if (!pool->resources[index]) {
                pool->resources[index] = malloc(pool->resource_size);
                if (!pool->resources[index]) {
                    pthread_mutex_unlock(&pool->pool_mutex);
                    return NULL;
                }
            }
            pool->in_use[index] = true;
            pool->next_free = (index + 1) % IO_EXPORT_MANAGER_01_RESOURCE_POOL_SIZE;
            pthread_mutex_unlock(&pool->pool_mutex);
            return pool->resources[index];
        }
    }
    
    pthread_mutex_unlock(&pool->pool_mutex);
    return NULL; // Pool exhausted
}

static void io_export_resource_pool_free(io_export_resource_pool_t* pool, void* resource) {
    if (!pool || !resource) return;
    
    pthread_mutex_lock(&pool->pool_mutex);
    
    // Find the resource and mark as unused
    for (uint32_t i = 0; i < IO_EXPORT_MANAGER_01_RESOURCE_POOL_SIZE; i++) {
        if (pool->resources[i] == resource) {
            pool->in_use[i] = false;
            break;
        }
    }
    
    pthread_mutex_unlock(&pool->pool_mutex);
}

static void io_export_resource_pool_cleanup(io_export_resource_pool_t* pool) {
    if (!pool) return;
    
    pthread_mutex_lock(&pool->pool_mutex);
    
    // Free all allocated resources
    for (uint32_t i = 0; i < IO_EXPORT_MANAGER_01_RESOURCE_POOL_SIZE; i++) {
        if (pool->resources[i]) {
            free(pool->resources[i]);
            pool->resources[i] = NULL;
        }
    }
    
    pthread_mutex_unlock(&pool->pool_mutex);
    pthread_mutex_destroy(&pool->pool_mutex);
}

// Memory tracking implementation
static int io_export_memory_tracker_init(io_export_memory_tracker_t* tracker, size_t budget) {
    if (!tracker || budget == 0) return -1;
    
    tracker->total_budget = budget;
    tracker->current_usage = 0;
    tracker->peak_usage = 0;
    tracker->allocation_count = 0;
    tracker->eviction_enabled = true;
    tracker->eviction_threshold = 0.8f; // 80%
    
    return 0;
}

static void* io_export_memory_alloc_tracked(io_export_memory_tracker_t* tracker, size_t size) {
    if (!tracker || size == 0) return NULL;
    
    // Check if allocation would exceed budget
    if (tracker->current_usage + size > tracker->total_budget) {
        io_export_memory_evict_if_needed(tracker);
        
        // Still over budget after eviction?
        if (tracker->current_usage + size > tracker->total_budget) {
            return NULL;
        }
    }
    
    void* ptr = malloc(size);
    if (ptr) {
        tracker->current_usage += size;
        tracker->allocation_count++;
        if (tracker->current_usage > tracker->peak_usage) {
            tracker->peak_usage = tracker->current_usage;
        }
    }
    
    return ptr;
}

static void io_export_memory_free_tracked(io_export_memory_tracker_t* tracker, void* ptr, size_t size) {
    if (!tracker || !ptr) return;
    
    free(ptr);
    if (tracker->current_usage >= size) {
        tracker->current_usage -= size;
    }
}

static void io_export_memory_evict_if_needed(io_export_memory_tracker_t* tracker) {
    if (!tracker || !tracker->eviction_enabled) return;
    
    float usage_ratio = (float)tracker->current_usage / tracker->total_budget;
    if (usage_ratio > tracker->eviction_threshold) {
        // Implementation would evict least recently used assets
        // For now, just reduce current usage artificially
        tracker->current_usage = (size_t)(tracker->total_budget * tracker->eviction_threshold * 0.9f);
    }
}

// Hot-reload file watching implementation
static void* io_export_file_watcher_thread(void* arg) {
    io_export_file_watcher_t* watcher = (io_export_file_watcher_t*)arg;
    if (!watcher) return NULL;
    
    char buffer[4096];
    while (watcher->watcher_running) {
        int length = read(watcher->inotify_fd, buffer, sizeof(buffer));
        if (length > 0) {
            int offset = 0;
            while (offset < length) {
                struct inotify_event* event = (struct inotify_event*)(buffer + offset);
                
                for (uint32_t i = 0; i < watcher->watcher_count; i++) {
                    if (watcher->watch_descriptors[i] == event->wd) {
                        if (watcher->reload_callback) {
                            watcher->reload_callback(watcher->watched_paths[i]);
                        }
                        break;
                    }
                }
                
                offset += sizeof(struct inotify_event) + event->len;
            }
        }
        usleep(100000); // 100ms sleep
    }
    
    return NULL;
}

static int io_export_file_watcher_init(io_export_file_watcher_t* watcher) {
    if (!watcher) return -1;
    
    watcher->inotify_fd = inotify_init();
    if (watcher->inotify_fd == -1) return -1;
    
    watcher->watcher_count = 0;
    watcher->watcher_running = false;
    watcher->reload_callback = NULL;
    
    return 0;
}

static void io_export_file_watcher_cleanup(io_export_file_watcher_t* watcher) {
    if (!watcher) return;
    
    watcher->watcher_running = false;
    
    if (watcher->watcher_thread) {
        pthread_join(watcher->watcher_thread, NULL);
    }
    
    // Remove all watches
    for (uint32_t i = 0; i < watcher->watcher_count; i++) {
        if (watcher->watch_descriptors[i] != -1) {
            inotify_rm_watch(watcher->inotify_fd, watcher->watch_descriptors[i]);
        }
    }
    
    if (watcher->inotify_fd != -1) {
        close(watcher->inotify_fd);
    }
    
    watcher->watcher_count = 0;
}

static int io_export_file_watcher_add_path(io_export_file_watcher_t* watcher, const char* path) {
    if (!watcher || !path || watcher->watcher_count >= IO_EXPORT_MANAGER_01_MAX_FILE_WATCHERS) {
        return -1;
    }
    
    uint32_t wd = inotify_add_watch(watcher->inotify_fd, path, IN_MODIFY | IN_CREATE | IN_DELETE);
    if (wd == -1) return -1;
    
    strncpy(watcher->watched_paths[watcher->watcher_count], path, 255);
    watcher->watched_paths[watcher->watcher_count][255] = '\0';
    watcher->watch_descriptors[watcher->watcher_count] = wd;
    watcher->watcher_count++;
    
    // Start watcher thread if not already running
    if (!watcher->watcher_running) {
        watcher->watcher_running = true;
        pthread_create(&watcher->watcher_thread, NULL, io_export_file_watcher_thread, watcher);
    }
    
    return 0;
}

// Telemetry implementation
static void io_export_telemetry_record_operation(io_export_telemetry_t* telemetry, double time_ms, size_t memory_bytes) {
    if (!telemetry) return;
    
    telemetry->operation_count++;
    telemetry->total_time_ms += time_ms;
    telemetry->total_memory_transferred += memory_bytes;
    
    if (telemetry->operation_count == 1) {
        telemetry->min_time_ms = time_ms;
        telemetry->max_time_ms = time_ms;
    } else {
        if (time_ms < telemetry->min_time_ms) telemetry->min_time_ms = time_ms;
        if (time_ms > telemetry->max_time_ms) telemetry->max_time_ms = time_ms;
    }
}

static void io_export_telemetry_reset(io_export_telemetry_t* telemetry) {
    if (!telemetry) return;
    
    telemetry->operation_count = 0;
    telemetry->total_time_ms = 0.0;
    telemetry->min_time_ms = 0.0;
    telemetry->max_time_ms = 0.0;
    telemetry->memory_operations = 0;
    telemetry->total_memory_transferred = 0;
    telemetry->error_count = 0;
}

// Compression implementation (simplified)
static int io_export_compress_data(const void* input, size_t input_size, void** output, size_t* output_size) {
    if (!input || input_size == 0 || !output || !output_size) return -1;
    
    // Simple compression simulation - in reality would use LZ4/ZSTD
    *output_size = input_size + 1024; // Add space for compression header
    *output = malloc(*output_size);
    if (!*output) return -1;
    
    // Copy data (in reality would compress)
    memcpy(*output, input, input_size);
    
    return 0;
}

static int io_export_decompress_data(const void* input, size_t input_size, void** output, size_t* output_size) {
    if (!input || input_size == 0 || !output || !output_size) return -1;
    
    // Simple decompression simulation - in reality would use LZ4/ZSTD
    *output_size = input_size; // Assume decompressed size equals input for simulation
    *output = malloc(*output_size);
    if (!*output) return -1;
    
    // Copy data (in reality would decompress)
    memcpy(*output, input, input_size);
    
    return 0;
}

// Scene parsing implementation (simplified)
static io_export_scene_t* io_export_parse_gltf_scene(const char* file_path) {
    if (!file_path) return NULL;
    
    // Simplified scene creation - in reality would parse actual glTF file
    io_export_scene_t* scene = malloc(sizeof(io_export_scene_t));
    if (!scene) return NULL;
    
    memset(scene, 0, sizeof(io_export_scene_t));
    strncpy(scene->name, "gltf_scene", 63);
    scene->name[63] = '\0';
    
    return scene;
}

static io_export_scene_t* io_export_parse_fbx_scene(const char* file_path) {
    if (!file_path) return NULL;
    
    // Simplified scene creation - in reality would parse actual FBX file
    io_export_scene_t* scene = malloc(sizeof(io_export_scene_t));
    if (!scene) return NULL;
    
    memset(scene, 0, sizeof(io_export_scene_t));
    strncpy(scene->name, "fbx_scene", 63);
    scene->name[63] = '\0';
    
    return scene;
}

static void io_export_scene_free(io_export_scene_t* scene) {
    if (!scene) return;
    
    // Free nodes
    if (scene->nodes) {
        for (uint32_t i = 0; i < scene->node_count; i++) {
            if (scene->nodes[i]) {
                free(scene->nodes[i]);
            }
        }
        free(scene->nodes);
    }
    
    // Free meshes
    if (scene->meshes) {
        for (uint32_t i = 0; i < scene->mesh_count; i++) {
            if (scene->meshes[i]) {
                if (scene->meshes[i]->vertices) free(scene->meshes[i]->vertices);
                if (scene->meshes[i]->indices) free(scene->meshes[i]->indices);
                if (scene->meshes[i]->normals) free(scene->meshes[i]->normals);
                if (scene->meshes[i]->uvs) free(scene->meshes[i]->uvs);
                free(scene->meshes[i]);
            }
        }
        free(scene->meshes);
    }
    
    // Free materials
    if (scene->materials) {
        free(scene->materials);
    }
    
    free(scene);
}

// Serialization implementation
static int io_export_serialize_state(io_export_manager_01_t* ctx, void** buffer, size_t* buffer_size) {
    if (!ctx || !buffer || !buffer_size) return -1;
    
    // Calculate required buffer size
    *buffer_size = sizeof(io_export_serialization_header_t) + sizeof(io_export_manager_01_t);
    *buffer = malloc(*buffer_size);
    if (!*buffer) return -1;
    
    // Write header
    io_export_serialization_header_t* header = (io_export_serialization_header_t*)*buffer;
    header->magic = ctx->serialization_header.magic;
    header->version = ctx->serialization_header.version;
    header->data_size = sizeof(io_export_manager_01_t);
    header->timestamp = time(NULL);
    
    // Write data (simplified - would serialize actual state)
    void* data = (char*)*buffer + sizeof(io_export_serialization_header_t);
    memcpy(data, ctx, sizeof(io_export_manager_01_t));
    
    // Calculate checksum (simplified)
    header->checksum = 0; // Would calculate actual checksum
    
    return 0;
}

static int io_export_deserialize_state(io_export_manager_01_t* ctx, const void* buffer, size_t buffer_size) {
    if (!ctx || !buffer || buffer_size < sizeof(io_export_serialization_header_t)) return -1;
    
    // Read header
    const io_export_serialization_header_t* header = (const io_export_serialization_header_t*)buffer;
    if (header->magic != ctx->serialization_header.magic) return -1;
    
    // Read data (simplified)
    const void* data = (const char*)buffer + sizeof(io_export_serialization_header_t);
    if (header->data_size > buffer_size - sizeof(io_export_serialization_header_t)) return -1;
    
    memcpy(ctx, data, sizeof(io_export_manager_01_t));
    
    return 0;
}

// Forward declarations from main file
extern uint32_t s_next_async_op_id;

// Async file loading thread function
static void* io_export_async_load_thread(void* arg) {
    io_export_async_operation_t* op = (io_export_async_operation_t*)arg;
    if (!op || !op->file_path) {
        op->has_error = true;
        op->is_complete = true;
        return NULL;
    }
    
    // Simulate file loading
    FILE* file = fopen(op->file_path, "rb");
    if (!file) {
        op->has_error = true;
        op->is_complete = true;
        return NULL;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    op->buffer_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Allocate buffer and read file
    op->buffer = malloc(op->buffer_size);
    if (op->buffer) {
        op->bytes_read = fread(op->buffer, 1, op->buffer_size, file);
    }
    
    fclose(file);
    op->is_complete = true;
    
    // Call callback if provided
    if (op->callback) {
        op->callback(op->buffer, op->bytes_read, op->user_data);
    }
    
    return NULL;
}

// Multi-threaded batch processing implementation
static int io_export_process_batch_parallel(io_export_manager_01_t* ctx, void** items, uint32_t item_count,
                                           void (*process_func)(void*)) {
    if (!ctx || !items || !process_func || item_count == 0) {
        return IO_EXPORT_ERROR_INVALID_PARAM;
    }
    
    // Simple parallel processing - in reality would use thread pool
    for (uint32_t i = 0; i < item_count; i++) {
        process_func(items[i]);
    }
    
    return IO_EXPORT_ERROR_NONE;
}

// Format conversion implementation
static int io_export_register_format_converter(io_export_manager_01_t* ctx, 
                                              const char* source_format, const char* target_format,
                                              int (*convert_func)(const void*, void**, size_t*)) {
    if (!ctx || !source_format || !target_format || !convert_func) {
        return IO_EXPORT_ERROR_INVALID_PARAM;
    }
    
    if (ctx->converter_count >= 16) {
        return IO_EXPORT_ERROR_INVALID_PARAM; // Converter array full
    }
    
    // Add converter to registry
    strncpy(ctx->format_converters[ctx->converter_count].source_format, source_format, 15);
    ctx->format_converters[ctx->converter_count].source_format[15] = '\0';
    strncpy(ctx->format_converters[ctx->converter_count].target_format, target_format, 15);
    ctx->format_converters[ctx->converter_count].target_format[15] = '\0';
    ctx->format_converters[ctx->converter_count].convert_func = convert_func;
    ctx->converter_count++;
    
    return IO_EXPORT_ERROR_NONE;
}

static int io_export_convert_format(io_export_manager_01_t* ctx, const char* source_format,
                                   const char* target_format, const void* input, void** output, size_t* output_size) {
    if (!ctx || !source_format || !target_format || !input || !output || !output_size) {
        return IO_EXPORT_ERROR_INVALID_PARAM;
    }
    
    // Find matching converter
    for (uint32_t i = 0; i < ctx->converter_count; i++) {
        if (strcmp(ctx->format_converters[i].source_format, source_format) == 0 &&
            strcmp(ctx->format_converters[i].target_format, target_format) == 0) {
            return ctx->format_converters[i].convert_func(input, output, output_size);
        }
    }
    
    return IO_EXPORT_ERROR_INVALID_PARAM; // No converter found
}

static uint32_t io_export_async_load_file(io_export_manager_01_t* ctx, const char* file_path, 
                                        void (*callback)(void*, size_t, void*), void* user_data) {
    if (!ctx || !file_path) {
        return 0;
    }
    
    pthread_mutex_lock(&ctx->async_mutex);
    
    if (ctx->async_op_count >= IO_EXPORT_MANAGER_01_MAX_ASYNC_OPERATIONS) {
        pthread_mutex_unlock(&ctx->async_mutex);
        return 0; // Queue full
    }
    
    // Find free slot
    uint32_t slot = 0;
    for (; slot < IO_EXPORT_MANAGER_01_MAX_ASYNC_OPERATIONS; slot++) {
        if (!ctx->async_ops[slot].is_complete && !ctx->async_ops[slot].thread_id) {
            break;
        }
    }
    
    if (slot >= IO_EXPORT_MANAGER_01_MAX_ASYNC_OPERATIONS) {
        pthread_mutex_unlock(&ctx->async_mutex);
        return 0; // No free slots
    }
    
    // Initialize operation
    ctx->async_ops[slot].id = s_next_async_op_id++;
    strncpy(ctx->async_ops[slot].file_path, file_path, 255);
    ctx->async_ops[slot].file_path[255] = '\0';
    ctx->async_ops[slot].buffer = NULL;
    ctx->async_ops[slot].buffer_size = 0;
    ctx->async_ops[slot].bytes_read = 0;
    ctx->async_ops[slot].is_complete = false;
    ctx->async_ops[slot].has_error = false;
    ctx->async_ops[slot].callback = callback;
    ctx->async_ops[slot].user_data = user_data;
    
    // Start thread
    pthread_create(&ctx->async_ops[slot].thread_id, NULL, io_export_async_load_thread, &ctx->async_ops[slot]);
    
    ctx->async_op_count++;
    pthread_mutex_unlock(&ctx->async_mutex);
    
    return ctx->async_ops[slot].id;
}

static int io_export_async_wait_completion(io_export_manager_01_t* ctx, uint32_t operation_id, uint32_t timeout_ms) {
    if (!ctx) {
        return IO_EXPORT_ERROR_INVALID_PARAM;
    }
    
    // Find operation
    pthread_mutex_lock(&ctx->async_mutex);
    io_export_async_operation_t* op = NULL;
    for (uint32_t i = 0; i < ctx->async_op_count; i++) {
        if (ctx->async_ops[i].id == operation_id) {
            op = &ctx->async_ops[i];
            break;
        }
    }
    
    if (!op) {
        pthread_mutex_unlock(&ctx->async_mutex);
        return IO_EXPORT_ERROR_INVALID_PARAM; // Operation not found
    }
    
    pthread_mutex_unlock(&ctx->async_mutex);
    
    // Wait for completion with timeout
    uint32_t elapsed = 0;
    while (!op->is_complete && elapsed < timeout_ms) {
        usleep(1000); // 1ms sleep
        elapsed++;
    }
    
    return op->is_complete ? IO_EXPORT_ERROR_NONE : IO_EXPORT_ERROR_ASYNC_BUSY;
}
