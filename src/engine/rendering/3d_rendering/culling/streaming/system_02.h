/*
 * culling_streaming_system_02.h
 *
 * Header file for culling_streaming_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef CULLING_STREAMING_SYSTEM_02_H
#define CULLING_STREAMING_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct culling_streaming_system_02 culling_streaming_system_02_t;
typedef struct culling_streaming_system_02_desc culling_streaming_system_02_desc_t;
typedef struct culling_streaming_system_02_stats culling_streaming_system_02_stats_t;

/* Creation and destruction */
int culling_streaming_system_02_create(culling_streaming_system_02_t** out_ctx, const culling_streaming_system_02_desc_t* desc);
int culling_streaming_system_02_destroy(culling_streaming_system_02_t* ctx);

/* Core operations */
int culling_streaming_system_02_create_system(culling_streaming_system_02_t* ctx, void* params);
int culling_streaming_system_02_destroy_system(culling_streaming_system_02_t* ctx, void* params);
int culling_streaming_system_02_tick(culling_streaming_system_02_t* ctx, void* params);
int culling_streaming_system_02_process(culling_streaming_system_02_t* ctx, void* params);
int culling_streaming_system_02_submit(culling_streaming_system_02_t* ctx, void* params);
int culling_streaming_system_02_execute(culling_streaming_system_02_t* ctx, void* params);
int culling_streaming_system_02_sync(culling_streaming_system_02_t* ctx, void* params);
int culling_streaming_system_02_query(culling_streaming_system_02_t* ctx, void* params);
int culling_streaming_system_02_configure(culling_streaming_system_02_t* ctx, void* params);
int culling_streaming_system_02_optimize(culling_streaming_system_02_t* ctx, void* params);

/* Utility functions */
int culling_streaming_system_02_get_stats(culling_streaming_system_02_t* ctx);
int culling_streaming_system_02_set_callback(culling_streaming_system_02_t* ctx);
int culling_streaming_system_02_get_memory_usage(culling_streaming_system_02_t* ctx);
int culling_streaming_system_02_optimize(culling_streaming_system_02_t* ctx);
int culling_streaming_system_02_debug_print(culling_streaming_system_02_t* ctx);

/* Module functions */
int culling_streaming_system_02_module_init(void);
int culling_streaming_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* CULLING_STREAMING_SYSTEM_02_H */
