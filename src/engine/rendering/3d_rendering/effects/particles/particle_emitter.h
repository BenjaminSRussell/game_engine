/*
 * particle_emitter.h
 * Particle emitter configuration
 *
 * Part of the Effects subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef EFFECTS_PARTICLE_EMITTER_H
#define EFFECTS_PARTICLE_EMITTER_H

#include "../../math/vec3.h"
#include "../../math/vec4.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct effects_particle_emitter_handle {
    uint32_t id;
} effects_particle_emitter_handle_t;

/**
 * Particle structure for CPU simulation
 */
typedef struct particle {
    vec3_t position;
    vec3_t velocity;
    vec4_t color;        // Assuming vec4_t exists, otherwise struct { r, g, b, a }
    vec3_t scale;
    float rotation;      // Rotation around Z axis (2D rotation)
    float rotation_speed;
    float lifetime;      // Total lifetime in seconds
    float age;           // Current age in seconds
    float size;          // Uniform size multiplier
    uint32_t random_seed;// For consistent randomness if needed
} particle_t;

/**
 * Emitter shapes
 */
typedef enum emitter_shape {
    EMITTER_SHAPE_POINT = 0,
    EMITTER_SHAPE_SPHERE,
    EMITTER_SHAPE_BOX,
    EMITTER_SHAPE_CONE,
    EMITTER_SHAPE_RING,
    EMITTER_SHAPE_MESH
} emitter_shape_t;

/**
 * Emitter parameters
 */
typedef struct emitter_params {
    // Transform
    vec3_t position;
    vec3_t rotation;
    vec3_t scale;
    
    // Emission
    float spawn_rate;           // Particles per second
    uint32_t burst_count;       // Particles per burst
    float burst_interval;       // Seconds between bursts
    bool loop;                  // Loop emission
    float duration;             // Duration if not looping
    
    // Shape properties
    emitter_shape_t shape;
    float shape_radius;         // For sphere, cone, ring
    vec3_t shape_dimensions;    // For box
    float cone_angle;           // For cone
    
    // Initial particle properties (ranges for randomization)
    struct { float min, max; } initial_speed;
    struct { float min, max; } initial_lifetime;
    struct { float min, max; } initial_size;
    struct { float min, max; } initial_rotation;
    struct { float min, max; } initial_rotation_speed;
    
    vec4_t start_color;
    vec4_t end_color;
    
    // Physics
    vec3_t gravity;
    float drag;
    
    bool simulate_on_gpu;       // Use compute shaders
    void* user_data;
} emitter_params_t;

typedef struct effects_particle_emitter_desc {
    emitter_params_t params;
    uint32_t flags;
    void* user_data;
} effects_particle_emitter_desc_t;

typedef struct effects_particle_emitter_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
    uint32_t particle_count;
    float active_time;
} effects_particle_emitter_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int effects_particle_emitter_init(void);
void effects_particle_emitter_shutdown(void);

/* Lifecycle */
int effects_particle_emitter_create(effects_particle_emitter_handle_t* out_handle, const effects_particle_emitter_desc_t* desc);
void effects_particle_emitter_destroy(effects_particle_emitter_handle_t handle);

/* Operations */
int effects_particle_emitter_update(effects_particle_emitter_handle_t handle, const void* data, size_t size);
bool effects_particle_emitter_is_valid(effects_particle_emitter_handle_t handle);
int effects_particle_emitter_get_info(effects_particle_emitter_handle_t handle, effects_particle_emitter_info_t* out_info);
void effects_particle_emitter_mark_dirty(effects_particle_emitter_handle_t handle);
int effects_particle_emitter_process_pending(void);

/* Statistics */
uint32_t effects_particle_emitter_get_count(void);
size_t effects_particle_emitter_get_memory_usage(void);
void effects_particle_emitter_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* EFFECTS_PARTICLE_EMITTER_H */
