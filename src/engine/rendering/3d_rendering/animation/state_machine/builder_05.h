/*
 * animation_state_machine_builder_05.h
 *
 * Header file for animation_state_machine_builder_05 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef ANIMATION_STATE_MACHINE_BUILDER_05_H
#define ANIMATION_STATE_MACHINE_BUILDER_05_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct animation_state_machine_builder_05 animation_state_machine_builder_05_t;
typedef struct animation_state_machine_builder_05_desc animation_state_machine_builder_05_desc_t;
typedef struct animation_state_machine_builder_05_stats animation_state_machine_builder_05_stats_t;

/* Creation and destruction */
int animation_state_machine_builder_05_create(animation_state_machine_builder_05_t** out_ctx, const animation_state_machine_builder_05_desc_t* desc);
int animation_state_machine_builder_05_destroy(animation_state_machine_builder_05_t* ctx);

/* Core operations */
int animation_state_machine_builder_05_begin(animation_state_machine_builder_05_t* ctx, void* params);
int animation_state_machine_builder_05_end(animation_state_machine_builder_05_t* ctx, void* params);
int animation_state_machine_builder_05_add(animation_state_machine_builder_05_t* ctx, void* params);
int animation_state_machine_builder_05_remove(animation_state_machine_builder_05_t* ctx, void* params);
int animation_state_machine_builder_05_modify(animation_state_machine_builder_05_t* ctx, void* params);
int animation_state_machine_builder_05_finalize(animation_state_machine_builder_05_t* ctx, void* params);
int animation_state_machine_builder_05_validate(animation_state_machine_builder_05_t* ctx, void* params);
int animation_state_machine_builder_05_optimize(animation_state_machine_builder_05_t* ctx, void* params);
int animation_state_machine_builder_05_compile(animation_state_machine_builder_05_t* ctx, void* params);
int animation_state_machine_builder_05_link(animation_state_machine_builder_05_t* ctx, void* params);

/* Utility functions */
int animation_state_machine_builder_05_get_stats(animation_state_machine_builder_05_t* ctx);
int animation_state_machine_builder_05_set_callback(animation_state_machine_builder_05_t* ctx);
int animation_state_machine_builder_05_get_memory_usage(animation_state_machine_builder_05_t* ctx);
int animation_state_machine_builder_05_optimize(animation_state_machine_builder_05_t* ctx);
int animation_state_machine_builder_05_debug_print(animation_state_machine_builder_05_t* ctx);

/* Module functions */
int animation_state_machine_builder_05_module_init(void);
int animation_state_machine_builder_05_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* ANIMATION_STATE_MACHINE_BUILDER_05_H */
