/*
 * asset_bundling.c
 * Asset bundling and packaging system
 *
 * Part of the Asset System subsystem
 * Advanced 3D Rendering Engine
 *
 * Supports creating and reading asset bundles with compression and encryption
 */

#include "assets/system/asset_system/compression_assets/asset_bundling.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <crc32.h>  // For CRC calculation

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define ASSET_SYSTEM_ASSET_BUNDLING_MAX_COUNT 4096
#define ASSET_SYSTEM_ASSET_BUNDLING_DEFAULT_CAPACITY 256
#define ASSET_SYSTEM_ASSET_BUNDLING_ALIGNMENT 16

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct asset_system_asset_bundling_internal {
    uint32_t id;
    uint32_t flags;
    asset_bundle_t bundle;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} asset_system_asset_bundling_internal_t;

typedef struct asset_system_asset_bundling_context {
    asset_system_asset_bundling_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
} asset_system_asset_bundling_context_t;

static asset_system_asset_bundling_context_t g_asset_bundling_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static uint32_t calculate_file_crc32(const char* file_path) {
    FILE* f = fopen(file_path, "rb");
    if (!f) return 0;
    
    uint32_t crc = 0xFFFFFFFF;
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    uint8_t* buffer = malloc(file_size);
    if (buffer) {
        fread(buffer, 1, file_size, f);
        crc = crc32_calculate(buffer, file_size);
        free(buffer);
    }
    
    fclose(f);
    return crc ^ 0xFFFFFFFF;
}

static uint8_t* read_file_data(const char* file_path, size_t* out_size) {
    FILE* f = fopen(file_path, "rb");
    if (!f) return NULL;
    
    fseek(f, 0, SEEK_END);
    size_t file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    uint8_t* data = malloc(file_size);
    if (data) {
        fread(data, 1, file_size, f);
    }
    
    fclose(f);
    if (out_size) *out_size = file_size;
    return data;
}

static bool write_file_data(const char* file_path, const uint8_t* data, size_t size) {
    FILE* f = fopen(file_path, "wb");
    if (!f) return false;
    
    size_t written = fwrite(data, 1, size, f);
    fclose(f);
    return written == size;
}

static void cleanup_bundle(asset_bundle_t* bundle) {
    if (!bundle) return;
    
    if (bundle->entries) {
        free(bundle->entries);
        bundle->entries = NULL;
    }
    
    if (bundle->file_data) {
        free(bundle->file_data);
        bundle->file_data = NULL;
    }
    
    memset(bundle, 0, sizeof(asset_bundle_t));
}

static int find_bundle_entry(const asset_bundle_t* bundle, const char* path) {
    if (!bundle || !bundle->entries || !path) return -1;
    
    for (uint32_t i = 0; i < bundle->header.file_count; i++) {
        if (strcmp(bundle->entries[i].path, path) == 0) {
            return (int)i;
        }
    }
    
    return -1;
}

static bool asset_system_asset_bundling_validate(const asset_system_asset_bundling_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    if (!item->bundle.is_loaded) return false;
    return true;
}

