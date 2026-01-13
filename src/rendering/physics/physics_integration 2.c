// src/engine/rendering/physics/physics_integration.c
// Physics Integration - Real-time physics simulation with rendering integration

#include <core/logger.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../render_pipeline.h"

// ============================================================================
// Physics Integration Types
// ============================================================================

typedef enum {
    PHYSICS_TYPE_RIGID_BODY,
    PHYSICS_TYPE_SOFT_BODY,
    PHYSICS_TYPE_FLUID,
    PHYSICS_TYPE_CLOTH,
    PHYSICS_TYPE_PARTICLE,
    PHYSICS_TYPE_COLLISION,
    PHYSICS_TYPE_CONSTRAINT,
    PHYSICS_TYPE_COUNT
} PhysicsType;

typedef struct {
    float position[3];
    float velocity[3];
    float acceleration[3];
    float rotation[4];      // Quaternion
    float angular_velocity[3];
    float angular_acceleration[3];
    float mass;
    float inverse_mass;
    float inertia_tensor[9]; // 3x3 matrix
    float inverse_inertia_tensor[9];
    bool is_static;
    bool is_kinematic;
    bool is_sleeping;
    float sleep_threshold;
} RigidBody;

typedef struct {
    float *vertices;
    uint32_t vertex_count;
    float *normals;
    uint32_t normal_count;
    float *indices;
    uint32_t index_count;
    float *masses;
    uint32_t mass_count;
    float *rest_lengths;
    uint32_t rest_length_count;
    float stiffness;
    float damping;
    bool is_animated;
} SoftBody;

typedef struct {
    float *particles;
    uint32_t particle_count;
    float *velocities;
    uint32_t velocity_count;
    float *densities;
    uint32_t density_count;
    float *pressures;
    uint32_t pressure_count;
    float viscosity;
    float rest_density;
    float smoothing_radius;
    float pressure_constant;
    float gravity[3];
    bool is_2d;
} FluidSimulation;

typedef struct {
    float *vertices;
    uint32_t vertex_count;
    float *normals;
    uint32_t normal_count;
    float *uvs;
    uint32_t uv_count;
    float *masses;
    uint32_t mass_count;
    float *rest_lengths;
    uint32_t rest_length_count;
    float stiffness;
    float damping;
    float thickness;
    bool is_animated;
} Cloth;

typedef struct {
    RigidBody *rigid_bodies;
    uint32_t rigid_body_count;
    uint32_t rigid_body_capacity;
    
    SoftBody *soft_bodies;
    uint32_t soft_body_count;
    uint32_t soft_body_capacity;
    
    FluidSimulation *fluids;
    uint32_t fluid_count;
    uint32_t fluid_capacity;
    
    Cloth *cloths;
    uint32_t cloth_count;
    uint32_t cloth_capacity;
    
    // Physics world parameters
    float gravity[3];
    float time_step;
    uint32_t max_substeps;
    float linear_damping;
    float angular_damping;
    
    // Collision detection
    bool enable_collision_detection;
    float collision_margin;
    uint32_t max_collision_pairs;
    
    // Performance
    bool enable_multithreading;
    uint32_t thread_count;
    bool enable_sleeping;
    float sleep_threshold;
    
    // Statistics
    uint64_t total_simulation_time_ms;
    uint32_t bodies_simulated;
    uint32_t collision_checks;
    uint32_t constraint_solves;
    float average_fps;
    
    // GPU resources
    void *physics_buffer;
    void *collision_buffer;
    void *constraint_buffer;
    
    bool initialized;
} PhysicsSystem;

static PhysicsSystem g_physics_system = {0};

// ============================================================================
// Physics Mathematics
// ============================================================================

static void vector3_add(const float *a, const float *b, float *result) {
    result[0] = a[0] + b[0];
    result[1] = a[1] + b[1];
    result[2] = a[2] + b[2];
}

static void vector3_subtract(const float *a, const float *b, float *result) {
    result[0] = a[0] - b[0];
    result[1] = a[1] - b[1];
    result[2] = a[2] - b[2];
}

static void vector3_multiply(const float *v, float scalar, float *result) {
    result[0] = v[0] * scalar;
    result[1] = v[1] * scalar;
    result[2] = v[2] * scalar;
}

static void vector3_cross(const float *a, const float *b, float *result) {
    result[0] = a[1] * b[2] - a[2] * b[1];
    result[1] = a[2] * b[0] - a[0] * b[2];
    result[2] = a[0] * b[1] - a[1] * b[0];
}

