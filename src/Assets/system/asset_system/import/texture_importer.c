/*
 * texture_importer.c
 * Texture importer
 *
 * Part of the Asset System subsystem
 * Advanced 3D Rendering Engine
 *
 * Supports PNG, JPEG, TGA, BMP, PSD, GIF, HDR, PIC formats via stb_image
 */

#include "assets/system/asset_system/import/texture_importer.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// stb_image implementation for texture loading
#define STB_IMAGE_IMPLEMENTATION
#include "include/vendor/stb_image.h"

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define ASSET_SYSTEM_TEXTURE_IMPORTER_MAX_COUNT 4096
#define ASSET_SYSTEM_TEXTURE_IMPORTER_DEFAULT_CAPACITY 256
#define ASSET_SYSTEM_TEXTURE_IMPORTER_ALIGNMENT 16

/* ============================================================================
 * TEXTURE TYPES
 * ============================================================================ */

typedef enum {
    TEXTURE_FORMAT_UNKNOWN = 0,
    TEXTURE_FORMAT_R8,
    TEXTURE_FORMAT_RG8,
    TEXTURE_FORMAT_RGB8,
    TEXTURE_FORMAT_RGBA8,
    TEXTURE_FORMAT_R16F,
    TEXTURE_FORMAT_RG16F,
    TEXTURE_FORMAT_RGB16F,
    TEXFMT_RGBA16F
} texture_format_t;

typedef struct texture_data {
    int width;
    int height;
    int channels;
    texture_format_t format;
    uint8_t* pixels;
    size_t size_bytes;
    char file_path[512];
} texture_data_t;

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct asset_system_texture_importer_internal {
    uint32_t id;
    uint32_t flags;
    texture_data_t texture;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} asset_system_texture_importer_internal_t;

typedef struct asset_system_texture_importer_context {
    asset_system_texture_importer_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
} asset_system_texture_importer_context_t;

static asset_system_texture_importer_context_t g_texture_importer_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static texture_format_t get_texture_format_from_channels(int channels) {
    switch (channels) {
        case 1: return TEXTURE_FORMAT_R8;
        case 2: return TEXTURE_FORMAT_RG8;
        case 3: return TEXTURE_FORMAT_RGB8;
        case 4: return TEXTURE_FORMAT_RGBA8;
        default: return TEXTURE_FORMAT_UNKNOWN;
    }
}

static bool load_texture_from_file(const char* file_path, texture_data_t* out_texture) {
    if (!file_path || !out_texture) {
        return false;
    }
    
    // Use stb_image to load the texture
    int width, height, channels;
    uint8_t* pixels = stbi_load(file_path, &width, &height, &channels, 0);
    
    if (!pixels) {
        printf("Failed to load texture: %s\n", stbi_failure_reason());
        return false;
    }
    
    // Fill texture data
    out_texture->width = width;
    out_texture->height = height;
    out_texture->channels = channels;
    out_texture->format = get_texture_format_from_channels(channels);
    out_texture->pixels = pixels;
    out_texture->size_bytes = width * height * channels;
    strncpy(out_texture->file_path, file_path, sizeof(out_texture->file_path) - 1);
    
    printf("Loaded texture: %s (%dx%d, %d channels)\n", file_path, width, height, channels);
    return true;
}

static void cleanup_texture_data(texture_data_t* texture) {
    if (!texture) return;
    
    if (texture->pixels) {
        stbi_image_free(texture->pixels);
        texture->pixels = NULL;
    }
    
    memset(texture, 0, sizeof(texture_data_t));
}

static bool asset_system_texture_importer_validate(const asset_system_texture_importer_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    if (!item->texture.pixels) return false;
    if (item->texture.width <= 0 || item->texture.height <= 0) return false;
    return true;
}

