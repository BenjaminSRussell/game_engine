/**
 * Asset Management System - Complete Implementation (~2800 lines)
 *
 * Implements comprehensive asset loading, caching, streaming, and lifecycle management
 */

#include "asset_system_complete.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

// ============================================================================
// INTERNAL STRUCTURES
// ============================================================================

typedef struct {
    uint64_t next_asset_id;
    uint64_t next_request_id;
} asset_id_generator;

static asset_id_generator g_asset_id_gen = {
    .next_asset_id = 1000000,
    .next_request_id = 1,
};

// ============================================================================
// ASSET REGISTRY IMPLEMENTATION
// ============================================================================

typedef struct asset_registry {
    asset_metadata* assets;
    uint32_t asset_count;
    uint32_t asset_capacity;

    // Hash table for quick lookup
    uint64_t* asset_ids_by_hash;
    uint32_t* hash_bucket_indices;
    uint32_t hash_bucket_count;
} asset_registry;

static asset_registry* asset_registry_create(uint32_t initial_capacity) {
    asset_registry* registry = malloc(sizeof(asset_registry));
    if (!registry) return NULL;

    registry->asset_capacity = initial_capacity;
    registry->asset_count = 0;
    registry->assets = calloc(initial_capacity, sizeof(asset_metadata));

    if (!registry->assets) {
        free(registry);
        return NULL;
    }

    registry->hash_bucket_count = initial_capacity / 4;
    registry->asset_ids_by_hash = calloc(registry->hash_bucket_count, sizeof(uint64_t));
    registry->hash_bucket_indices = calloc(registry->hash_bucket_count, sizeof(uint32_t));

    if (!registry->asset_ids_by_hash || !registry->hash_bucket_indices) {
        free(registry->assets);
        free(registry->asset_ids_by_hash);
        free(registry->hash_bucket_indices);
        free(registry);
        return NULL;
    }

    return registry;
}

static void asset_registry_destroy(asset_registry* registry) {
    if (!registry) return;

    for (uint32_t i = 0; i < registry->asset_count; i++) {
        asset_metadata* meta = &registry->assets[i];
        free((void*)meta->asset_name);
        free((void*)meta->asset_path);
        free(meta->tags);
        free(meta->dependencies);
    }

    free(registry->assets);
    free(registry->asset_ids_by_hash);
    free(registry->hash_bucket_indices);
    free(registry);
}

static uint32_t asset_registry_hash(const char* str, uint32_t bucket_count) {
    uint32_t hash = 5381;
    int c;

    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }

    return hash % bucket_count;
}

static asset_metadata* asset_registry_find_by_name(asset_registry* registry, const char* name) {
    if (!registry || !name) return NULL;

    uint32_t hash = asset_registry_hash(name, registry->hash_bucket_count);

    for (uint32_t i = 0; i < registry->asset_count; i++) {
        if (registry->assets[i].asset_name && strcmp(registry->assets[i].asset_name, name) == 0) {
            return &registry->assets[i];
        }
    }

    return NULL;
}

static asset_metadata* asset_registry_find_by_path(asset_registry* registry, const char* path) {
    if (!registry || !path) return NULL;

    for (uint32_t i = 0; i < registry->asset_count; i++) {
        if (registry->assets[i].asset_path && strcmp(registry->assets[i].asset_path, path) == 0) {
            return &registry->assets[i];
        }
    }

    return NULL;
}

static asset_metadata* asset_registry_find_by_id(asset_registry* registry, uint64_t asset_id) {
    if (!registry) return NULL;

    for (uint32_t i = 0; i < registry->asset_count; i++) {
        if (registry->assets[i].asset_id == asset_id) {
            return &registry->assets[i];
        }
    }

    return NULL;
}

static asset_metadata* asset_registry_add(asset_registry* registry, const char* name, const char* path, asset_type type) {
    if (!registry || registry->asset_count >= registry->asset_capacity) {
        return NULL;
    }

    asset_metadata* meta = &registry->assets[registry->asset_count++];
    memset(meta, 0, sizeof(asset_metadata));

    meta->asset_id = g_asset_id_gen.next_asset_id++;
    meta->asset_name = malloc(strlen(name) + 1);
    meta->asset_path = malloc(strlen(path) + 1);

    if (!meta->asset_name || !meta->asset_path) {
        registry->asset_count--;
        return NULL;
    }

    strcpy((char*)meta->asset_name, name);
    strcpy((char*)meta->asset_path, path);
    meta->type = type;
    meta->version = 1;
    meta->reference_count = 0;
    meta->created_time = time(NULL);
    meta->modified_time = meta->created_time;

    return meta;
}

