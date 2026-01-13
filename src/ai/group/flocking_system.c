// Flocking and Crowd Simulation System implementation
#include "ai/group/flocking_system.h"
#include <core/logger.h>
#include <math/vec3.h>
#include <math/math.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Global obstacles array for flocking
static Obstacle* g_obstacles = NULL;
static u32 g_obstacle_count = 0;
static u32 g_obstacle_capacity = 0;

// MARK: - Helper Functions

static Vec3 vec3_normalize_safe(Vec3 v) {
    float length = vec3_length(v);
    if (length < 0.0001f) {
        return (Vec3){0, 0, 0};
    }
    return vec3_mul(v, 1.0f / length);
}

static Vec3 boid_seek_force(const Boid* boid, Vec3 target) {
    Vec3 desired = vec3_sub(target, boid->position);
    desired = vec3_normalize_safe(desired);
    desired = vec3_mul(desired, boid->max_speed);
    
    Vec3 steer = vec3_sub(desired, boid->velocity);
    float steer_length = vec3_length(steer);
    if (steer_length > boid->max_force) {
        steer = vec3_mul(vec3_normalize_safe(steer), boid->max_force);
    }
    
    return steer;
}

// MARK: - Flocking System Management

bool flocking_init(Flock* flock, const FlockConfig* config) {
    if (!flock || !config) {
        LOG_ERROR("Invalid parameters for flocking_init");
        return false;
    }
    
    memset(flock, 0, sizeof(Flock));
    
    // Copy configuration
    flock->config = *config;
    
    // Allocate boid array
    flock->capacity = (u32)config->boid_count_limit;
    if (flock->capacity > 0) {
        flock->boids = (Boid*)calloc(flock->capacity, sizeof(Boid));
        if (!flock->boids) {
            LOG_ERROR("Failed to allocate boid array");
            return false;
        }
    }
    
    flock->initialized = true;
    LOG_INFO("Flocking system initialized with capacity: %u", flock->capacity);
    return true;
}

void flocking_shutdown(Flock* flock) {
    if (!flock || !flock->initialized) {
        return;
    }
    
    if (flock->boids) {
        free(flock->boids);
        flock->boids = NULL;
    }
    
    memset(flock, 0, sizeof(Flock));
    LOG_INFO("Flocking system shutdown");
}

void flocking_update(Flock* flock, float delta_time) {
    if (!flock || !flock->initialized || delta_time <= 0.0f) {
        return;
    }
    
    // Update each boid
    for (u32 i = 0; i < flock->boid_count; i++) {
        Boid* boid = &flock->boids[i];
        
        // Reset acceleration
        boid->acceleration = (Vec3){0, 0, 0};
        
        // Calculate flocking forces
        Vec3 separation = flocking_calculate_separation(boid, flock, flock->config.separation_radius);
        Vec3 alignment = flocking_calculate_alignment(boid, flock, flock->config.alignment_radius);
        Vec3 cohesion = flocking_calculate_cohesion(boid, flock, flock->config.cohesion_radius);
        
        // Apply weighted forces
        separation = vec3_mul(separation, boid->separation_weight);
        alignment = vec3_mul(alignment, boid->alignment_weight);
        cohesion = vec3_mul(cohesion, boid->cohesion_weight);
        
        boid_apply_force(boid, separation);
        boid_apply_force(boid, alignment);
        boid_apply_force(boid, cohesion);
        
        // Obstacle avoidance
        if (flock->config.enable_obstacle_avoidance && g_obstacles) {
            Vec3 avoidance = flocking_calculate_obstacle_avoidance(boid, g_obstacles, g_obstacle_count, flock->config.obstacle_avoidance_radius);
            avoidance = vec3_mul(avoidance, boid->obstacle_avoidance_weight);
            boid_apply_force(boid, avoidance);
        }
        
        // Wander behavior
        if (flock->config.enable_wander) {
            Vec3 wander = flocking_calculate_wander(boid, boid->wander_strength);
            boid_apply_force(boid, wander);
        }
        
        // Update boid physics
        boid_update(boid, boid->acceleration, delta_time);
        
        // Limit speed
        float speed = vec3_length(boid->velocity);
        if (speed > boid->max_speed) {
            boid->velocity = vec3_mul(vec3_normalize_safe(boid->velocity), boid->max_speed);
        }
        
        // Update position
        boid->position = vec3_add(boid->position, vec3_mul(boid->velocity, delta_time));
    }
    
    // Update flock statistics
    flocking_calculate_flock_statistics(flock);
}

