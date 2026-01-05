/*
 * core_resource_renderer_03.h
 *
 * Header file for core_resource_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef CORE_RESOURCE_RENDERER_03_H
#define CORE_RESOURCE_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct core_resource_renderer_03 core_resource_renderer_03_t;
typedef struct core_resource_renderer_03_desc core_resource_renderer_03_desc_t;
typedef struct core_resource_renderer_03_stats core_resource_renderer_03_stats_t;

/* Creation and destruction */
int core_resource_renderer_03_create(core_resource_renderer_03_t** out_ctx, const core_resource_renderer_03_desc_t* desc);
int core_resource_renderer_03_destroy(core_resource_renderer_03_t* ctx);

/* Core operations */
int core_resource_renderer_03_render(core_resource_renderer_03_t* ctx, void* params);
int core_resource_renderer_03_prepare(core_resource_renderer_03_t* ctx, void* params);
int core_resource_renderer_03_bind(core_resource_renderer_03_t* ctx, void* params);
int core_resource_renderer_03_draw(core_resource_renderer_03_t* ctx, void* params);
int core_resource_renderer_03_dispatch(core_resource_renderer_03_t* ctx, void* params);
int core_resource_renderer_03_submit_commands(core_resource_renderer_03_t* ctx, void* params);
int core_resource_renderer_03_build_commands(core_resource_renderer_03_t* ctx, void* params);
int core_resource_renderer_03_sort(core_resource_renderer_03_t* ctx, void* params);
int core_resource_renderer_03_batch(core_resource_renderer_03_t* ctx, void* params);
int core_resource_renderer_03_cull(core_resource_renderer_03_t* ctx, void* params);

/* Utility functions */
int core_resource_renderer_03_get_stats(core_resource_renderer_03_t* ctx);
int core_resource_renderer_03_set_callback(core_resource_renderer_03_t* ctx);
int core_resource_renderer_03_get_memory_usage(core_resource_renderer_03_t* ctx);
int core_resource_renderer_03_optimize(core_resource_renderer_03_t* ctx);
int core_resource_renderer_03_debug_print(core_resource_renderer_03_t* ctx);

/* Module functions */
int core_resource_renderer_03_module_init(void);
int core_resource_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* CORE_RESOURCE_RENDERER_03_H */