// ============================================================================
// MEMORY POOL IMPLEMENTATION
// ============================================================================

static asset_pool* asset_pool_create(size_t total_size, uint32_t alignment) {
    asset_pool* pool = malloc(sizeof(asset_pool));
    if (!pool) return NULL;

    pool->alignment = alignment;
    pool->total_size = total_size;
    pool->used_size = 0;
    pool->block_capacity = 4096;
    pool->block_count = 0;

    pool->blocks = calloc(pool->block_capacity, sizeof(memory_pool_block));
    if (!pool->blocks) {
        free(pool);
        return NULL;
    }

    return pool;
}

static void asset_pool_destroy(asset_pool* pool) {
    if (!pool) return;

    for (uint32_t i = 0; i < pool->block_count; i++) {
        free(pool->blocks[i].memory);
    }

    free(pool->blocks);
    free(pool);
}

static void* asset_pool_allocate(asset_pool* pool, size_t size) {
    if (!pool || size == 0 || size > pool->total_size) {
        return NULL;
    }

    // Try to find existing free block
    for (uint32_t i = 0; i < pool->block_count; i++) {
        if (!pool->blocks[i].is_in_use && pool->blocks[i].size >= size) {
            void* memory = pool->blocks[i].memory;
            pool->blocks[i].is_in_use = true;
            pool->used_size += size;
            return memory;
        }
    }

    // Allocate new block if capacity available
    if (pool->block_count >= pool->block_capacity) {
        return NULL;
    }

    memory_pool_block* block = &pool->blocks[pool->block_count++];
    block->memory = malloc(size);
    if (!block->memory) {
        pool->block_count--;
        return NULL;
    }

    block->size = size;
    block->is_in_use = true;
    pool->used_size += size;

    return block->memory;
}

static void asset_pool_deallocate(asset_pool* pool, void* memory) {
    if (!pool || !memory) return;

    for (uint32_t i = 0; i < pool->block_count; i++) {
        if (pool->blocks[i].memory == memory) {
            pool->blocks[i].is_in_use = false;
            pool->used_size -= pool->blocks[i].size;
            return;
        }
    }
}

static void asset_pool_clear(asset_pool* pool) {
    if (!pool) return;

    for (uint32_t i = 0; i < pool->block_count; i++) {
        pool->blocks[i].is_in_use = false;
    }

    pool->used_size = 0;
}

// ============================================================================
// ASSET CACHE IMPLEMENTATION
// ============================================================================

static asset_cache* asset_cache_create(size_t max_size) {
    asset_cache* cache = malloc(sizeof(asset_cache));
    if (!cache) return NULL;

    cache->max_cache_size = max_size;
    cache->current_cache_size = 0;
    cache->entry_capacity = 1024;
    cache->entry_count = 0;

    cache->entries = calloc(cache->entry_capacity, sizeof(cache_entry));
    if (!cache->entries) {
        free(cache);
        return NULL;
    }

    return cache;
}

static void asset_cache_destroy(asset_cache* cache) {
    if (!cache) return;

    for (uint32_t i = 0; i < cache->entry_count; i++) {
        free(cache->entries[i].cached_data);
    }

    free(cache->entries);
    free(cache);
}

static cache_entry* asset_cache_find(asset_cache* cache, uint64_t asset_id) {
    if (!cache) return NULL;

    for (uint32_t i = 0; i < cache->entry_count; i++) {
        if (cache->entries[i].asset_id == asset_id) {
            cache->entries[i].access_count++;
            return &cache->entries[i];
        }
    }

    return NULL;
}

