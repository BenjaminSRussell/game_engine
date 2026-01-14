// Physics Demo: Fluid Simulation - Particle-based fluid dynamics
#include "physics_demo_framework.c"
#include "engine/include/math/math_all.h"
#include <stdlib.h>

#define MAX_PARTICLES 500
#define PARTICLE_RADIUS 0.05f
#define SMOOTHING_RADIUS 0.15f
#define REST_DENSITY 1000.0f
#define GAS_CONSTANT 2000.0f
#define VISCOSITY 0.018f

typedef struct {
    Vec3 position;
    Vec3 velocity;
    Vec3 force;
    float density;
    float pressure;
    bool active;
} FluidParticle;

static FluidParticle particles[MAX_PARTICLES];
static int particle_count = 0;
static RigidBody* container_walls[5];
static bool demo_reset = false;
static float time_accumulator = 0.0f;

void fluid_simulation_init(PhysicsWorld* world) {
    LOG_INFO("Initializing Fluid Simulation Demo");
    
    // Clear existing particles
    memset(particles, 0, sizeof(particles));
    particle_count = 0;
    
    // Clear existing container
    for (int i = 0; i < 5; i++) {
        if (container_walls[i]) {
            physics_world_remove_body(world, container_walls[i]);
            rigid_body_destroy(container_walls[i]);
            container_walls[i] = NULL;
        }
    }
    
    // Create container walls
    float wall_thickness = 0.1f;
    float container_size = 3.0f;
    
    // Bottom
    container_walls[0] = physics_demo_create_box(
        (Vec3){0, -container_size/2 - wall_thickness/2, 0}, 
        (Vec3){container_size, wall_thickness, container_size}, 0.0f);
    
    // Left wall
    container_walls[1] = physics_demo_create_box(
        (Vec3){-container_size/2 - wall_thickness/2, 0, 0}, 
        (Vec3){wall_thickness, container_size, container_size}, 0.0f);
    
    // Right wall
    container_walls[2] = physics_demo_create_box(
        (Vec3){container_size/2 + wall_thickness/2, 0, 0}, 
        (Vec3){wall_thickness, container_size, container_size}, 0.0f);
    
    // Back wall
    container_walls[3] = physics_demo_create_box(
        (Vec3){0, 0, -container_size/2 - wall_thickness/2}, 
        (Vec3){container_size, container_size, wall_thickness}, 0.0f);
    
    // Front wall (with opening)
    container_walls[4] = physics_demo_create_box(
        (Vec3){0, 0, container_size/2 + wall_thickness/2}, 
        (Vec3){container_size, container_size*0.7f, wall_thickness}, 0.0f);
    
    // Create fluid particles
    for (int x = -10; x <= 10; x++) {
        for (int y = 0; y <= 15; y++) {
            for (int z = -5; z <= 5; z++) {
                if (particle_count >= MAX_PARTICLES) break;
                
                particles[particle_count].position = (Vec3){
                    x * PARTICLE_RADIUS * 2.5f,
                    y * PARTICLE_RADIUS * 2.5f - 1.0f,
                    z * PARTICLE_RADIUS * 2.5f
                };
                particles[particle_count].velocity = (Vec3){0, 0, 0};
                particles[particle_count].force = (Vec3){0, 0, 0};
                particles[particle_count].density = REST_DENSITY;
                particles[particle_count].pressure = 0.0f;
                particles[particle_count].active = true;
                particle_count++;
            }
        }
    }
    
    time_accumulator = 0.0f;
    demo_reset = false;
}

void fluid_simulation_compute_density_pressure(void) {
    // Reset densities
    for (int i = 0; i < particle_count; i++) {
        particles[i].density = 0.0f;
    }
    
    // Compute density using SPH (Smoothed Particle Hydrodynamics)
    for (int i = 0; i < particle_count; i++) {
        if (!particles[i].active) continue;
        
        for (int j = 0; j < particle_count; j++) {
            if (!particles[j].active) continue;
            
            Vec3 diff = vec3_sub(particles[i].position, particles[j].position);
            float dist_sq = vec3_length_sq(diff);
            
            if (dist_sq < SMOOTHING_RADIUS * SMOOTHING_RADIUS) {
                float dist = sqrtf(dist_sq);
                // Poly6 kernel for density
                float influence = 315.0f / (64.0f * M_PI * powf(SMOOTHING_RADIUS, 9));
                influence *= powf(SMOOTHING_RADIUS * SMOOTHING_RADIUS - dist_sq, 3);
                particles[i].density += influence;
            }
        }
        
        // Compute pressure from density
        particles[i].pressure = GAS_CONSTANT * (particles[i].density - REST_DENSITY);
    }
}

