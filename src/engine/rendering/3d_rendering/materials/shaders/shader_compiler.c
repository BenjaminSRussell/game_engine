#include "shader_compiler.h"
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

#define MATERIALS_SHADER_COMPILER_MAX_COUNT 4096
#define MATERIALS_SHADER_COMPILER_DEFAULT_CAPACITY 256
#define MATERIALS_SHADER_COMPILER_TEMP_DIR ".shader_cache/temp"

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct materials_shader_compiler_internal {
    uint32_t id;
    uint32_t flags;
    void* spirv_data;
    size_t spirv_size;
    bool initialized;
    bool dirty;
    char* last_error;
} materials_shader_compiler_internal_t;

typedef struct materials_shader_compiler_context {
    materials_shader_compiler_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} materials_shader_compiler_context_t;

static materials_shader_compiler_context_t g_shader_compiler_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void materials_shader_compiler_cleanup_internal(materials_shader_compiler_internal_t* item) {
    if (!item) return;
    if (item->spirv_data) {
        free(item->spirv_data);
        item->spirv_data = NULL;
    }
    if (item->last_error) {
        free(item->last_error);
        item->last_error = NULL;
    }
    item->spirv_size = 0;
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int materials_shader_compiler_init(void) {
    if (g_shader_compiler_ctx.initialized) {
        return 0; // Already initialized
    }

    g_shader_compiler_ctx.capacity = MATERIALS_SHADER_COMPILER_DEFAULT_CAPACITY;
    g_shader_compiler_ctx.items = calloc(g_shader_compiler_ctx.capacity, sizeof(materials_shader_compiler_internal_t));
    if (!g_shader_compiler_ctx.items) {
        return -1;
    }

    // Ensure temp directory exists
    system("mkdir -p " MATERIALS_SHADER_COMPILER_TEMP_DIR);

    g_shader_compiler_ctx.count = 0;
    g_shader_compiler_ctx.initialized = true;

    return 0;
}

void materials_shader_compiler_shutdown(void) {
    if (!g_shader_compiler_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_shader_compiler_ctx.count; i++) {
        materials_shader_compiler_cleanup_internal(&g_shader_compiler_ctx.items[i]);
    }

    free(g_shader_compiler_ctx.items);
    g_shader_compiler_ctx.items = NULL;
    g_shader_compiler_ctx.count = 0;
    g_shader_compiler_ctx.capacity = 0;
    g_shader_compiler_ctx.initialized = false;
}

int materials_shader_compiler_create(materials_shader_compiler_handle_t* out_handle, const materials_shader_compiler_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_shader_compiler_ctx.initialized) {
        return -2;
    }

    if (g_shader_compiler_ctx.count >= g_shader_compiler_ctx.capacity) {
        return -3;
    }

    uint32_t index = g_shader_compiler_ctx.count++;
    materials_shader_compiler_internal_t* item = &g_shader_compiler_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->spirv_data = NULL;
    item->spirv_size = 0;
    item->initialized = true;
    item->dirty = true;
    item->last_error = NULL;

    out_handle->id = index;
    return 0;
}

void materials_shader_compiler_destroy(materials_shader_compiler_handle_t handle) {
    if (handle.id >= g_shader_compiler_ctx.count) {
        return;
    }

    materials_shader_compiler_cleanup_internal(&g_shader_compiler_ctx.items[handle.id]);
}

int materials_shader_compiler_update(materials_shader_compiler_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_shader_compiler_ctx.count) {
        return -1;
    }

    materials_shader_compiler_internal_t* item = &g_shader_compiler_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    // data is the GLSL/HLSL source string
    // size is the string length
    const char* source = (const char*)data;
    
    char input_tmp[256];
    char output_tmp[256];
    snprintf(input_tmp, sizeof(input_tmp), MATERIALS_SHADER_COMPILER_TEMP_DIR "/shader_%u.glsl", handle.id);
    snprintf(output_tmp, sizeof(output_tmp), MATERIALS_SHADER_COMPILER_TEMP_DIR "/shader_%u.spv", handle.id);

    // Write source to temp file
    FILE* f = fopen(input_tmp, "w");
    if (!f) return -3;
    fwrite(source, 1, size, f);
    fclose(f);

    // Use glslc to compile
    char command[1024];
    snprintf(command, sizeof(command), "glslc %s -o %s 2>&1", input_tmp, output_tmp);

    FILE* pipe = popen(command, "r");
    if (!pipe) return -4;

    char buffer[1024];
    size_t error_len = 0;
    char* error_msg = NULL;

    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        size_t len = strlen(buffer);
        error_msg = realloc(error_msg, error_len + len + 1);
        memcpy(error_msg + error_len, buffer, len);
        error_len += len;
        error_msg[error_len] = '\0';
    }

    int result = pclose(pipe);
    if (result != 0) {
        if (item->last_error) free(item->last_error);
        item->last_error = error_msg;
        return -5;
    }

    if (error_msg) free(error_msg);

    // Read compiled SPIR-V
    f = fopen(output_tmp, "rb");
    if (!f) return -6;

    fseek(f, 0, SEEK_END);
    size_t spv_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    void* spv_data = malloc(spv_size);
    if (!spv_data) {
        fclose(f);
        return -7;
    }

    fread(spv_data, 1, spv_size, f);
    fclose(f);

    // Update item
    if (item->spirv_data) free(item->spirv_data);
    item->spirv_data = spv_data;
    item->spirv_size = spv_size;
    item->dirty = false;

    // Cleanup temp files
    unlink(input_tmp);
    unlink(output_tmp);

    return 0;
}

bool materials_shader_compiler_is_valid(materials_shader_compiler_handle_t handle) {
    if (handle.id >= g_shader_compiler_ctx.count) {
        return false;
    }
    return g_shader_compiler_ctx.items[handle.id].initialized;
}

int materials_shader_compiler_get_info(materials_shader_compiler_handle_t handle, materials_shader_compiler_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_shader_compiler_ctx.count) {
        return -2;
    }

    const materials_shader_compiler_internal_t* item = &g_shader_compiler_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void materials_shader_compiler_mark_dirty(materials_shader_compiler_handle_t handle) {
    if (handle.id < g_shader_compiler_ctx.count) {
        g_shader_compiler_ctx.items[handle.id].dirty = true;
    }
}

int materials_shader_compiler_process_pending(void) {
    int processed = 0;
    // This is a stub for batch processing if needed
    return processed;
}

uint32_t materials_shader_compiler_get_count(void) {
    return g_shader_compiler_ctx.count;
}

size_t materials_shader_compiler_get_memory_usage(void) {
    size_t total = sizeof(g_shader_compiler_ctx);
    total += g_shader_compiler_ctx.capacity * sizeof(materials_shader_compiler_internal_t);

    for (uint32_t i = 0; i < g_shader_compiler_ctx.count; i++) {
        total += g_shader_compiler_ctx.items[i].spirv_size;
    }

    return total;
}

void materials_shader_compiler_debug_print(void) {
    printf("Shader Compiler Stats:\n");
    printf("  Count: %u\n", g_shader_compiler_ctx.count);
    printf("  Memory Usage: %zu bytes\n", materials_shader_compiler_get_memory_usage());
}