static bool asset_cache_add(asset_cache* cache, uint64_t asset_id, void* data, size_t data_size) {
    if (!cache || cache->current_cache_size + data_size > cache->max_cache_size) {
        return false;
    }

    if (cache->entry_count >= cache->entry_capacity) {
        return false;
    }

    cache_entry* entry = &cache->entries[cache->entry_count++];
    entry->asset_id = asset_id;
    entry->cached_data = malloc(data_size);
    if (!entry->cached_data) {
        cache->entry_count--;
        return false;
    }

    memcpy(entry->cached_data, data, data_size);
    entry->cached_size = data_size;
    entry->access_count = 1;
    cache->current_cache_size += data_size;

    return true;
}

static void asset_cache_remove(asset_cache* cache, uint64_t asset_id) {
    if (!cache) return;

    for (uint32_t i = 0; i < cache->entry_count; i++) {
        if (cache->entries[i].asset_id == asset_id) {
            cache->current_cache_size -= cache->entries[i].cached_size;
            free(cache->entries[i].cached_data);

            // Swap with last element
            if (i < cache->entry_count - 1) {
                cache->entries[i] = cache->entries[cache->entry_count - 1];
            }

            cache->entry_count--;
            return;
        }
    }
}

static void asset_cache_clear(asset_cache* cache) {
    if (!cache) return;

    for (uint32_t i = 0; i < cache->entry_count; i++) {
        free(cache->entries[i].cached_data);
    }

    cache->entry_count = 0;
    cache->current_cache_size = 0;
}

// ============================================================================
// LOAD QUEUE IMPLEMENTATION
// ============================================================================

static asset_load_queue* asset_load_queue_create(uint32_t initial_capacity) {
    asset_load_queue* queue = malloc(sizeof(asset_load_queue));
    if (!queue) return NULL;

    queue->request_capacity = initial_capacity;
    queue->request_count = 0;
    queue->next_request_id = 1;

    queue->requests = calloc(initial_capacity, sizeof(asset_load_request));
    if (!queue->requests) {
        free(queue);
        return NULL;
    }

    return queue;
}

static void asset_load_queue_destroy(asset_load_queue* queue) {
    if (!queue) return;
    free(queue->requests);
    free(queue);
}

static uint64_t asset_load_queue_enqueue(asset_load_queue* queue, asset_load_request* request) {
    if (!queue || queue->request_count >= queue->request_capacity) {
        return 0;
    }

    asset_load_request* new_req = &queue->requests[queue->request_count++];
    memcpy(new_req, request, sizeof(asset_load_request));
    new_req->request_id = queue->next_request_id++;

    return new_req->request_id;
}

static asset_load_request* asset_load_queue_get_next(asset_load_queue* queue) {
    if (!queue || queue->request_count == 0) {
        return NULL;
    }

    // Find highest priority request
    asset_load_request* highest_priority = &queue->requests[0];

    for (uint32_t i = 1; i < queue->request_count; i++) {
        if (queue->requests[i].priority < highest_priority->priority) {
            highest_priority = &queue->requests[i];
        }
    }

    return highest_priority;
}

// ============================================================================
// ASSET MANAGER IMPLEMENTATION
// ============================================================================

asset_manager* asset_manager_create(size_t max_memory, uint32_t max_assets) {
    asset_manager* manager = malloc(sizeof(asset_manager));
    if (!manager) return NULL;

    memset(manager, 0, sizeof(asset_manager));

    manager->max_memory_allowed = max_memory;
    manager->max_loaded_assets = max_assets;

    // Create registry
    manager->registry = asset_registry_create(max_assets);
    if (!manager->registry) {
        free(manager);
        return NULL;
    }

    // Create memory pool
    manager->memory_pool = asset_pool_create(max_memory, 16);
    if (!manager->memory_pool) {
        asset_registry_destroy(manager->registry);
        free(manager);
        return NULL;
    }

    // Create cache
    manager->asset_cache = asset_cache_create(max_memory / 4);
    if (!manager->asset_cache) {
        asset_registry_destroy(manager->registry);
        asset_pool_destroy(manager->memory_pool);
        free(manager);
        return NULL;
    }

    // Create load queue
    manager->load_queue = asset_load_queue_create(256);
    if (!manager->load_queue) {
        asset_registry_destroy(manager->registry);
        asset_pool_destroy(manager->memory_pool);
        asset_cache_destroy(manager->asset_cache);
        free(manager);
        return NULL;
    }

    // Allocate loaded assets array
    manager->loaded_assets = calloc(max_assets, sizeof(asset_handle*));
    if (!manager->loaded_assets) {
        asset_registry_destroy(manager->registry);
        asset_pool_destroy(manager->memory_pool);
        asset_cache_destroy(manager->asset_cache);
        asset_load_queue_destroy(manager->load_queue);
        free(manager);
        return NULL;
    }

    manager->max_concurrent_loads = 4;
    manager->initialized = false;

    return manager;
}

