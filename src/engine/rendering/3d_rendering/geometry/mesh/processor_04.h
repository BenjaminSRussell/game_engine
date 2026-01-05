/*
 * geometry_mesh_processor_04.h
 *
 * Header file for geometry_mesh_processor_04 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_MESH_PROCESSOR_04_H
#define GEOMETRY_MESH_PROCESSOR_04_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct geometry_mesh_processor_04 geometry_mesh_processor_04_t;
typedef struct geometry_mesh_processor_04_desc geometry_mesh_processor_04_desc_t;
typedef struct geometry_mesh_processor_04_stats geometry_mesh_processor_04_stats_t;

/* Creation and destruction */
int geometry_mesh_processor_04_create(geometry_mesh_processor_04_t** out_ctx, const geometry_mesh_processor_04_desc_t* desc);
int geometry_mesh_processor_04_destroy(geometry_mesh_processor_04_t* ctx);

/* Core operations */
int geometry_mesh_processor_04_process_batch(geometry_mesh_processor_04_t* ctx, void* params);
int geometry_mesh_processor_04_process_single(geometry_mesh_processor_04_t* ctx, void* params);
int geometry_mesh_processor_04_transform(geometry_mesh_processor_04_t* ctx, void* params);
int geometry_mesh_processor_04_filter(geometry_mesh_processor_04_t* ctx, void* params);
int geometry_mesh_processor_04_aggregate(geometry_mesh_processor_04_t* ctx, void* params);
int geometry_mesh_processor_04_dispatch(geometry_mesh_processor_04_t* ctx, void* params);
int geometry_mesh_processor_04_finalize(geometry_mesh_processor_04_t* ctx, void* params);
int geometry_mesh_processor_04_validate_input(geometry_mesh_processor_04_t* ctx, void* params);
int geometry_mesh_processor_04_optimize_output(geometry_mesh_processor_04_t* ctx, void* params);
int geometry_mesh_processor_04_profile(geometry_mesh_processor_04_t* ctx, void* params);

/* Utility functions */
int geometry_mesh_processor_04_get_stats(geometry_mesh_processor_04_t* ctx);
int geometry_mesh_processor_04_set_callback(geometry_mesh_processor_04_t* ctx);
int geometry_mesh_processor_04_get_memory_usage(geometry_mesh_processor_04_t* ctx);
int geometry_mesh_processor_04_optimize(geometry_mesh_processor_04_t* ctx);
int geometry_mesh_processor_04_debug_print(geometry_mesh_processor_04_t* ctx);

/* Module functions */
int geometry_mesh_processor_04_module_init(void);
int geometry_mesh_processor_04_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_MESH_PROCESSOR_04_H */
