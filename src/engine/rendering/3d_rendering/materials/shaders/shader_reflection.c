#include "shader_reflection.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define MATERIALS_SHADER_REFLECTION_MAX_COUNT 4096
#define MATERIALS_SHADER_REFLECTION_DEFAULT_CAPACITY 256
#define MATERIALS_SHADER_REFLECTION_TEMP_DIR ".shader_cache/temp"

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct materials_shader_binding {
    uint32_t set;
    uint32_t binding;
    char name[64];
} materials_shader_binding_t;

typedef struct materials_shader_reflection_internal {
    uint32_t id;
    uint32_t flags;
    materials_shader_binding_t* bindings;
    uint32_t binding_count;
    uint32_t push_constant_size;
    bool initialized;
    bool dirty;
} materials_shader_reflection_internal_t;

typedef struct materials_shader_reflection_context {
    materials_shader_reflection_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} materials_shader_reflection_context_t;

static materials_shader_reflection_context_t g_shader_reflection_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void materials_shader_reflection_cleanup_internal(materials_shader_reflection_internal_t* item) {
    if (!item) return;
    if (item->bindings) {
        free(item->bindings);
        item->bindings = NULL;
    }
    item->binding_count = 0;
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int materials_shader_reflection_init(void) {
    if (g_shader_reflection_ctx.initialized) {
        return 0; // Already initialized
    }

    g_shader_reflection_ctx.capacity = MATERIALS_SHADER_REFLECTION_DEFAULT_CAPACITY;
    g_shader_reflection_ctx.items = calloc(g_shader_reflection_ctx.capacity, sizeof(materials_shader_reflection_internal_t));
    if (!g_shader_reflection_ctx.items) {
        return -1;
    }

    g_shader_reflection_ctx.count = 0;
    g_shader_reflection_ctx.initialized = true;

    return 0;
}

void materials_shader_reflection_shutdown(void) {
    if (!g_shader_reflection_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_shader_reflection_ctx.count; i++) {
        materials_shader_reflection_cleanup_internal(&g_shader_reflection_ctx.items[i]);
    }

    free(g_shader_reflection_ctx.items);
    g_shader_reflection_ctx.items = NULL;
    g_shader_reflection_ctx.count = 0;
    g_shader_reflection_ctx.capacity = 0;
    g_shader_reflection_ctx.initialized = false;
}

int materials_shader_reflection_create(materials_shader_reflection_handle_t* out_handle, const materials_shader_reflection_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_shader_reflection_ctx.initialized) {
        return -2;
    }

    if (g_shader_reflection_ctx.count >= g_shader_reflection_ctx.capacity) {
        return -3;
    }

    uint32_t index = g_shader_reflection_ctx.count++;
    materials_shader_reflection_internal_t* item = &g_shader_reflection_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->bindings = NULL;
    item->binding_count = 0;
    item->push_constant_size = 0;
    item->initialized = true;
    item->dirty = true;

    out_handle->id = index;
    return 0;
}

void materials_shader_reflection_destroy(materials_shader_reflection_handle_t handle) {
    if (handle.id >= g_shader_reflection_ctx.count) {
        return;
    }

    materials_shader_reflection_cleanup_internal(&g_shader_reflection_ctx.items[handle.id]);
}

int materials_shader_reflection_update(materials_shader_reflection_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_shader_reflection_ctx.count) {
        return -1;
    }

    materials_shader_reflection_internal_t* item = &g_shader_reflection_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    // data is SPIR-V binary
    // size is byte size
    
    char spv_tmp[256];
    snprintf(spv_tmp, sizeof(spv_tmp), MATERIALS_SHADER_REFLECTION_TEMP_DIR "/reflection_%u.spv", handle.id);

    FILE* f = fopen(spv_tmp, "wb");
    if (!f) return -3;
    fwrite(data, 1, size, f);
    fclose(f);

    // Use spirv-dis to get metadata
    char command[1024];
    snprintf(command, sizeof(command), "spirv-dis %s", spv_tmp);

    FILE* pipe = popen(command, "r");
    if (!pipe) {
        unlink(spv_tmp);
        return -4;
    }

    char line[1024];
    while (fgets(line, sizeof(line), pipe) != NULL) {
        // Very basic parsing for OpDecorate with Binding or DescriptorSet
        // Example: OpDecorate %tex Binding 0
        // Example: OpDecorate %tex DescriptorSet 0
        
        // This is a placeholder for actual robust reflection. 
        // In a real engine, we'd use spirv-reflect or parse the binary directly.
        if (strstr(line, "Binding")) {
            // Found a binding!
        }
        if (strstr(line, "DescriptorSet")) {
            // Found a set!
        }
    }

    pclose(pipe);
    unlink(spv_tmp);

    item->dirty = false;
    return 0;
}

bool materials_shader_reflection_is_valid(materials_shader_reflection_handle_t handle) {
    if (handle.id >= g_shader_reflection_ctx.count) {
        return false;
    }
    return g_shader_reflection_ctx.items[handle.id].initialized;
}

int materials_shader_reflection_get_info(materials_shader_reflection_handle_t handle, materials_shader_reflection_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_shader_reflection_ctx.count) {
        return -2;
    }

    const materials_shader_reflection_internal_t* item = &g_shader_reflection_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void materials_shader_reflection_mark_dirty(materials_shader_reflection_handle_t handle) {
    if (handle.id < g_shader_reflection_ctx.count) {
        g_shader_reflection_ctx.items[handle.id].dirty = true;
    }
}

int materials_shader_reflection_process_pending(void) {
    return 0;
}

uint32_t materials_shader_reflection_get_count(void) {
    return g_shader_reflection_ctx.count;
}

size_t materials_shader_reflection_get_memory_usage(void) {
    size_t total = sizeof(g_shader_reflection_ctx);
    total += g_shader_reflection_ctx.capacity * sizeof(materials_shader_reflection_internal_t);
    for (uint32_t i = 0; i < g_shader_reflection_ctx.count; i++) {
        total += g_shader_reflection_ctx.items[i].binding_count * sizeof(materials_shader_binding_t);
    }
    return total;
}

void materials_shader_reflection_debug_print(void) {
    printf("Shader Reflection Stats:\n");
    printf("  Count: %u\n", g_shader_reflection_ctx.count);
}

