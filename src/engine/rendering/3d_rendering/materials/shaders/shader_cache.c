#include "shader_cache.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define MATERIALS_SHADER_CACHE_MAX_COUNT 4096
#define MATERIALS_SHADER_CACHE_DEFAULT_CAPACITY 256
#define MATERIALS_SHADER_CACHE_DIR ".shader_cache/bin"

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct materials_shader_cache_internal {
    uint32_t id;
    uint32_t flags;
    uint64_t source_hash;
    void* spirv_data;
    size_t spirv_size;
    bool initialized;
    bool dirty;
} materials_shader_cache_internal_t;

typedef struct materials_shader_cache_context {
    materials_shader_cache_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} materials_shader_cache_context_t;

static materials_shader_cache_context_t g_shader_cache_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

// DJB2 hash for string/binary data
static uint64_t materials_shader_cache_hash(const void* data, size_t size) {
    uint64_t hash = 5381;
    const uint8_t* bytes = (const uint8_t*)data;
    for (size_t i = 0; i < size; i++) {
        hash = ((hash << 5) + hash) + bytes[i];
    }
    return hash;
}

static void materials_shader_cache_cleanup_internal(materials_shader_cache_internal_t* item) {
    if (!item) return;
    if (item->spirv_data) {
        free(item->spirv_data);
        item->spirv_data = NULL;
    }
    item->spirv_size = 0;
    item->source_hash = 0;
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int materials_shader_cache_init(void) {
    if (g_shader_cache_ctx.initialized) {
        return 0; // Already initialized
    }

    g_shader_cache_ctx.capacity = MATERIALS_SHADER_CACHE_DEFAULT_CAPACITY;
    g_shader_cache_ctx.items = calloc(g_shader_cache_ctx.capacity, sizeof(materials_shader_cache_internal_t));
    if (!g_shader_cache_ctx.items) {
        return -1;
    }

    // Ensure cache directory exists
    system("mkdir -p " MATERIALS_SHADER_CACHE_DIR);

    g_shader_cache_ctx.count = 0;
    g_shader_cache_ctx.initialized = true;

    return 0;
}

void materials_shader_cache_shutdown(void) {
    if (!g_shader_cache_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_shader_cache_ctx.count; i++) {
        materials_shader_cache_cleanup_internal(&g_shader_cache_ctx.items[i]);
    }

    free(g_shader_cache_ctx.items);
    g_shader_cache_ctx.items = NULL;
    g_shader_cache_ctx.count = 0;
    g_shader_cache_ctx.capacity = 0;
    g_shader_cache_ctx.initialized = false;
}

int materials_shader_cache_create(materials_shader_cache_handle_t* out_handle, const materials_shader_cache_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_shader_cache_ctx.initialized) {
        return -2;
    }

    if (g_shader_cache_ctx.count >= g_shader_cache_ctx.capacity) {
        return -3;
    }

    uint32_t index = g_shader_cache_ctx.count++;
    materials_shader_cache_internal_t* item = &g_shader_cache_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->spirv_data = NULL;
    item->spirv_size = 0;
    item->source_hash = 0;
    item->initialized = true;
    item->dirty = true;

    out_handle->id = index;
    return 0;
}

void materials_shader_cache_destroy(materials_shader_cache_handle_t handle) {
    if (handle.id >= g_shader_cache_ctx.count) {
        return;
    }

    materials_shader_cache_cleanup_internal(&g_shader_cache_ctx.items[handle.id]);
}

int materials_shader_cache_update(materials_shader_cache_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_shader_cache_ctx.count) {
        return -1;
    }

    materials_shader_cache_internal_t* item = &g_shader_cache_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    // data is SPIR-V or source if we are checking for hits
    // For now, assume this update sets the "content" we are caching
    item->source_hash = materials_shader_cache_hash(data, size);
    
    char cache_path[256];
    snprintf(cache_path, sizeof(cache_path), MATERIALS_SHADER_CACHE_DIR "/%llx.spv", item->source_hash);

    // Try to load from disk if not dirty
    FILE* f = fopen(cache_path, "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        size_t spv_size = ftell(f);
        fseek(f, 0, SEEK_SET);

        void* spv_data = malloc(spv_size);
        if (spv_data) {
            fread(spv_data, 1, spv_size, f);
            if (item->spirv_data) free(item->spirv_data);
            item->spirv_data = spv_data;
            item->spirv_size = spv_size;
            item->dirty = false;
        }
        fclose(f);
    }

    return 0;
}

bool materials_shader_cache_is_valid(materials_shader_cache_handle_t handle) {
    if (handle.id >= g_shader_cache_ctx.count) {
        return false;
    }
    return g_shader_cache_ctx.items[handle.id].initialized && !g_shader_cache_ctx.items[handle.id].dirty;
}

int materials_shader_cache_get_info(materials_shader_cache_handle_t handle, materials_shader_cache_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_shader_cache_ctx.count) {
        return -2;
    }

    const materials_shader_cache_internal_t* item = &g_shader_cache_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void materials_shader_cache_mark_dirty(materials_shader_cache_handle_t handle) {
    if (handle.id < g_shader_cache_ctx.count) {
        g_shader_cache_ctx.items[handle.id].dirty = true;
    }
}

int materials_shader_cache_process_pending(void) {
    return 0;
}

uint32_t materials_shader_cache_get_count(void) {
    return g_shader_cache_ctx.count;
}

size_t materials_shader_cache_get_memory_usage(void) {
    size_t total = sizeof(g_shader_cache_ctx);
    total += g_shader_cache_ctx.capacity * sizeof(materials_shader_cache_internal_t);
    for (uint32_t i = 0; i < g_shader_cache_ctx.count; i++) {
        total += g_shader_cache_ctx.items[i].spirv_size;
    }
    return total;
}

void materials_shader_cache_debug_print(void) {
    printf("Shader Cache Stats:\n");
    printf("  Count: %u\n", g_shader_cache_ctx.count);
    printf("  Memory: %zu bytes\n", materials_shader_cache_get_memory_usage());
}

