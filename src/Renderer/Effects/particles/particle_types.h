/*
 * particle_types.h
 * Core particle data structures
 * Uses engine's Vec3 type from include/math/vec3.h
 */

#ifndef PARTICLE_TYPES_H
#define PARTICLE_TYPES_H

#include <stdint.h>
#include "core/types.h"
// Use the public math header
#include "math/vec3.h"

#ifdef __cplusplus
extern "C" {
#endif

// RGBA color type
typedef struct color {
    float r, g, b, a;
} color_t;

// Core particle structure
// Matches GPU buffer layout for efficient transfer
typedef struct particle {
    // Position and age (vec4 for GPU alignment)
    union {
        struct { float x, y, z, age; };
        Vec3 position;
    };
    
    // Velocity and lifetime (vec4 for GPU alignment)
    union {
        struct { float vx, vy, vz, lifetime; };
        Vec3 velocity;
    };
    
    // Color (rgba)
    color_t color;
    
    // Size, rotation, mass, flags
    // Uses union to allow access to flags as both float (for GPU layout) and u32 (for bitwise ops)
    union {
        struct { float size, rotation, mass, f_flags; };
        struct { float _pad1, _pad2, _pad3; u32 flags; };
    };
    
    // Acceleration and rotation speed
    union {
        struct { float ax, ay, az, rotation_speed; };
        Vec3 acceleration;
    };
} particle_t;

// Particle emitter parameters
typedef struct emitter_params {
    Vec3 position;
    Vec3 velocity;
    Vec3 acceleration;
    color_t color_start;
    color_t color_end;
    float size_start;
    float size_end;
    float rotation_start;
    float rotation_speed;
    float lifetime;
    float emission_rate;
    float mass;
    u32 flags;
} emitter_params_t;

// Particle system statistics
typedef struct particle_stats {
    u32 active_particles;
    u32 max_particles;
    u32 particles_spawned;
    u32 particles_killed;
    float gpu_simulation_time_ms;
    float cpu_simulation_time_ms;
    bool using_gpu;
} particle_stats_t;

// Particle flags
enum PARTICLE_FLAGS {
    PARTICLE_FLAG_ALIVE       = 1 << 0,
    PARTICLE_FLAG_DEAD        = 1 << 1,
    PARTICLE_FLAG_COLLIDES    = 1 << 2,
    PARTICLE_FLAG_EMITS_LIGHT = 1 << 3,
    PARTICLE_FLAG_TRAIL       = 1 << 4,
    PARTICLE_FLAG_MESH        = 1 << 5
};

#ifdef __cplusplus
}
#endif

#endif /* PARTICLE_TYPES_H */