void fluid_simulation_compute_forces(void) {
    // Reset forces
    for (int i = 0; i < particle_count; i++) {
        particles[i].force = (Vec3){0, -9.81f, 0}; // Gravity
    }
    
    // Compute pressure and viscosity forces
    for (int i = 0; i < particle_count; i++) {
        if (!particles[i].active) continue;
        
        for (int j = i + 1; j < particle_count; j++) {
            if (!particles[j].active) continue;
            
            Vec3 diff = vec3_sub(particles[i].position, particles[j].position);
            float dist_sq = vec3_length_sq(diff);
            
            if (dist_sq < SMOOTHING_RADIUS * SMOOTHING_RADIUS && dist_sq > 0.001f) {
                float dist = sqrtf(dist_sq);
                Vec3 normal = vec3_scale(diff, 1.0f / dist);
                
                // Pressure force (Spiky kernel gradient)
                float pressure_term = -45.0f / (M_PI * powf(SMOOTHING_RADIUS, 6));
                pressure_term *= powf(SMOOTHING_RADIUS - dist, 2);
                pressure_term *= (particles[i].pressure + particles[j].pressure) / (2.0f * particles[j].density);
                
                Vec3 pressure_force = vec3_scale(normal, pressure_term);
                
                // Viscosity force (Viscosity kernel Laplacian)
                float viscosity_term = 45.0f / (M_PI * powf(SMOOTHING_RADIUS, 6));
                viscosity_term *= (SMOOTHING_RADIUS - dist);
                viscosity_term *= VISCOSITY / particles[j].density;
                
                Vec3 vel_diff = vec3_sub(particles[j].velocity, particles[i].velocity);
                Vec3 viscosity_force = vec3_scale(vel_diff, viscosity_term);
                
                // Apply forces
                particles[i].force = vec3_add(particles[i].force, pressure_force);
                particles[i].force = vec3_add(particles[i].force, viscosity_force);
                
                particles[j].force = vec3_sub(particles[j].force, pressure_force);
                particles[j].force = vec3_add(particles[j].force, viscosity_force);
            }
        }
    }
}

void fluid_simulation_integrate(float dt) {
    for (int i = 0; i < particle_count; i++) {
        if (!particles[i].active) continue;
        
        // Update velocity and position (Euler integration)
        Vec3 acceleration = vec3_scale(particles[i].force, 1.0f / particles[i].density);
        particles[i].velocity = vec3_add(particles[i].velocity, vec3_scale(acceleration, dt));
        particles[i].position = vec3_add(particles[i].position, vec3_scale(particles[i].velocity, dt));
        
        // Simple collision with container
        if (particles[i].position.y < -1.4f) {
            particles[i].position.y = -1.4f;
            particles[i].velocity.y *= -0.5f; // Damping
        }
        if (fabsf(particles[i].position.x) > 1.4f) {
            particles[i].velocity.x *= -0.5f;
            particles[i].position.x = (particles[i].position.x > 0) ? 1.4f : -1.4f;
        }
        if (fabsf(particles[i].position.z) > 1.4f) {
            particles[i].velocity.z *= -0.5f;
            particles[i].position.z = (particles[i].position.z > 0) ? 1.4f : -1.4f;
        }
        
        // Velocity damping
        particles[i].velocity = vec3_scale(particles[i].velocity, 0.99f);
    }
}

