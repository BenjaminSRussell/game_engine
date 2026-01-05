/*
 * effects_particles_renderer_03.h
 *
 * Header file for effects_particles_renderer_03 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef EFFECTS_PARTICLES_RENDERER_03_H
#define EFFECTS_PARTICLES_RENDERER_03_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct effects_particles_renderer_03 effects_particles_renderer_03_t;
typedef struct effects_particles_renderer_03_desc effects_particles_renderer_03_desc_t;
typedef struct effects_particles_renderer_03_stats effects_particles_renderer_03_stats_t;

/* Creation and destruction */
int effects_particles_renderer_03_create(effects_particles_renderer_03_t** out_ctx, const effects_particles_renderer_03_desc_t* desc);
int effects_particles_renderer_03_destroy(effects_particles_renderer_03_t* ctx);

/* Core operations */
int effects_particles_renderer_03_render(effects_particles_renderer_03_t* ctx, void* params);
int effects_particles_renderer_03_prepare(effects_particles_renderer_03_t* ctx, void* params);
int effects_particles_renderer_03_bind(effects_particles_renderer_03_t* ctx, void* params);
int effects_particles_renderer_03_draw(effects_particles_renderer_03_t* ctx, void* params);
int effects_particles_renderer_03_dispatch(effects_particles_renderer_03_t* ctx, void* params);
int effects_particles_renderer_03_submit_commands(effects_particles_renderer_03_t* ctx, void* params);
int effects_particles_renderer_03_build_commands(effects_particles_renderer_03_t* ctx, void* params);
int effects_particles_renderer_03_sort(effects_particles_renderer_03_t* ctx, void* params);
int effects_particles_renderer_03_batch(effects_particles_renderer_03_t* ctx, void* params);
int effects_particles_renderer_03_cull(effects_particles_renderer_03_t* ctx, void* params);

/* Utility functions */
int effects_particles_renderer_03_get_stats(effects_particles_renderer_03_t* ctx);
int effects_particles_renderer_03_set_callback(effects_particles_renderer_03_t* ctx);
int effects_particles_renderer_03_get_memory_usage(effects_particles_renderer_03_t* ctx);
int effects_particles_renderer_03_optimize(effects_particles_renderer_03_t* ctx);
int effects_particles_renderer_03_debug_print(effects_particles_renderer_03_t* ctx);

/* Module functions */
int effects_particles_renderer_03_module_init(void);
int effects_particles_renderer_03_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* EFFECTS_PARTICLES_RENDERER_03_H */
