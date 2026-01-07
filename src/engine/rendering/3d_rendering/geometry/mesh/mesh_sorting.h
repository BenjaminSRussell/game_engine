/*
 * mesh_sorting.h
 * Mesh sorting strategies for optimized rendering
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef MESH_SORTING_H
#define MESH_SORTING_H

#include <stdint.h>
#include <stdbool.h>

/* Using forward declarations to avoid circular dependencies */
typedef struct static_mesh_draw_info static_mesh_draw_info_t;

/* Types of sorting needed for different render passes */
typedef enum mesh_sort_mode {
    MESH_SORT_MODE_MATERIAL,    /* Minimize state changes (group by material) */
    MESH_SORT_MODE_FRONT_TO_BACK, /* Optimize early-Z (opaque objects) */
    MESH_SORT_MODE_BACK_TO_FRONT, /* Correct blending (transparent objects) */
} mesh_sort_mode_t;

/**
 * @brief Key generator function type for custom sorting logic
 */
typedef uint64_t (*sort_key_gen_func_t)(const static_mesh_draw_info_t* item, void* user_data);

/**
 * @brief Sort an array of mesh draw items
 * @param items Array of draw items to sort
 * @param count Number of items in the array
 * @param mode Sorting mode to use
 * @param camera_pos Global camera position (needed for distance sorting)
 */
void mesh_sort_items(static_mesh_draw_info_t* items, uint32_t count, mesh_sort_mode_t mode, const float camera_pos[3]);

/**
 * @brief Sort items using a Radix sort (faster for large counts)
 * Uses internal 64-bit keys generated based on the mode
 */
void mesh_sort_radix(static_mesh_draw_info_t* items, uint32_t count, mesh_sort_mode_t mode, const float camera_pos[3]);

#endif /* MESH_SORTING_H */