void flocking_render(Flock* flock) {
    if (!flock || !flock->initialized) {
        return;
    }
    
    // Render each boid
    for (u32 i = 0; i < flock->boid_count; i++) {
        Boid* boid = &flock->boids[i];
        
        // TODO: Render boid as sphere or custom mesh
        // Use boid->position, boid->color, boid->size
    }
}

// MARK: - Boid Management

bool flocking_add_boid(Flock* flock, const Boid* boid) {
    if (!flock || !flock->initialized || !boid) {
        return false;
    }
    
    if (flock->boid_count >= flock->capacity) {
        LOG_ERROR("Flock capacity exceeded");
        return false;
    }
    
    // Copy boid
    flock->boids[flock->boid_count] = *boid;
    
    // Set default weights from config if not specified
    if (boid->separation_weight == 0.0f) {
        flock->boids[flock->boid_count].separation_weight = flock->config.separation_weight;
    }
    if (boid->alignment_weight == 0.0f) {
        flock->boids[flock->boid_count].alignment_weight = flock->config.alignment_weight;
    }
    if (boid->cohesion_weight == 0.0f) {
        flock->boids[flock->boid_count].cohesion_weight = flock->config.cohesion_weight;
    }
    if (boid->obstacle_avoidance_weight == 0.0f) {
        flock->boids[flock->boid_count].obstacle_avoidance_weight = flock->config.obstacle_avoidance_weight;
    }
    
    flock->boid_count++;
    LOG_DEBUG("Added boid to flock (total: %u)", flock->boid_count);
    return true;
}

bool flocking_remove_boid(Flock* flock, Entity entity) {
    if (!flock || !flock->initialized) {
        return false;
    }
    
    for (u32 i = 0; i < flock->boid_count; i++) {
        if (flock->boids[i].entity.id == entity.id) {
            // Shift remaining boids
            for (u32 j = i; j < flock->boid_count - 1; j++) {
                flock->boids[j] = flock->boids[j + 1];
            }
            flock->boid_count--;
            LOG_DEBUG("Removed boid from flock (total: %u)", flock->boid_count);
            return true;
        }
    }
    
    return false;
}

Boid* flocking_get_boid(Flock* flock, Entity entity) {
    if (!flock || !flock->initialized) {
        return NULL;
    }
    
    for (u32 i = 0; i < flock->boid_count; i++) {
        if (flock->boids[i].entity.id == entity.id) {
            return &flock->boids[i];
        }
    }
    
    return NULL;
}

void flocking_clear_boids(Flock* flock) {
    if (!flock || !flock->initialized) {
        return;
    }
    
    flock->boid_count = 0;
    LOG_INFO("Cleared all boids from flock");
}

// MARK: - Flocking Behaviors

Vec3 flocking_calculate_separation(const Boid* boid, const Flock* flock, float radius) {
    Vec3 steer = {0, 0, 0};
    u32 count = 0;
    
    for (u32 i = 0; i < flock->boid_count; i++) {
        if (flock->boids[i].entity.id == boid->entity.id) {
            continue; // Skip self
        }
        
        float d = vec3_distance(boid->position, flock->boids[i].position);
        if (d > 0 && d < radius) {
            Vec3 diff = vec3_sub(boid->position, flock->boids[i].position);
            diff = vec3_normalize_safe(diff);
            diff = vec3_mul(diff, 1.0f / d); // Weight by distance
            steer = vec3_add(steer, diff);
            count++;
        }
    }
    
    if (count > 0) {
        steer = vec3_mul(steer, 1.0f / count);
        steer = vec3_normalize_safe(steer);
        steer = vec3_mul(steer, boid->max_speed);
        steer = vec3_sub(steer, boid->velocity);
        
        // Limit to max force
        float steer_length = vec3_length(steer);
        if (steer_length > boid->max_force) {
            steer = vec3_mul(vec3_normalize_safe(steer), boid->max_force);
        }
    }
    
    return steer;
}