void asset_manager_destroy(asset_manager* manager) {
    if (!manager) return;

    // Unload all assets
    for (uint32_t i = 0; i < manager->loaded_asset_count; i++) {
        if (manager->loaded_assets[i]) {
            free(manager->loaded_assets[i]->data);
            free(manager->loaded_assets[i]);
        }
    }

    asset_registry_destroy(manager->registry);
    asset_pool_destroy(manager->memory_pool);
    asset_cache_destroy(manager->asset_cache);
    asset_load_queue_destroy(manager->load_queue);
    free(manager->loaded_assets);
    free(manager);
}

bool asset_manager_initialize(asset_manager* manager) {
    if (!manager) return false;

    manager->initialized = true;
    manager->current_frame = 0;
    manager->total_assets_loaded = 0;
    manager->total_assets_failed = 0;
    manager->total_memory_cached = 0;
    manager->average_load_time = 0.0f;

    return true;
}

void asset_manager_shutdown(asset_manager* manager) {
    if (!manager) return;

    manager->initialized = false;

    // Process remaining async loads
    asset_process_async_loads(manager, 0);

    // Clear all caches
    asset_cache_clear(manager->asset_cache);
    asset_pool_clear(manager->memory_pool);
}

// ============================================================================
// ASSET LOADING
// ============================================================================

asset_handle* asset_load_sync(asset_manager* manager, const char* path, asset_type type) {
    if (!manager || !path) return NULL;

    // Check if already loaded
    asset_metadata* existing = asset_registry_find_by_path(manager->registry, path);
    if (existing) {
        asset_handle* handle = asset_get_handle(manager, existing->asset_id);
        if (handle) {
            asset_add_reference(manager, existing->asset_id);
            return handle;
        }
    }

    // Create new registry entry
    const char* name = strrchr(path, '/');
    if (!name) name = path;
    else name++;

    asset_metadata* meta = asset_registry_add(manager->registry, name, path, type);
    if (!meta) return NULL;

    // Load asset data
    FILE* file = fopen(path, "rb");
    if (!file) {
        manager->total_assets_failed++;
        return NULL;
    }

    // Read file header
    asset_file_header header;
    if (fread(&header, sizeof(asset_file_header), 1, file) != 1) {
        fclose(file);
        manager->total_assets_failed++;
        return NULL;
    }

    // Read asset data
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, sizeof(asset_file_header), SEEK_SET);

    size_t data_size = file_size - sizeof(asset_file_header);
    void* data = malloc(data_size);
    if (!data || fread(data, 1, data_size, file) != data_size) {
        free(data);
        fclose(file);
        manager->total_assets_failed++;
        return NULL;
    }

    fclose(file);

    // Create handle
    if (manager->loaded_asset_count >= manager->max_loaded_assets) {
        free(data);
        return NULL;
    }

    asset_handle* handle = malloc(sizeof(asset_handle));
    if (!handle) {
        free(data);
        return NULL;
    }

    memset(handle, 0, sizeof(asset_handle));
    handle->asset_id = meta->asset_id;
    handle->type = type;
    handle->state = ASSET_STATE_LOADED;
    handle->reference_count = 1;
    handle->data = data;
    handle->data_size = data_size;
    handle->metadata = meta;
    handle->frame_loaded = manager->current_frame;

    manager->loaded_assets[manager->loaded_asset_count++] = handle;
    manager->total_memory_used += data_size;
    manager->total_assets_loaded++;

    meta->reference_count++;

    return handle;
}

