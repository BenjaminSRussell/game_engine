/*
 * geometry_compression_manager_01.h
 *
 * Header file for geometry_compression_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_COMPRESSION_MANAGER_01_H
#define GEOMETRY_COMPRESSION_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct geometry_compression_manager_01 geometry_compression_manager_01_t;
typedef struct geometry_compression_manager_01_desc geometry_compression_manager_01_desc_t;
typedef struct geometry_compression_manager_01_stats geometry_compression_manager_01_stats_t;

/* Creation and destruction */
int geometry_compression_manager_01_create(geometry_compression_manager_01_t** out_ctx, const geometry_compression_manager_01_desc_t* desc);
int geometry_compression_manager_01_destroy(geometry_compression_manager_01_t* ctx);

/* Core operations */
int geometry_compression_manager_01_init(geometry_compression_manager_01_t* ctx, void* params);
int geometry_compression_manager_01_shutdown(geometry_compression_manager_01_t* ctx, void* params);
int geometry_compression_manager_01_update(geometry_compression_manager_01_t* ctx, void* params);
int geometry_compression_manager_01_create(geometry_compression_manager_01_t* ctx, void* params);
int geometry_compression_manager_01_destroy(geometry_compression_manager_01_t* ctx, void* params);
int geometry_compression_manager_01_get(geometry_compression_manager_01_t* ctx, void* params);
int geometry_compression_manager_01_set(geometry_compression_manager_01_t* ctx, void* params);
int geometry_compression_manager_01_reset(geometry_compression_manager_01_t* ctx, void* params);
int geometry_compression_manager_01_validate(geometry_compression_manager_01_t* ctx, void* params);
int geometry_compression_manager_01_flush(geometry_compression_manager_01_t* ctx, void* params);

/* Utility functions */
int geometry_compression_manager_01_get_stats(geometry_compression_manager_01_t* ctx);
int geometry_compression_manager_01_set_callback(geometry_compression_manager_01_t* ctx);
int geometry_compression_manager_01_get_memory_usage(geometry_compression_manager_01_t* ctx);
int geometry_compression_manager_01_optimize(geometry_compression_manager_01_t* ctx);
int geometry_compression_manager_01_debug_print(geometry_compression_manager_01_t* ctx);

/* Module functions */
int geometry_compression_manager_01_module_init(void);
int geometry_compression_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_COMPRESSION_MANAGER_01_H */
