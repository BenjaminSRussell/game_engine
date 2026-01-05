/*
 * shading_specular_system_02.h
 *
 * Header file for shading_specular_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef SHADING_SPECULAR_SYSTEM_02_H
#define SHADING_SPECULAR_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct shading_specular_system_02 shading_specular_system_02_t;
typedef struct shading_specular_system_02_desc shading_specular_system_02_desc_t;
typedef struct shading_specular_system_02_stats shading_specular_system_02_stats_t;

/* Creation and destruction */
int shading_specular_system_02_create(shading_specular_system_02_t** out_ctx, const shading_specular_system_02_desc_t* desc);
int shading_specular_system_02_destroy(shading_specular_system_02_t* ctx);

/* Core operations */
int shading_specular_system_02_create_system(shading_specular_system_02_t* ctx, void* params);
int shading_specular_system_02_destroy_system(shading_specular_system_02_t* ctx, void* params);
int shading_specular_system_02_tick(shading_specular_system_02_t* ctx, void* params);
int shading_specular_system_02_process(shading_specular_system_02_t* ctx, void* params);
int shading_specular_system_02_submit(shading_specular_system_02_t* ctx, void* params);
int shading_specular_system_02_execute(shading_specular_system_02_t* ctx, void* params);
int shading_specular_system_02_sync(shading_specular_system_02_t* ctx, void* params);
int shading_specular_system_02_query(shading_specular_system_02_t* ctx, void* params);
int shading_specular_system_02_configure(shading_specular_system_02_t* ctx, void* params);
int shading_specular_system_02_optimize(shading_specular_system_02_t* ctx, void* params);

/* Utility functions */
int shading_specular_system_02_get_stats(shading_specular_system_02_t* ctx);
int shading_specular_system_02_set_callback(shading_specular_system_02_t* ctx);
int shading_specular_system_02_get_memory_usage(shading_specular_system_02_t* ctx);
int shading_specular_system_02_optimize(shading_specular_system_02_t* ctx);
int shading_specular_system_02_debug_print(shading_specular_system_02_t* ctx);

/* Module functions */
int shading_specular_system_02_module_init(void);
int shading_specular_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* SHADING_SPECULAR_SYSTEM_02_H */
