/*
 * texture_compression_manager_01.h
 *
 * Header file for texture_compression_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef TEXTURE_COMPRESSION_MANAGER_01_H
#define TEXTURE_COMPRESSION_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct texture_compression_manager_01 texture_compression_manager_01_t;
typedef struct texture_compression_manager_01_desc texture_compression_manager_01_desc_t;
typedef struct texture_compression_manager_01_stats texture_compression_manager_01_stats_t;

/* Creation and destruction */
int texture_compression_manager_01_create(texture_compression_manager_01_t** out_ctx, const texture_compression_manager_01_desc_t* desc);
int texture_compression_manager_01_destroy(texture_compression_manager_01_t* ctx);

/* Core operations */
int texture_compression_manager_01_init(texture_compression_manager_01_t* ctx, void* params);
int texture_compression_manager_01_shutdown(texture_compression_manager_01_t* ctx, void* params);
int texture_compression_manager_01_update(texture_compression_manager_01_t* ctx, void* params);
int texture_compression_manager_01_create(texture_compression_manager_01_t* ctx, void* params);
int texture_compression_manager_01_destroy(texture_compression_manager_01_t* ctx, void* params);
int texture_compression_manager_01_get(texture_compression_manager_01_t* ctx, void* params);
int texture_compression_manager_01_set(texture_compression_manager_01_t* ctx, void* params);
int texture_compression_manager_01_reset(texture_compression_manager_01_t* ctx, void* params);
int texture_compression_manager_01_validate(texture_compression_manager_01_t* ctx, void* params);
int texture_compression_manager_01_flush(texture_compression_manager_01_t* ctx, void* params);

/* Utility functions */
int texture_compression_manager_01_get_stats(texture_compression_manager_01_t* ctx);
int texture_compression_manager_01_set_callback(texture_compression_manager_01_t* ctx);
int texture_compression_manager_01_get_memory_usage(texture_compression_manager_01_t* ctx);
int texture_compression_manager_01_optimize(texture_compression_manager_01_t* ctx);
int texture_compression_manager_01_debug_print(texture_compression_manager_01_t* ctx);

/* Module functions */
int texture_compression_manager_01_module_init(void);
int texture_compression_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* TEXTURE_COMPRESSION_MANAGER_01_H */