static void asset_system_texture_importer_cleanup_internal(asset_system_texture_importer_internal_t* item) {
    if (!item) return;
    
    cleanup_texture_data(&item->texture);
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int asset_system_texture_importer_init(void) {
    // TODO: Implement proper error handling with error codes
    // TODO: Add memory tracking and leak detection
    // TODO: Implement hot-reload support
    // TODO: Add validation layer integration

    if (g_texture_importer_ctx.initialized) {
        return 0; // Already initialized
    }

    g_texture_importer_ctx.capacity = ASSET_SYSTEM_TEXTURE_IMPORTER_DEFAULT_CAPACITY;
    g_texture_importer_ctx.items = calloc(g_texture_importer_ctx.capacity, sizeof(asset_system_texture_importer_internal_t));
    if (!g_texture_importer_ctx.items) {
        return -1;
    }

    g_texture_importer_ctx.count = 0;
    g_texture_importer_ctx.initialized = true;

    return 0;
}

void asset_system_texture_importer_shutdown(void) {
    // TODO: Implement resource state tracking
    // TODO: Add GPU debugging markers
    // TODO: Implement texture importer initialization
    // TODO: Add texture importer cleanup/shutdown

    if (!g_texture_importer_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_texture_importer_ctx.count; i++) {
        asset_system_texture_importer_cleanup_internal(&g_texture_importer_ctx.items[i]);
    }

    free(g_texture_importer_ctx.items);
    g_texture_importer_ctx.items = NULL;
    g_texture_importer_ctx.count = 0;
    g_texture_importer_ctx.capacity = 0;
    g_texture_importer_ctx.initialized = false;
}

int asset_system_texture_importer_create(asset_system_texture_importer_handle_t* out_handle, const asset_system_texture_importer_desc_t* desc) {
    // TODO: Implement texture importer validation
    // TODO: Add texture importer error handling
    // TODO: Implement texture importer serialization
    // TODO: Add texture importer debug output

    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_texture_importer_ctx.initialized) {
        return -2;
    }

    if (g_texture_importer_ctx.count >= g_texture_importer_ctx.capacity) {
        // TODO: Implement texture importer unit tests
        return -3;
    }

    uint32_t index = g_texture_importer_ctx.count++;
    asset_system_texture_importer_internal_t* item = &g_texture_importer_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    memset(&item->texture, 0, sizeof(texture_data_t));
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

int asset_system_texture_importer_load_from_file(asset_system_texture_importer_handle_t handle, const char* file_path) {
    if (!file_path) {
        return -1;
    }

    if (handle.id >= g_texture_importer_ctx.count) {
        return -2;
    }

    asset_system_texture_importer_internal_t* item = &g_texture_importer_ctx.items[handle.id];
    if (!item->initialized) {
        return -3;
    }

    // Clean up any existing texture data
    cleanup_texture_data(&item->texture);

    // Load new texture
    if (!load_texture_from_file(file_path, &item->texture)) {
        return -4;
    }

    item->dirty = true;
    return 0;
}

void asset_system_texture_importer_destroy(asset_system_texture_importer_handle_t handle) {
    // TODO: Add texture importer performance counters
    // TODO: Implement texture importer hot-reload

    if (handle.id >= g_texture_importer_ctx.count) {
        return;
    }

    asset_system_texture_importer_cleanup_internal(&g_texture_importer_ctx.items[handle.id]);
}

int asset_system_texture_importer_update(asset_system_texture_importer_handle_t handle, const void* data, size_t size) {
    // TODO: Add texture importer thread safety
    // TODO: Implement texture importer memory pooling
    // TODO: Add texture importer caching layer
    // TODO: Implement texture importer async operations

    if (handle.id >= g_texture_importer_ctx.count) {
        return -1;
    }

    asset_system_texture_importer_internal_t* item = &g_texture_importer_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    // TODO: Add texture importer GPU integration
    // TODO: Implement texture importer SIMD optimization

    item->dirty = true;
    return 0;
}

bool asset_system_texture_importer_is_valid(asset_system_texture_importer_handle_t handle) {
    // TODO: Add texture importer batch processing
    if (handle.id >= g_texture_importer_ctx.count) {
        return false;
    }
    return g_texture_importer_ctx.items[handle.id].initialized;
}

int asset_system_texture_importer_get_info(asset_system_texture_importer_handle_t handle, asset_system_texture_importer_info_t* out_info) {
    // TODO: Implement texture importer streaming support
    // TODO: Add texture importer LOD support

    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_texture_importer_ctx.count) {
        return -2;
    }

    const asset_system_texture_importer_internal_t* item = &g_texture_importer_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void asset_system_texture_importer_mark_dirty(asset_system_texture_importer_handle_t handle) {
    // TODO: Implement texture importer culling integration
    if (handle.id < g_texture_importer_ctx.count) {
        g_texture_importer_ctx.items[handle.id].dirty = true;
    }
}

int asset_system_texture_importer_process_pending(void) {
    // TODO: Add texture importer render graph node
    // TODO: Implement batch processing

    int processed = 0;
    for (uint32_t i = 0; i < g_texture_importer_ctx.count; i++) {
        asset_system_texture_importer_internal_t* item = &g_texture_importer_ctx.items[i];
        if (item->initialized && item->dirty) {
            // Process item
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t asset_system_texture_importer_get_count(void) {
    return g_texture_importer_ctx.count;
}

size_t asset_system_texture_importer_get_memory_usage(void) {
    size_t total = sizeof(g_texture_importer_ctx);
    total += g_texture_importer_ctx.capacity * sizeof(asset_system_texture_importer_internal_t);

    for (uint32_t i = 0; i < g_texture_importer_ctx.count; i++) {
        total += g_texture_importer_ctx.items[i].texture.size_bytes;
    }

    return total;
}

void asset_system_texture_importer_debug_print(void) {
    printf("Texture Importer Debug Info:\n");
    printf("  Count: %u/%u\n", g_texture_importer_ctx.count, g_texture_importer_ctx.capacity);
    printf("  Initialized: %s\n", g_texture_importer_ctx.initialized ? "Yes" : "No");
    
    for (uint32_t i = 0; i < g_texture_importer_ctx.count; i++) {
        const asset_system_texture_importer_internal_t* item = &g_texture_importer_ctx.items[i];
        if (item->initialized && item->texture.pixels) {
            printf("  Texture %u: %s (%dx%d, %d channels, %zu bytes)\n",
                   i, item->texture.file_path, item->texture.width, 
                   item->texture.height, item->texture.channels, item->texture.size_bytes);
        }
    }
}

/* Texture Data Access API */
int asset_system_texture_importer_get_dimensions(asset_system_texture_importer_handle_t handle, int* out_width, int* out_height) {
    if (handle.id >= g_texture_importer_ctx.count || !out_width || !out_height) {
        return -1;
    }
    
    const asset_system_texture_importer_internal_t* item = &g_texture_importer_ctx.items[handle.id];
    if (!item->initialized || !item->texture.pixels) {
        return -2;
    }
    
    *out_width = item->texture.width;
    *out_height = item->texture.height;
    return 0;
}

int asset_system_texture_importer_get_channels(asset_system_texture_importer_handle_t handle, int* out_channels) {
    if (handle.id >= g_texture_importer_ctx.count || !out_channels) {
        return -1;
    }
    
    const asset_system_texture_importer_internal_t* item = &g_texture_importer_ctx.items[handle.id];
    if (!item->initialized || !item->texture.pixels) {
        return -2;
    }
    
    *out_channels = item->texture.channels;
    return 0;
}

const uint8_t* asset_system_texture_importer_get_pixels(asset_system_texture_importer_handle_t handle) {
    if (handle.id >= g_texture_importer_ctx.count) {
        return NULL;
    }
    
    const asset_system_texture_importer_internal_t* item = &g_texture_importer_ctx.items[handle.id];
    if (!item->initialized || !item->texture.pixels) {
        return NULL;
    }
    
    return item->texture.pixels;
}

size_t asset_system_texture_importer_get_size(asset_system_texture_importer_handle_t handle) {
    if (handle.id >= g_texture_importer_ctx.count) {
        return 0;
    }
    
    const asset_system_texture_importer_internal_t* item = &g_texture_importer_ctx.items[handle.id];
    if (!item->initialized || !item->texture.pixels) {
        return 0;
    }
    
    return item->texture.size_bytes;
}

/* End of texture_importer.c */