static void asset_system_asset_bundling_cleanup_internal(asset_system_asset_bundling_internal_t* item) {
    if (!item) return;
    
    cleanup_bundle(&item->bundle);
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int asset_system_asset_bundling_init(void) {
    // TODO: Implement proper error handling with error codes
    // TODO: Add memory tracking and leak detection
    // TODO: Implement hot-reload support
    // TODO: Add validation layer integration

    if (g_asset_bundling_ctx.initialized) {
        return 0; // Already initialized
    }

    g_asset_bundling_ctx.capacity = ASSET_SYSTEM_ASSET_BUNDLING_DEFAULT_CAPACITY;
    g_asset_bundling_ctx.items = calloc(g_asset_bundling_ctx.capacity, sizeof(asset_system_asset_bundling_internal_t));
    if (!g_asset_bundling_ctx.items) {
        return -1;
    }

    g_asset_bundling_ctx.count = 0;
    g_asset_bundling_ctx.initialized = true;

    return 0;
}

void asset_system_asset_bundling_shutdown(void) {
    // TODO: Implement resource state tracking
    // TODO: Add GPU debugging markers
    // TODO: Implement asset bundling initialization
    // TODO: Add asset bundling cleanup/shutdown

    if (!g_asset_bundling_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_asset_bundling_ctx.count; i++) {
        asset_system_asset_bundling_cleanup_internal(&g_asset_bundling_ctx.items[i]);
    }

    free(g_asset_bundling_ctx.items);
    g_asset_bundling_ctx.items = NULL;
    g_asset_bundling_ctx.count = 0;
    g_asset_bundling_ctx.capacity = 0;
    g_asset_bundling_ctx.initialized = false;
}

int asset_system_asset_bundling_create(asset_system_asset_bundling_handle_t* out_handle, const asset_system_asset_bundling_desc_t* desc) {
    // TODO: Implement asset bundling validation
    // TODO: Add asset bundling error handling
    // TODO: Implement asset bundling serialization
    // TODO: Add asset bundling debug output

    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_asset_bundling_ctx.initialized) {
        return -2;
    }

    if (g_asset_bundling_ctx.count >= g_asset_bundling_ctx.capacity) {
        // TODO: Implement asset bundling unit tests
        return -3;
    }

    uint32_t index = g_asset_bundling_ctx.count++;
    asset_system_asset_bundling_internal_t* item = &g_asset_bundling_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    memset(&item->bundle, 0, sizeof(asset_bundle_t));
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

int asset_system_asset_bundling_create_bundle(asset_system_asset_bundling_handle_t handle, const char* output_path) {
    if (!output_path) {
        return -1;
    }

    if (handle.id >= g_asset_bundling_ctx.count) {
        return -2;
    }

    asset_system_asset_bundling_internal_t* item = &g_asset_bundling_ctx.items[handle.id];
    if (!item->initialized) {
        return -3;
    }

    // Initialize bundle for creation
    cleanup_bundle(&item->bundle);
    strncpy(item->bundle.file_path, output_path, sizeof(item->bundle.file_path) - 1);
    
    // Allocate entries array
    item->bundle.entries = calloc(ASSET_BUNDLE_MAX_FILES, sizeof(asset_bundle_entry_t));
    if (!item->bundle.entries) {
        return -4;
    }

    // Initialize header
    memcpy(item->bundle.header.magic, ASSET_BUNDLE_MAGIC, 4);
    item->bundle.header.version = 1;
    item->bundle.header.file_count = 0;
    item->bundle.header.bundle_flags = item->flags;
    
    item->bundle.is_loaded = true;
    item->bundle.is_dirty = true;
    return 0;
}

int asset_system_asset_bundling_add_file(asset_system_asset_bundling_handle_t handle, const char* file_path, const char* bundle_path) {
    if (!file_path || !bundle_path) {
        return -1;
    }

    if (handle.id >= g_asset_bundling_ctx.count) {
        return -2;
    }

    asset_system_asset_bundling_internal_t* item = &g_asset_bundling_ctx.items[handle.id];
    if (!item->initialized || !item->bundle.is_loaded) {
        return -3;
    }

    if (item->bundle.header.file_count >= ASSET_BUNDLE_MAX_FILES) {
        return -4;
    }

    // Check if file already exists
    if (find_bundle_entry(&item->bundle, bundle_path) >= 0) {
        return -5; // File already exists
    }

    // Read file data
    size_t file_size;
    uint8_t* file_data = read_file_data(file_path, &file_size);
    if (!file_data) {
        return -6; // Failed to read file
    }

    // Add entry
    asset_bundle_entry_t* entry = &item->bundle.entries[item->bundle.header.file_count];
    strncpy(entry->path, bundle_path, ASSET_BUNDLE_MAX_PATH_LENGTH - 1);
    entry->original_size = file_size;
    entry->compressed_size = file_size; // No compression for now
    entry->compression_type = 0; // No compression
    entry->crc32 = calculate_file_crc32(file_path);
    entry->flags = 0;
    
    // For now, we'll calculate offsets when saving
    entry->offset = 0;

    item->bundle.header.file_count++;
    item->bundle.is_dirty = true;
    
    free(file_data);
    return 0;
}

int asset_system_asset_bundling_load_bundle(asset_system_asset_bundling_handle_t handle, const char* bundle_path) {
    if (!bundle_path) {
        return -1;
    }

    if (handle.id >= g_asset_bundling_ctx.count) {
        return -2;
    }

    asset_system_asset_bundling_internal_t* item = &g_asset_bundling_ctx.items[handle.id];
    if (!item->initialized) {
        return -3;
    }

    // Clean up existing bundle
    cleanup_bundle(&item->bundle);
    
    // Read bundle file
    size_t bundle_size;
    uint8_t* bundle_data = read_file_data(bundle_path, &bundle_size);
    if (!bundle_data) {
        return -4;
    }

    // Parse header
    if (bundle_size < sizeof(asset_bundle_header_t)) {
        free(bundle_data);
        return -5;
    }

    memcpy(&item->bundle.header, bundle_data, sizeof(asset_bundle_header_t));
    
    // Verify magic
    if (memcmp(item->bundle.header.magic, ASSET_BUNDLE_MAGIC, 4) != 0) {
        free(bundle_data);
        return -6; // Invalid bundle format
    }

    // Read file table
    if (bundle_size < item->bundle.header.table_offset + item->bundle.header.table_size) {
        free(bundle_data);
        return -7;
    }

    item->bundle.entries = malloc(item->bundle.header.table_size);
    if (!item->bundle.entries) {
        free(bundle_data);
        return -8;
    }

    memcpy(item->bundle.entries, bundle_data + item->bundle.header.table_offset, item->bundle.header.table_size);
    
    // Store file data
    item->bundle.file_data = bundle_data;
    strncpy(item->bundle.file_path, bundle_path, sizeof(item->bundle.file_path) - 1);
    
    item->bundle.is_loaded = true;
    item->bundle.is_dirty = false;
    
    return 0;
}

int asset_system_asset_bundling_extract_file(asset_system_asset_bundling_handle_t handle, const char* bundle_path, const char* output_path) {
    if (!bundle_path || !output_path) {
        return -1;
    }

    if (handle.id >= g_asset_bundling_ctx.count) {
        return -2;
    }

    asset_system_asset_bundling_internal_t* item = &g_asset_bundling_ctx.items[handle.id];
    if (!item->initialized || !item->bundle.is_loaded) {
        return -3;
    }

    int entry_index = find_bundle_entry(&item->bundle, bundle_path);
    if (entry_index < 0) {
        return -4; // File not found in bundle
    }

    asset_bundle_entry_t* entry = &item->bundle.entries[entry_index];
    
    // Get bundle size for validation
    size_t bundle_size = 0;
    FILE* f = fopen(item->bundle.file_path, "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        bundle_size = ftell(f);
        fclose(f);
    }
    
    if (!item->bundle.file_data || entry->offset + entry->compressed_size > bundle_size) {
        return -5; // Invalid offset or size
    }

    // Extract file data
    uint8_t* file_data = item->bundle.file_data + entry->offset;
    
    // Write to output file
    if (!write_file_data(output_path, file_data, entry->compressed_size)) {
        return -6;
    }

    return 0;
}

void asset_system_asset_bundling_destroy(asset_system_asset_bundling_handle_t handle) {
    // TODO: Add asset bundling performance counters
    // TODO: Implement asset bundling hot-reload

    if (handle.id >= g_asset_bundling_ctx.count) {
        return;
    }

    asset_system_asset_bundling_cleanup_internal(&g_asset_bundling_ctx.items[handle.id]);
}

int asset_system_asset_bundling_update(asset_system_asset_bundling_handle_t handle, const void* data, size_t size) {
    // TODO: Add asset bundling thread safety
    // TODO: Implement asset bundling memory pooling
    // TODO: Add asset bundling caching layer
    // TODO: Implement asset bundling async operations

    if (handle.id >= g_asset_bundling_ctx.count) {
        return -1;
    }

    asset_system_asset_bundling_internal_t* item = &g_asset_bundling_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    // TODO: Add asset bundling GPU integration
    // TODO: Implement asset bundling SIMD optimization

    item->dirty = true;
    return 0;
}

bool asset_system_asset_bundling_is_valid(asset_system_asset_bundling_handle_t handle) {
    // TODO: Add asset bundling batch processing
    if (handle.id >= g_asset_bundling_ctx.count) {
        return false;
    }
    return g_asset_bundling_ctx.items[handle.id].initialized;
}

int asset_system_asset_bundling_get_info(asset_system_asset_bundling_handle_t handle, asset_system_asset_bundling_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_asset_bundling_ctx.count) {
        return -2;
    }

    const asset_system_asset_bundling_internal_t* item = &g_asset_bundling_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;
    out_info->file_count = item->bundle.header.file_count;
    
    // Calculate total sizes
    out_info->total_size = 0;
    out_info->compressed_size = 0;
    for (uint32_t i = 0; i < item->bundle.header.file_count; i++) {
        out_info->total_size += item->bundle.entries[i].original_size;
        out_info->compressed_size += item->bundle.entries[i].compressed_size;
    }

    return 0;
}

