/*
 * geometry_compression_system_02.h
 *
 * Header file for geometry_compression_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_COMPRESSION_SYSTEM_02_H
#define GEOMETRY_COMPRESSION_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct geometry_compression_system_02 geometry_compression_system_02_t;
typedef struct geometry_compression_system_02_desc geometry_compression_system_02_desc_t;
typedef struct geometry_compression_system_02_stats geometry_compression_system_02_stats_t;

/* Creation and destruction */
int geometry_compression_system_02_create(geometry_compression_system_02_t** out_ctx, const geometry_compression_system_02_desc_t* desc);
int geometry_compression_system_02_destroy(geometry_compression_system_02_t* ctx);

/* Core operations */
int geometry_compression_system_02_create_system(geometry_compression_system_02_t* ctx, void* params);
int geometry_compression_system_02_destroy_system(geometry_compression_system_02_t* ctx, void* params);
int geometry_compression_system_02_tick(geometry_compression_system_02_t* ctx, void* params);
int geometry_compression_system_02_process(geometry_compression_system_02_t* ctx, void* params);
int geometry_compression_system_02_submit(geometry_compression_system_02_t* ctx, void* params);
int geometry_compression_system_02_execute(geometry_compression_system_02_t* ctx, void* params);
int geometry_compression_system_02_sync(geometry_compression_system_02_t* ctx, void* params);
int geometry_compression_system_02_query(geometry_compression_system_02_t* ctx, void* params);
int geometry_compression_system_02_configure(geometry_compression_system_02_t* ctx, void* params);
int geometry_compression_system_02_optimize(geometry_compression_system_02_t* ctx, void* params);

/* Utility functions */
int geometry_compression_system_02_get_stats(geometry_compression_system_02_t* ctx);
int geometry_compression_system_02_set_callback(geometry_compression_system_02_t* ctx);
int geometry_compression_system_02_get_memory_usage(geometry_compression_system_02_t* ctx);
int geometry_compression_system_02_optimize(geometry_compression_system_02_t* ctx);
int geometry_compression_system_02_debug_print(geometry_compression_system_02_t* ctx);

/* Module functions */
int geometry_compression_system_02_module_init(void);
int geometry_compression_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_COMPRESSION_SYSTEM_02_H */
