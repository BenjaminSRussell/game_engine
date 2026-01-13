/*
 * physics_particles_manager_01.h
 *
 * Header file for physics_particles_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef PHYSICS_PARTICLES_MANAGER_01_H
#define PHYSICS_PARTICLES_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct physics_particles_manager_01 physics_particles_manager_01_t;
typedef struct physics_particles_manager_01_desc physics_particles_manager_01_desc_t;
typedef struct physics_particles_manager_01_stats physics_particles_manager_01_stats_t;

/* Creation and destruction */
int physics_particles_manager_01_create(physics_particles_manager_01_t** out_ctx, const physics_particles_manager_01_desc_t* desc);
int physics_particles_manager_01_destroy(physics_particles_manager_01_t* ctx);

/* Core operations */
int physics_particles_manager_01_init(physics_particles_manager_01_t* ctx, void* params);
int physics_particles_manager_01_shutdown(physics_particles_manager_01_t* ctx, void* params);
int physics_particles_manager_01_update(physics_particles_manager_01_t* ctx, void* params);
int physics_particles_manager_01_create_legacy(physics_particles_manager_01_t* ctx, void* params);
int physics_particles_manager_01_destroy_legacy(physics_particles_manager_01_t* ctx, void* params);
int physics_particles_manager_01_get(physics_particles_manager_01_t* ctx, void* params);
int physics_particles_manager_01_set(physics_particles_manager_01_t* ctx, void* params);
int physics_particles_manager_01_reset(physics_particles_manager_01_t* ctx, void* params);
int physics_particles_manager_01_validate(physics_particles_manager_01_t* ctx, void* params);
int physics_particles_manager_01_flush(physics_particles_manager_01_t* ctx, void* params);

/* Utility functions */
int physics_particles_manager_01_get_stats(physics_particles_manager_01_t* ctx);
int physics_particles_manager_01_set_callback(physics_particles_manager_01_t* ctx);
int physics_particles_manager_01_get_memory_usage(physics_particles_manager_01_t* ctx);
int physics_particles_manager_01_optimize(physics_particles_manager_01_t* ctx);
int physics_particles_manager_01_debug_print(physics_particles_manager_01_t* ctx);

/* Module functions */
int physics_particles_manager_01_module_init(void);
int physics_particles_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* PHYSICS_PARTICLES_MANAGER_01_H */