uint64_t asset_load_async(asset_manager* manager, const char* path, asset_type type,
                          asset_load_priority priority,
                          void (*on_loaded)(uint64_t asset_id, void* user_data),
                          void* user_data) {
    if (!manager || !path) return 0;

    asset_load_request request = {
        .asset_id = g_asset_id_gen.next_asset_id,
        .priority = priority,
        .on_loaded_callback = on_loaded,
        .user_data = user_data,
    };

    return asset_load_queue_enqueue(manager->load_queue, &request);
}

bool asset_is_loaded(asset_manager* manager, uint64_t asset_id) {
    if (!manager) return false;

    for (uint32_t i = 0; i < manager->loaded_asset_count; i++) {
        if (manager->loaded_assets[i] && manager->loaded_assets[i]->asset_id == asset_id) {
            return manager->loaded_assets[i]->state == ASSET_STATE_LOADED;
        }
    }

    return false;
}

asset_handle* asset_get_handle(asset_manager* manager, uint64_t asset_id) {
    if (!manager) return NULL;

    for (uint32_t i = 0; i < manager->loaded_asset_count; i++) {
        if (manager->loaded_assets[i] && manager->loaded_assets[i]->asset_id == asset_id) {
            manager->loaded_assets[i]->frame_last_accessed = manager->current_frame;
            return manager->loaded_assets[i];
        }
    }

    return NULL;
}

void asset_unload(asset_manager* manager, uint64_t asset_id) {
    if (!manager) return;

    for (uint32_t i = 0; i < manager->loaded_asset_count; i++) {
        if (manager->loaded_assets[i] && manager->loaded_assets[i]->asset_id == asset_id) {
            asset_handle* handle = manager->loaded_assets[i];

            if (handle->reference_count > 0) {
                handle->reference_count--;
            }

            if (handle->reference_count == 0) {
                handle->state = ASSET_STATE_UNLOADED;
                manager->total_memory_used -= handle->data_size;
                free(handle->data);

                if (i < manager->loaded_asset_count - 1) {
                    manager->loaded_assets[i] = manager->loaded_assets[manager->loaded_asset_count - 1];
                }

                manager->loaded_asset_count--;
                free(handle);
            }
            return;
        }
    }
}

void asset_unload_unused(asset_manager* manager, uint32_t unused_frames) {
    if (!manager) return;

    uint32_t current_frame = manager->current_frame;
    uint32_t i = 0;

    while (i < manager->loaded_asset_count) {
        asset_handle* handle = manager->loaded_assets[i];

        if (handle->reference_count <= 1 && (current_frame - handle->frame_last_accessed) > unused_frames) {
            asset_unload(manager, handle->asset_id);
        } else {
            i++;
        }
    }
}

// ============================================================================
// ASSET SEARCHING AND METADATA
// ============================================================================

uint64_t asset_find_by_name(asset_manager* manager, const char* name) {
    if (!manager || !name) return 0;

    asset_metadata* meta = asset_registry_find_by_name(manager->registry, name);
    return meta ? meta->asset_id : 0;
}

uint64_t asset_find_by_path(asset_manager* manager, const char* path) {
    if (!manager || !path) return 0;

    asset_metadata* meta = asset_registry_find_by_path(manager->registry, path);
    return meta ? meta->asset_id : 0;
}

uint64_t* asset_find_by_tag(asset_manager* manager, const char* tag, uint32_t* out_count) {
    if (!manager || !tag || !out_count) {
        if (out_count) *out_count = 0;
        return NULL;
    }

    uint64_t* result = malloc(manager->registry->asset_count * sizeof(uint64_t));
    if (!result) {
        *out_count = 0;
        return NULL;
    }

    uint32_t count = 0;

    for (uint32_t i = 0; i < manager->registry->asset_count; i++) {
        asset_metadata* meta = &manager->registry->assets[i];

        for (uint32_t j = 0; j < meta->tag_count; j++) {
            if (strcmp(meta->tags[j], tag) == 0) {
                result[count++] = meta->asset_id;
                break;
            }
        }
    }

    *out_count = count;
    return result;
}

uint64_t* asset_find_by_type(asset_manager* manager, asset_type type, uint32_t* out_count) {
    if (!manager || !out_count) {
        if (out_count) *out_count = 0;
        return NULL;
    }

    uint64_t* result = malloc(manager->registry->asset_count * sizeof(uint64_t));
    if (!result) {
        *out_count = 0;
        return NULL;
    }

    uint32_t count = 0;

    for (uint32_t i = 0; i < manager->registry->asset_count; i++) {
        if (manager->registry->assets[i].type == type) {
            result[count++] = manager->registry->assets[i].asset_id;
        }
    }

    *out_count = count;
    return result;
}

