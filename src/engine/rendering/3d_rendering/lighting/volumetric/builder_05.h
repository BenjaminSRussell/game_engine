/*
 * lighting_volumetric_builder_05.h
 *
 * Header file for lighting_volumetric_builder_05 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_VOLUMETRIC_BUILDER_05_H
#define LIGHTING_VOLUMETRIC_BUILDER_05_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct lighting_volumetric_builder_05 lighting_volumetric_builder_05_t;
typedef struct lighting_volumetric_builder_05_desc lighting_volumetric_builder_05_desc_t;
typedef struct lighting_volumetric_builder_05_stats lighting_volumetric_builder_05_stats_t;

/* Creation and destruction */
int lighting_volumetric_builder_05_create(lighting_volumetric_builder_05_t** out_ctx, const lighting_volumetric_builder_05_desc_t* desc);
int lighting_volumetric_builder_05_destroy(lighting_volumetric_builder_05_t* ctx);

/* Core operations */
int lighting_volumetric_builder_05_begin(lighting_volumetric_builder_05_t* ctx, void* params);
int lighting_volumetric_builder_05_end(lighting_volumetric_builder_05_t* ctx, void* params);
int lighting_volumetric_builder_05_add(lighting_volumetric_builder_05_t* ctx, void* params);
int lighting_volumetric_builder_05_remove(lighting_volumetric_builder_05_t* ctx, void* params);
int lighting_volumetric_builder_05_modify(lighting_volumetric_builder_05_t* ctx, void* params);
int lighting_volumetric_builder_05_finalize(lighting_volumetric_builder_05_t* ctx, void* params);
int lighting_volumetric_builder_05_validate(lighting_volumetric_builder_05_t* ctx, void* params);
int lighting_volumetric_builder_05_optimize(lighting_volumetric_builder_05_t* ctx, void* params);
int lighting_volumetric_builder_05_compile(lighting_volumetric_builder_05_t* ctx, void* params);
int lighting_volumetric_builder_05_link(lighting_volumetric_builder_05_t* ctx, void* params);

/* Utility functions */
int lighting_volumetric_builder_05_get_stats(lighting_volumetric_builder_05_t* ctx);
int lighting_volumetric_builder_05_set_callback(lighting_volumetric_builder_05_t* ctx);
int lighting_volumetric_builder_05_get_memory_usage(lighting_volumetric_builder_05_t* ctx);
int lighting_volumetric_builder_05_optimize(lighting_volumetric_builder_05_t* ctx);
int lighting_volumetric_builder_05_debug_print(lighting_volumetric_builder_05_t* ctx);

/* Module functions */
int lighting_volumetric_builder_05_module_init(void);
int lighting_volumetric_builder_05_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_VOLUMETRIC_BUILDER_05_H */
