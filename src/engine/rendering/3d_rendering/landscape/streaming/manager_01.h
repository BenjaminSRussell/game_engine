/*
 * landscape_streaming_manager_01.h
 *
 * Header file for landscape_streaming_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef LANDSCAPE_STREAMING_MANAGER_01_H
#define LANDSCAPE_STREAMING_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct landscape_streaming_manager_01 landscape_streaming_manager_01_t;
typedef struct landscape_streaming_manager_01_desc landscape_streaming_manager_01_desc_t;
typedef struct landscape_streaming_manager_01_stats landscape_streaming_manager_01_stats_t;

/* Creation and destruction */
int landscape_streaming_manager_01_create(landscape_streaming_manager_01_t** out_ctx, const landscape_streaming_manager_01_desc_t* desc);
int landscape_streaming_manager_01_destroy(landscape_streaming_manager_01_t* ctx);

/* Core operations */
int landscape_streaming_manager_01_init(landscape_streaming_manager_01_t* ctx, void* params);
int landscape_streaming_manager_01_shutdown(landscape_streaming_manager_01_t* ctx, void* params);
int landscape_streaming_manager_01_update(landscape_streaming_manager_01_t* ctx, void* params);
int landscape_streaming_manager_01_create(landscape_streaming_manager_01_t* ctx, void* params);
int landscape_streaming_manager_01_destroy(landscape_streaming_manager_01_t* ctx, void* params);
int landscape_streaming_manager_01_get(landscape_streaming_manager_01_t* ctx, void* params);
int landscape_streaming_manager_01_set(landscape_streaming_manager_01_t* ctx, void* params);
int landscape_streaming_manager_01_reset(landscape_streaming_manager_01_t* ctx, void* params);
int landscape_streaming_manager_01_validate(landscape_streaming_manager_01_t* ctx, void* params);
int landscape_streaming_manager_01_flush(landscape_streaming_manager_01_t* ctx, void* params);

/* Utility functions */
int landscape_streaming_manager_01_get_stats(landscape_streaming_manager_01_t* ctx);
int landscape_streaming_manager_01_set_callback(landscape_streaming_manager_01_t* ctx);
int landscape_streaming_manager_01_get_memory_usage(landscape_streaming_manager_01_t* ctx);
int landscape_streaming_manager_01_optimize(landscape_streaming_manager_01_t* ctx);
int landscape_streaming_manager_01_debug_print(landscape_streaming_manager_01_t* ctx);

/* Module functions */
int landscape_streaming_manager_01_module_init(void);
int landscape_streaming_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* LANDSCAPE_STREAMING_MANAGER_01_H */
