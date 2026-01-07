#include "postprocess/lens_flare/lens_flare.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "rendering/core/texture.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <include/math/math.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define POSTPROCESSING_LENS_FLARE_MAX_COUNT 4096
#define POSTPROCESSING_LENS_FLARE_DEFAULT_CAPACITY 256
#define POSTPROCESSING_LENS_FLARE_ALIGNMENT 16
#define MAX_FLARE_ELEMENTS 16

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef Vec3 vec3_t;
typedef Vec2 vec2_t;
typedef TextureID texture_handle_t;

typedef struct flare_element {
    float offset;
    float scale;
    vec3_t color;
    texture_handle_t texture;
} flare_element_t;

typedef struct postprocessing_lens_flare_internal {
    uint32_t id;
    uint32_t flags;
    void* data;
    size_t data_size;
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
    flare_element_t elements[MAX_FLARE_ELEMENTS];
    uint32_t element_count;
    float intensity;
} postprocessing_lens_flare_internal_t;

typedef struct postprocessing_lens_flare_context {
    postprocessing_lens_flare_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
} postprocessing_lens_flare_context_t;

static postprocessing_lens_flare_context_t g_lens_flare_ctx = {0};

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================ */

static void render_flare_element(flare_element_t* element, vec2_t light_pos_screen, vec2_t center, float brightness) {
    // Placeholder for rendering a quad with element->texture
    // Position based on light_pos_screen and element->offset
}

/* ============================================================================
 * LENS FLARE LOGIC
 * ============================================================================ */

void render_lens_flare(postprocessing_lens_flare_handle_t handle, vec3_t light_pos_world, texture_handle_t output) {
    if (handle.id >= g_lens_flare_ctx.count) return;
    postprocessing_lens_flare_internal_t* item = &g_lens_flare_ctx.items[handle.id];

    // 1. Project light_pos_world to screen space
    // vec2_t light_pos_screen = world_to_screen(light_pos_world);
    
    // 2. Check if visible (using depth buffer occlusion query or similar)
    // if (!is_visible(light_pos_screen)) return;
    
    // 3. Render elements along the line from light center to screen center
    // vec2_t center = vec2(0.5f, 0.5f);
    // vec2_t dir = center - light_pos_screen;
    
    for (uint32_t i = 0; i < item->element_count; i++) {
        // render_flare_element(&item->elements[i], light_pos_screen, center, item->intensity);
    }
}

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static bool postprocessing_lens_flare_validate(const postprocessing_lens_flare_internal_t* item) {
    if (!item) return false;
    if (!item->initialized) return false;
    return true;
}

static void postprocessing_lens_flare_cleanup_internal(postprocessing_lens_flare_internal_t* item) {
    if (!item) return;
    if (item->data) {
        free(item->data);
        item->data = NULL;
    }
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int postprocessing_lens_flare_init(void) {
    if (g_lens_flare_ctx.initialized) {
        return 0; // Already initialized
    }

    g_lens_flare_ctx.capacity = POSTPROCESSING_LENS_FLARE_DEFAULT_CAPACITY;
    g_lens_flare_ctx.items = calloc(g_lens_flare_ctx.capacity, sizeof(postprocessing_lens_flare_internal_t));
    if (!g_lens_flare_ctx.items) {
        return -1;
    }

    g_lens_flare_ctx.count = 0;
    g_lens_flare_ctx.initialized = true;

    return 0;
}

void postprocessing_lens_flare_shutdown(void) {
    if (!g_lens_flare_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_lens_flare_ctx.count; i++) {
        postprocessing_lens_flare_cleanup_internal(&g_lens_flare_ctx.items[i]);
    }

    free(g_lens_flare_ctx.items);
    g_lens_flare_ctx.items = NULL;
    g_lens_flare_ctx.count = 0;
    g_lens_flare_ctx.capacity = 0;
    g_lens_flare_ctx.initialized = false;
}

int postprocessing_lens_flare_create(postprocessing_lens_flare_handle_t* out_handle, const postprocessing_lens_flare_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_lens_flare_ctx.initialized) {
        return -2;
    }

    if (g_lens_flare_ctx.count >= g_lens_flare_ctx.capacity) {
        return -3;
    }

    uint32_t index = g_lens_flare_ctx.count++;
    postprocessing_lens_flare_internal_t* item = &g_lens_flare_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->data = NULL;
    item->data_size = 0;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;
    item->intensity = 1.0f;
    item->element_count = 0;

    out_handle->id = index;
    return 0;
}

void postprocessing_lens_flare_destroy(postprocessing_lens_flare_handle_t handle) {
    if (handle.id >= g_lens_flare_ctx.count) {
        return;
    }
    postprocessing_lens_flare_cleanup_internal(&g_lens_flare_ctx.items[handle.id]);
}

int postprocessing_lens_flare_update(postprocessing_lens_flare_handle_t handle, const void* data, size_t size) {
    if (handle.id >= g_lens_flare_ctx.count) {
        return -1;
    }

    postprocessing_lens_flare_internal_t* item = &g_lens_flare_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }
    
    // Update params
    item->dirty = true;
    return 0;
}

bool postprocessing_lens_flare_is_valid(postprocessing_lens_flare_handle_t handle) {
    if (handle.id >= g_lens_flare_ctx.count) {
        return false;
    }
    return g_lens_flare_ctx.items[handle.id].initialized;
}

int postprocessing_lens_flare_get_info(postprocessing_lens_flare_handle_t handle, postprocessing_lens_flare_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_lens_flare_ctx.count) {
        return -2;
    }

    const postprocessing_lens_flare_internal_t* item = &g_lens_flare_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void postprocessing_lens_flare_mark_dirty(postprocessing_lens_flare_handle_t handle) {
    if (handle.id < g_lens_flare_ctx.count) {
        g_lens_flare_ctx.items[handle.id].dirty = true;
    }
}

int postprocessing_lens_flare_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_lens_flare_ctx.count; i++) {
        postprocessing_lens_flare_internal_t* item = &g_lens_flare_ctx.items[i];
        if (item->initialized && item->dirty) {
            item->dirty = false;
            processed++;
        }
    }
    return processed;
}

uint32_t postprocessing_lens_flare_get_count(void) {
    return g_lens_flare_ctx.count;
}

size_t postprocessing_lens_flare_get_memory_usage(void) {
    size_t total = sizeof(g_lens_flare_ctx);
    total += g_lens_flare_ctx.capacity * sizeof(postprocessing_lens_flare_internal_t);
    for (uint32_t i = 0; i < g_lens_flare_ctx.count; i++) {
        total += g_lens_flare_ctx.items[i].data_size;
    }
    return total;
}

void postprocessing_lens_flare_debug_print(void) {
    // Debug printing implementation
}

void postprocessing_lens_flare_add_element(postprocessing_lens_flare_handle_t handle, float offset, float scale, vec3_t color, texture_handle_t texture) {
     if (handle.id >= g_lens_flare_ctx.count) return;
     postprocessing_lens_flare_internal_t* item = &g_lens_flare_ctx.items[handle.id];
     
     if (item->element_count < MAX_FLARE_ELEMENTS) {
         flare_element_t* elem = &item->elements[item->element_count++];
         elem->offset = offset;
         elem->scale = scale;
         elem->color = color;
         elem->texture = texture;
     }
}

/* End of lens_flare.c */
