/*
 * mesh_sorting.c
 * Mesh sorting implementation
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#include "geometry/mesh/mesh_sorting.h"
#include <stdlib.h>
#include <string.h>
#include <include/math/math.h>

#include "geometry/mesh/static_mesh_draw.h"

/* ============================================================================
 * PRIVATE TYPES
 * ============================================================================ */

typedef struct sort_item {
    uint64_t key;
    uint32_t original_index;
} sort_item_t;

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================ */

/* Calculate squared distance between two points */
static float distance_sq(const float p1[3], const float p2[3]) {
    float dx = p1[0] - p2[0];
    float dy = p1[1] - p2[1];
    float dz = p1[2] - p2[2];
    return dx*dx + dy*dy + dz*dz;
}

/* 
 * Generate a 64-bit sort key
 * High bits: Material ID (for batching) or Depth (for transparency)
 * Low bits: Depth or Material ID
 */
static uint64_t generate_key(const static_mesh_draw_info_t* item, mesh_sort_mode_t mode, const float camera_pos[3]) {
    uint64_t key = 0;
    
    // Placeholder values - in real engine we'd get these from the mesh/material pointers
    uint32_t material_id = (uint32_t)((uintptr_t)item->material & 0xFFFFFFFF);
    
    // Calculate depth
    float object_pos[3] = {0,0,0};
    if (item->transforms) {
        // Assume first column of matrix is translation for this simple implementation
        // Real implementation would extract translation properly
        object_pos[0] = item->transforms[12];
        object_pos[1] = item->transforms[13];
        object_pos[2] = item->transforms[14];
    }
    
    float dist = distance_sq(camera_pos, object_pos);
    
    // Normalize distance to 32-bit integer range
    // Inverting for back-to-front if needed
    uint32_t depth_int = (uint32_t)(dist * 100.0f); // Simple scaling
    
    switch (mode) {
        case MESH_SORT_MODE_MATERIAL:
            // Material is primary key (high bits), depth secondary
            key = ((uint64_t)material_id << 32) | depth_int;
            break;
            
        case MESH_SORT_MODE_FRONT_TO_BACK:
            // Depth is primary key (ascending)
            key = ((uint64_t)depth_int << 32) | material_id;
            break;
            
        case MESH_SORT_MODE_BACK_TO_FRONT:
            // Depth is primary key (descending)
            // Invert depth by subtracting from max
            key = ((uint64_t)(0xFFFFFFFF - depth_int) << 32) | material_id;
            break;
    }
    
    return key;
}

/* Comparison function for qsort */
static int compare_sort_items(const void* a, const void* b) {
    const sort_item_t* sa = (const sort_item_t*)a;
    const sort_item_t* sb = (const sort_item_t*)b;
    
    if (sa->key < sb->key) return -1;
    if (sa->key > sb->key) return 1;
    return 0;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

void mesh_sort_items(static_mesh_draw_info_t* items, uint32_t count, mesh_sort_mode_t mode, const float camera_pos[3]) {
    if (!items || count <= 1) return;
    
    // Allocate temporary sort array
    sort_item_t* sort_buffer = (sort_item_t*)malloc(count * sizeof(sort_item_t));
    if (!sort_buffer) return;
    
    // Generate keys
    for (uint32_t i = 0; i < count; i++) {
        sort_buffer[i].key = generate_key(&items[i], mode, camera_pos);
        sort_buffer[i].original_index = i;
    }
    
    // Sort keys
    qsort(sort_buffer, count, sizeof(sort_item_t), compare_sort_items);
    
    // Reorder items
    // Ideally we would do this in-place or with a temp buffer, 
    // but for simplicity we'll Alloc a temp buffer for items
    static_mesh_draw_info_t* temp_items = (static_mesh_draw_info_t*)malloc(count * sizeof(static_mesh_draw_info_t));
    if (temp_items) {
        for (uint32_t i = 0; i < count; i++) {
            temp_items[i] = items[sort_buffer[i].original_index];
        }
        memcpy(items, temp_items, count * sizeof(static_mesh_draw_info_t));
        free(temp_items);
    }
    
    free(sort_buffer);
}

void mesh_sort_radix(static_mesh_draw_info_t* items, uint32_t count, mesh_sort_mode_t mode, const float camera_pos[3]) {
    // For now, redirect to standard sort. 
    // Real implementation would use 3-pass or 4-pass radix sort on the 64-bit keys
    mesh_sort_items(items, count, mode, camera_pos);
}
