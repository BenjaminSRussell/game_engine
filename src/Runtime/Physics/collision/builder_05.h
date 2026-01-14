/*
 * physics_collision_builder_05.h
 *
 * Header file for physics_collision_builder_05 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef PHYSICS_COLLISION_BUILDER_05_H
#define PHYSICS_COLLISION_BUILDER_05_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct physics_collision_builder_05 physics_collision_builder_05_t;
typedef struct physics_collision_builder_05_desc physics_collision_builder_05_desc_t;
typedef struct physics_collision_builder_05_stats physics_collision_builder_05_stats_t;

/* Creation and destruction */
int physics_collision_builder_05_create(physics_collision_builder_05_t** out_ctx, const physics_collision_builder_05_desc_t* desc);
int physics_collision_builder_05_destroy(physics_collision_builder_05_t* ctx);

/* Core operations */
int physics_collision_builder_05_begin(physics_collision_builder_05_t* ctx, void* params);
int physics_collision_builder_05_end(physics_collision_builder_05_t* ctx, void* params);
int physics_collision_builder_05_add(physics_collision_builder_05_t* ctx, void* params);
int physics_collision_builder_05_remove(physics_collision_builder_05_t* ctx, void* params);
int physics_collision_builder_05_modify(physics_collision_builder_05_t* ctx, void* params);
int physics_collision_builder_05_finalize(physics_collision_builder_05_t* ctx, void* params);
int physics_collision_builder_05_validate(physics_collision_builder_05_t* ctx, void* params);
int physics_collision_builder_05_optimize(physics_collision_builder_05_t* ctx, void* params);
int physics_collision_builder_05_compile(physics_collision_builder_05_t* ctx, void* params);
int physics_collision_builder_05_link(physics_collision_builder_05_t* ctx, void* params);

/* Utility functions */
int physics_collision_builder_05_get_stats(physics_collision_builder_05_t* ctx);
int physics_collision_builder_05_set_callback(physics_collision_builder_05_t* ctx);
int physics_collision_builder_05_get_memory_usage(physics_collision_builder_05_t* ctx);
int physics_collision_builder_05_optimize(physics_collision_builder_05_t* ctx);
int physics_collision_builder_05_debug_print(physics_collision_builder_05_t* ctx);

/* Module functions */
int physics_collision_builder_05_module_init(void);
int physics_collision_builder_05_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* PHYSICS_COLLISION_BUILDER_05_H */
