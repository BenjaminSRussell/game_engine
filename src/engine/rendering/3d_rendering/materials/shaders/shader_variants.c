/*
 * shader_variants.c
 * Shader permutation system
 *
 * Part of the Materials subsystem
 * Advanced 3D Rendering Engine
 */

#include "shader_variants.h"
#include "shader_compiler.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ============================================================================
 * INTERNAL TYPES
 * ============================================================================ */

typedef struct materials_shader_variant_def {
    char name[64];
    char value[64];
} materials_shader_variant_def_t;

typedef struct materials_shader_variants_internal {
    uint32_t id;
    uint32_t flags;
    materials_shader_variant_def_t* variants;
    uint32_t variant_count;
    char* base_source;
    materials_shader_compiler_handle_t compiled_shader;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} materials_shader_variants_internal_t;

typedef struct materials_shader_variants_context {
    materials_shader_variants_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} materials_shader_variants_context_t;

static materials_shader_variants_context_t g_shader_variants_ctx = {0};

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int materials_shader_variants_init(void) {
    if (g_shader_variants_ctx.initialized) {
        return 0;
    }

    g_shader_variants_ctx.capacity = 256;
    g_shader_variants_ctx.items = calloc(g_shader_variants_ctx.capacity, sizeof(materials_shader_variants_internal_t));
    if (!g_shader_variants_ctx.items) {
        return -1;
    }

    g_shader_variants_ctx.count = 0;
    g_shader_variants_ctx.initialized = true;

    return 0;
}

void materials_shader_variants_shutdown(void) {
    if (!g_shader_variants_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_shader_variants_ctx.count; i++) {
        materials_shader_variants_internal_t* item = &g_shader_variants_ctx.items[i];
        if (item->base_source) free(item->base_source);
        if (item->variants) free(item->variants);
    }

    free(g_shader_variants_ctx.items);
    g_shader_variants_ctx.items = NULL;
    g_shader_variants_ctx.count = 0;
    g_shader_variants_ctx.capacity = 0;
    g_shader_variants_ctx.initialized = false;
}

int materials_shader_variants_create(materials_shader_variants_handle_t* out_handle, const materials_shader_variants_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_shader_variants_ctx.initialized) {
        return -2;
    }

    if (g_shader_variants_ctx.count >= g_shader_variants_ctx.capacity) {
        return -3;
    }

    uint32_t index = g_shader_variants_ctx.count++;
    materials_shader_variants_internal_t* item = &g_shader_variants_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;
    item->variant_count = 0;
    item->variants = NULL;
    item->base_source = NULL;

    // Create a compiler handle
    materials_shader_compiler_desc_t compiler_desc = {0};
    materials_shader_compiler_create(&item->compiled_shader, &compiler_desc);

    out_handle->id = index;
    return 0;
}

void materials_shader_variants_destroy(materials_shader_variants_handle_t handle) {
    if (handle.id >= g_shader_variants_ctx.count) {
        return;
    }
    
    materials_shader_variants_internal_t* item = &g_shader_variants_ctx.items[handle.id];
    if (item->base_source) free(item->base_source);
    if (item->variants) free(item->variants);
    materials_shader_compiler_destroy(item->compiled_shader);
    item->initialized = false;
}

int materials_shader_variants_update(materials_shader_variants_handle_t handle, const void* data, size_t size) {
    // data is expected to be key=value strings or a struct, but for now let's assume it's just source code
    // Ideally we'd separate variants set from source set
    
    if (handle.id >= g_shader_variants_ctx.count) {
        return -1;
    }

    materials_shader_variants_internal_t* item = &g_shader_variants_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    // Set base source
    if (item->base_source) free(item->base_source);
    item->base_source = malloc(size + 1);
    memcpy(item->base_source, data, size);
    item->base_source[size] = '\0';

    item->dirty = true;
    return 0;
}

bool materials_shader_variants_is_valid(materials_shader_variants_handle_t handle) {
    if (handle.id >= g_shader_variants_ctx.count) {
        return false;
    }
    return g_shader_variants_ctx.items[handle.id].initialized;
}

int materials_shader_variants_get_info(materials_shader_variants_handle_t handle, materials_shader_variants_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_shader_variants_ctx.count) {
        return -2;
    }

    const materials_shader_variants_internal_t* item = &g_shader_variants_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void materials_shader_variants_mark_dirty(materials_shader_variants_handle_t handle) {
    if (handle.id < g_shader_variants_ctx.count) {
        g_shader_variants_ctx.items[handle.id].dirty = true;
    }
}

int materials_shader_variants_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_shader_variants_ctx.count; i++) {
        materials_shader_variants_internal_t* item = &g_shader_variants_ctx.items[i];
        if (item->initialized && item->dirty && item->base_source) {
            // Recompile with variants
            // TODO: Append variants to source
            // For now, just pass base source
            materials_shader_compiler_update(item->compiled_shader, item->base_source, strlen(item->base_source));
            
            item->dirty = false;
            processed++;
        }
    }
    return processed;
}

uint32_t materials_shader_variants_get_count(void) {
    return g_shader_variants_ctx.count;
}

size_t materials_shader_variants_get_memory_usage(void) {
    size_t total = sizeof(g_shader_variants_ctx) + g_shader_variants_ctx.capacity * sizeof(materials_shader_variants_internal_t);
     for (uint32_t i = 0; i < g_shader_variants_ctx.count; i++) {
        if (g_shader_variants_ctx.items[i].base_source) {
            total += strlen(g_shader_variants_ctx.items[i].base_source);
        }
    }
    return total;
}

void materials_shader_variants_debug_print(void) {
    printf("Shader Variants Stats:\n");
    printf("  Count: %u\n", g_shader_variants_ctx.count);
}
