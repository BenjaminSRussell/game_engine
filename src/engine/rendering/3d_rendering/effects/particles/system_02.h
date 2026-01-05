/*
 * effects_particles_system_02.h
 *
 * Header file for effects_particles_system_02 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef EFFECTS_PARTICLES_SYSTEM_02_H
#define EFFECTS_PARTICLES_SYSTEM_02_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct effects_particles_system_02 effects_particles_system_02_t;
typedef struct effects_particles_system_02_desc effects_particles_system_02_desc_t;
typedef struct effects_particles_system_02_stats effects_particles_system_02_stats_t;

/* Creation and destruction */
int effects_particles_system_02_create(effects_particles_system_02_t** out_ctx, const effects_particles_system_02_desc_t* desc);
int effects_particles_system_02_destroy(effects_particles_system_02_t* ctx);

/* Core operations */
int effects_particles_system_02_create_system(effects_particles_system_02_t* ctx, void* params);
int effects_particles_system_02_destroy_system(effects_particles_system_02_t* ctx, void* params);
int effects_particles_system_02_tick(effects_particles_system_02_t* ctx, void* params);
int effects_particles_system_02_process(effects_particles_system_02_t* ctx, void* params);
int effects_particles_system_02_submit(effects_particles_system_02_t* ctx, void* params);
int effects_particles_system_02_execute(effects_particles_system_02_t* ctx, void* params);
int effects_particles_system_02_sync(effects_particles_system_02_t* ctx, void* params);
int effects_particles_system_02_query(effects_particles_system_02_t* ctx, void* params);
int effects_particles_system_02_configure(effects_particles_system_02_t* ctx, void* params);
int effects_particles_system_02_optimize(effects_particles_system_02_t* ctx, void* params);

/* Utility functions */
int effects_particles_system_02_get_stats(effects_particles_system_02_t* ctx);
int effects_particles_system_02_set_callback(effects_particles_system_02_t* ctx);
int effects_particles_system_02_get_memory_usage(effects_particles_system_02_t* ctx);
int effects_particles_system_02_optimize(effects_particles_system_02_t* ctx);
int effects_particles_system_02_debug_print(effects_particles_system_02_t* ctx);

/* Module functions */
int effects_particles_system_02_module_init(void);
int effects_particles_system_02_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* EFFECTS_PARTICLES_SYSTEM_02_H */
