/*
 * shadow_atlas.c
 * Shadow map atlas management
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#include "shadow_atlas.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>


/* ============================================================================
 * RECTANGLE ALLOCATOR (for atlas packing)
 * ============================================================================ */

typedef struct atlas_rect {
    uint32_t x, y;
    uint32_t width, height;
} atlas_rect_t;

typedef struct atlas_node {
    atlas_rect_t rect;
    bool occupied;
    struct atlas_node* child[2];  // Binary tree for splitting
} atlas_node_t;

typedef struct rect_allocator {
    atlas_node_t* root;
    uint32_t atlas_size;
} rect_allocator_t;

static atlas_node_t* atlas_node_create(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    atlas_node_t* node = (atlas_node_t*)calloc(1, sizeof(atlas_node_t));
    if (!node) return NULL;
    
    node->rect.x = x;
    node->rect.y = y;
    node->rect.width = width;
    node->rect.height = height;
    node->occupied = false;
    node->child[0] = NULL;
    node->child[1] = NULL;
    
    return node;
}

static void atlas_node_destroy(atlas_node_t* node) {
    if (!node) return;
    atlas_node_destroy(node->child[0]);
    atlas_node_destroy(node->child[1]);
    free(node);
}

static atlas_node_t* atlas_node_insert(atlas_node_t* node, uint32_t width, uint32_t height) {
    if (!node) return NULL;
    
    // If we're not a leaf, try inserting into children
    if (node->child[0] || node->child[1]) {
        atlas_node_t* new_node = atlas_node_insert(node->child[0], width, height);
        if (new_node) return new_node;
        return atlas_node_insert(node->child[1], width, height);
    }
    
    // If this node is occupied, can't use it
    if (node->occupied) return NULL;
    
    // If we're too small, return
    if (width > node->rect.width || height > node->rect.height) return NULL;
    
    // If we're a perfect fit, use this node
    if (width == node->rect.width && height == node->rect.height) {
        node->occupied = true;
        return node;
    }
    
    // Otherwise, split this node and create children
    uint32_t dw = node->rect.width - width;
    uint32_t dh = node->rect.height - height;
    
    if (dw > dh) {
        // Split horizontally
        node->child[0] = atlas_node_create(node->rect.x, node->rect.y, width, node->rect.height);
        node->child[1] = atlas_node_create(node->rect.x + width, node->rect.y, 
                                          node->rect.width - width, node->rect.height);
    } else {
        // Split vertically
        node->child[0] = atlas_node_create(node->rect.x, node->rect.y, node->rect.width, height);
        node->child[1] = atlas_node_create(node->rect.x, node->rect.y + height, 
                                          node->rect.width, node->rect.height - height);
    }
    
    return atlas_node_insert(node->child[0], width, height);
}

static rect_allocator_t* rect_allocator_create(uint32_t size) {
    rect_allocator_t* allocator = (rect_allocator_t*)calloc(1, sizeof(rect_allocator_t));
    if (!allocator) return NULL;
    
    allocator->atlas_size = size;
    allocator->root = atlas_node_create(0, 0, size, size);
    
    if (!allocator->root) {
        free(allocator);
        return NULL;
    }
    
    return allocator;
}

static void rect_allocator_destroy(rect_allocator_t* allocator) {
    if (!allocator) return;
    atlas_node_destroy(allocator->root);
    free(allocator);
}

static bool rect_allocator_allocate(rect_allocator_t* allocator, uint32_t width, uint32_t height, 
                                    uint32_t* out_x, uint32_t* out_y) {
    if (!allocator || !out_x || !out_y) return false;
    
    atlas_node_t* node = atlas_node_insert(allocator->root, width, height);
    if (!node) return false;
    
    *out_x = node->rect.x;
    *out_y = node->rect.y;
    return true;
}

/* ============================================================================
 * SHADOW ATLAS
 * ============================================================================ */

#define MAX_SHADOW_LIGHTS 64

typedef struct shadow_slot {
    uint32_t x, y;
    uint32_t size;
    bool active;
    uint32_t light_id;
} shadow_slot_t;

typedef struct shadow_atlas {
    texture_handle_t texture;
    uint32_t size;
    rect_allocator_t* allocator;
    shadow_slot_t slots[MAX_SHADOW_LIGHTS];
    uint32_t slot_count;
} shadow_atlas_t;

