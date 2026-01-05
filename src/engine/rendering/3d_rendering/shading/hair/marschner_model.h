/*
 * marschner_model.h
 * Marschner hair model
 *
 * Part of the Shading subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef SHADING_MARSCHNER_MODEL_H
#define SHADING_MARSCHNER_MODEL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct shading_marschner_model_handle {
    uint32_t id;
} shading_marschner_model_handle_t;

typedef struct shading_marschner_model_desc {
    uint32_t flags;
    void* user_data;
} shading_marschner_model_desc_t;

typedef struct shading_marschner_model_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} shading_marschner_model_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int shading_marschner_model_init(void);
void shading_marschner_model_shutdown(void);

/* Lifecycle */
int shading_marschner_model_create(shading_marschner_model_handle_t* out_handle, const shading_marschner_model_desc_t* desc);
void shading_marschner_model_destroy(shading_marschner_model_handle_t handle);

/* Operations */
int shading_marschner_model_update(shading_marschner_model_handle_t handle, const void* data, size_t size);
bool shading_marschner_model_is_valid(shading_marschner_model_handle_t handle);
int shading_marschner_model_get_info(shading_marschner_model_handle_t handle, shading_marschner_model_info_t* out_info);
void shading_marschner_model_mark_dirty(shading_marschner_model_handle_t handle);
int shading_marschner_model_process_pending(void);

/* Statistics */
uint32_t shading_marschner_model_get_count(void);
size_t shading_marschner_model_get_memory_usage(void);
void shading_marschner_model_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* SHADING_MARSCHNER_MODEL_H */
