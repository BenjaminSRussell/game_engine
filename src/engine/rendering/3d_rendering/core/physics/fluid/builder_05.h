/*
 * physics_fluid_builder_05.h
 *
 * Header file for physics_fluid_builder_05 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef PHYSICS_FLUID_BUILDER_05_H
#define PHYSICS_FLUID_BUILDER_05_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct physics_fluid_builder_05 physics_fluid_builder_05_t;
typedef struct physics_fluid_builder_05_desc physics_fluid_builder_05_desc_t;
typedef struct physics_fluid_builder_05_stats physics_fluid_builder_05_stats_t;

/* Creation and destruction */
int physics_fluid_builder_05_create(physics_fluid_builder_05_t** out_ctx, const physics_fluid_builder_05_desc_t* desc);
int physics_fluid_builder_05_destroy(physics_fluid_builder_05_t* ctx);

/* Core operations */
int physics_fluid_builder_05_begin(physics_fluid_builder_05_t* ctx, void* params);
int physics_fluid_builder_05_end(physics_fluid_builder_05_t* ctx, void* params);
int physics_fluid_builder_05_add(physics_fluid_builder_05_t* ctx, void* params);
int physics_fluid_builder_05_remove(physics_fluid_builder_05_t* ctx, void* params);
int physics_fluid_builder_05_modify(physics_fluid_builder_05_t* ctx, void* params);
int physics_fluid_builder_05_finalize(physics_fluid_builder_05_t* ctx, void* params);
int physics_fluid_builder_05_validate(physics_fluid_builder_05_t* ctx, void* params);
int physics_fluid_builder_05_optimize(physics_fluid_builder_05_t* ctx, void* params);
int physics_fluid_builder_05_compile(physics_fluid_builder_05_t* ctx, void* params);
int physics_fluid_builder_05_link(physics_fluid_builder_05_t* ctx, void* params);

/* Utility functions */
int physics_fluid_builder_05_get_stats(physics_fluid_builder_05_t* ctx);
int physics_fluid_builder_05_set_callback(physics_fluid_builder_05_t* ctx);
int physics_fluid_builder_05_get_memory_usage(physics_fluid_builder_05_t* ctx);
int physics_fluid_builder_05_optimize(physics_fluid_builder_05_t* ctx);
int physics_fluid_builder_05_debug_print(physics_fluid_builder_05_t* ctx);

/* Module functions */
int physics_fluid_builder_05_module_init(void);
int physics_fluid_builder_05_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* PHYSICS_FLUID_BUILDER_05_H */