Vec3 flocking_calculate_alignment(const Boid* boid, const Flock* flock, float radius) {
    Vec3 sum = {0, 0, 0};
    u32 count = 0;
    
    for (u32 i = 0; i < flock->boid_count; i++) {
        if (flock->boids[i].entity.id == boid->entity.id) {
            continue; // Skip self
        }
        
        float d = vec3_distance(boid->position, flock->boids[i].position);
        if (d > 0 && d < radius) {
            sum = vec3_add(sum, flock->boids[i].velocity);
            count++;
        }
    }
    
    if (count > 0) {
        sum = vec3_mul(sum, 1.0f / count);
        sum = vec3_normalize_safe(sum);
        sum = vec3_mul(sum, boid->max_speed);
        Vec3 steer = vec3_sub(sum, boid->velocity);
        
        // Limit to max force
        float steer_length = vec3_length(steer);
        if (steer_length > boid->max_force) {
            steer = vec3_mul(vec3_normalize_safe(steer), boid->max_force);
        }
        
        return steer;
    }
    
    return (Vec3){0, 0, 0};
}

Vec3 flocking_calculate_cohesion(const Boid* boid, const Flock* flock, float radius) {
    Vec3 sum = {0, 0, 0};
    u32 count = 0;
    
    for (u32 i = 0; i < flock->boid_count; i++) {
        if (flock->boids[i].entity.id == boid->entity.id) {
            continue; // Skip self
        }
        
        float d = vec3_distance(boid->position, flock->boids[i].position);
        if (d > 0 && d < radius) {
            sum = vec3_add(sum, flock->boids[i].position);
            count++;
        }
    }
    
    if (count > 0) {
        sum = vec3_mul(sum, 1.0f / count);
        return boid_seek_force(boid, sum);
    }
    
    return (Vec3){0, 0, 0};
}

Vec3 flocking_calculate_obstacle_avoidance(const Boid* boid, const Obstacle* obstacles, u32 obstacle_count, float radius) {
    Vec3 steer = {0, 0, 0};
    u32 count = 0;
    
    for (u32 i = 0; i < obstacle_count; i++) {
        float d = vec3_distance(boid->position, obstacles[i].position);
        if (d < radius + obstacles[i].radius) {
            Vec3 diff = vec3_sub(boid->position, obstacles[i].position);
            diff = vec3_normalize_safe(diff);
            diff = vec3_mul(diff, 1.0f / (d + 0.01f)); // Weight by distance
            steer = vec3_add(steer, diff);
            count++;
        }
    }
    
    if (count > 0) {
        steer = vec3_mul(steer, 1.0f / count);
        steer = vec3_normalize_safe(steer);
        steer = vec3_mul(steer, boid->max_speed);
        steer = vec3_sub(steer, boid->velocity);
        
        // Limit to max force
        float steer_length = vec3_length(steer);
        if (steer_length > boid->max_force) {
            steer = vec3_mul(vec3_normalize_safe(steer), boid->max_force);
        }
    }
    
    return steer;
}

Vec3 flocking_calculate_wander(const Boid* boid, float strength) {
    // Update wander angle
    float wander_angle = boid->wander_angle + (rand() / (float)RAND_MAX - 0.5f) * 0.3f;
    
    // Calculate wander force
    Vec3 wander_force = {
        cosf(wander_angle) * strength,
        (rand() / (float)RAND_MAX - 0.5f) * strength * 0.5f,
        sinf(wander_angle) * strength
    };
    
    // Update wander angle in boid
    ((Boid*)boid)->wander_angle = wander_angle; // Cast away const
    
    return wander_force;
}