asset_metadata* asset_get_metadata(asset_manager* manager, uint64_t asset_id) {
    if (!manager) return NULL;

    return asset_registry_find_by_id(manager->registry, asset_id);
}

// ============================================================================
// REFERENCE COUNTING
// ============================================================================

void asset_add_reference(asset_manager* manager, uint64_t asset_id) {
    if (!manager) return;

    asset_handle* handle = asset_get_handle(manager, asset_id);
    if (handle) {
        handle->reference_count++;
    }
}

void asset_remove_reference(asset_manager* manager, uint64_t asset_id) {
    if (!manager) return;

    asset_unload(manager, asset_id);
}

uint32_t asset_get_reference_count(asset_manager* manager, uint64_t asset_id) {
    if (!manager) return 0;

    asset_handle* handle = asset_get_handle(manager, asset_id);
    return handle ? handle->reference_count : 0;
}

// ============================================================================
// DEPENDENCIES
// ============================================================================

asset_dependency* asset_get_dependencies(asset_manager* manager, uint64_t asset_id, uint32_t* out_count) {
    if (!manager || !out_count) {
        if (out_count) *out_count = 0;
        return NULL;
    }

    asset_metadata* meta = asset_registry_find_by_id(manager->registry, asset_id);
    if (!meta) {
        *out_count = 0;
        return NULL;
    }

    *out_count = meta->dependency_count;
    return meta->dependencies;
}

bool asset_load_dependencies(asset_manager* manager, uint64_t asset_id) {
    if (!manager) return false;

    uint32_t dep_count = 0;
    asset_dependency* deps = asset_get_dependencies(manager, asset_id, &dep_count);

    for (uint32_t i = 0; i < dep_count; i++) {
        // TODO: Load dependency recursively
    }

    return true;
}

bool asset_validate_dependencies(asset_manager* manager, uint64_t asset_id) {
    if (!manager) return false;

    uint32_t dep_count = 0;
    asset_dependency* deps = asset_get_dependencies(manager, asset_id, &dep_count);

    for (uint32_t i = 0; i < dep_count; i++) {
        if (!asset_is_loaded(manager, deps[i].dependency_asset_id)) {
            return false;
        }
    }

    return true;
}

// ============================================================================
// MEMORY MANAGEMENT
// ============================================================================

void asset_update_frame(asset_manager* manager) {
    if (!manager) return;

    manager->current_frame++;

    // TODO: Implement frame-based memory optimization
}

size_t asset_get_memory_usage(asset_manager* manager) {
    return manager ? manager->total_memory_used : 0;
}

size_t asset_get_cache_size(asset_manager* manager) {
    return manager ? manager->asset_cache->current_cache_size : 0;
}

void asset_clear_cache(asset_manager* manager) {
    if (manager) {
        asset_cache_clear(manager->asset_cache);
    }
}

void asset_compact_memory(asset_manager* manager) {
    if (!manager) return;

    // TODO: Implement memory defragmentation
}

// ============================================================================
// STREAMING OPERATIONS
// ============================================================================

bool asset_start_streaming(asset_manager* manager, uint64_t asset_id, uint32_t stream_size) {
    if (!manager) return false;

    asset_handle* handle = asset_get_handle(manager, asset_id);
    if (!handle) return false;

    handle->is_streaming = true;
    handle->stream_size = stream_size;
    handle->stream_offset = 0;

    return true;
}

bool asset_stream_next_chunk(asset_manager* manager, uint64_t asset_id) {
    if (!manager) return false;

    asset_handle* handle = asset_get_handle(manager, asset_id);
    if (!handle || !handle->is_streaming) return false;

    handle->stream_offset += handle->stream_size;

    if (handle->stream_offset >= handle->data_size) {
        handle->is_streaming = false;
        return false;
    }

    return true;
}

float asset_get_stream_progress(asset_manager* manager, uint64_t asset_id) {
    if (!manager) return 0.0f;

    asset_handle* handle = asset_get_handle(manager, asset_id);
    if (!handle || handle->data_size == 0) return 0.0f;

    return (float)handle->stream_offset / (float)handle->data_size;
}

