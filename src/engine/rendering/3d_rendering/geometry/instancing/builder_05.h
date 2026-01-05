/*
 * geometry_instancing_builder_05.h
 *
 * Header file for geometry_instancing_builder_05 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_INSTANCING_BUILDER_05_H
#define GEOMETRY_INSTANCING_BUILDER_05_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct geometry_instancing_builder_05 geometry_instancing_builder_05_t;
typedef struct geometry_instancing_builder_05_desc geometry_instancing_builder_05_desc_t;
typedef struct geometry_instancing_builder_05_stats geometry_instancing_builder_05_stats_t;

/* Creation and destruction */
int geometry_instancing_builder_05_create(geometry_instancing_builder_05_t** out_ctx, const geometry_instancing_builder_05_desc_t* desc);
int geometry_instancing_builder_05_destroy(geometry_instancing_builder_05_t* ctx);

/* Core operations */
int geometry_instancing_builder_05_begin(geometry_instancing_builder_05_t* ctx, void* params);
int geometry_instancing_builder_05_end(geometry_instancing_builder_05_t* ctx, void* params);
int geometry_instancing_builder_05_add(geometry_instancing_builder_05_t* ctx, void* params);
int geometry_instancing_builder_05_remove(geometry_instancing_builder_05_t* ctx, void* params);
int geometry_instancing_builder_05_modify(geometry_instancing_builder_05_t* ctx, void* params);
int geometry_instancing_builder_05_finalize(geometry_instancing_builder_05_t* ctx, void* params);
int geometry_instancing_builder_05_validate(geometry_instancing_builder_05_t* ctx, void* params);
int geometry_instancing_builder_05_optimize(geometry_instancing_builder_05_t* ctx, void* params);
int geometry_instancing_builder_05_compile(geometry_instancing_builder_05_t* ctx, void* params);
int geometry_instancing_builder_05_link(geometry_instancing_builder_05_t* ctx, void* params);

/* Utility functions */
int geometry_instancing_builder_05_get_stats(geometry_instancing_builder_05_t* ctx);
int geometry_instancing_builder_05_set_callback(geometry_instancing_builder_05_t* ctx);
int geometry_instancing_builder_05_get_memory_usage(geometry_instancing_builder_05_t* ctx);
int geometry_instancing_builder_05_optimize(geometry_instancing_builder_05_t* ctx);
int geometry_instancing_builder_05_debug_print(geometry_instancing_builder_05_t* ctx);

/* Module functions */
int geometry_instancing_builder_05_module_init(void);
int geometry_instancing_builder_05_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_INSTANCING_BUILDER_05_H */