static shadow_atlas_t g_shadow_atlas = {0};
static bool g_shadow_atlas_initialized = false;

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int lighting_shadow_atlas_init(void) {
    if (g_shadow_atlas_initialized) {
        return 0; // Already initialized
    }
    
    // Default atlas size: 4096x4096
    g_shadow_atlas.size = 4096;
    g_shadow_atlas.allocator = rect_allocator_create(g_shadow_atlas.size);
    
    if (!g_shadow_atlas.allocator) {
        return -1;
    }
    
    g_shadow_atlas.texture = (texture_handle_t){{{0}}};
    g_shadow_atlas.slot_count = 0;
    memset(g_shadow_atlas.slots, 0, sizeof(g_shadow_atlas.slots));
    
    g_shadow_atlas_initialized = true;
    return 0;
}

void lighting_shadow_atlas_shutdown(void) {
    if (!g_shadow_atlas_initialized) {
        return;
    }
    
    rect_allocator_destroy(g_shadow_atlas.allocator);
    g_shadow_atlas.allocator = NULL;
    g_shadow_atlas.slot_count = 0;
    
    g_shadow_atlas_initialized = false;
}

int lighting_shadow_atlas_create(lighting_shadow_atlas_handle_t* out_handle, 
                                 const lighting_shadow_atlas_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }
    
    if (!g_shadow_atlas_initialized) {
        return -2;
    }
    
    // For now, just return a simple handle
    out_handle->id = 0;
    return 0;
}

void lighting_shadow_atlas_destroy(lighting_shadow_atlas_handle_t handle) {
    (void)handle;
    // Handle destruction logic
}

int lighting_shadow_atlas_allocate_slot(uint32_t light_id, uint32_t shadow_map_size,
                                        uint32_t* out_x, uint32_t* out_y) {
    if (!g_shadow_atlas_initialized || !out_x || !out_y) {
        return -1;
    }
    
    if (g_shadow_atlas.slot_count >= MAX_SHADOW_LIGHTS) {
        return -2; // Atlas full
    }
    
    uint32_t x, y;
    if (!rect_allocator_allocate(g_shadow_atlas.allocator, shadow_map_size, shadow_map_size, &x, &y)) {
        return -3; // No space in atlas
    }
    
    // Store the slot
    shadow_slot_t* slot = &g_shadow_atlas.slots[g_shadow_atlas.slot_count++];
    slot->x = x;
    slot->y = y;
    slot->size = shadow_map_size;
    slot->active = true;
    slot->light_id = light_id;
    
    *out_x = x;
    *out_y = y;
    
    return 0;
}

void lighting_shadow_atlas_free_slot(uint32_t light_id) {
    if (!g_shadow_atlas_initialized) {
        return;
    }
    
    // Find and deactivate the slot
    for (uint32_t i = 0; i < g_shadow_atlas.slot_count; i++) {
        if (g_shadow_atlas.slots[i].light_id == light_id && g_shadow_atlas.slots[i].active) {
            g_shadow_atlas.slots[i].active = false;
            // Note: We don't actually free the space in the allocator (would need more complex logic)
            // In practice, atlas is rebuilt periodically or uses a more sophisticated allocator
            break;
        }
    }
}

int lighting_shadow_atlas_update(lighting_shadow_atlas_handle_t handle, const void* data, size_t size) {
    (void)handle;
    (void)data;
    (void)size;
    
    if (!g_shadow_atlas_initialized) {
        return -1;
    }
    
    return 0;
}

bool lighting_shadow_atlas_is_valid(lighting_shadow_atlas_handle_t handle) {
    (void)handle;
    return g_shadow_atlas_initialized;
}

int lighting_shadow_atlas_get_info(lighting_shadow_atlas_handle_t handle, 
                                   lighting_shadow_atlas_info_t* out_info) {
    if (!out_info) {
        return -1;
    }
    
    if (!g_shadow_atlas_initialized) {
        return -2;
    }
    
    out_info->id = handle.id;
    out_info->flags = 0;
    out_info->initialized = g_shadow_atlas_initialized;
    
    return 0;
}

void lighting_shadow_atlas_mark_dirty(lighting_shadow_atlas_handle_t handle) {
    (void)handle;
    // Mark atlas as needing update
}

int lighting_shadow_atlas_process_pending(void) {
    if (!g_shadow_atlas_initialized) {
        return 0;
    }
    
    // Process any pending atlas updates
    return 0;
}

uint32_t lighting_shadow_atlas_get_count(void) {
    return g_shadow_atlas.slot_count;
}

size_t lighting_shadow_atlas_get_memory_usage(void) {
    size_t total = sizeof(shadow_atlas_t);
    // Add allocator overhead
    return total;
}

void lighting_shadow_atlas_debug_print(void) {
    if (!g_shadow_atlas_initialized) {
        return;
    }
    
    // Debug output implementation
}

texture_handle_t lighting_shadow_atlas_get_texture(void) {
    return g_shadow_atlas.texture;
}

uint32_t lighting_shadow_atlas_get_size(void) {
    return g_shadow_atlas.size;
}

/* End of shadow_atlas.c */
