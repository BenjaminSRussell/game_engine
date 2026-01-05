/*
 * animation_ragdoll_builder_05.h
 *
 * Header file for animation_ragdoll_builder_05 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef ANIMATION_RAGDOLL_BUILDER_05_H
#define ANIMATION_RAGDOLL_BUILDER_05_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct animation_ragdoll_builder_05 animation_ragdoll_builder_05_t;
typedef struct animation_ragdoll_builder_05_desc animation_ragdoll_builder_05_desc_t;
typedef struct animation_ragdoll_builder_05_stats animation_ragdoll_builder_05_stats_t;

/* Creation and destruction */
int animation_ragdoll_builder_05_create(animation_ragdoll_builder_05_t** out_ctx, const animation_ragdoll_builder_05_desc_t* desc);
int animation_ragdoll_builder_05_destroy(animation_ragdoll_builder_05_t* ctx);

/* Core operations */
int animation_ragdoll_builder_05_begin(animation_ragdoll_builder_05_t* ctx, void* params);
int animation_ragdoll_builder_05_end(animation_ragdoll_builder_05_t* ctx, void* params);
int animation_ragdoll_builder_05_add(animation_ragdoll_builder_05_t* ctx, void* params);
int animation_ragdoll_builder_05_remove(animation_ragdoll_builder_05_t* ctx, void* params);
int animation_ragdoll_builder_05_modify(animation_ragdoll_builder_05_t* ctx, void* params);
int animation_ragdoll_builder_05_finalize(animation_ragdoll_builder_05_t* ctx, void* params);
int animation_ragdoll_builder_05_validate(animation_ragdoll_builder_05_t* ctx, void* params);
int animation_ragdoll_builder_05_optimize(animation_ragdoll_builder_05_t* ctx, void* params);
int animation_ragdoll_builder_05_compile(animation_ragdoll_builder_05_t* ctx, void* params);
int animation_ragdoll_builder_05_link(animation_ragdoll_builder_05_t* ctx, void* params);

/* Utility functions */
int animation_ragdoll_builder_05_get_stats(animation_ragdoll_builder_05_t* ctx);
int animation_ragdoll_builder_05_set_callback(animation_ragdoll_builder_05_t* ctx);
int animation_ragdoll_builder_05_get_memory_usage(animation_ragdoll_builder_05_t* ctx);
int animation_ragdoll_builder_05_optimize(animation_ragdoll_builder_05_t* ctx);
int animation_ragdoll_builder_05_debug_print(animation_ragdoll_builder_05_t* ctx);

/* Module functions */
int animation_ragdoll_builder_05_module_init(void);
int animation_ragdoll_builder_05_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* ANIMATION_RAGDOLL_BUILDER_05_H */
