/*
 * editor_icons.c
 * Billboard icons rendering
 *
 * Part of the Editor subsystem
 * Advanced 3D Rendering Engine
 *
 * Implements rendering of icons for lights, cameras, and audio sources
 */

#include "editor_icons.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define ICONS_MAX_COUNT 1024
#define ICONS_DEFAULT_SIZE 1.0f
#define ICONS_RENDER_DISTANCE 100.0f

/* ============================================================================
 * MATH TYPES
 * ============================================================================ */

typedef struct vec3 {
    float x, y, z;
} vec3_t;

typedef struct vec4 {
    float x, y, z, w;
} vec4_t;

/* ============================================================================
 * ICON TYPES
 * ============================================================================ */

typedef enum icon_type {
    ICON_TYPE_LIGHT_POINT = 0,
    ICON_TYPE_LIGHT_DIR,
    ICON_TYPE_LIGHT_SPOT,
    ICON_TYPE_CAMERA,
    ICON_TYPE_AUDIO,
    ICON_TYPE_PARTICLE
} icon_type_t;

typedef struct icon_instance {
    vec3_t position;
    icon_type_t type;
    vec4_t color;
    float scale;
    bool visible;
    uint32_t entity_id;
} icon_instance_t;

typedef struct editor_editor_icons_internal {
    uint32_t id;
    uint32_t flags;
    
    icon_instance_t icons[ICONS_MAX_COUNT];
    uint32_t icon_count;
    
    float global_scale;
    bool show_icons;
    
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} editor_editor_icons_internal_t;

typedef struct editor_editor_icons_context {
    editor_editor_icons_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    bool initialized;
} editor_editor_icons_context_t;

static editor_editor_icons_context_t g_icons_ctx = {0};

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int editor_editor_icons_init(void) {
    if (g_icons_ctx.initialized) {
        return 0;
    }

    g_icons_ctx.capacity = 1;
    g_icons_ctx.items = calloc(g_icons_ctx.capacity, sizeof(editor_editor_icons_internal_t));
    if (!g_icons_ctx.items) {
        return -1;
    }

    g_icons_ctx.count = 0;
    g_icons_ctx.initialized = true;

    return 0;
}

void editor_editor_icons_shutdown(void) {
    if (!g_icons_ctx.initialized) {
        return;
    }

    free(g_icons_ctx.items);
    g_icons_ctx.items = NULL;
    g_icons_ctx.count = 0;
    g_icons_ctx.capacity = 0;
    g_icons_ctx.initialized = false;
}

int editor_editor_icons_create(editor_editor_icons_handle_t* out_handle, 
                                 const editor_editor_icons_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_icons_ctx.initialized) {
        return -2;
    }

    if (g_icons_ctx.count >= g_icons_ctx.capacity) {
        return -3;
    }

    uint32_t index = g_icons_ctx.count++;
    editor_editor_icons_internal_t* item = &g_icons_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->icon_count = 0;
    item->global_scale = 1.0f;
    item->show_icons = true;
    
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void editor_editor_icons_destroy(editor_editor_icons_handle_t handle) {
    if (handle.id >= g_icons_ctx.count) {
        return;
    }

    g_icons_ctx.items[handle.id].initialized = false;
}

int editor_editor_icons_add_icon(editor_editor_icons_handle_t handle,
                                   vec3_t position, icon_type_t type,
                                   uint32_t entity_id) {
    if (handle.id >= g_icons_ctx.count) return -1;
    
    editor_editor_icons_internal_t* item = &g_icons_ctx.items[handle.id];
    if (item->icon_count >= ICONS_MAX_COUNT) return -2;
    
    icon_instance_t* icon = &item->icons[item->icon_count++];
    icon->position = position;
    icon->type = type;
    icon->entity_id = entity_id;
    icon->visible = true;
    icon->scale = 1.0f;
    icon->color = (vec4_t){1, 1, 1, 1};
    
    item->dirty = true;
    return 0;
}

int editor_editor_icons_clear(editor_editor_icons_handle_t handle) {
    if (handle.id >= g_icons_ctx.count) return -1;
    g_icons_ctx.items[handle.id].icon_count = 0;
    g_icons_ctx.items[handle.id].dirty = true;
    return 0;
}

int editor_editor_icons_update(editor_editor_icons_handle_t handle, 
                                 const void* data, size_t size) {
    if (handle.id >= g_icons_ctx.count) {
        return -1;
    }

    g_icons_ctx.items[handle.id].dirty = true;
    return 0;
}

bool editor_editor_icons_is_valid(editor_editor_icons_handle_t handle) {
    if (handle.id >= g_icons_ctx.count) {
        return false;
    }
    return g_icons_ctx.items[handle.id].initialized;
}

int editor_editor_icons_get_info(editor_editor_icons_handle_t handle, 
                                   editor_editor_icons_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_icons_ctx.count) {
        return -2;
    }

    const editor_editor_icons_internal_t* item = &g_icons_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void editor_editor_icons_mark_dirty(editor_editor_icons_handle_t handle) {
    if (handle.id < g_icons_ctx.count) {
        g_icons_ctx.items[handle.id].dirty = true;
    }
}

int editor_editor_icons_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_icons_ctx.count; i++) {
        editor_editor_icons_internal_t* item = &g_icons_ctx.items[i];
        if (item->initialized && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }

    return processed;
}

uint32_t editor_editor_icons_get_count(void) {
    return g_icons_ctx.count;
}

size_t editor_editor_icons_get_memory_usage(void) {
    size_t total = sizeof(g_icons_ctx);
    total += g_icons_ctx.capacity * sizeof(editor_editor_icons_internal_t);
    return total;
}

void editor_editor_icons_debug_print(void) {
    // Debug output
}

/* End of editor_icons.c */