// MARK: - Obstacle Management

void flocking_set_obstacles(Flock* flock, const Obstacle* obstacles, u32 count) {
    if (!flock || !flock->initialized) {
        return;
    }
    
    // Reallocate global obstacles array if needed
    if (count > g_obstacle_capacity) {
        Obstacle* new_obstacles = (Obstacle*)realloc(g_obstacles, count * sizeof(Obstacle));
        if (!new_obstacles) {
            LOG_ERROR("Failed to reallocate obstacles array");
            return;
        }
        g_obstacles = new_obstacles;
        g_obstacle_capacity = count;
    }
    
    // Copy obstacles
    if (obstacles && count > 0) {
        memcpy(g_obstacles, obstacles, count * sizeof(Obstacle));
        g_obstacle_count = count;
    } else {
        g_obstacle_count = 0;
    }
    
    LOG_INFO("Set %u obstacles for flocking", g_obstacle_count);
}

void flocking_add_obstacle(Flock* flock, const Obstacle* obstacle) {
    if (!flock || !flock->initialized || !obstacle) {
        return;
    }
    
    // Reallocate if needed
    if (g_obstacle_count >= g_obstacle_capacity) {
        u32 new_capacity = g_obstacle_capacity == 0 ? 16 : g_obstacle_capacity * 2;
        Obstacle* new_obstacles = (Obstacle*)realloc(g_obstacles, new_capacity * sizeof(Obstacle));
        if (!new_obstacles) {
            LOG_ERROR("Failed to reallocate obstacles array");
            return;
        }
        g_obstacles = new_obstacles;
        g_obstacle_capacity = new_capacity;
    }
    
    // Add obstacle
    g_obstacles[g_obstacle_count++] = *obstacle;
    LOG_DEBUG("Added obstacle to flocking system (total: %u)", g_obstacle_count);
}

void flocking_clear_obstacles(Flock* flock) {
    if (!flock || !flock->initialized) {
        return;
    }
    
    g_obstacle_count = 0;
    LOG_INFO("Cleared all obstacles from flocking system");
}

// MARK: - Utility Functions

void flocking_calculate_flock_statistics(Flock* flock) {
    if (!flock || !flock->initialized || flock->boid_count == 0) {
        return;
    }
    
    Vec3 center_of_mass = {0, 0, 0};
    Vec3 average_velocity = {0, 0, 0};
    
    for (u32 i = 0; i < flock->boid_count; i++) {
        center_of_mass = vec3_add(center_of_mass, flock->boids[i].position);
        average_velocity = vec3_add(average_velocity, flock->boids[i].velocity);
    }
    
    center_of_mass = vec3_mul(center_of_mass, 1.0f / flock->boid_count);
    average_velocity = vec3_mul(average_velocity, 1.0f / flock->boid_count);
    
    flock->center_of_mass = center_of_mass;
    flock->average_velocity = average_velocity;
}

Vec3 flocking_get_center_of_mass(const Flock* flock) {
    if (!flock || !flock->initialized) {
        return (Vec3){0, 0, 0};
    }
    return flock->center_of_mass;
}

Vec3 flocking_get_average_velocity(const Flock* flock) {
    if (!flock || !flock->initialized) {
        return (Vec3){0, 0, 0};
    }
    return flock->average_velocity;
}

u32 flocking_get_boid_count(const Flock* flock) {
    if (!flock || !flock->initialized) {
        return 0;
    }
    return flock->boid_count;
}

// MARK: - Configuration

void flocking_set_config(Flock* flock, const FlockConfig* config) {
    if (!flock || !flock->initialized || !config) {
        return;
    }
    
    flock->config = *config;
    
    // Update existing boids with new config values
    for (u32 i = 0; i < flock->boid_count; i++) {
        flock->boids[i].max_speed = config->max_speed;
        flock->boids[i].max_force = config->max_force;
        flock->boids[i].perception_radius = config->perception_radius;
    }
    
    LOG_INFO("Updated flock configuration");
}