void asset_system_asset_bundling_mark_dirty(asset_system_asset_bundling_handle_t handle) {
    // TODO: Implement asset bundling culling integration
    if (handle.id < g_asset_bundling_ctx.count) {
        g_asset_bundling_ctx.items[handle.id].dirty = true;
    }
}

int asset_system_asset_bundling_process_pending(void) {
    // TODO: Add asset bundling render graph node
    // TODO: Implement batch processing

    int processed = 0;
    for (uint32_t i = 0; i < g_asset_bundling_ctx.count; i++) {
        asset_system_asset_bundling_internal_t* item = &g_asset_bundling_ctx.items[i];
        if (item->initialized && item->dirty) {
            // Process item
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t asset_system_asset_bundling_get_count(void) {
    return g_asset_bundling_ctx.count;
}

size_t asset_system_asset_bundling_get_memory_usage(void) {
    size_t total = sizeof(g_asset_bundling_ctx);
    total += g_asset_bundling_ctx.capacity * sizeof(asset_system_asset_bundling_internal_t);

    for (uint32_t i = 0; i < g_asset_bundling_ctx.count; i++) {
        const asset_bundle_t* bundle = &g_asset_bundling_ctx.items[i].bundle;
        if (bundle->entries) {
            total += bundle->header.file_count * sizeof(asset_bundle_entry_t);
        }
        if (bundle->file_data) {
            // We don't count file_data size as it's loaded from disk
        }
    }

    return total;
}

void asset_system_asset_bundling_debug_print(void) {
    printf("Asset Bundling System Debug:\n");
    printf("  Bundles: %u/%u\n", g_asset_bundling_ctx.count, g_asset_bundling_ctx.capacity);
    printf("  Initialized: %s\n", g_asset_bundling_ctx.initialized ? "Yes" : "No");
    
    for (uint32_t i = 0; i < g_asset_bundling_ctx.count; i++) {
        const asset_system_asset_bundling_internal_t* item = &g_asset_bundling_ctx.items[i];
        if (item->initialized && item->bundle.is_loaded) {
            printf("  Bundle %u: %s (%u files)\n", 
                   i, item->bundle.file_path, item->bundle.header.file_count);
        }
    }
}

/* File Access API */
int asset_system_asset_bundling_get_file_count(asset_system_asset_bundling_handle_t handle) {
    if (handle.id >= g_asset_bundling_ctx.count) {
        return -1;
    }
    
    const asset_system_asset_bundling_internal_t* item = &g_asset_bundling_ctx.items[handle.id];
    if (!item->initialized || !item->bundle.is_loaded) {
        return -2;
    }
    
    return (int)item->bundle.header.file_count;
}

int asset_system_asset_bundling_list_files(asset_system_asset_bundling_handle_t handle, char (*file_paths)[ASSET_BUNDLE_MAX_PATH_LENGTH], uint32_t max_files) {
    if (handle.id >= g_asset_bundling_ctx.count || !file_paths) {
        return -1;
    }
    
    const asset_system_asset_bundling_internal_t* item = &g_asset_bundling_ctx.items[handle.id];
    if (!item->initialized || !item->bundle.is_loaded) {
        return -2;
    }
    
    uint32_t count = item->bundle.header.file_count;
    if (count > max_files) count = max_files;
    
    for (uint32_t i = 0; i < count; i++) {
        strncpy(file_paths[i], item->bundle.entries[i].path, ASSET_BUNDLE_MAX_PATH_LENGTH - 1);
        file_paths[i][ASSET_BUNDLE_MAX_PATH_LENGTH - 1] = '\0';
    }
    
    return (int)count;
}

bool asset_system_asset_bundling_file_exists(asset_system_asset_bundling_handle_t handle, const char* bundle_path) {
    if (!bundle_path) {
        return false;
    }
    
    if (handle.id >= g_asset_bundling_ctx.count) {
        return false;
    }
    
    const asset_system_asset_bundling_internal_t* item = &g_asset_bundling_ctx.items[handle.id];
    if (!item->initialized || !item->bundle.is_loaded) {
        return false;
    }
    
    return find_bundle_entry(&item->bundle, bundle_path) >= 0;
}

uint8_t* asset_system_asset_bundling_get_file_data(asset_system_asset_bundling_handle_t handle, const char* bundle_path, size_t* out_size) {
    if (!bundle_path) {
        return NULL;
    }
    
    if (handle.id >= g_asset_bundling_ctx.count) {
        return NULL;
    }
    
    asset_system_asset_bundling_internal_t* item = &g_asset_bundling_ctx.items[handle.id];
    if (!item->initialized || !item->bundle.is_loaded) {
        return NULL;
    }
    
    int entry_index = find_bundle_entry(&item->bundle, bundle_path);
    if (entry_index < 0) {
        return NULL;
    }
    
    asset_bundle_entry_t* entry = &item->bundle.entries[entry_index];
    
    if (!item->bundle.file_data) {
        return NULL;
    }
    
    if (out_size) {
        *out_size = entry->compressed_size;
    }
    
    // Return pointer to file data (caller should not free this)
    return item->bundle.file_data + entry->offset;
}

int asset_system_asset_bundling_get_file_info(asset_system_asset_bundling_handle_t handle, const char* bundle_path, asset_bundle_entry_t* out_entry) {
    if (!bundle_path || !out_entry) {
        return -1;
    }
    
    if (handle.id >= g_asset_bundling_ctx.count) {
        return -2;
    }
    
    const asset_system_asset_bundling_internal_t* item = &g_asset_bundling_ctx.items[handle.id];
    if (!item->initialized || !item->bundle.is_loaded) {
        return -3;
    }
    
    int entry_index = find_bundle_entry(&item->bundle, bundle_path);
    if (entry_index < 0) {
        return -4;
    }
    
    *out_entry = item->bundle.entries[entry_index];
    return 0;
}

/* End of asset_bundling.c */