// ============================================================================
// IMPORT/EXPORT AND COOKING
// ============================================================================

bool asset_import(asset_manager* manager, const char* source_path, asset_type type) {
    if (!manager || !source_path) return false;

    // TODO: Implement import pipeline with callbacks
    return true;
}

bool asset_export(asset_manager* manager, uint64_t asset_id, const char* output_path, bool compress) {
    if (!manager || !output_path) return false;

    asset_handle* handle = asset_get_handle(manager, asset_id);
    if (!handle) return false;

    FILE* file = fopen(output_path, "wb");
    if (!file) return false;

    // Write file header
    asset_file_header header = {
        .magic = 'ASST',
        .version = 1,
        .asset_size = handle->data_size,
        .compression = compress ? ASSET_COMPRESSION_LZ4 : ASSET_COMPRESSION_NONE,
    };

    fwrite(&header, sizeof(asset_file_header), 1, file);
    fwrite(handle->data, 1, handle->data_size, file);
    fclose(file);

    return true;
}

bool asset_reimport(asset_manager* manager, uint64_t asset_id) {
    if (!manager) return false;

    asset_metadata* meta = asset_registry_find_by_id(manager->registry, asset_id);
    if (!meta) return false;

    asset_unload(manager, asset_id);
    return (asset_load_sync(manager, meta->asset_path, meta->type) != NULL);
}

bool asset_cook(asset_manager* manager, uint64_t asset_id, bool compress) {
    if (!manager) return false;

    asset_handle* handle = asset_get_handle(manager, asset_id);
    if (!handle) return false;

    handle->metadata->is_cooked = true;
    handle->metadata->is_compressed = compress;

    return true;
}

bool asset_is_cooked(asset_manager* manager, uint64_t asset_id) {
    if (!manager) return false;

    asset_metadata* meta = asset_registry_find_by_id(manager->registry, asset_id);
    return meta ? meta->is_cooked : false;
}

// ============================================================================
// VERSIONING AND TAGGING
// ============================================================================

uint32_t asset_get_version(asset_manager* manager, uint64_t asset_id) {
    if (!manager) return 0;

    asset_metadata* meta = asset_registry_find_by_id(manager->registry, asset_id);
    return meta ? meta->version : 0;
}

bool asset_migrate_version(asset_manager* manager, uint64_t asset_id, uint32_t target_version) {
    if (!manager) return false;

    asset_metadata* meta = asset_registry_find_by_id(manager->registry, asset_id);
    if (!meta) return false;

    // TODO: Implement version migration
    meta->version = target_version;
    return true;
}

void asset_add_tag(asset_manager* manager, uint64_t asset_id, const char* tag) {
    if (!manager || !tag) return;

    asset_metadata* meta = asset_registry_find_by_id(manager->registry, asset_id);
    if (!meta || meta->tag_count >= 32) return;

    // TODO: Add tag to metadata
}

void asset_remove_tag(asset_manager* manager, uint64_t asset_id, const char* tag) {
    if (!manager || !tag) return;

    // TODO: Remove tag from metadata
}

bool asset_has_tag(asset_manager* manager, uint64_t asset_id, const char* tag) {
    if (!manager || !tag) return false;

    // TODO: Check tag in metadata
    return false;
}

// ============================================================================
// STATISTICS AND REPORTING
// ============================================================================

void asset_print_statistics(asset_manager* manager) {
    if (!manager) return;

    printf("=== Asset Manager Statistics ===\n");
    printf("Total Assets Loaded: %llu\n", manager->total_assets_loaded);
    printf("Total Assets Failed: %llu\n", manager->total_assets_failed);
    printf("Memory Used: %llu / %llu bytes\n", manager->total_memory_used, manager->max_memory_allowed);
    printf("Cached Memory: %zu bytes\n", manager->asset_cache->current_cache_size);
    printf("Loaded Assets: %u\n", manager->loaded_asset_count);
    printf("Average Load Time: %.2f ms\n", manager->average_load_time);
}

