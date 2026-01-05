/*
 * tools_pipeline_system_02.h
 *
 * Header file for tools_pipeline_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef TOOLS_PIPELINE_SYSTEM_02_H
#define TOOLS_PIPELINE_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct tools_pipeline_system_02 tools_pipeline_system_02_t;
typedef struct tools_pipeline_system_02_desc tools_pipeline_system_02_desc_t;
typedef struct tools_pipeline_system_02_stats tools_pipeline_system_02_stats_t;

/* Creation and destruction */
int tools_pipeline_system_02_create(tools_pipeline_system_02_t** out_ctx, const tools_pipeline_system_02_desc_t* desc);
int tools_pipeline_system_02_destroy(tools_pipeline_system_02_t* ctx);

/* Core operations */
int tools_pipeline_system_02_create_system(tools_pipeline_system_02_t* ctx, void* params);
int tools_pipeline_system_02_destroy_system(tools_pipeline_system_02_t* ctx, void* params);
int tools_pipeline_system_02_tick(tools_pipeline_system_02_t* ctx, void* params);
int tools_pipeline_system_02_process(tools_pipeline_system_02_t* ctx, void* params);
int tools_pipeline_system_02_submit(tools_pipeline_system_02_t* ctx, void* params);
int tools_pipeline_system_02_execute(tools_pipeline_system_02_t* ctx, void* params);
int tools_pipeline_system_02_sync(tools_pipeline_system_02_t* ctx, void* params);
int tools_pipeline_system_02_query(tools_pipeline_system_02_t* ctx, void* params);
int tools_pipeline_system_02_configure(tools_pipeline_system_02_t* ctx, void* params);
int tools_pipeline_system_02_optimize(tools_pipeline_system_02_t* ctx, void* params);

/* Utility functions */
int tools_pipeline_system_02_get_stats(tools_pipeline_system_02_t* ctx);
int tools_pipeline_system_02_set_callback(tools_pipeline_system_02_t* ctx);
int tools_pipeline_system_02_get_memory_usage(tools_pipeline_system_02_t* ctx);
int tools_pipeline_system_02_optimize(tools_pipeline_system_02_t* ctx);
int tools_pipeline_system_02_debug_print(tools_pipeline_system_02_t* ctx);

/* Module functions */
int tools_pipeline_system_02_module_init(void);
int tools_pipeline_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* TOOLS_PIPELINE_SYSTEM_02_H */
