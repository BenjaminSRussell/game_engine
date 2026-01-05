/*
 * shading_clearcoat_system_02.h
 *
 * Header file for shading_clearcoat_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef SHADING_CLEARCOAT_SYSTEM_02_H
#define SHADING_CLEARCOAT_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct shading_clearcoat_system_02 shading_clearcoat_system_02_t;
typedef struct shading_clearcoat_system_02_desc shading_clearcoat_system_02_desc_t;
typedef struct shading_clearcoat_system_02_stats shading_clearcoat_system_02_stats_t;

/* Creation and destruction */
int shading_clearcoat_system_02_create(shading_clearcoat_system_02_t** out_ctx, const shading_clearcoat_system_02_desc_t* desc);
int shading_clearcoat_system_02_destroy(shading_clearcoat_system_02_t* ctx);

/* Core operations */
int shading_clearcoat_system_02_create_system(shading_clearcoat_system_02_t* ctx, void* params);
int shading_clearcoat_system_02_destroy_system(shading_clearcoat_system_02_t* ctx, void* params);
int shading_clearcoat_system_02_tick(shading_clearcoat_system_02_t* ctx, void* params);
int shading_clearcoat_system_02_process(shading_clearcoat_system_02_t* ctx, void* params);
int shading_clearcoat_system_02_submit(shading_clearcoat_system_02_t* ctx, void* params);
int shading_clearcoat_system_02_execute(shading_clearcoat_system_02_t* ctx, void* params);
int shading_clearcoat_system_02_sync(shading_clearcoat_system_02_t* ctx, void* params);
int shading_clearcoat_system_02_query(shading_clearcoat_system_02_t* ctx, void* params);
int shading_clearcoat_system_02_configure(shading_clearcoat_system_02_t* ctx, void* params);
int shading_clearcoat_system_02_optimize(shading_clearcoat_system_02_t* ctx, void* params);

/* Utility functions */
int shading_clearcoat_system_02_get_stats(shading_clearcoat_system_02_t* ctx);
int shading_clearcoat_system_02_set_callback(shading_clearcoat_system_02_t* ctx);
int shading_clearcoat_system_02_get_memory_usage(shading_clearcoat_system_02_t* ctx);
int shading_clearcoat_system_02_optimize(shading_clearcoat_system_02_t* ctx);
int shading_clearcoat_system_02_debug_print(shading_clearcoat_system_02_t* ctx);

/* Module functions */
int shading_clearcoat_system_02_module_init(void);
int shading_clearcoat_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* SHADING_CLEARCOAT_SYSTEM_02_H */