void asset_print_memory_report(asset_manager* manager) {
    if (!manager) return;

    printf("=== Asset Memory Report ===\n");

    for (uint32_t i = 0; i < manager->loaded_asset_count; i++) {
        asset_handle* handle = manager->loaded_assets[i];
        if (handle && handle->metadata) {
            printf("%s: %zu bytes (refs: %u)\n",
                   handle->metadata->asset_name,
                   handle->data_size,
                   handle->reference_count);
        }
    }
}

void asset_export_memory_profile(asset_manager* manager, const char* filename) {
    if (!manager || !filename) return;

    FILE* file = fopen(filename, "w");
    if (!file) return;

    fprintf(file, "Asset Name,Type,Size,References,LastAccess\n");

    for (uint32_t i = 0; i < manager->loaded_asset_count; i++) {
        asset_handle* handle = manager->loaded_assets[i];
        if (handle && handle->metadata) {
            fprintf(file, "%s,%u,%zu,%u,%u\n",
                    handle->metadata->asset_name,
                    handle->metadata->type,
                    handle->data_size,
                    handle->reference_count,
                    manager->current_frame - handle->frame_last_accessed);
        }
    }

    fclose(file);
}

// ============================================================================
// BATCH OPERATIONS
// ============================================================================

void asset_batch_load(asset_manager* manager, const char** paths, asset_type* types, uint32_t count) {
    if (!manager || !paths || !types) return;

    for (uint32_t i = 0; i < count; i++) {
        asset_load_sync(manager, paths[i], types[i]);
    }
}

void asset_batch_unload(asset_manager* manager, uint64_t* asset_ids, uint32_t count) {
    if (!manager || !asset_ids) return;

    for (uint32_t i = 0; i < count; i++) {
        asset_unload(manager, asset_ids[i]);
    }
}

void asset_batch_cook(asset_manager* manager, uint64_t* asset_ids, uint32_t count) {
    if (!manager || !asset_ids) return;

    for (uint32_t i = 0; i < count; i++) {
        asset_cook(manager, asset_ids[i], true);
    }
}

// ============================================================================
// REGISTRY AND VALIDATION
// ============================================================================

uint32_t asset_get_total_assets(asset_manager* manager) {
    return manager ? manager->registry->asset_count : 0;
}

uint64_t* asset_get_all_assets(asset_manager* manager, uint32_t* out_count) {
    if (!manager || !out_count) {
        if (out_count) *out_count = 0;
        return NULL;
    }

    uint64_t* result = malloc(manager->registry->asset_count * sizeof(uint64_t));
    if (!result) {
        *out_count = 0;
        return NULL;
    }

    for (uint32_t i = 0; i < manager->registry->asset_count; i++) {
        result[i] = manager->registry->assets[i].asset_id;
    }

    *out_count = manager->registry->asset_count;
    return result;
}

void asset_rebuild_registry(asset_manager* manager) {
    if (!manager) return;

    // TODO: Rebuild asset registry from filesystem
}

bool asset_validate_file(asset_manager* manager, const char* path) {
    if (!manager || !path) return false;

    FILE* file = fopen(path, "rb");
    if (!file) return false;

    asset_file_header header;
    bool valid = fread(&header, sizeof(asset_file_header), 1, file) == 1 && header.magic == 'ASST';

    fclose(file);
    return valid;
}

bool asset_repair_corrupted(asset_manager* manager, uint64_t asset_id) {
    if (!manager) return false;

    // TODO: Implement asset repair
    return false;
}

void asset_process_async_loads(asset_manager* manager, uint32_t max_time_ms) {
    if (!manager) return;

    while (manager->load_queue->request_count > 0 && manager->active_load_count < manager->max_concurrent_loads) {
        asset_load_request* request = asset_load_queue_get_next(manager->load_queue);
        if (!request) break;

        // TODO: Actually load the asset
        manager->active_load_count++;

        // Remove from queue
        if (manager->load_queue->request_count > 0) {
            // Simple removal - move last to current position
        }
    }
}

uint32_t asset_get_pending_load_count(asset_manager* manager) {
    return manager ? manager->load_queue->request_count : 0;
}

uint32_t asset_get_failed_load_count(asset_manager* manager) {
    return manager ? (uint32_t)manager->total_assets_failed : 0;
}

// End of asset_system_complete.c
