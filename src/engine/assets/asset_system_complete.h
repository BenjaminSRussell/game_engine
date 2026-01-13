/**
 * Complete Asset Management System
 *
 * A production-grade asset system supporting:
 * - Unified asset format with metadata
 * - Asynchronous loading and streaming
 * - Dependency tracking and resolution
 * - Reference counting and lifecycle management
 * - Asset versioning and migration
 * - Hot reloading in editor
 * - Memory pooling and caching
 * - Import and export pipelines
 * - Asset tagging and searching
 * - Async I/O with priorities
 *
 * Total: ~2800 lines for complete system
 */

#ifndef ASSET_SYSTEM_COMPLETE_H
#define ASSET_SYSTEM_COMPLETE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>

// ============================================================================
// CORE ENUMERATIONS
// ============================================================================

typedef enum {
    ASSET_TYPE_UNDEFINED,
    ASSET_TYPE_TEXTURE,
    ASSET_TYPE_MESH,
    ASSET_TYPE_MATERIAL,
    ASSET_TYPE_SHADER,
    ASSET_TYPE_ANIMATION,
    ASSET_TYPE_SKELETON,
    ASSET_TYPE_SOUND,
    ASSET_TYPE_MUSIC,
    ASSET_TYPE_FONT,
    ASSET_TYPE_SCENE,
    ASSET_TYPE_BLUEPRINT,
    ASSET_TYPE_DATA_TABLE,
    ASSET_TYPE_PARTICLE_SYSTEM,
    ASSET_TYPE_PARTICLE_EMITTER,
    ASSET_TYPE_EFFECT,
    ASSET_TYPE_CONFIG,
    ASSET_TYPE_LOCALIZATION,
    ASSET_TYPE_VIDEO,
    ASSET_TYPE_USER_DEFINED,
} asset_type;

typedef enum {
    ASSET_STATE_UNLOADED,
    ASSET_STATE_LOADING,
    ASSET_STATE_LOADED,
    ASSET_STATE_FAILED,
    ASSET_STATE_UNLOADING,
    ASSET_STATE_ASYNC_LOAD_QUEUED,
    ASSET_STATE_ASYNC_LOAD_IN_PROGRESS,
} asset_state;

typedef enum {
    ASSET_LOAD_PRIORITY_CRITICAL,
    ASSET_LOAD_PRIORITY_HIGH,
    ASSET_LOAD_PRIORITY_NORMAL,
    ASSET_LOAD_PRIORITY_LOW,
    ASSET_LOAD_PRIORITY_DEFERRED,
} asset_load_priority;

typedef enum {
    ASSET_COMPRESSION_NONE,
    ASSET_COMPRESSION_LZ4,
    ASSET_COMPRESSION_ZSTD,
    ASSET_COMPRESSION_DEFLATE,
} asset_compression_type;

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

typedef struct asset_manager asset_manager;
typedef struct asset_registry asset_registry;
typedef struct asset_handle asset_handle;
typedef struct asset_metadata asset_metadata;
typedef struct asset_dependency asset_dependency;
typedef struct asset_import_context asset_import_context;
typedef struct asset_pool asset_pool;
typedef struct asset_cache asset_cache;

// ============================================================================
// ASSET METADATA STRUCTURE
// ============================================================================

typedef struct {
    uint32_t magic;  // "ASST"
    uint32_t version;
    uint64_t creation_time;
    uint64_t last_modified_time;
    uint64_t asset_size;
    asset_compression_type compression;
    uint32_t crc32;
} asset_file_header;

typedef struct asset_metadata {
    uint64_t asset_id;
    const char* asset_name;
    const char* asset_path;
    asset_type type;
    uint32_t version;

    size_t uncompressed_size;
    size_t compressed_size;
    asset_compression_type compression;

    uint32_t width;      // For textures/images
    uint32_t height;     // For textures/images
    uint32_t depth;      // For 3D assets
    uint32_t mip_levels; // For textures

    uint32_t memory_pool_size;
    uint32_t reference_count;

    // Tags for searching
    const char** tags;
    uint32_t tag_count;

    // Dependencies
    struct asset_dependency* dependencies;
    uint32_t dependency_count;

    // Timestamps
    time_t created_time;
    time_t modified_time;
    time_t accessed_time;

    // Versioning
    uint32_t major_version;
    uint32_t minor_version;
    uint32_t patch_version;

    const char* creator;
    const char* description;

    bool is_cooked;
    bool is_streaming;
    bool is_compressed;
} asset_metadata;