FlockConfig flocking_get_default_config(void) {
    FlockConfig config = {0};
    
    config.separation_radius = 25.0f;
    config.alignment_radius = 50.0f;
    config.cohesion_radius = 50.0f;
    config.obstacle_avoidance_radius = 30.0f;
    
    config.max_speed = 2.0f;
    config.max_force = 0.05f;
    config.perception_radius = 50.0f;
    
    config.separation_weight = 1.5f;
    config.alignment_weight = 1.0f;
    config.cohesion_weight = 1.0f;
    config.obstacle_avoidance_weight = 2.0f;
    
    config.boid_count_limit = 100;
    config.enable_obstacle_avoidance = true;
    config.enable_wander = true;
    config.enable_3d_flocking = true;
    
    return config;
}

void flocking_update_boid_weights(Flock* flock, float separation, float alignment, float cohesion, float obstacle_avoidance) {
    if (!flock || !flock->initialized) {
        return;
    }
    
    for (u32 i = 0; i < flock->boid_count; i++) {
        flock->boids[i].separation_weight = separation;
        flock->boids[i].alignment_weight = alignment;
        flock->boids[i].cohesion_weight = cohesion;
        flock->boids[i].obstacle_avoidance_weight = obstacle_avoidance;
    }
    
    LOG_DEBUG("Updated boid weights");
}

// MARK: - Crowd Simulation

bool crowd_simulation_init(CrowdSimulation* crowd, u32 max_flocks) {
    if (!crowd || max_flocks == 0) {
        LOG_ERROR("Invalid parameters for crowd_simulation_init");
        return false;
    }
    
    memset(crowd, 0, sizeof(CrowdSimulation));
    
    crowd->flocks = (Flock*)calloc(max_flocks, sizeof(Flock));
    if (!crowd->flocks) {
        LOG_ERROR("Failed to allocate flocks array");
        return false;
    }
    
    crowd->capacity = max_flocks;
    crowd->flock_separation_radius = 100.0f;
    crowd->flock_avoidance_weight = 1.0f;
    crowd->enable_inter_flock_interactions = true;
    
    LOG_INFO("Crowd simulation initialized with capacity: %u", max_flocks);
    return true;
}

void crowd_simulation_shutdown(CrowdSimulation* crowd) {
    if (!crowd) {
        return;
    }
    
    // Shutdown each flock
    for (u32 i = 0; i < crowd->flock_count; i++) {
        flocking_shutdown(&crowd->flocks[i]);
    }
    
    if (crowd->flocks) {
        free(crowd->flocks);
        crowd->flocks = NULL;
    }
    
    memset(crowd, 0, sizeof(CrowdSimulation));
    LOG_INFO("Crowd simulation shutdown");
}

void crowd_simulation_update(CrowdSimulation* crowd, float delta_time) {
    if (!crowd || delta_time <= 0.0f) {
        return;
    }
    
    // Update each flock
    for (u32 i = 0; i < crowd->flock_count; i++) {
        flocking_update(&crowd->flocks[i], delta_time);
    }
    
    // Handle inter-flock interactions if enabled
    if (crowd->enable_inter_flock_interactions) {
        for (u32 i = 0; i < crowd->flock_count; i++) {
            for (u32 j = i + 1; j < crowd->flock_count; j++) {
                Vec3 flock_i_center = flocking_get_center_of_mass(&crowd->flocks[i]);
                Vec3 flock_j_center = flocking_get_center_of_mass(&crowd->flocks[j]);
                
                float distance = vec3_distance(flock_i_center, flock_j_center);
                if (distance < crowd->flock_separation_radius && distance > 0.1f) {
                    // Apply separation force between flocks
                    Vec3 separation = vec3_sub(flock_i_center, flock_j_center);
                    separation = vec3_normalize_safe(separation);
                    separation = vec3_mul(separation, crowd->flock_avoidance_weight / distance);
                    
                    // Apply force to all boids in both flocks
                    for (u32 k = 0; k < crowd->flocks[i].boid_count; k++) {
                        boid_apply_force(&crowd->flocks[i].boids[k], separation);
                    }
                    
                    for (u32 k = 0; k < crowd->flocks[j].boid_count; k++) {
                        boid_apply_force(&crowd->flocks[j].boids[k], vec3_mul(separation, -1.0f));
                    }
                }
            }
        }
    }
}

