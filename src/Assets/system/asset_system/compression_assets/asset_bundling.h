/*
 * asset_bundling.h
 * Asset bundling and packaging system
 *
 * Part of the Asset System subsystem
 * Advanced 3D Rendering Engine
 *
 * Supports creating and reading asset bundles with compression and encryption
 */

#ifndef ASSET_SYSTEM_ASSET_BUNDLING_H
#define ASSET_SYSTEM_ASSET_BUNDLING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define ASSET_BUNDLE_MAGIC "ASB1"  // Asset Bundle v1
#define ASSET_BUNDLE_MAX_FILES 4096
#define ASSET_BUNDLE_MAX_PATH_LENGTH 256
#define ASSET_BUNDLE_COMPRESSION_LZ4 1
#define ASSET_BUNDLE_COMPRESSION_ZSTD 2
#define ASSET_BUNDLE_ENCRYPTION_AES 1

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct asset_bundle_entry {
    char path[ASSET_BUNDLE_MAX_PATH_LENGTH];
    uint64_t offset;
    uint64_t compressed_size;
    uint64_t original_size;
    uint32_t crc32;
    uint16_t compression_type;
    uint16_t flags;
} asset_bundle_entry_t;

typedef struct asset_bundle_header {
    char magic[4];
    uint32_t version;
    uint32_t file_count;
    uint64_t table_offset;
    uint64_t table_size;
    uint32_t bundle_flags;
    uint32_t reserved[3];
} asset_bundle_header_t;

typedef struct asset_bundle {
    asset_bundle_header_t header;
    asset_bundle_entry_t* entries;
    uint8_t* file_data;
    char file_path[512];
    bool is_loaded;
    bool is_dirty;
} asset_bundle_t;

typedef struct asset_system_asset_bundling_handle {
    uint32_t id;
} asset_system_asset_bundling_handle_t;

typedef struct asset_system_asset_bundling_desc {
    uint32_t flags;
    uint32_t compression_type;
    uint32_t encryption_type;
    void* user_data;
} asset_system_asset_bundling_desc_t;

typedef struct asset_system_asset_bundling_info {
    uint32_t id;
    uint32_t flags;
    uint32_t file_count;
    uint64_t total_size;
    uint64_t compressed_size;
    bool initialized;
} asset_system_asset_bundling_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int asset_system_asset_bundling_init(void);
void asset_system_asset_bundling_shutdown(void);

/* Lifecycle */
int asset_system_asset_bundling_create(asset_system_asset_bundling_handle_t* out_handle, const asset_system_asset_bundling_desc_t* desc);
int asset_system_asset_bundling_create_bundle(asset_system_asset_bundling_handle_t handle, const char* output_path);
int asset_system_asset_bundling_add_file(asset_system_asset_bundling_handle_t handle, const char* file_path, const char* bundle_path);
int asset_system_asset_bundling_load_bundle(asset_system_asset_bundling_handle_t handle, const char* bundle_path);
int asset_system_asset_bundling_extract_file(asset_system_asset_bundling_handle_t handle, const char* bundle_path, const char* output_path);
void asset_system_asset_bundling_destroy(asset_system_asset_bundling_handle_t handle);

/* Bundle Operations */
int asset_system_asset_bundling_update(asset_system_asset_bundling_handle_t handle, const void* data, size_t size);
bool asset_system_asset_bundling_is_valid(asset_system_asset_bundling_handle_t handle);
int asset_system_asset_bundling_get_info(asset_system_asset_bundling_handle_t handle, asset_system_asset_bundling_info_t* out_info);
void asset_system_asset_bundling_mark_dirty(asset_system_asset_bundling_handle_t handle);
int asset_system_asset_bundling_process_pending(void);

/* File Access */
int asset_system_asset_bundling_get_file_count(asset_system_asset_bundling_handle_t handle);
int asset_system_asset_bundling_list_files(asset_system_asset_bundling_handle_t handle, char (*file_paths)[ASSET_BUNDLE_MAX_PATH_LENGTH], uint32_t max_files);
bool asset_system_asset_bundling_file_exists(asset_system_asset_bundling_handle_t handle, const char* bundle_path);
uint8_t* asset_system_asset_bundling_get_file_data(asset_system_asset_bundling_handle_t handle, const char* bundle_path, size_t* out_size);
int asset_system_asset_bundling_get_file_info(asset_system_asset_bundling_handle_t handle, const char* bundle_path, asset_bundle_entry_t* out_entry);

/* Statistics */
uint32_t asset_system_asset_bundling_get_count(void);
size_t asset_system_asset_bundling_get_memory_usage(void);
void asset_system_asset_bundling_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ASSET_SYSTEM_ASSET_BUNDLING_H */
