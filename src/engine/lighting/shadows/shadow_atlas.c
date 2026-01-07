/*
 * shadow_atlas.c
 * Shadow map atlas management
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#include "lighting/shadows/shadow_atlas.h"
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

#define MAX_SHADOW_LIGHTS 256
#define MAX_ATLAS_INSTANCES 4

typedef struct shadow_slot {
    atlas_allocation_t alloc;
    bool active;
    uint32_t light_id;
} shadow_slot_t;

typedef struct shadow_atlas {
    uint32_t id;
    texture_handle_t texture;
    uint32_t width;
    uint32_t height;
    lighting_shadow_format_t format;
    bool enable_mipmaps;
    uint32_t cascade_count;
    
    rect_allocator_t* allocator;
    shadow_slot_t slots[MAX_SHADOW_LIGHTS];
    uint32_t slot_count;
    
    bool active;
} shadow_atlas_t;

static shadow_atlas_t g_atlases[MAX_ATLAS_INSTANCES];
static bool g_system_initialized = false;

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int lighting_shadow_atlas_init(void) {
    if (g_system_initialized) return 0;
    
    memset(g_atlases, 0, sizeof(g_atlases));
    g_system_initialized = true;
    return 0;
}

void lighting_shadow_atlas_shutdown(void) {
    if (!g_system_initialized) return;
    
    for (int i = 0; i < MAX_ATLAS_INSTANCES; i++) {
        if (g_atlases[i].active) {
            rect_allocator_destroy(g_atlases[i].allocator);
            g_atlases[i].active = false;
        }
    }
    g_system_initialized = false;
}

int lighting_shadow_atlas_create(lighting_shadow_atlas_handle_t* out_handle, 
                                 const lighting_shadow_atlas_desc_t* desc) {
    if (!out_handle || !desc || !g_system_initialized) return -1;
    
    // Find free atlas slot
    int slot_idx = -1;
    for (int i = 0; i < MAX_ATLAS_INSTANCES; i++) {
        if (!g_atlases[i].active) {
            slot_idx = i;
            break;
        }
    }
    
    if (slot_idx == -1) return -2; // No free atlas instances
    
    shadow_atlas_t* atlas = &g_atlases[slot_idx];
    atlas->id = slot_idx + 1; // 1-based ID
    atlas->width = desc->width > 0 ? desc->width : 4096;
    atlas->height = desc->height > 0 ? desc->height : 4096;
    atlas->format = desc->format;
    atlas->cascade_count = desc->cascade_count;
    atlas->enable_mipmaps = desc->enable_mipmaps;
    
    atlas->allocator = rect_allocator_create(atlas->width);
    if (!atlas->allocator) return -3;
    
    atlas->slot_count = 0;
    memset(atlas->slots, 0, sizeof(atlas->slots));
    atlas->active = true;
    
    out_handle->id = atlas->id;
    return 0;
}

void lighting_shadow_atlas_destroy(lighting_shadow_atlas_handle_t handle) {
    if (!g_system_initialized || handle.id == 0 || handle.id > MAX_ATLAS_INSTANCES) return;
    
    int idx = handle.id - 1;
    if (g_atlases[idx].active) {
        rect_allocator_destroy(g_atlases[idx].allocator);
        g_atlases[idx].active = false;
    }
}

int lighting_shadow_atlas_allocate_slot(lighting_shadow_atlas_handle_t handle, 
                                        uint32_t light_id, 
                                        uint32_t shadow_map_size,
                                        atlas_allocation_t* out_allocation) {
    if (!g_system_initialized || !out_allocation) return -1;
    if (handle.id == 0 || handle.id > MAX_ATLAS_INSTANCES) return -1;
    
    shadow_atlas_t* atlas = &g_atlases[handle.id - 1];
    if (!atlas->active) return -1;
    
    // Check if we have space in slots array
    if (atlas->slot_count >= MAX_SHADOW_LIGHTS) return -2;
    
    uint32_t x, y;
    // Simple allocation for now - ignores cascades packing logic which might want alignment
    if (!rect_allocator_allocate(atlas->allocator, shadow_map_size, shadow_map_size, &x, &y)) {
        return -3; // Full
    }
    
    // Find free slot index
    int free_slot_idx = -1;
    for (int i = 0; i < MAX_SHADOW_LIGHTS; i++) {
        if (!atlas->slots[i].active) {
            free_slot_idx = i;
            break;
        }
    }
    
    if (free_slot_idx == -1) return -2; // Should not happen given check above
    
    shadow_slot_t* slot = &atlas->slots[free_slot_idx];
    slot->active = true;
    slot->light_id = light_id;
    slot->alloc.x = x;
    slot->alloc.y = y;
    slot->alloc.width = shadow_map_size;
    slot->alloc.height = shadow_map_size;
    slot->alloc.mip_level = 0;
    slot->alloc.layer = 0;
    
    // Calculate UVs
    slot->alloc.uv_min[0] = (float)x / (float)atlas->width;
    slot->alloc.uv_min[1] = (float)y / (float)atlas->height;
    slot->alloc.uv_max[0] = (float)(x + shadow_map_size) / (float)atlas->width;
    slot->alloc.uv_max[1] = (float)(y + shadow_map_size) / (float)atlas->height;
    
    *out_allocation = slot->alloc;
    atlas->slot_count++;
    
    return 0;
}

void lighting_shadow_atlas_free_slot(lighting_shadow_atlas_handle_t handle, uint32_t light_id) {
    if (!g_system_initialized || handle.id == 0 || handle.id > MAX_ATLAS_INSTANCES) return;
    
    shadow_atlas_t* atlas = &g_atlases[handle.id - 1];
    if (!atlas->active) return;
    
    for (int i = 0; i < MAX_SHADOW_LIGHTS; i++) {
        if (atlas->slots[i].active && atlas->slots[i].light_id == light_id) {
            atlas->slots[i].active = false;
            atlas->slot_count--;
            // fragmentation happens here, allocator doesn't support free yet
            break;
        }
    }
}

int lighting_shadow_atlas_resize(lighting_shadow_atlas_handle_t handle, uint32_t new_width, uint32_t new_height) {
    if (!g_system_initialized || handle.id == 0 || handle.id > MAX_ATLAS_INSTANCES) return -1;
    
    shadow_atlas_t* atlas = &g_atlases[handle.id - 1];
    if (!atlas->active) return -1;
    
    // 1. Create new allocator
    rect_allocator_t* new_allocator = rect_allocator_create(new_width);
    if (!new_allocator) return -3;
    
    // 2. Try to re-allocate all active slots
    // This is a naive implementation; fully correct one would sort by size descending
    for (int i = 0; i < MAX_SHADOW_LIGHTS; i++) {
        if (atlas->slots[i].active) {
            uint32_t x, y;
            if (!rect_allocator_allocate(new_allocator, atlas->slots[i].alloc.width, atlas->slots[i].alloc.height, &x, &y)) {
                // Failed to fit in new size
                rect_allocator_destroy(new_allocator);
                return -4;
            }
            // Update slot usage temporarily (would commit this transactionally in real system)
            atlas->slots[i].alloc.x = x;
            atlas->slots[i].alloc.y = y;
            // Update UVs
             atlas->slots[i].alloc.uv_min[0] = (float)x / (float)new_width;
            atlas->slots[i].alloc.uv_min[1] = (float)y / (float)new_height;
            atlas->slots[i].alloc.uv_max[0] = (float)(x + atlas->slots[i].alloc.width) / (float)new_width;
            atlas->slots[i].alloc.uv_max[1] = (float)(y + atlas->slots[i].alloc.height) / (float)new_height;
        }
    }
    
    // 3. Swap allocators
    rect_allocator_destroy(atlas->allocator);
    atlas->allocator = new_allocator;
    atlas->width = new_width;
    atlas->height = new_height;
    
    // Mark dirty so textures can be recreated
    lighting_shadow_atlas_mark_dirty(handle);
    
    return 0;
}

int lighting_shadow_atlas_defragment(lighting_shadow_atlas_handle_t handle) {
    // Re-use resize logic with same size
    if (!g_system_initialized || handle.id == 0 || handle.id > MAX_ATLAS_INSTANCES) return -1;
    shadow_atlas_t* atlas = &g_atlases[handle.id - 1];
    if (!atlas->active) return -1;
    
    return lighting_shadow_atlas_resize(handle, atlas->width, atlas->height);
}

int lighting_shadow_atlas_clear_region(lighting_shadow_atlas_handle_t handle, uint32_t x, uint32_t y, uint32_t width, uint32_t height, float clear_value) {
    // In a real implementation this would enqueue a clear command
    // For now, valid stub
    (void)handle; (void)x; (void)y; (void)width; (void)height; (void)clear_value;
    return 0;
}

int lighting_shadow_atlas_update(lighting_shadow_atlas_handle_t handle, const void* data, size_t size) {
    (void)handle; (void)data; (void)size;
    return 0;
}

bool lighting_shadow_atlas_is_valid(lighting_shadow_atlas_handle_t handle) {
    if (!g_system_initialized || handle.id == 0 || handle.id > MAX_ATLAS_INSTANCES) return false;
    return g_atlases[handle.id - 1].active;
}

int lighting_shadow_atlas_get_info(lighting_shadow_atlas_handle_t handle, 
                                   lighting_shadow_atlas_info_t* out_info) {
    if (!out_info || !g_system_initialized || handle.id == 0 || handle.id > MAX_ATLAS_INSTANCES) return -1;
    
    shadow_atlas_t* atlas = &g_atlases[handle.id - 1];
    if (!atlas->active) return -1;
    
    out_info->id = atlas->id;
    out_info->width = atlas->width;
    out_info->height = atlas->height;
    out_info->format = atlas->format;
    out_info->active_allocations = atlas->slot_count;
    out_info->used_memory_bytes = atlas->width * atlas->height * 4; // Approx for now
    out_info->flags = 0;
    out_info->initialized = true;
    
    return 0;
}

void lighting_shadow_atlas_mark_dirty(lighting_shadow_atlas_handle_t handle) {
    (void)handle;
}

int lighting_shadow_atlas_process_pending(void) {
    return 0;
}

uint32_t lighting_shadow_atlas_get_count(void) {
    uint32_t count = 0;
    if (!g_system_initialized) return 0;
    for (int i = 0; i < MAX_ATLAS_INSTANCES; i++) {
        if (g_atlases[i].active) count++;
    }
    return count;
}

size_t lighting_shadow_atlas_get_memory_usage(void) {
    // Aggregate memory of all atlases
    return sizeof(g_atlases) + 1024; // Placeholder
}

void lighting_shadow_atlas_debug_print(void) {
}

texture_handle_t lighting_shadow_atlas_get_texture(void) {
    // Returns texture of first active atlas for legacy compatibility
    if (!g_system_initialized) return (texture_handle_t){{{0}}};
    for (int i = 0; i < MAX_ATLAS_INSTANCES; i++) {
        if (g_atlases[i].active) return g_atlases[i].texture;
    }
    return (texture_handle_t){{{0}}};
}

uint32_t lighting_shadow_atlas_get_size(void) {
    if (!g_system_initialized) return 0;
    for (int i = 0; i < MAX_ATLAS_INSTANCES; i++) {
            if (g_atlases[i].active) return g_atlases[i].width;
    }
    return 0;
}