void crowd_simulation_render(CrowdSimulation* crowd) {
    if (!crowd) {
        return;
    }
    
    // Render each flock
    for (u32 i = 0; i < crowd->flock_count; i++) {
        flocking_render(&crowd->flocks[i]);
    }
}

bool crowd_simulation_add_flock(CrowdSimulation* crowd, const Flock* flock) {
    if (!crowd || !flock || crowd->flock_count >= crowd->capacity) {
        return false;
    }
    
    crowd->flocks[crowd->flock_count] = *flock;
    crowd->flock_count++;
    
    LOG_DEBUG("Added flock to crowd simulation (total: %u)", crowd->flock_count);
    return true;
}

bool crowd_simulation_remove_flock(CrowdSimulation* crowd, u32 flock_index) {
    if (!crowd || flock_index >= crowd->flock_count) {
        return false;
    }
    
    // Shutdown the flock being removed
    flocking_shutdown(&crowd->flocks[flock_index]);
    
    // Shift remaining flocks
    for (u32 i = flock_index; i < crowd->flock_count - 1; i++) {
        crowd->flocks[i] = crowd->flocks[i + 1];
    }
    
    crowd->flock_count--;
    LOG_DEBUG("Removed flock from crowd simulation (total: %u)", crowd->flock_count);
    return true;
}

Flock* crowd_simulation_get_flock(CrowdSimulation* crowd, u32 flock_index) {
    if (!crowd || flock_index >= crowd->flock_count) {
        return NULL;
    }
    
    return &crowd->flocks[flock_index];
}

// MARK: - Helper Functions

Boid* boid_create(Entity entity, Vec3 position, Vec3 velocity, Vec3 color, float size) {
    Boid* boid = (Boid*)calloc(1, sizeof(Boid));
    if (!boid) {
        LOG_ERROR("Failed to allocate boid");
        return NULL;
    }
    
    boid->entity = entity;
    boid->position = position;
    boid->velocity = velocity;
    boid->acceleration = (Vec3){0, 0, 0};
    boid->color = color;
    boid->size = size;
    
    // Default parameters
    boid->max_speed = 2.0f;
    boid->max_force = 0.05f;
    boid->perception_radius = 50.0f;
    
    boid->wander_angle = (rand() / (float)RAND_MAX) * 2.0f * M_PI;
    boid->wander_strength = 0.1f;
    
    LOG_DEBUG("Created boid");
    return boid;
}

void boid_destroy(Boid* boid) {
    if (boid) {
        free(boid);
    }
}

void boid_update(Boid* boid, Vec3 force, float delta_time) {
    if (!boid) {
        return;
    }
    
    // F = ma, so a = F/m (assuming m = 1)
    boid->acceleration = force;
    
    // Update velocity: v = v0 + a*t
    boid->velocity = vec3_add(boid->velocity, vec3_mul(boid->acceleration, delta_time));
    
    // Limit velocity to max speed
    float speed = vec3_length(boid->velocity);
    if (speed > boid->max_speed) {
        boid->velocity = vec3_mul(vec3_normalize_safe(boid->velocity), boid->max_speed);
    }
}

void boid_apply_force(Boid* boid, Vec3 force) {
    if (!boid) {
        return;
    }
    
    boid->acceleration = vec3_add(boid->acceleration, force);
}

void boid_seek(Boid* boid, Vec3 target, float weight) {
    if (!boid) {
        return;
    }
    
    Vec3 desired = vec3_sub(target, boid->position);
    desired = vec3_normalize_safe(desired);
    desired = vec3_mul(desired, boid->max_speed);
    
    Vec3 steer = vec3_sub(desired, boid->velocity);
    
    // Limit to max force
    float steer_length = vec3_length(steer);
    if (steer_length > boid->max_force) {
        steer = vec3_mul(vec3_normalize_safe(steer), boid->max_force);
    }
    
    steer = vec3_mul(steer, weight);
    boid_apply_force(boid, steer);
}

