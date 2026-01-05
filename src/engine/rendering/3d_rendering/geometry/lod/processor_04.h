/*
 * geometry_lod_processor_04.h
 *
 * Header file for geometry_lod_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_LOD_PROCESSOR_04_H
#define GEOMETRY_LOD_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct geometry_lod_processor_04 geometry_lod_processor_04_t;
typedef struct geometry_lod_processor_04_desc geometry_lod_processor_04_desc_t;
typedef struct geometry_lod_processor_04_stats geometry_lod_processor_04_stats_t;

/* Creation and destruction */
int geometry_lod_processor_04_create(geometry_lod_processor_04_t** out_ctx, const geometry_lod_processor_04_desc_t* desc);
int geometry_lod_processor_04_destroy(geometry_lod_processor_04_t* ctx);

/* Core operations */
int geometry_lod_processor_04_process_batch(geometry_lod_processor_04_t* ctx, void* params);
int geometry_lod_processor_04_process_single(geometry_lod_processor_04_t* ctx, void* params);
int geometry_lod_processor_04_transform(geometry_lod_processor_04_t* ctx, void* params);
int geometry_lod_processor_04_filter(geometry_lod_processor_04_t* ctx, void* params);
int geometry_lod_processor_04_aggregate(geometry_lod_processor_04_t* ctx, void* params);
int geometry_lod_processor_04_dispatch(geometry_lod_processor_04_t* ctx, void* params);
int geometry_lod_processor_04_finalize(geometry_lod_processor_04_t* ctx, void* params);
int geometry_lod_processor_04_validate_input(geometry_lod_processor_04_t* ctx, void* params);
int geometry_lod_processor_04_optimize_output(geometry_lod_processor_04_t* ctx, void* params);
int geometry_lod_processor_04_profile(geometry_lod_processor_04_t* ctx, void* params);

/* Utility functions */
int geometry_lod_processor_04_get_stats(geometry_lod_processor_04_t* ctx);
int geometry_lod_processor_04_set_callback(geometry_lod_processor_04_t* ctx);
int geometry_lod_processor_04_get_memory_usage(geometry_lod_processor_04_t* ctx);
int geometry_lod_processor_04_optimize(geometry_lod_processor_04_t* ctx);
int geometry_lod_processor_04_debug_print(geometry_lod_processor_04_t* ctx);

/* Module functions */
int geometry_lod_processor_04_module_init(void);
int geometry_lod_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_LOD_PROCESSOR_04_H */
