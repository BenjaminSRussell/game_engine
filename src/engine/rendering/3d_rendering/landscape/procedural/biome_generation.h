/*
 * biome_generation.h
 * Biome generation
 *
 * Part of the Landscape subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LANDSCAPE_BIOME_GENERATION_H
#define LANDSCAPE_BIOME_GENERATION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct landscape_biome_generation_handle {
    uint32_t id;
} landscape_biome_generation_handle_t;

typedef struct landscape_biome_generation_desc {
    uint32_t flags;
    void* user_data;
} landscape_biome_generation_desc_t;

typedef struct landscape_biome_generation_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} landscape_biome_generation_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int landscape_biome_generation_init(void);
void landscape_biome_generation_shutdown(void);

/* Lifecycle */
int landscape_biome_generation_create(landscape_biome_generation_handle_t* out_handle, const landscape_biome_generation_desc_t* desc);
void landscape_biome_generation_destroy(landscape_biome_generation_handle_t handle);

/* Operations */
int landscape_biome_generation_update(landscape_biome_generation_handle_t handle, const void* data, size_t size);
bool landscape_biome_generation_is_valid(landscape_biome_generation_handle_t handle);
int landscape_biome_generation_get_info(landscape_biome_generation_handle_t handle, landscape_biome_generation_info_t* out_info);
void landscape_biome_generation_mark_dirty(landscape_biome_generation_handle_t handle);
int landscape_biome_generation_process_pending(void);

/* Statistics */
uint32_t landscape_biome_generation_get_count(void);
size_t landscape_biome_generation_get_memory_usage(void);
void landscape_biome_generation_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LANDSCAPE_BIOME_GENERATION_H */
