/*
 * materials_blending_system_02.h
 *
 * Header file for materials_blending_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef MATERIALS_BLENDING_SYSTEM_02_H
#define MATERIALS_BLENDING_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct materials_blending_system_02 materials_blending_system_02_t;
typedef struct materials_blending_system_02_desc materials_blending_system_02_desc_t;
typedef struct materials_blending_system_02_stats materials_blending_system_02_stats_t;

/* Creation and destruction */
int materials_blending_system_02_create(materials_blending_system_02_t** out_ctx, const materials_blending_system_02_desc_t* desc);
int materials_blending_system_02_destroy(materials_blending_system_02_t* ctx);

/* Core operations */
int materials_blending_system_02_create_system(materials_blending_system_02_t* ctx, void* params);
int materials_blending_system_02_destroy_system(materials_blending_system_02_t* ctx, void* params);
int materials_blending_system_02_tick(materials_blending_system_02_t* ctx, void* params);
int materials_blending_system_02_process(materials_blending_system_02_t* ctx, void* params);
int materials_blending_system_02_submit(materials_blending_system_02_t* ctx, void* params);
int materials_blending_system_02_execute(materials_blending_system_02_t* ctx, void* params);
int materials_blending_system_02_sync(materials_blending_system_02_t* ctx, void* params);
int materials_blending_system_02_query(materials_blending_system_02_t* ctx, void* params);
int materials_blending_system_02_configure(materials_blending_system_02_t* ctx, void* params);
int materials_blending_system_02_optimize(materials_blending_system_02_t* ctx, void* params);

/* Utility functions */
int materials_blending_system_02_get_stats(materials_blending_system_02_t* ctx);
int materials_blending_system_02_set_callback(materials_blending_system_02_t* ctx);
int materials_blending_system_02_get_memory_usage(materials_blending_system_02_t* ctx);
int materials_blending_system_02_optimize(materials_blending_system_02_t* ctx);
int materials_blending_system_02_debug_print(materials_blending_system_02_t* ctx);

/* Module functions */
int materials_blending_system_02_module_init(void);
int materials_blending_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* MATERIALS_BLENDING_SYSTEM_02_H */