// ============================================================================
// DEPENDENCY TRACKING
// ============================================================================

typedef struct asset_dependency {
    uint64_t dependency_asset_id;
    const char* dependency_name;
    asset_type dependency_type;
    bool is_hard_reference;  // vs soft reference
    bool is_loaded;
    struct asset_dependency* next;
} asset_dependency;

typedef struct {
    asset_dependency** dependency_list;
    uint32_t dependency_count;
    uint32_t max_dependencies;
} asset_dependency_graph;

// ============================================================================
// ASYNC LOADING STRUCTURES
// ============================================================================

typedef struct {
    uint64_t request_id;
    uint64_t asset_id;
    asset_load_priority priority;
    bool is_blocking;
    void (*on_loaded_callback)(uint64_t asset_id, void* user_data);
    void* user_data;
} asset_load_request;

typedef struct {
    asset_load_request* requests;
    uint32_t request_count;
    uint32_t request_capacity;
    uint32_t next_request_id;
} asset_load_queue;

// ============================================================================
// MEMORY POOLING
// ============================================================================

typedef struct {
    void* memory;
    size_t size;
    bool is_in_use;
    uint32_t allocation_frame;
    const char* owner_asset;
} memory_pool_block;

typedef struct asset_pool {
    memory_pool_block* blocks;
    uint32_t block_count;
    uint32_t block_capacity;
    size_t total_size;
    size_t used_size;
    uint32_t alignment;
} asset_pool;

// ============================================================================
// ASSET CACHE STRUCTURES
// ============================================================================

typedef struct {
    uint64_t asset_id;
    void* cached_data;
    size_t cached_size;
    uint64_t last_access_frame;
    uint32_t access_count;
} cache_entry;

typedef struct asset_cache {
    cache_entry* entries;
    uint32_t entry_count;
    uint32_t entry_capacity;
    size_t max_cache_size;
    size_t current_cache_size;
} asset_cache;

// ============================================================================
// ASSET HANDLE
// ============================================================================

typedef struct asset_handle {
    uint64_t asset_id;
    asset_type type;
    asset_state state;
    uint32_t reference_count;

    void* data;
    size_t data_size;

    asset_metadata* metadata;

    uint32_t frame_loaded;
    uint32_t frame_last_accessed;

    bool is_streaming;
    uint32_t stream_offset;
    uint32_t stream_size;
} asset_handle;

// ============================================================================
// ASSET MANAGER
// ============================================================================

typedef struct asset_manager {
    asset_registry* registry;
    asset_pool* memory_pool;
    asset_cache* asset_cache;
    asset_load_queue* load_queue;

    asset_handle** loaded_assets;
    uint32_t loaded_asset_count;
    uint32_t max_loaded_assets;

    uint32_t current_frame;
    uint64_t total_memory_used;
    uint64_t max_memory_allowed;

    // Import/Export
    void* (*import_callback)(const char* path, asset_type type, asset_metadata* metadata);
    bool (*export_callback)(const char* path, void* asset_data, asset_metadata* metadata);

    // Threading
    uint32_t max_concurrent_loads;
    uint32_t active_load_count;

    // Statistics
    uint64_t total_assets_loaded;
    uint64_t total_assets_failed;
    uint64_t total_memory_cached;
    float average_load_time;

    bool initialized;
} asset_manager;

// ============================================================================
// IMPORT/EXPORT STRUCTURES
// ============================================================================

typedef struct {
    const char* source_file;
    asset_type target_type;
    asset_metadata* output_metadata;
    void* custom_data;
} asset_import_info;

typedef struct {
    void* asset_data;
    asset_metadata* metadata;
    const char* output_path;
    bool compress;
} asset_export_info;

// ============================================================================
// ASSET MANAGER API
// ============================================================================

// Initialization and shutdown
asset_manager* asset_manager_create(size_t max_memory, uint32_t max_assets);
void asset_manager_destroy(asset_manager* manager);
bool asset_manager_initialize(asset_manager* manager);
void asset_manager_shutdown(asset_manager* manager);

// Asset loading
asset_handle* asset_load_sync(asset_manager* manager, const char* path, asset_type type);
uint64_t asset_load_async(asset_manager* manager, const char* path, asset_type type,
                          asset_load_priority priority,
                          void (*on_loaded)(uint64_t asset_id, void* user_data),
                          void* user_data);
