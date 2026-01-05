/*
 * texture_streaming_manager_01.h
 *
 * Header file for texture_streaming_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef TEXTURE_STREAMING_MANAGER_01_H
#define TEXTURE_STREAMING_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct texture_streaming_manager_01 texture_streaming_manager_01_t;
typedef struct texture_streaming_manager_01_desc texture_streaming_manager_01_desc_t;
typedef struct texture_streaming_manager_01_stats texture_streaming_manager_01_stats_t;

/* Creation and destruction */
int texture_streaming_manager_01_create(texture_streaming_manager_01_t** out_ctx, const texture_streaming_manager_01_desc_t* desc);
int texture_streaming_manager_01_destroy(texture_streaming_manager_01_t* ctx);

/* Core operations */
int texture_streaming_manager_01_init(texture_streaming_manager_01_t* ctx, void* params);
int texture_streaming_manager_01_shutdown(texture_streaming_manager_01_t* ctx, void* params);
int texture_streaming_manager_01_update(texture_streaming_manager_01_t* ctx, void* params);
int texture_streaming_manager_01_create(texture_streaming_manager_01_t* ctx, void* params);
int texture_streaming_manager_01_destroy(texture_streaming_manager_01_t* ctx, void* params);
int texture_streaming_manager_01_get(texture_streaming_manager_01_t* ctx, void* params);
int texture_streaming_manager_01_set(texture_streaming_manager_01_t* ctx, void* params);
int texture_streaming_manager_01_reset(texture_streaming_manager_01_t* ctx, void* params);
int texture_streaming_manager_01_validate(texture_streaming_manager_01_t* ctx, void* params);
int texture_streaming_manager_01_flush(texture_streaming_manager_01_t* ctx, void* params);

/* Utility functions */
int texture_streaming_manager_01_get_stats(texture_streaming_manager_01_t* ctx);
int texture_streaming_manager_01_set_callback(texture_streaming_manager_01_t* ctx);
int texture_streaming_manager_01_get_memory_usage(texture_streaming_manager_01_t* ctx);
int texture_streaming_manager_01_optimize(texture_streaming_manager_01_t* ctx);
int texture_streaming_manager_01_debug_print(texture_streaming_manager_01_t* ctx);

/* Module functions */
int texture_streaming_manager_01_module_init(void);
int texture_streaming_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* TEXTURE_STREAMING_MANAGER_01_H */
