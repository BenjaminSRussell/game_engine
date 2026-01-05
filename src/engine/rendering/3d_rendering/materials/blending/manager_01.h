/*
 * materials_blending_manager_01.h
 *
 * Header file for materials_blending_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef MATERIALS_BLENDING_MANAGER_01_H
#define MATERIALS_BLENDING_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct materials_blending_manager_01 materials_blending_manager_01_t;
typedef struct materials_blending_manager_01_desc materials_blending_manager_01_desc_t;
typedef struct materials_blending_manager_01_stats materials_blending_manager_01_stats_t;

/* Creation and destruction */
int materials_blending_manager_01_create(materials_blending_manager_01_t** out_ctx, const materials_blending_manager_01_desc_t* desc);
int materials_blending_manager_01_destroy(materials_blending_manager_01_t* ctx);

/* Core operations */
int materials_blending_manager_01_init(materials_blending_manager_01_t* ctx, void* params);
int materials_blending_manager_01_shutdown(materials_blending_manager_01_t* ctx, void* params);
int materials_blending_manager_01_update(materials_blending_manager_01_t* ctx, void* params);
int materials_blending_manager_01_create(materials_blending_manager_01_t* ctx, void* params);
int materials_blending_manager_01_destroy(materials_blending_manager_01_t* ctx, void* params);
int materials_blending_manager_01_get(materials_blending_manager_01_t* ctx, void* params);
int materials_blending_manager_01_set(materials_blending_manager_01_t* ctx, void* params);
int materials_blending_manager_01_reset(materials_blending_manager_01_t* ctx, void* params);
int materials_blending_manager_01_validate(materials_blending_manager_01_t* ctx, void* params);
int materials_blending_manager_01_flush(materials_blending_manager_01_t* ctx, void* params);

/* Utility functions */
int materials_blending_manager_01_get_stats(materials_blending_manager_01_t* ctx);
int materials_blending_manager_01_set_callback(materials_blending_manager_01_t* ctx);
int materials_blending_manager_01_get_memory_usage(materials_blending_manager_01_t* ctx);
int materials_blending_manager_01_optimize(materials_blending_manager_01_t* ctx);
int materials_blending_manager_01_debug_print(materials_blending_manager_01_t* ctx);

/* Module functions */
int materials_blending_manager_01_module_init(void);
int materials_blending_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* MATERIALS_BLENDING_MANAGER_01_H */
