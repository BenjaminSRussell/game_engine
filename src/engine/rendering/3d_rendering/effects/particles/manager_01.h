/*
 * effects_particles_manager_01.h
 *
 * Header file for effects_particles_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef EFFECTS_PARTICLES_MANAGER_01_H
#define EFFECTS_PARTICLES_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct effects_particles_manager_01 effects_particles_manager_01_t;
typedef struct effects_particles_manager_01_desc effects_particles_manager_01_desc_t;
typedef struct effects_particles_manager_01_stats effects_particles_manager_01_stats_t;

/* Creation and destruction */
int effects_particles_manager_01_create(effects_particles_manager_01_t** out_ctx, const effects_particles_manager_01_desc_t* desc);
int effects_particles_manager_01_destroy(effects_particles_manager_01_t* ctx);

/* Core operations */
int effects_particles_manager_01_init(effects_particles_manager_01_t* ctx, void* params);
int effects_particles_manager_01_shutdown(effects_particles_manager_01_t* ctx, void* params);
int effects_particles_manager_01_update(effects_particles_manager_01_t* ctx, void* params);
int effects_particles_manager_01_create(effects_particles_manager_01_t* ctx, void* params);
int effects_particles_manager_01_destroy(effects_particles_manager_01_t* ctx, void* params);
int effects_particles_manager_01_get(effects_particles_manager_01_t* ctx, void* params);
int effects_particles_manager_01_set(effects_particles_manager_01_t* ctx, void* params);
int effects_particles_manager_01_reset(effects_particles_manager_01_t* ctx, void* params);
int effects_particles_manager_01_validate(effects_particles_manager_01_t* ctx, void* params);
int effects_particles_manager_01_flush(effects_particles_manager_01_t* ctx, void* params);

/* Utility functions */
int effects_particles_manager_01_get_stats(effects_particles_manager_01_t* ctx);
int effects_particles_manager_01_set_callback(effects_particles_manager_01_t* ctx);
int effects_particles_manager_01_get_memory_usage(effects_particles_manager_01_t* ctx);
int effects_particles_manager_01_optimize(effects_particles_manager_01_t* ctx);
int effects_particles_manager_01_debug_print(effects_particles_manager_01_t* ctx);

/* Module functions */
int effects_particles_manager_01_module_init(void);
int effects_particles_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* EFFECTS_PARTICLES_MANAGER_01_H */
