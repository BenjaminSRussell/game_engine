/*
 * shading_diffuse_system_02.h
 *
 * Header file for shading_diffuse_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef SHADING_DIFFUSE_SYSTEM_02_H
#define SHADING_DIFFUSE_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct shading_diffuse_system_02 shading_diffuse_system_02_t;
typedef struct shading_diffuse_system_02_desc shading_diffuse_system_02_desc_t;
typedef struct shading_diffuse_system_02_stats shading_diffuse_system_02_stats_t;

/* Creation and destruction */
int shading_diffuse_system_02_create(shading_diffuse_system_02_t** out_ctx, const shading_diffuse_system_02_desc_t* desc);
int shading_diffuse_system_02_destroy(shading_diffuse_system_02_t* ctx);

/* Core operations */
int shading_diffuse_system_02_create_system(shading_diffuse_system_02_t* ctx, void* params);
int shading_diffuse_system_02_destroy_system(shading_diffuse_system_02_t* ctx, void* params);
int shading_diffuse_system_02_tick(shading_diffuse_system_02_t* ctx, void* params);
int shading_diffuse_system_02_process(shading_diffuse_system_02_t* ctx, void* params);
int shading_diffuse_system_02_submit(shading_diffuse_system_02_t* ctx, void* params);
int shading_diffuse_system_02_execute(shading_diffuse_system_02_t* ctx, void* params);
int shading_diffuse_system_02_sync(shading_diffuse_system_02_t* ctx, void* params);
int shading_diffuse_system_02_query(shading_diffuse_system_02_t* ctx, void* params);
int shading_diffuse_system_02_configure(shading_diffuse_system_02_t* ctx, void* params);
int shading_diffuse_system_02_optimize(shading_diffuse_system_02_t* ctx, void* params);

/* Utility functions */
int shading_diffuse_system_02_get_stats(shading_diffuse_system_02_t* ctx);
int shading_diffuse_system_02_set_callback(shading_diffuse_system_02_t* ctx);
int shading_diffuse_system_02_get_memory_usage(shading_diffuse_system_02_t* ctx);
int shading_diffuse_system_02_optimize(shading_diffuse_system_02_t* ctx);
int shading_diffuse_system_02_debug_print(shading_diffuse_system_02_t* ctx);

/* Module functions */
int shading_diffuse_system_02_module_init(void);
int shading_diffuse_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* SHADING_DIFFUSE_SYSTEM_02_H */
