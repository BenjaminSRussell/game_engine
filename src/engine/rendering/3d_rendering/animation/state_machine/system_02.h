/*
 * animation_state_machine_system_02.h
 *
 * Header file for animation_state_machine_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef ANIMATION_STATE_MACHINE_SYSTEM_02_H
#define ANIMATION_STATE_MACHINE_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct animation_state_machine_system_02 animation_state_machine_system_02_t;
typedef struct animation_state_machine_system_02_desc animation_state_machine_system_02_desc_t;
typedef struct animation_state_machine_system_02_stats animation_state_machine_system_02_stats_t;

/* Creation and destruction */
int animation_state_machine_system_02_create(animation_state_machine_system_02_t** out_ctx, const animation_state_machine_system_02_desc_t* desc);
int animation_state_machine_system_02_destroy(animation_state_machine_system_02_t* ctx);

/* Core operations */
int animation_state_machine_system_02_create_system(animation_state_machine_system_02_t* ctx, void* params);
int animation_state_machine_system_02_destroy_system(animation_state_machine_system_02_t* ctx, void* params);
int animation_state_machine_system_02_tick(animation_state_machine_system_02_t* ctx, void* params);
int animation_state_machine_system_02_process(animation_state_machine_system_02_t* ctx, void* params);
int animation_state_machine_system_02_submit(animation_state_machine_system_02_t* ctx, void* params);
int animation_state_machine_system_02_execute(animation_state_machine_system_02_t* ctx, void* params);
int animation_state_machine_system_02_sync(animation_state_machine_system_02_t* ctx, void* params);
int animation_state_machine_system_02_query(animation_state_machine_system_02_t* ctx, void* params);
int animation_state_machine_system_02_configure(animation_state_machine_system_02_t* ctx, void* params);
int animation_state_machine_system_02_optimize(animation_state_machine_system_02_t* ctx, void* params);

/* Utility functions */
int animation_state_machine_system_02_get_stats(animation_state_machine_system_02_t* ctx);
int animation_state_machine_system_02_set_callback(animation_state_machine_system_02_t* ctx);
int animation_state_machine_system_02_get_memory_usage(animation_state_machine_system_02_t* ctx);
int animation_state_machine_system_02_optimize(animation_state_machine_system_02_t* ctx);
int animation_state_machine_system_02_debug_print(animation_state_machine_system_02_t* ctx);

/* Module functions */
int animation_state_machine_system_02_module_init(void);
int animation_state_machine_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* ANIMATION_STATE_MACHINE_SYSTEM_02_H */
