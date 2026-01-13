/*
 * particle_rendering.h
 * Particle billboard rendering
 *
 * Part of the Effects subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef EFFECTS_PARTICLE_RENDERING_H
#define EFFECTS_PARTICLE_RENDERING_H

#include "effects/gpu_particles/particle_buffer.h"
#include "rendering/forward/transparency.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct effects_particle_rendering_handle {
    uint32_t id;
} effects_particle_rendering_handle_t;

typedef struct effects_particle_rendering_desc {
    // Pipeline configuration
    bool enable_depth_write;
    bool enable_depth_test;
    bool enable_soft_particles; // Soft particles using depth buffer
    float soft_particle_fade_distance;
    
    BlendMode blend_mode;       // Forward transparency blend mode
    
    bool enable_lighting;       // Simple per-particle lighting
    
    uint32_t flags;
    void* user_data;
} effects_particle_rendering_desc_t;

typedef struct effects_particle_rendering_info {
    uint32_t id;
    uint32_t draw_calls;
    uint32_t visible_particles;
    bool initialized;
} effects_particle_rendering_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int effects_particle_rendering_init(void);
void effects_particle_rendering_shutdown(void);

/* Lifecycle */
int effects_particle_rendering_create(effects_particle_rendering_handle_t* out_handle, const effects_particle_rendering_desc_t* desc);
void effects_particle_rendering_destroy(effects_particle_rendering_handle_t handle);

/* Operations */
int effects_particle_rendering_update(effects_particle_rendering_handle_t handle, const void* data, size_t size);
bool effects_particle_rendering_is_valid(effects_particle_rendering_handle_t handle);
int effects_particle_rendering_get_info(effects_particle_rendering_handle_t handle, effects_particle_rendering_info_t* out_info);
void effects_particle_rendering_mark_dirty(effects_particle_rendering_handle_t handle);
int effects_particle_rendering_process_pending(void);

/* Statistics */
uint32_t effects_particle_rendering_get_count(void);
size_t effects_particle_rendering_get_memory_usage(void);
void effects_particle_rendering_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* EFFECTS_PARTICLE_RENDERING_H */