void boid_flee(Boid* boid, Vec3 threat, float weight) {
    if (!boid) {
        return;
    }
    
    Vec3 desired = vec3_sub(boid->position, threat);
    desired = vec3_normalize_safe(desired);
    desired = vec3_mul(desired, boid->max_speed);
    
    Vec3 steer = vec3_sub(desired, boid->velocity);
    
    // Limit to max force
    float steer_length = vec3_length(steer);
    if (steer_length > boid->max_force) {
        steer = vec3_mul(vec3_normalize_safe(steer), boid->max_force);
    }
    
    steer = vec3_mul(steer, weight);
    boid_apply_force(boid, steer);
}

void boid_arrive(Boid* boid, Vec3 target, float slowing_radius, float weight) {
    if (!boid) {
        return;
    }
    
    Vec3 desired = vec3_sub(target, boid->position);
    float d = vec3_length(desired);
    
    if (d < slowing_radius) {
        // Inside slowing radius, map speed to radius
        float speed = (d / slowing_radius) * boid->max_speed;
        desired = vec3_normalize_safe(desired);
        desired = vec3_mul(desired, speed);
    } else {
        // Outside slowing radius, full speed
        desired = vec3_normalize_safe(desired);
        desired = vec3_mul(desired, boid->max_speed);
    }
    
    Vec3 steer = vec3_sub(desired, boid->velocity);
    
    // Limit to max force
    float steer_length = vec3_length(steer);
    if (steer_length > boid->max_force) {
        steer = vec3_mul(vec3_normalize_safe(steer), boid->max_force);
    }
    
    steer = vec3_mul(steer, weight);
    boid_apply_force(boid, steer);
}

// MARK: - Legacy Compatibility Functions

void flocking_init_legacy(void) {
    // Legacy function - creates a default global flock
    LOG_INFO("Legacy flocking_init called - use flocking_init(Flock*, FlockConfig*) instead");
}

void flocking_add_boid_legacy(void *boid) {
    // Legacy function - not implemented in new system
    (void)boid;
    LOG_WARN("Legacy flocking_add_boid called - use flocking_add_boid(Flock*, Boid*) instead");
}

void flocking_update_legacy(float dt) {
    // Legacy function - not implemented in new system
    (void)dt;
    LOG_WARN("Legacy flocking_update called - use flocking_update(Flock*, float) instead");
}

// MARK: - Debug and Visualization

void flocking_debug_print_flock_info(const Flock* flock) {
    if (!flock || !flock->initialized) {
        printf("Flock is not initialized\n");
        return;
    }
    
    printf("=== FLOCK INFO ===\n");
    printf("Boid Count: %u/%u\n", flock->boid_count, flock->capacity);
    printf("Center of Mass: (%.2f, %.2f, %.2f)\n", 
           flock->center_of_mass.x, flock->center_of_mass.y, flock->center_of_mass.z);
    printf("Average Velocity: (%.2f, %.2f, %.2f)\n", 
           flock->average_velocity.x, flock->average_velocity.y, flock->average_velocity.z);
    printf("Max Speed: %.2f\n", flock->config.max_speed);
    printf("Max Force: %.2f\n", flock->config.max_force);
    printf("================\n");
}

void flocking_debug_render_boid_connections(const Flock* flock) {
    if (!flock || !flock->initialized || flock->boid_count < 2) {
        return;
    }
    
    // TODO: Render lines between nearby boids for visualization
    // This would integrate with the renderer to show perception ranges
}

void flocking_debug_render_perception_radii(const Flock* flock) {
    if (!flock || !flock->initialized) {
        return;
    }
    
    // TODO: Render perception spheres for each boid
    // This would integrate with the renderer to show perception ranges
}
