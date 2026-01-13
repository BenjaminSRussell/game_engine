/*
 * cloth_simulation.h
 * Cloth Physics Simulation
 *
 * Part of the Physics/Animation subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CLOTH_SIMULATION_H
#define CLOTH_SIMULATION_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * API
 * ============================================================================ */

int cloth_simulation_init(void);
void cloth_simulation_shutdown(void);
void cloth_simulation_update(float dt);

/* Object Management */
typedef struct cloth_simulation_particle_desc {
    float x, y, z;
    float inv_mass;
    bool pinned;
} cloth_simulation_particle_desc_t;

typedef struct cloth_simulation_constraint_desc {
    uint32_t a, b;
    float rest_length;
    float stiffness;
    bool tearable;
} cloth_simulation_constraint_desc_t;

typedef struct cloth_simulation_triangle_desc {
    uint32_t a, b, c;
    float rest_area;
} cloth_simulation_triangle_desc_t;

typedef struct cloth_simulation_sphere_collider_desc {
    float x, y, z;
    float radius;
    float friction;
    float restitution;
} cloth_simulation_sphere_collider_desc_t;

typedef struct cloth_simulation_capsule_collider_desc {
    float ax, ay, az;
    float bx, by, bz;
    float radius;
    float friction;
    float restitution;
} cloth_simulation_capsule_collider_desc_t;

typedef struct cloth_simulation_desc {
    uint32_t particle_count;
    const cloth_simulation_particle_desc_t* particles;
    uint32_t distance_constraint_count;
    const cloth_simulation_constraint_desc_t* distance_constraints;
    uint32_t bending_constraint_count;
    const cloth_simulation_constraint_desc_t* bending_constraints;
    uint32_t triangle_count;
    const cloth_simulation_triangle_desc_t* triangles;
    uint32_t sphere_collider_count;
    const cloth_simulation_sphere_collider_desc_t* sphere_colliders;
    uint32_t capsule_collider_count;
    const cloth_simulation_capsule_collider_desc_t* capsule_colliders;
    float damping;
    float self_collision_radius;
    float tear_factor;
    float distance_stiffness;
    float bending_stiffness;
    float collision_stiffness;
    float volume_stiffness;
    float collision_friction;
    float collision_restitution;
    float wind_strength;
    float wind_dir_x, wind_dir_y, wind_dir_z;
    uint32_t solver_iterations;
    bool use_gpu;
    bool active;
} cloth_simulation_desc_t;

uint32_t cloth_simulation_create(const cloth_simulation_desc_t* desc);
void cloth_simulation_destroy(uint32_t id);
int cloth_simulation_configure(uint32_t id, const cloth_simulation_desc_t* desc);
int cloth_simulation_set_sphere_colliders(uint32_t id, const cloth_simulation_sphere_collider_desc_t* colliders, uint32_t count);
int cloth_simulation_set_capsule_colliders(uint32_t id, const cloth_simulation_capsule_collider_desc_t* colliders, uint32_t count);
int cloth_simulation_set_active(uint32_t id, bool active);
int cloth_simulation_set_wind(uint32_t id, float strength, float dir_x, float dir_y, float dir_z);
int cloth_simulation_set_solver_iterations(uint32_t id, uint32_t iterations);

/* Additional API Functions */
int cloth_simulation_set_pinned_particles(uint32_t id, const uint32_t* particle_indices, uint32_t count, bool pinned);
size_t cloth_simulation_serialize_state(uint32_t id, void* out, size_t max_size);
bool cloth_simulation_deserialize_state(uint32_t id, const void* data, size_t size);
int cloth_simulation_get_memory_usage(uint32_t id, size_t* out_usage);

#ifdef __cplusplus
}
#endif

#endif /* CLOTH_SIMULATION_H */
