#ifndef FLOCKING_SYSTEM_H
#define FLOCKING_SYSTEM_H

#include <core/types.h>
#include <ecs/ecs.h>
#include <math/vec3.h>
#include <stdbool.h>
#include <stddef.h>

// Boid structure for flocking behavior
typedef struct {
    Entity entity;
    
    // Position and velocity
    Vec3 position;
    Vec3 velocity;
    Vec3 acceleration;
    
    // Flocking parameters
    float max_speed;
    float max_force;
    float perception_radius;
    
    // Weights for flocking behaviors
    float separation_weight;
    float alignment_weight;
    float cohesion_weight;
    float obstacle_avoidance_weight;
    
    // Individual characteristics
    float wander_angle;
    float wander_strength;
    
    // Visual properties
    Vec3 color;
    float size;
} Boid;

// Flock configuration
typedef struct {
    float separation_radius;
    float alignment_radius;
    float cohesion_radius;
    float obstacle_avoidance_radius;
    
    float max_speed;
    float max_force;
    float perception_radius;
    
    // Default weights
    float separation_weight;
    float alignment_weight;
    float cohesion_weight;
    float obstacle_avoidance_weight;
    
    // Simulation parameters
    float boid_count_limit;
    bool enable_obstacle_avoidance;
    bool enable_wander;
    bool enable_3d_flocking;
} FlockConfig;

// Flock structure
typedef struct {
    Boid* boids;
    u32 boid_count;
    u32 capacity;
    
    FlockConfig config;
    Vec3 center_of_mass;
    Vec3 average_velocity;
    
    bool initialized;
} Flock;

// Obstacle for avoidance
typedef struct {
    Vec3 position;
    float radius;
    Vec3 normal;
} Obstacle;

// MARK: - Flocking System Management

bool flocking_init(Flock* flock, const FlockConfig* config);
void flocking_shutdown(Flock* flock);
void flocking_update(Flock* flock, float delta_time);
void flocking_render(Flock* flock);

// MARK: - Boid Management

bool flocking_add_boid(Flock* flock, const Boid* boid);
bool flocking_remove_boid(Flock* flock, Entity entity);
Boid* flocking_get_boid(Flock* flock, Entity entity);
void flocking_clear_boids(Flock* flock);

// MARK: - Flocking Behaviors

Vec3 flocking_calculate_separation(const Boid* boid, const Flock* flock, float radius);
Vec3 flocking_calculate_alignment(const Boid* boid, const Flock* flock, float radius);
Vec3 flocking_calculate_cohesion(const Boid* boid, const Flock* flock, float radius);
Vec3 flocking_calculate_obstacle_avoidance(const Boid* boid, const Obstacle* obstacles, u32 obstacle_count, float radius);
Vec3 flocking_calculate_wander(const Boid* boid, float strength);

// MARK: - Obstacle Management

void flocking_set_obstacles(Flock* flock, const Obstacle* obstacles, u32 count);
void flocking_add_obstacle(Flock* flock, const Obstacle* obstacle);
void flocking_clear_obstacles(Flock* flock);

// MARK: - Utility Functions

void flocking_calculate_flock_statistics(Flock* flock);
Vec3 flocking_get_center_of_mass(const Flock* flock);
Vec3 flocking_get_average_velocity(const Flock* flock);
u32 flocking_get_boid_count(const Flock* flock);

// MARK: - Configuration

void flocking_set_config(Flock* flock, const FlockConfig* config);
FlockConfig flocking_get_default_config(void);
void flocking_update_boid_weights(Flock* flock, float separation, float alignment, float cohesion, float obstacle_avoidance);

// MARK: - Crowd Simulation

typedef struct {
    Flock* flocks;
    u32 flock_count;
    u32 capacity;
    
    // Inter-flock interactions
    float flock_separation_radius;
    float flock_avoidance_weight;
    
    bool enable_inter_flock_interactions;
} CrowdSimulation;

bool crowd_simulation_init(CrowdSimulation* crowd, u32 max_flocks);
void crowd_simulation_shutdown(CrowdSimulation* crowd);
void crowd_simulation_update(CrowdSimulation* crowd, float delta_time);
void crowd_simulation_render(CrowdSimulation* crowd);

bool crowd_simulation_add_flock(CrowdSimulation* crowd, const Flock* flock);
bool crowd_simulation_remove_flock(CrowdSimulation* crowd, u32 flock_index);
Flock* crowd_simulation_get_flock(CrowdSimulation* crowd, u32 flock_index);

// MARK: - Helper Functions

Boid* boid_create(Entity entity, Vec3 position, Vec3 velocity, Vec3 color, float size);
void boid_destroy(Boid* boid);
void boid_update(Boid* boid, Vec3 force, float delta_time);
void boid_apply_force(Boid* boid, Vec3 force);
void boid_seek(Boid* boid, Vec3 target, float weight);
void boid_flee(Boid* boid, Vec3 threat, float weight);
void boid_arrive(Boid* boid, Vec3 target, float slowing_radius, float weight);

// MARK: - Debug and Visualization

void flocking_debug_print_flock_info(const Flock* flock);
void flocking_debug_render_boid_connections(const Flock* flock);
void flocking_debug_render_perception_radii(const Flock* flock);

#endif // FLOCKING_SYSTEM_H