void fluid_simulation_update(PhysicsWorld* world, float dt) {
    // Fixed timestep for stability
    const float fixed_dt = 0.005f; // 5ms timestep
    time_accumulator += dt;
    
    while (time_accumulator >= fixed_dt) {
        fluid_simulation_compute_density_pressure();
        fluid_simulation_compute_forces();
        fluid_simulation_integrate(fixed_dt);
        time_accumulator -= fixed_dt;
    }
    
    // Handle input
    if (key_pressed(KEY_SPACE)) {
        demo_reset = true;
    }
    
    // Add particles on mouse click (simplified)
    if (key_pressed(KEY_P)) {
        for (int i = 0; i < 10; i++) {
            if (particle_count < MAX_PARTICLES) {
                particles[particle_count].position = (Vec3){
                    (rand() % 100 - 50) * 0.01f,
                    2.0f,
                    (rand() % 100 - 50) * 0.01f
                };
                particles[particle_count].velocity = (Vec3){
                    (rand() % 200 - 100) * 0.01f,
                    0.0f,
                    (rand() % 200 - 100) * 0.01f
                };
                particles[particle_count].force = (Vec3){0, -9.81f, 0};
                particles[particle_count].density = REST_DENSITY;
                particles[particle_count].pressure = 0.0f;
                particles[particle_count].active = true;
                particle_count++;
            }
        }
    }
    
    // Apply explosion force on 'E' key
    if (key_pressed(KEY_E)) {
        Vec3 explosion_center = {0.0f, 0.0f, 0.0f};
        float explosion_force = 500.0f;
        
        for (int i = 0; i < particle_count; i++) {
            if (!particles[i].active) continue;
            
            Vec3 diff = vec3_sub(particles[i].position, explosion_center);
            float dist = vec3_length(diff);
            
            if (dist < 2.0f && dist > 0.1f) {
                Vec3 force_dir = vec3_normalize(diff);
                float force_magnitude = explosion_force * (1.0f - dist / 2.0f);
                Vec3 explosion_force_vec = vec3_scale(force_dir, force_magnitude);
                particles[i].velocity = vec3_add(particles[i].velocity, explosion_force_vec);
            }
        }
    }
    
    if (demo_reset) {
        fluid_simulation_init(world);
    }
}

void fluid_simulation_render(Renderer* renderer, PhysicsWorld* world) {
    // Render container walls
    for (int i = 0; i < 5; i++) {
        physics_demo_render_body(renderer, container_walls[i]);
    }
    
    // Render fluid particles
    renderer_set_color(renderer, (Vec3){0.2, 0.4, 0.8});
    for (int i = 0; i < particle_count; i++) {
        if (!particles[i].active) continue;
        
        // Color based on velocity
        float speed = vec3_length(particles[i].velocity);
        Vec3 color = {
            0.2f + fminf(speed * 0.1f, 0.8f),
            0.4f,
            0.8f - fminf(speed * 0.1f, 0.6f)
        };
        renderer_set_color(renderer, color);
        
        renderer_draw_sphere(renderer, PARTICLE_RADIUS, particles[i].position);
    }
    
    // Render debug info
    physics_demo_render_debug_info(renderer, world);
    
    // Render demo-specific info
    renderer_set_text_color(renderer, (Vec3){0, 1, 0});
    renderer_draw_text(renderer, "Fluid Simulation Demo", 10, 320, 14);
    renderer_set_text_color(renderer, (Vec3){0.8, 0.8, 0.8});
    renderer_draw_text(renderer, "Space: Reset | P: Add particles | E: Explosion", 10, 340, 12);
    
    char particle_text[64];
    snprintf(particle_text, sizeof(particle_text), "Particles: %d/%d", particle_count, MAX_PARTICLES);
    renderer_draw_text(renderer, particle_text, 10, 360, 12);
}

void fluid_simulation_cleanup(PhysicsWorld* world) {
    // Clear particles
    memset(particles, 0, sizeof(particles));
    particle_count = 0;
    
    // Clear container
    for (int i = 0; i < 5; i++) {
        if (container_walls[i]) {
            physics_world_remove_body(world, container_walls[i]);
            rigid_body_destroy(container_walls[i]);
            container_walls[i] = NULL;
        }
    }
}

// Register the demo
REGISTER_DEMO(Fluid Simulation, fluid_simulation_init, fluid_simulation_update, fluid_simulation_render, fluid_simulation_cleanup);
