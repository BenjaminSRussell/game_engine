/*
 * rendering_nanite_system_02.h
 *
 * Header file for rendering_nanite_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef RENDERING_NANITE_SYSTEM_02_H
#define RENDERING_NANITE_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct rendering_nanite_system_02 rendering_nanite_system_02_t;
typedef struct rendering_nanite_system_02_desc rendering_nanite_system_02_desc_t;
typedef struct rendering_nanite_system_02_stats rendering_nanite_system_02_stats_t;

/* Creation and destruction */
int rendering_nanite_system_02_create(rendering_nanite_system_02_t** out_ctx, const rendering_nanite_system_02_desc_t* desc);
int rendering_nanite_system_02_destroy(rendering_nanite_system_02_t* ctx);

/* Core operations */
int rendering_nanite_system_02_create_system(rendering_nanite_system_02_t* ctx, void* params);
int rendering_nanite_system_02_destroy_system(rendering_nanite_system_02_t* ctx, void* params);
int rendering_nanite_system_02_tick(rendering_nanite_system_02_t* ctx, void* params);
int rendering_nanite_system_02_process(rendering_nanite_system_02_t* ctx, void* params);
int rendering_nanite_system_02_submit(rendering_nanite_system_02_t* ctx, void* params);
int rendering_nanite_system_02_execute(rendering_nanite_system_02_t* ctx, void* params);
int rendering_nanite_system_02_sync(rendering_nanite_system_02_t* ctx, void* params);
int rendering_nanite_system_02_query(rendering_nanite_system_02_t* ctx, void* params);
int rendering_nanite_system_02_configure(rendering_nanite_system_02_t* ctx, void* params);
int rendering_nanite_system_02_optimize(rendering_nanite_system_02_t* ctx, void* params);

/* Utility functions */
int rendering_nanite_system_02_get_stats(rendering_nanite_system_02_t* ctx);
int rendering_nanite_system_02_set_callback(rendering_nanite_system_02_t* ctx);
int rendering_nanite_system_02_get_memory_usage(rendering_nanite_system_02_t* ctx);
int rendering_nanite_system_02_optimize(rendering_nanite_system_02_t* ctx);
int rendering_nanite_system_02_debug_print(rendering_nanite_system_02_t* ctx);

/* Module functions */
int rendering_nanite_system_02_module_init(void);
int rendering_nanite_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_NANITE_SYSTEM_02_H */
