/*
 * texture_streaming_system_02.h
 *
 * Header file for texture_streaming_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef TEXTURE_STREAMING_SYSTEM_02_H
#define TEXTURE_STREAMING_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct texture_streaming_system_02 texture_streaming_system_02_t;
typedef struct texture_streaming_system_02_desc texture_streaming_system_02_desc_t;
typedef struct texture_streaming_system_02_stats texture_streaming_system_02_stats_t;

/* Creation and destruction */
int texture_streaming_system_02_create(texture_streaming_system_02_t** out_ctx, const texture_streaming_system_02_desc_t* desc);
int texture_streaming_system_02_destroy(texture_streaming_system_02_t* ctx);

/* Core operations */
int texture_streaming_system_02_create_system(texture_streaming_system_02_t* ctx, void* params);
int texture_streaming_system_02_destroy_system(texture_streaming_system_02_t* ctx, void* params);
int texture_streaming_system_02_tick(texture_streaming_system_02_t* ctx, void* params);
int texture_streaming_system_02_process(texture_streaming_system_02_t* ctx, void* params);
int texture_streaming_system_02_submit(texture_streaming_system_02_t* ctx, void* params);
int texture_streaming_system_02_execute(texture_streaming_system_02_t* ctx, void* params);
int texture_streaming_system_02_sync(texture_streaming_system_02_t* ctx, void* params);
int texture_streaming_system_02_query(texture_streaming_system_02_t* ctx, void* params);
int texture_streaming_system_02_configure(texture_streaming_system_02_t* ctx, void* params);
int texture_streaming_system_02_optimize(texture_streaming_system_02_t* ctx, void* params);

/* Utility functions */
int texture_streaming_system_02_get_stats(texture_streaming_system_02_t* ctx);
int texture_streaming_system_02_set_callback(texture_streaming_system_02_t* ctx);
int texture_streaming_system_02_get_memory_usage(texture_streaming_system_02_t* ctx);
int texture_streaming_system_02_optimize(texture_streaming_system_02_t* ctx);
int texture_streaming_system_02_debug_print(texture_streaming_system_02_t* ctx);

/* Module functions */
int texture_streaming_system_02_module_init(void);
int texture_streaming_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* TEXTURE_STREAMING_SYSTEM_02_H */