bool asset_is_loaded(asset_manager* manager, uint64_t asset_id);
asset_handle* asset_get_handle(asset_manager* manager, uint64_t asset_id);
void asset_unload(asset_manager* manager, uint64_t asset_id);
void asset_unload_unused(asset_manager* manager, uint32_t unused_frames);

// Asset searching and metadata
uint64_t asset_find_by_name(asset_manager* manager, const char* name);
uint64_t asset_find_by_path(asset_manager* manager, const char* path);
uint64_t* asset_find_by_tag(asset_manager* manager, const char* tag, uint32_t* out_count);
uint64_t* asset_find_by_type(asset_manager* manager, asset_type type, uint32_t* out_count);
asset_metadata* asset_get_metadata(asset_manager* manager, uint64_t asset_id);

// Reference counting
void asset_add_reference(asset_manager* manager, uint64_t asset_id);
void asset_remove_reference(asset_manager* manager, uint64_t asset_id);
uint32_t asset_get_reference_count(asset_manager* manager, uint64_t asset_id);

// Dependencies
asset_dependency* asset_get_dependencies(asset_manager* manager, uint64_t asset_id, uint32_t* out_count);
bool asset_load_dependencies(asset_manager* manager, uint64_t asset_id);
bool asset_validate_dependencies(asset_manager* manager, uint64_t asset_id);

// Memory management
void asset_update_frame(asset_manager* manager);
size_t asset_get_memory_usage(asset_manager* manager);
size_t asset_get_cache_size(asset_manager* manager);
void asset_clear_cache(asset_manager* manager);
void asset_compact_memory(asset_manager* manager);

// Streaming
bool asset_start_streaming(asset_manager* manager, uint64_t asset_id, uint32_t stream_size);
bool asset_stream_next_chunk(asset_manager* manager, uint64_t asset_id);
float asset_get_stream_progress(asset_manager* manager, uint64_t asset_id);

// Import/Export
bool asset_import(asset_manager* manager, const char* source_path, asset_type type);
bool asset_export(asset_manager* manager, uint64_t asset_id, const char* output_path, bool compress);
bool asset_reimport(asset_manager* manager, uint64_t asset_id);

// Hot reload (Editor)
bool asset_hot_reload(asset_manager* manager, uint64_t asset_id);
void asset_register_hot_reload_listener(asset_manager* manager,
                                        void (*on_reload)(uint64_t asset_id, void* user_data),
                                        void* user_data);

// Cooking and compression
bool asset_cook(asset_manager* manager, uint64_t asset_id, bool compress);
bool asset_is_cooked(asset_manager* manager, uint64_t asset_id);

// Versioning
uint32_t asset_get_version(asset_manager* manager, uint64_t asset_id);
bool asset_migrate_version(asset_manager* manager, uint64_t asset_id, uint32_t target_version);

// Statistics
void asset_print_statistics(asset_manager* manager);
void asset_print_memory_report(asset_manager* manager);
void asset_export_memory_profile(asset_manager* manager, const char* filename);

// Advanced: Tagging and categorization
void asset_add_tag(asset_manager* manager, uint64_t asset_id, const char* tag);
void asset_remove_tag(asset_manager* manager, uint64_t asset_id, const char* tag);
bool asset_has_tag(asset_manager* manager, uint64_t asset_id, const char* tag);

// Advanced: Batch operations
void asset_batch_load(asset_manager* manager, const char** paths, asset_type* types, uint32_t count);
void asset_batch_unload(asset_manager* manager, uint64_t* asset_ids, uint32_t count);
void asset_batch_cook(asset_manager* manager, uint64_t* asset_ids, uint32_t count);

// Registry operations
uint32_t asset_get_total_assets(asset_manager* manager);
uint64_t* asset_get_all_assets(asset_manager* manager, uint32_t* out_count);
void asset_rebuild_registry(asset_manager* manager);

// Validation and repair
bool asset_validate_file(asset_manager* manager, const char* path);
bool asset_repair_corrupted(asset_manager* manager, uint64_t asset_id);

// Async queue management
void asset_process_async_loads(asset_manager* manager, uint32_t max_time_ms);
uint32_t asset_get_pending_load_count(asset_manager* manager);
uint32_t asset_get_failed_load_count(asset_manager* manager);

#endif // ASSET_SYSTEM_COMPLETE_H
