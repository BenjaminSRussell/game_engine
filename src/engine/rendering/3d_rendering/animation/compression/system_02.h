/*
 * animation_compression_system_02.h
 *
 * Header file for animation_compression_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef ANIMATION_COMPRESSION_SYSTEM_02_H
#define ANIMATION_COMPRESSION_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct animation_compression_system_02 animation_compression_system_02_t;
typedef struct animation_compression_system_02_desc animation_compression_system_02_desc_t;
typedef struct animation_compression_system_02_stats animation_compression_system_02_stats_t;

/* Creation and destruction */
int animation_compression_system_02_create(animation_compression_system_02_t** out_ctx, const animation_compression_system_02_desc_t* desc);
int animation_compression_system_02_destroy(animation_compression_system_02_t* ctx);

/* Core operations */
int animation_compression_system_02_create_system(animation_compression_system_02_t* ctx, void* params);
int animation_compression_system_02_destroy_system(animation_compression_system_02_t* ctx, void* params);
int animation_compression_system_02_tick(animation_compression_system_02_t* ctx, void* params);
int animation_compression_system_02_process(animation_compression_system_02_t* ctx, void* params);
int animation_compression_system_02_submit(animation_compression_system_02_t* ctx, void* params);
int animation_compression_system_02_execute(animation_compression_system_02_t* ctx, void* params);
int animation_compression_system_02_sync(animation_compression_system_02_t* ctx, void* params);
int animation_compression_system_02_query(animation_compression_system_02_t* ctx, void* params);
int animation_compression_system_02_configure(animation_compression_system_02_t* ctx, void* params);
int animation_compression_system_02_optimize(animation_compression_system_02_t* ctx, void* params);

/* Utility functions */
int animation_compression_system_02_get_stats(animation_compression_system_02_t* ctx);
int animation_compression_system_02_set_callback(animation_compression_system_02_t* ctx);
int animation_compression_system_02_get_memory_usage(animation_compression_system_02_t* ctx);
int animation_compression_system_02_optimize(animation_compression_system_02_t* ctx);
int animation_compression_system_02_debug_print(animation_compression_system_02_t* ctx);

/* Module functions */
int animation_compression_system_02_module_init(void);
int animation_compression_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* ANIMATION_COMPRESSION_SYSTEM_02_H */
