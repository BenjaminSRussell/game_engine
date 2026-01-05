/*
 * landscape_streaming_system_02.h
 *
 * Header file for landscape_streaming_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef LANDSCAPE_STREAMING_SYSTEM_02_H
#define LANDSCAPE_STREAMING_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct landscape_streaming_system_02 landscape_streaming_system_02_t;
typedef struct landscape_streaming_system_02_desc landscape_streaming_system_02_desc_t;
typedef struct landscape_streaming_system_02_stats landscape_streaming_system_02_stats_t;

/* Creation and destruction */
int landscape_streaming_system_02_create(landscape_streaming_system_02_t** out_ctx, const landscape_streaming_system_02_desc_t* desc);
int landscape_streaming_system_02_destroy(landscape_streaming_system_02_t* ctx);

/* Core operations */
int landscape_streaming_system_02_create_system(landscape_streaming_system_02_t* ctx, void* params);
int landscape_streaming_system_02_destroy_system(landscape_streaming_system_02_t* ctx, void* params);
int landscape_streaming_system_02_tick(landscape_streaming_system_02_t* ctx, void* params);
int landscape_streaming_system_02_process(landscape_streaming_system_02_t* ctx, void* params);
int landscape_streaming_system_02_submit(landscape_streaming_system_02_t* ctx, void* params);
int landscape_streaming_system_02_execute(landscape_streaming_system_02_t* ctx, void* params);
int landscape_streaming_system_02_sync(landscape_streaming_system_02_t* ctx, void* params);
int landscape_streaming_system_02_query(landscape_streaming_system_02_t* ctx, void* params);
int landscape_streaming_system_02_configure(landscape_streaming_system_02_t* ctx, void* params);
int landscape_streaming_system_02_optimize(landscape_streaming_system_02_t* ctx, void* params);

/* Utility functions */
int landscape_streaming_system_02_get_stats(landscape_streaming_system_02_t* ctx);
int landscape_streaming_system_02_set_callback(landscape_streaming_system_02_t* ctx);
int landscape_streaming_system_02_get_memory_usage(landscape_streaming_system_02_t* ctx);
int landscape_streaming_system_02_optimize(landscape_streaming_system_02_t* ctx);
int landscape_streaming_system_02_debug_print(landscape_streaming_system_02_t* ctx);

/* Module functions */
int landscape_streaming_system_02_module_init(void);
int landscape_streaming_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* LANDSCAPE_STREAMING_SYSTEM_02_H */