static float vector3_dot(const float *a, const float *b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

static float vector3_length(const float *v) {
    return sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

static void vector3_normalize(float *v) {
    float length = vector3_length(v);
    if (length > 0.0f) {
        v[0] /= length;
        v[1] /= length;
        v[2] /= length;
    }
}

static void quaternion_multiply(const float *q1, const float *q2, float *result) {
    result[0] = q1[0] * q2[0] - q1[1] * q2[1] - q1[2] * q2[2] - q1[3] * q2[3];
    result[1] = q1[0] * q2[1] + q1[1] * q2[0] + q1[2] * q2[3] - q1[3] * q2[2];
    result[2] = q1[0] * q2[2] - q1[1] * q2[3] + q1[2] * q2[0] + q1[3] * q2[1];
    result[3] = q1[0] * q2[3] + q1[1] * q2[2] - q1[2] * q2[1] + q1[3] * q2[0];
}

static void quaternion_normalize(float *q) {
    float length = sqrtf(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
    if (length > 0.0f) {
        q[0] /= length;
        q[1] /= length;
        q[2] /= length;
        q[3] /= length;
    }
}

static void matrix_multiply_3x3(const float *m1, const float *m2, float *result) {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            result[i * 3 + j] = 0.0f;
            for (int k = 0; k < 3; k++) {
                result[i * 3 + j] += m1[i * 3 + k] * m2[k * 3 + j];
            }
        }
    }
}

// ============================================================================
// Rigid Body Physics
// ============================================================================

static void rigid_body_integrate(RigidBody *body, float dt) {
    if (!body || body->is_static || body->is_sleeping) return;
    
    // Update velocity
    vector3_add(body->velocity, body->acceleration, body->velocity);
    vector3_multiply(body->velocity, dt, body->velocity);
    
    // Update position
    vector3_add(body->position, body->velocity, body->position);
    vector3_multiply(body->position, dt, body->position);
    
    // Update angular velocity
    vector3_add(body->angular_velocity, body->angular_acceleration, body->angular_velocity);
    vector3_multiply(body->angular_velocity, dt, body->angular_velocity);
    
    // Update rotation
    float rotation_delta[4];
    float angular_velocity_quat[4] = {0.0f, body->angular_velocity[0], body->angular_velocity[1], body->angular_velocity[2]};
    quaternion_multiply(body->rotation, angular_velocity_quat, rotation_delta);
    quaternion_normalize(rotation_delta);
    quaternion_multiply(rotation_delta, body->rotation, body->rotation);
    quaternion_normalize(body->rotation);
    
    // Apply damping
    vector3_multiply(body->velocity, 1.0f - g_physics_system.linear_damping * dt, body->velocity);
    vector3_multiply(body->angular_velocity, 1.0f - g_physics_system.angular_damping * dt, body->angular_velocity);
    
    // Check if body should sleep
    float linear_speed = vector3_length(body->velocity);
    float angular_speed = vector3_length(body->angular_velocity);
    
    if (linear_speed < body->sleep_threshold && angular_speed < body->sleep_threshold) {
        body->is_sleeping = true;
        body->velocity[0] = body->velocity[1] = body->velocity[2] = 0.0f;
        body->angular_velocity[0] = body->angular_velocity[1] = body->angular_velocity[2] = 0.0f;
    }
}

static void rigid_body_apply_force(RigidBody *body, const float *force, const float *point) {
    if (!body || body->is_static || body->is_sleeping) return;
    
    // Apply linear force
    float linear_acceleration[3];
    vector3_multiply(force, body->inverse_mass, linear_acceleration);
    vector3_add(body->acceleration, linear_acceleration, body->acceleration);
    
    // Apply torque (cross product of lever arm and force)
    float lever_arm[3];
    vector3_subtract(point, body->position, lever_arm);
    
    float torque[3];
    vector3_cross(lever_arm, force, torque);
    
    // Convert torque to angular acceleration
    float angular_acceleration[3];
    matrix_multiply_3x3(body->inverse_inertia_tensor, torque, angular_acceleration);
    vector3_add(body->angular_acceleration, angular_acceleration, body->angular_acceleration);
    
    // Apply inverse inertia tensor for proper angular dynamics
    float inv_inertia[9];
    matrix_multiply_3x3(body->inverse_inertia_tensor, torque, inv_inertia);
    vector3_add(body->angular_acceleration, inv_inertia, body->angular_acceleration);
    
    // Wake up body
    body->is_sleeping = false;
}

// ============================================================================
// Soft Body Physics
// ============================================================================

static void soft_body_integrate(SoftBody *soft_body, float dt) {
    if (!soft_body || !soft_body->is_animated) return;
    
    // Integrate particle positions
    for (uint32_t i = 0; i < soft_body->vertex_count; i++) {
        uint32_t index = i * 3;
        
        // Apply spring forces between particles
        for (uint32_t j = 0; j < soft_body->rest_length_count; j++) {
            // Calculate spring force using Hooke's law: F = -k * (x - x0)
            uint32_t particle1_idx = (j * 2) * 3;
            uint32_t particle2_idx = (j * 2 + 1) * 3;
            
            if (particle1_idx < soft_body->vertex_count * 3 && particle2_idx < soft_body->vertex_count * 3) {
                float displacement[3];
                vector3_subtract(&soft_body->vertices[particle2_idx], &soft_body->vertices[particle1_idx], displacement);
                
                float current_length = vector3_length(displacement);
                float rest_length = soft_body->rest_lengths[j];
                
                if (current_length > 0.0f) {
                    float spring_force_magnitude = soft_body->stiffness * (current_length - rest_length);
                    
                    float force_direction[3];
                    vector3_normalize(displacement);
                    vector3_multiply(force_direction, spring_force_magnitude, force_direction);
                    
                    // Implement spring forces between particles
                    float spring_force[3];
                    vector3_multiply(force_direction, spring_force_magnitude, spring_force);
                    
                    // Apply equal and opposite forces
                    float acceleration1[3];
                    float acceleration2[3];
                    vector3_multiply(spring_force, 1.0f / soft_body->masses[i], acceleration1);
                    vector3_multiply(spring_force, -1.0f / soft_body->masses[particle2_idx / 3], acceleration2);
                    
                    // Add to particle accelerations (would need per-particle acceleration array)
                    // For now, apply to current particle
                    vector3_add(&soft_body->vertices[index], acceleration1, &soft_body->vertices[index]);
                }
            }
        }
        
        // Apply damping forces to reduce oscillations
        float velocity[3] = {
            (soft_body->vertices[index] - soft_body->vertices[index - 3]) / dt, // Simplified velocity
            (soft_body->vertices[index + 1] - soft_body->vertices[index - 2]) / dt,
            (soft_body->vertices[index + 2] - soft_body->vertices[index - 1]) / dt
        };
        
        // Implement damping forces
        float damping_force[3];
        vector3_multiply(velocity, -soft_body->damping, damping_force);
        
        // Apply damping acceleration
        float damping_acceleration[3];
        vector3_multiply(damping_force, 1.0f / soft_body->masses[i], damping_acceleration);
        vector3_add(&soft_body->vertices[index], damping_acceleration, &soft_body->vertices[index]);
        
        // Integrate particle positions using Verlet integration
        float acceleration[3] = {0.0f, -9.81f, 0.0f}; // Gravity
        
        // Integrate particle positions
        float old_velocity[3] = {
            (soft_body->vertices[index] - soft_body->vertices[index - 3]) / dt,
            (soft_body->vertices[index + 1] - soft_body->vertices[index - 2]) / dt,
            (soft_body->vertices[index + 2] - soft_body->vertices[index - 1]) / dt
        };
        
        // Verlet integration: x(t+dt) = x(t) + v(t)*dt + a*dt^2
        soft_body->vertices[index] += old_velocity[0] * dt + acceleration[0] * dt * dt;
        soft_body->vertices[index + 1] += old_velocity[1] * dt + acceleration[1] * dt * dt;
        soft_body->vertices[index + 2] += old_velocity[2] * dt + acceleration[2] * dt * dt;
    }
}

// ============================================================================
// Fluid Simulation
// ============================================================================

static void fluid_simulation_integrate(FluidSimulation *fluid, float dt) {
    if (!fluid) return;
    
    // SPH (Smoothed Particle Hydrodynamics) simulation
    for (uint32_t i = 0; i < fluid->particle_count; i++) {
        uint32_t index = i * 3;
        
        // Calculate density
        float density = 0.0f;
        for (uint32_t j = 0; j < fluid->particle_count; j++) {
            uint32_t neighbor_index = j * 3;
            
            float distance_vector[3];
            vector3_subtract(&fluid->particles[index], &fluid->particles[neighbor_index], distance_vector);
            float distance = vector3_length(distance_vector);
            
            if (distance < fluid->smoothing_radius) {
                // Apply Poly6 smoothing kernel for density calculation
                float h = fluid->smoothing_radius;
                float h_squared = h * h;
                float h_ninth = h_squared * h_squared * h_squared * h_squared * h;
                
                float diff = h_squared - distance * distance;
                float kernel = 315.0f / (64.0f * M_PI * h_ninth) * diff * diff * diff;
                
                density += kernel;
            }
        }
        
        fluid->densities[i] = density;
        
        // Calculate pressure
        fluid->pressures[i] = fluid->pressure_constant * (density - fluid->rest_density);
        
        // Calculate pressure force
        float pressure_force[3] = {0.0f, 0.0f, 0.0f};
        for (uint32_t j = 0; j < fluid->particle_count; j++) {
            if (i == j) continue;
            
            uint32_t neighbor_index = j * 3;
            
            float distance_vector[3];
            vector3_subtract(&fluid->particles[index], &fluid->particles[neighbor_index], distance_vector);
            float distance = vector3_length(distance_vector);
            
            if (distance < fluid->smoothing_radius) {
                // Apply Spiky kernel gradient for pressure calculation
                float h = fluid->smoothing_radius;
                float h_sixth = h * h * h * h * h * h;
                
                float diff = h - distance;
                float kernel_magnitude = -45.0f / (M_PI * h_sixth) * diff * diff;
                
                float pressure_gradient = (fluid->pressures[i] + fluid->pressures[j]) / (2.0f * density);
                float force_magnitude = pressure_gradient * kernel_magnitude;
                
                float force_direction[3];
                if (distance > 0.0f) {
                    force_direction[0] = distance_vector[0] / distance;
                    force_direction[1] = distance_vector[1] / distance;
                    force_direction[2] = distance_vector[2] / distance;
                } else {
                    force_direction[0] = force_direction[1] = force_direction[2] = 0.0f;
                }
                
                vector3_multiply(force_direction, force_magnitude, force_direction);
                vector3_add(pressure_force, force_direction, pressure_force);
            }
        }
        
        // Apply viscosity forces using viscosity kernel
        float viscosity_force[3] = {0.0f, 0.0f, 0.0f};
        for (uint32_t j = 0; j < fluid->particle_count; j++) {
            if (i == j) continue;
            
            uint32_t neighbor_index = j * 3;
            
            float distance_vector[3];
            vector3_subtract(&fluid->particles[index], &fluid->particles[neighbor_index], distance_vector);
            float distance = vector3_length(distance_vector);
            
            if (distance < fluid->smoothing_radius) {
                // Apply viscosity kernel (Laplacian of velocity)
                float h = fluid->smoothing_radius;
                float h_sixth = h * h * h * h * h * h;
                
                float kernel_laplacian = 45.0f / (M_PI * h_sixth) * (h - distance);
                
                // Apply smoothing kernel
                float smoothing_factor = 1.0f - (distance / fluid->smoothing_radius);
                smoothing_factor = smoothing_factor * smoothing_factor * smoothing_factor; // Cubic smoothing
                
                float velocity_diff[3];
                vector3_subtract(&fluid->velocities[neighbor_index], &fluid->velocities[index], velocity_diff);
                
                float viscosity_factor = fluid->viscosity * kernel_laplacian * smoothing_factor / fluid->densities[j];
                vector3_multiply(velocity_diff, viscosity_factor, velocity_diff);
                vector3_add(viscosity_force, velocity_diff, viscosity_force);
            }
        }
        
        // Apply gravity
        float gravity_force[3];
        vector3_multiply(fluid->gravity, fluid->densities[i], gravity_force);
        
        // Total force
        float total_force[3];
        vector3_add(pressure_force, gravity_force, total_force);
        vector3_add(total_force, viscosity_force, total_force);
        
        // Update velocity
        float acceleration[3];
        vector3_multiply(total_force, 1.0f / fluid->densities[i], acceleration);
        vector3_add(&fluid->velocities[index], acceleration, &fluid->velocities[index]);
        vector3_multiply(&fluid->velocities[index], dt, &fluid->velocities[index]);
        
        // Update position
        vector3_add(&fluid->particles[index], &fluid->velocities[index], &fluid->particles[index]);
        vector3_multiply(&fluid->particles[index], dt, &fluid->particles[index]);
    }
}

// ============================================================================
// Cloth Simulation
// ============================================================================

static void cloth_integrate(Cloth *cloth, float dt) {
    if (!cloth || !cloth->is_animated) return;
    
    // Mass-spring system for cloth
    for (uint32_t i = 0; i < cloth->vertex_count; i++) {
        uint32_t index = i * 3;
        
        // Apply spring forces between cloth vertices using mass-spring model
        for (uint32_t j = 0; j < cloth->rest_length_count; j++) {
            // Structural springs (horizontal and vertical)
            uint32_t v1_idx = j * 3;
            uint32_t v2_idx = ((j + 1) % cloth->vertex_count) * 3;
            
            if (v2_idx < cloth->vertex_count * 3) {
                float displacement[3];
                vector3_subtract(&cloth->vertices[v2_idx], &cloth->vertices[v1_idx], displacement);
                
                float current_length = vector3_length(displacement);
                float rest_length = cloth->rest_lengths[j];
                
                if (current_length > 0.0f) {
                    float spring_force = cloth->stiffness * (current_length - rest_length);
                    
                    float force_direction[3];
                    force_direction[0] = displacement[0] / current_length;
                    force_direction[1] = displacement[1] / current_length;
                    force_direction[2] = displacement[2] / current_length;
                    
                    float spring_acceleration[3];
                    vector3_multiply(force_direction, spring_force / cloth->masses[i], spring_acceleration);
                    vector3_add(&cloth->vertices[index], spring_acceleration, &cloth->vertices[index]);
                }
            }
            
            // Shear springs (diagonal)
            if (j < cloth->rest_length_count / 2) {
                uint32_t diag_idx = (j + cloth->vertex_count / 2) % cloth->vertex_count;
                uint32_t v3_idx = diag_idx * 3;
                
                if (v3_idx < cloth->vertex_count * 3) {
                    float diag_displacement[3];
                    vector3_subtract(&cloth->vertices[v3_idx], &cloth->vertices[v1_idx], diag_displacement);
                    
                    float diag_length = vector3_length(diag_displacement);
                    float diag_rest_length = rest_length * 1.414f; // sqrt(2) for diagonal
                    
                    if (diag_length > 0.0f) {
                        float shear_force = cloth->stiffness * 0.5f * (diag_length - diag_rest_length);
                        
                        float shear_direction[3];
                        shear_direction[0] = diag_displacement[0] / diag_length;
                        shear_direction[1] = diag_displacement[1] / diag_length;
                        shear_direction[2] = diag_displacement[2] / diag_length;
                        
                        float shear_acceleration[3];
                        vector3_multiply(shear_direction, shear_force / cloth->masses[i], shear_acceleration);
                        vector3_add(&cloth->vertices[index], shear_acceleration, &cloth->vertices[index]);
                    }
                }
            }
        }
        
        // Apply gravity
        float gravity_force[3];
        vector3_multiply(g_physics_system.gravity, cloth->masses[i], gravity_force);
        
        // Apply damping
        float damping_force[3];
        vector3_multiply(&cloth->vertices[index], -cloth->damping, damping_force);
        
        // Total force
        float total_force[3];
        vector3_add(gravity_force, damping_force, total_force);
        
        // Update velocity
        float acceleration[3];
        vector3_multiply(total_force, 1.0f / cloth->masses[i], acceleration);
        vector3_add(&cloth->vertices[index], acceleration, &cloth->vertices[index]);
        vector3_multiply(&cloth->vertices[index], dt, &cloth->vertices[index]);
        
        // Update position
        vector3_add(&cloth->vertices[index], &cloth->vertices[index], &cloth->vertices[index]);
        vector3_multiply(&cloth->vertices[index], dt, &cloth->vertices[index]);
    }
}

// ============================================================================
// Collision Detection
// ============================================================================

static void detect_collisions(void) {
    if (!g_physics_system.enable_collision_detection) return;
    
    // Simple sphere-sphere collision detection
    for (uint32_t i = 0; i < g_physics_system.rigid_body_count; i++) {
        for (uint32_t j = i + 1; j < g_physics_system.rigid_body_count; j++) {
            RigidBody *body1 = &g_physics_system.rigid_bodies[i];
            RigidBody *body2 = &g_physics_system.rigid_bodies[j];
            
            if (body1->is_static && body2->is_static) continue;
            
            // Calculate distance between bodies
            float distance_vector[3];
            vector3_subtract(body1->position, body2->position, distance_vector);
            float distance = vector3_length(distance_vector);
            
            // Check collision (simplified - assuming unit spheres)
            if (distance < 2.0f) {
                // Collision detected
                g_physics_system.collision_checks++;
                
                // Calculate collision response
                float normal[3];
                vector3_normalize(distance_vector);
                
                // Separate bodies
                float penetration = 2.0f - distance;
                float separation_vector[3];
                vector3_multiply(normal, penetration * 0.5f, separation_vector);
                
                if (!body1->is_static) {
                    vector3_add(body1->position, separation_vector, body1->position);
                }
                
                if (!body2->is_static) {
                    vector3_subtract(body2->position, separation_vector, body2->position);
                }
                
                // Calculate impulse
                float relative_velocity[3];
                vector3_subtract(body1->velocity, body2->velocity, relative_velocity);
                
                float velocity_along_normal = vector3_dot(relative_velocity, normal);
                
                if (velocity_along_normal > 0.0f) continue; // Bodies moving apart
                
                float restitution = 0.8f; // Coefficient of restitution
                float impulse_magnitude = -(1.0f + restitution) * velocity_along_normal;
                impulse_magnitude /= body1->inverse_mass + body2->inverse_mass;
                
                float impulse[3];
                vector3_multiply(normal, impulse_magnitude, impulse);
                
                // Apply impulse
                if (!body1->is_static) {
                    float velocity_change1[3];
                    vector3_multiply(impulse, body1->inverse_mass, velocity_change1);
                    vector3_add(body1->velocity, velocity_change1, body1->velocity);
                    body1->is_sleeping = false;
                }
                
                if (!body2->is_static) {
                    float velocity_change2[3];
                    vector3_multiply(impulse, -body2->inverse_mass, velocity_change2);
                    vector3_add(body2->velocity, velocity_change2, body2->velocity);
                    body2->is_sleeping = false;
                }
            }
        }
    }
}

// ============================================================================
// Physics System API
// ============================================================================

bool physics_system_init(uint32_t max_rigid_bodies, uint32_t max_soft_bodies, uint32_t max_fluids, uint32_t max_cloths,
                        float gravity_x, float gravity_y, float gravity_z, float time_step, uint32_t max_substeps) {
    if (g_physics_system.initialized) {
        LOG_WARN("Physics system already initialized");
        return true;
    }
    
    memset(&g_physics_system, 0, sizeof(PhysicsSystem));
    
    g_physics_system.rigid_body_capacity = max_rigid_bodies;
    g_physics_system.soft_body_capacity = max_soft_bodies;
    g_physics_system.fluid_capacity = max_fluids;
    g_physics_system.cloth_capacity = max_cloths;
    
    g_physics_system.gravity[0] = gravity_x;
    g_physics_system.gravity[1] = gravity_y;
    g_physics_system.gravity[2] = gravity_z;
    g_physics_system.time_step = time_step;
    g_physics_system.max_substeps = max_substeps;
    g_physics_system.linear_damping = 0.01f;
    g_physics_system.angular_damping = 0.01f;
    
    g_physics_system.enable_collision_detection = true;
    g_physics_system.collision_margin = 0.01f;
    g_physics_system.max_collision_pairs = 1000;
    
    g_physics_system.enable_multithreading = true;
    g_physics_system.thread_count = 4;
    g_physics_system.enable_sleeping = true;
    g_physics_system.sleep_threshold = 0.1f;
    
    // Allocate memory
    g_physics_system.rigid_bodies = calloc(max_rigid_bodies, sizeof(RigidBody));
    g_physics_system.soft_bodies = calloc(max_soft_bodies, sizeof(SoftBody));
    g_physics_system.fluids = calloc(max_fluids, sizeof(FluidSimulation));
    g_physics_system.cloths = calloc(max_cloths, sizeof(Cloth));
    
    if (!g_physics_system.rigid_bodies || !g_physics_system.soft_bodies || 
        !g_physics_system.fluids || !g_physics_system.cloths) {
        LOG_ERROR("Failed to allocate physics system memory");
        free(g_physics_system.rigid_bodies);
        free(g_physics_system.soft_bodies);
        free(g_physics_system.fluids);
        free(g_physics_system.cloths);
        return false;
    }
    
    // Create GPU resources for physics simulation
    g_physics_system.physics_buffer = create_buffer(max_rigid_bodies * sizeof(RigidBody) + 
                                                   max_soft_bodies * sizeof(SoftBody) +
                                                   max_fluids * sizeof(FluidSimulation) +
                                                   max_cloths * sizeof(Cloth));
    
    g_physics_system.collision_buffer = create_buffer(g_physics_system.max_collision_pairs * sizeof(CollisionPair));
    
    // Estimate constraint count (simplified: 3 constraints per rigid body)
    uint32_t max_constraints = max_rigid_bodies * 3;
    g_physics_system.constraint_buffer = create_buffer(max_constraints * sizeof(Constraint));
    
    if (!g_physics_system.physics_buffer || !g_physics_system.collision_buffer || !g_physics_system.constraint_buffer) {
        LOG_ERROR("Failed to create GPU resources for physics system");
        // Cleanup partial allocation
        if (g_physics_system.physics_buffer) destroy_buffer(g_physics_system.physics_buffer);
        if (g_physics_system.collision_buffer) destroy_buffer(g_physics_system.collision_buffer);
        if (g_physics_system.constraint_buffer) destroy_buffer(g_physics_system.constraint_buffer);
        return false;
    }
    
    g_physics_system.initialized = true;
    LOG_INFO("Physics system initialized (rigid: %u, soft: %u, fluids: %u, cloths: %u, gravity: (%.1f, %.1f, %.1f), dt: %.3f, substeps: %u)",
             max_rigid_bodies, max_soft_bodies, max_fluids, max_cloths, gravity_x, gravity_y, gravity_z, time_step, max_substeps);
    return true;
}

void physics_system_shutdown(void) {
    if (!g_physics_system.initialized)
        return;
    
    LOG_INFO("Shutting down physics system");
    
    // Free memory
    free(g_physics_system.rigid_bodies);
    free(g_physics_system.soft_bodies);
    free(g_physics_system.fluids);
    free(g_physics_system.cloths);
    
    // Destroy GPU resources
    if (g_physics_system.physics_buffer) {
        destroy_buffer(g_physics_system.physics_buffer);
        g_physics_system.physics_buffer = NULL;
    }
    
    if (g_physics_system.collision_buffer) {
        destroy_buffer(g_physics_system.collision_buffer);
        g_physics_system.collision_buffer = NULL;
    }
    
    if (g_physics_system.constraint_buffer) {
        destroy_buffer(g_physics_system.constraint_buffer);
        g_physics_system.constraint_buffer = NULL;
    }
    
    memset(&g_physics_system, 0, sizeof(PhysicsSystem));
    
    LOG_INFO("Physics system shutdown complete");
}

void physics_system_update(float dt) {
    if (!g_physics_system.initialized) return;
    
    uint64_t start_time = get_time_nanos();
    
    // Substep integration
    float substep_dt = dt / (float)g_physics_system.max_substeps;
    
    for (uint32_t substep = 0; substep < g_physics_system.max_substeps; substep++) {
        // Update rigid bodies
        for (uint32_t i = 0; i < g_physics_system.rigid_body_count; i++) {
            rigid_body_integrate(&g_physics_system.rigid_bodies[i], substep_dt);
            g_physics_system.bodies_simulated++;
        }
        
        // Update soft bodies
        for (uint32_t i = 0; i < g_physics_system.soft_body_count; i++) {
            soft_body_integrate(&g_physics_system.soft_bodies[i], substep_dt);
            g_physics_system.bodies_simulated++;
        }
        
        // Update fluids
        for (uint32_t i = 0; i < g_physics_system.fluid_count; i++) {
            fluid_simulation_integrate(&g_physics_system.fluids[i], substep_dt);
            g_physics_system.bodies_simulated++;
        }
        
        // Update cloths
        for (uint32_t i = 0; i < g_physics_system.cloth_count; i++) {
            cloth_integrate(&g_physics_system.cloths[i], substep_dt);
            g_physics_system.bodies_simulated++;
        }
        
        // Collision detection
        detect_collisions();
    }
    
    uint64_t end_time = get_time_nanos();
    g_physics_system.total_simulation_time_ms += nanos_to_ms(end_time - start_time);
    g_physics_system.average_fps = 1000.0f / (g_physics_system.total_simulation_time_ms / (g_physics_system.bodies_simulated + 1));
    
    LOG_DEBUG("Physics update: %.2f ms, bodies: %u, collisions: %u, fps: %.1f",
             g_physics_system.total_simulation_time_ms, g_physics_system.bodies_simulated, g_physics_system.collision_checks, g_physics_system.average_fps);
}

uint32_t physics_system_create_rigid_body(const float *position, const float *velocity, float mass, bool is_static) {
    if (!g_physics_system.initialized || g_physics_system.rigid_body_count >= g_physics_system.rigid_body_capacity) {
        return UINT32_MAX;
    }
    
    RigidBody *body = &g_physics_system.rigid_bodies[g_physics_system.rigid_body_count++];
    
    if (position) {
        body->position[0] = position[0];
        body->position[1] = position[1];
        body->position[2] = position[2];
    }
    
    if (velocity) {
        body->velocity[0] = velocity[0];
        body->velocity[1] = velocity[1];
        body->velocity[2] = velocity[2];
    }
    
    body->mass = mass;
    body->inverse_mass = mass > 0.0f ? 1.0f / mass : 0.0f;
    body->is_static = is_static;
    body->is_kinematic = false;
    body->is_sleeping = false;
    body->sleep_threshold = g_physics_system.sleep_threshold;
    
    // Initialize rotation to identity
    body->rotation[0] = 1.0f;
    body->rotation[1] = body->rotation[2] = body->rotation[3] = 0.0f;
    
    // Initialize inertia tensor (simplified - sphere)
    float inertia = 0.4f * mass * 1.0f * 1.0f; // Sphere with radius 1.0
    body->inertia_tensor[0] = body->inertia_tensor[4] = body->inertia_tensor[8] = inertia;
    body->inverse_inertia_tensor[0] = body->inverse_inertia_tensor[4] = body->inverse_inertia_tensor[8] = 1.0f / inertia;
    
    return g_physics_system.rigid_body_count - 1;
}

void physics_system_apply_force(uint32_t body_id, const float *force, const float *point) {
    if (!g_physics_system.initialized || body_id >= g_physics_system.rigid_body_count) {
        return;
    }
    
    rigid_body_apply_force(&g_physics_system.rigid_bodies[body_id], force, point);
}

void physics_system_set_gravity(float x, float y, float z) {
    if (!g_physics_system.initialized) return;
    
    g_physics_system.gravity[0] = x;
    g_physics_system.gravity[1] = y;
    g_physics_system.gravity[2] = z;
    
    LOG_DEBUG("Updated gravity: (%.1f, %.1f, %.1f)", x, y, z);
}

void physics_system_get_body_position(uint32_t body_id, float *position) {
    if (!g_physics_system.initialized || body_id >= g_physics_system.rigid_body_count || !position) {
        return;
    }
    
    RigidBody *body = &g_physics_system.rigid_bodies[body_id];
    position[0] = body->position[0];
    position[1] = body->position[1];
    position[2] = body->position[2];
}

void physics_system_get_body_rotation(uint32_t body_id, float *rotation) {
    if (!g_physics_system.initialized || body_id >= g_physics_system.rigid_body_count || !rotation) {
        return;
    }
    
    RigidBody *body = &g_physics_system.rigid_bodies[body_id];
    rotation[0] = body->rotation[0];
    rotation[1] = body->rotation[1];
    rotation[2] = body->rotation[2];
    rotation[3] = body->rotation[3];
}

void physics_system_set_body_position(uint32_t body_id, const float *position) {
    if (!g_physics_system.initialized || body_id >= g_physics_system.rigid_body_count || !position) {
        return;
    }
    
    RigidBody *body = &g_physics_system.rigid_bodies[body_id];
    body->position[0] = position[0];
    body->position[1] = position[1];
    body->position[2] = position[2];
    body->is_sleeping = false;
}

void physics_system_set_body_velocity(uint32_t body_id, const float *velocity) {
    if (!g_physics_system.initialized || body_id >= g_physics_system.rigid_body_count || !velocity) {
        return;
    }
    
    RigidBody *body = &g_physics_system.rigid_bodies[body_id];
    body->velocity[0] = velocity[0];
    body->velocity[1] = velocity[1];
    body->velocity[2] = velocity[2];
    body->is_sleeping = false;
}

void physics_system_get_stats(float *simulation_time, uint32_t *bodies_simulated, uint32_t *collision_checks, float *average_fps) {
    if (!g_physics_system.initialized) return;
    
    if (simulation_time) *simulation_time = g_physics_system.total_simulation_time_ms;
    if (bodies_simulated) *bodies_simulated = g_physics_system.bodies_simulated;
    if (collision_checks) *collision_checks = g_physics_system.collision_checks;
    if (average_fps) *average_fps = g_physics_system.average_fps;
    
    LOG_DEBUG("Physics stats: time=%.2f ms, bodies=%u, collisions=%u, fps=%.1f",
             *simulation_time, *bodies_simulated, *collision_checks, *average_fps);
}

bool physics_system_is_initialized(void) {
    return g_physics_system.initialized;
}
