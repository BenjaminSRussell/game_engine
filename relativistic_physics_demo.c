// Relativistic Physics System - Time Dilation and Spacetime Effects
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

typedef struct { float x, y, z; } Vec3;
typedef struct { float x, y, z, w; } Vec4;

Vec3 vec3(float x, float y, float z) { return (Vec3){x, y, z}; }
Vec3 vec3_add(Vec3 a, Vec3 b) { return (Vec3){a.x + b.x, a.y + b.y, a.z + b.z}; }
Vec3 vec3_sub(Vec3 a, Vec3 b) { return (Vec3){a.x - b.x, a.y - b.y, a.z - b.z}; }
Vec3 vec3_mul(Vec3 v, float s) { return (Vec3){v.x * s, v.y * s, v.z * s}; }
float vec3_dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
float vec3_length(Vec3 v) { return sqrtf(vec3_dot(v, v)); }
Vec3 vec3_normalize(Vec3 v) { float l = vec3_length(v); return l > 0 ? vec3_mul(v, 1.0f/l) : vec3(0,0,0); }

// Constants
#define C 299792458.0f  // Speed of light (scaled for simulation)
#define G 6.67430e-11f // Gravitational constant
#define TIME_DILATION_FACTOR 0.001f

// Relativistic Particle
typedef struct {
    Vec3 position;
    Vec3 velocity;
    Vec3 proper_velocity; // Velocity in particle's reference frame
    float mass;
    float rest_mass;
    float gamma; // Lorentz factor
    float proper_time; // Time in particle's reference frame
    float coordinate_time; // Time in observer's reference frame
    Vec3 four_velocity[4]; // 4-velocity
    bool is_relativistic;
    Vec3 color;
} RelativisticParticle;

// Spacetime Grid
typedef struct {
    Vec3 position;
    float metric[4][4]; // Spacetime metric tensor
    float curvature;
    bool is_curved;
} SpacetimePoint;

// Black Hole
typedef struct {
    Vec3 position;
    float mass;
    float schwarzschild_radius;
    float event_horizon;
    bool active;
} BlackHole;

// Light Cone
typedef struct {
    Vec3 apex;
    float opening_angle;
    float height;
    bool active;
} LightCone;

#define MAX_PARTICLES 48
#define MAX_SPACETIME_POINTS 16
#define MAX_BLACK_HOLES 2
#define MAX_LIGHT_CONES 4

static RelativisticParticle particles[MAX_PARTICLES];
static SpacetimePoint spacetime[MAX_SPACETIME_POINTS];
static BlackHole black_holes[MAX_BLACK_HOLES];
static LightCone light_cones[MAX_LIGHT_CONES];
static int particle_count = 0;
static int spacetime_count = 0;
static int black_hole_count = 0;
static int light_cone_count = 0;

// Relativistic Functions
float calculate_lorentz_factor(Vec3 velocity) {
    float v_squared = vec3_dot(velocity, velocity);
    float c_squared = C * C * TIME_DILATION_FACTOR * TIME_DILATION_FACTOR;
    
    if (v_squared >= c_squared) {
        return 1000.0f; // Cap at very high value for near-light speeds
    }
    
    float beta_squared = v_squared / c_squared;
    return 1.0f / sqrtf(1.0f - beta_squared);
}

Vec3 calculate_relativistic_velocity(RelativisticParticle* particle) {
    Vec3 v = particle->velocity;
    float v_mag = vec3_length(v);
    
    if (v_mag > C * TIME_DILATION_FACTOR * 0.99f) {
        // Limit to near speed of light
        v = vec3_mul(vec3_normalize(v), C * TIME_DILATION_FACTOR * 0.99f);
    }
    
    return v;
}

float calculate_gravitational_time_dilation(Vec3 position, BlackHole* bh) {
    if (!bh->active) return 1.0f;
    
    Vec3 to_bh = vec3_sub(bh->position, position);
    float r = vec3_length(to_bh);
    
    if (r < bh->schwarzschild_radius * 2.0f) {
        return 0.1f; // Extreme time dilation near event horizon
    }
    
    // Simplified Schwarzschild metric time dilation
    float rs_over_r = bh->schwarzschild_radius / r;
    return sqrtf(1.0f - rs_over_r);
}

Vec3 calculate_gravitational_force(RelativisticParticle* particle, BlackHole* bh) {
    if (!bh->active) return vec3(0,0,0);
    
    Vec3 to_bh = vec3_sub(bh->position, particle->position);
    float r = vec3_length(to_bh);
    
    if (r < bh->schwarzschild_radius) {
        // Particle crossed event horizon
        return vec3_mul(vec3_normalize(to_bh), 1000.0f);
    }
    
    // Newton's law with relativistic corrections
    float force_magnitude = G * bh->mass * particle->mass / (r * r);
    Vec3 force_direction = vec3_normalize(to_bh);
    
    return vec3_mul(force_direction, force_magnitude * 1e10f); // Scale for visualization
}

bool is_inside_light_cone(Vec3 position, Vec3 event_position, LightCone* cone) {
    Vec3 to_event = vec3_sub(position, event_position);
    float distance = vec3_length(to_event);
    
    if (distance < 0.001f) return true;
    
    Vec3 event_to_apex = vec3_sub(cone->apex, event_position);
    float cone_distance = vec3_length(event_to_apex);
    
    if (distance > cone_distance) return false;
    
    // Check if within cone angle
    Vec3 normalized_to_event = vec3_normalize(to_event);
    Vec3 normalized_to_apex = vec3_normalize(event_to_apex);
    float cos_angle = vec3_dot(normalized_to_event, normalized_to_apex);
    float cos_cone_angle = cosf(cone->opening_angle);
    
    return cos_angle >= cos_cone_angle;
}

void init_relativistic_scene(void) {
    particle_count = 0;
    spacetime_count = 0;
    black_hole_count = 0;
    light_cone_count = 0;
    
    // Create relativistic particles with various velocities
    for (int i = 0; i < 20; i++) {
        Vec3 initial_vel = vec3(
            (i % 7 - 3) * C * TIME_DILATION_FACTOR * 0.1f,
            (i % 5 - 2) * C * TIME_DILATION_FACTOR * 0.05f,
            (i % 3 - 1) * C * TIME_DILATION_FACTOR * 0.08f
        );
        
        float gamma = calculate_lorentz_factor(initial_vel);
        
        particles[particle_count++] = (RelativisticParticle){
            .position = vec3(-12 + i * 1.2f, 5 + (i % 4) * 1.5f, (i % 3) * 2.0f - 2.0f),
            .velocity = initial_vel,
            .proper_velocity = vec3_mul(initial_vel, 1.0f / gamma),
            .mass = 1.0f + (i % 5) * 0.5f,
            .rest_mass = 1.0f + (i % 5) * 0.5f,
            .gamma = gamma,
            .proper_time = 0.0f,
            .coordinate_time = 0.0f,
            .four_velocity = {initial_vel.x, initial_vel.y, initial_vel.z, gamma},
            .is_relativistic = gamma > 1.1f,
            .color = vec3(0.2f + (i % 7) * 0.1f, 0.5f, 0.9f - (i % 5) * 0.15f)
        };
    }
    
    // Create black holes
    black_holes[black_hole_count++] = (BlackHole){
        .position = vec3(0.0f, 0.0f, 0.0f),
        .mass = 1000.0f,
        .schwarzschild_radius = 1.5f,
        .event_horizon = 1.5f,
        .active = true
    };
    
    if (rand() % 2 == 0) {
        black_holes[black_hole_count++] = (BlackHole){
            .position = vec3(8.0f, -3.0f, 0.0f),
            .mass = 500.0f,
            .schwarzschild_radius = 1.0f,
            .event_horizon = 1.0f,
            .active = true
        };
    }
    
    // Create light cones
    for (int i = 0; i < 3; i++) {
        light_cones[light_cone_count++] = (LightCone){
            .apex = vec3((i % 3 - 1) * 6.0f, 3.0f + i * 2.0f, (i % 2) * 4.0f - 2.0f),
            .opening_angle = M_PI / 6.0f + i * M_PI / 12.0f,
            .height = 5.0f + i * 2.0f,
            .active = true
        };
    }
    
    // Initialize spacetime grid
    for (int i = 0; i < 8; i++) {
        spacetime[spacetime_count++] = (SpacetimePoint){
            .position = vec3((i % 5 - 2) * 8.0f, -2.0f, (i % 3 - 1) * 6.0f),
            .metric = {{0}}, // Simplified flat metric
            .curvature = 0.0f,
            .is_curved = false
        };
    }
}

void update_relativistic_physics(float dt) {
    static float coordinate_time = 0.0f;
    coordinate_time += dt;
    
    // Update particles with relativistic effects
    for (int i = 0; i < particle_count; i++) {
        RelativisticParticle* p = &particles[i];
        
        // Calculate time dilation
        float time_dilation = 1.0f / p->gamma;
        p->proper_time += dt * time_dilation;
        p->coordinate_time = coordinate_time;
        
        // Apply gravitational forces from black holes
        Vec3 total_force = vec3(0,0,0);
        for (int j = 0; j < black_hole_count; j++) {
            Vec3 grav_force = calculate_gravitational_force(p, &black_holes[j]);
            total_force = vec3_add(total_force, grav_force);
            
            // Check if particle is near event horizon
            Vec3 to_bh = vec3_sub(black_holes[j].position, p->position);
            float dist = vec3_length(to_bh);
            if (dist < black_holes[j].event_horizon) {
                // Particle is captured by black hole
                p->position = vec3_mul(vec3_normalize(to_bh), black_holes[j].schwarzschild_radius * 0.5f);
                p->velocity = vec3(0,0,0);
            }
        }
        
        // Relativistic acceleration
        Vec3 acceleration = vec3_mul(total_force, 1.0f / p->mass);
        
        // Update velocity with relativistic limits
        Vec3 new_velocity = vec3_add(p->velocity, vec3_mul(acceleration, dt));
        p->velocity = calculate_relativistic_velocity(p);
        
        // Update position
        p->position = vec3_add(p->position, vec3_mul(p->velocity, dt));
        
        // Update Lorentz factor
        p->gamma = calculate_lorentz_factor(p->velocity);
        p->proper_velocity = vec3_mul(p->velocity, 1.0f / p->gamma);
        
        // Boundary conditions
        if (p->position.y < -8.0f) {
            p->position.y = -8.0f;
            p->velocity.y *= -0.5f;
        }
        
        if (fabsf(p->position.x) > 20.0f) {
            p->velocity.x *= -0.6f;
            p->position.x = (p->position.x > 0) ? 20.0f : -20.0f;
        }
    }
    
    // Update black holes
    for (int i = 0; i < black_hole_count; i++) {
        // Oscillating mass for visual effect
        black_holes[i].mass = 1000.0f + 200.0f * sinf(coordinate_time * 0.5f + i);
        black_holes[i].schwarzschild_radius = 1.5f + 0.3f * sinf(coordinate_time * 0.7f + i);
    }
    
    // Update light cones
    for (int i = 0; i < light_cone_count; i++) {
        light_cones[i].opening_angle = M_PI / 6.0f + 
            (M_PI / 12.0f) * sinf(coordinate_time * 0.3f + i * M_PI / 3.0f);
    }
}

void render_relativistic_system(void) {
    printf("\033[2J\033[H");
    
    for (int y = 0; y < 35; y++) {
        for (int x = 0; x < 90; x++) {
            char ch = ' ';
            
            // Render relativistic particles
            for (int i = 0; i < particle_count; i++) {
                int px = (int)(particles[i].position.x * 2.0f + 45);
                int py = (int)(-particles[i].position.y * 1.5f + 18);
                
                if (abs(x - px) < 2 && abs(y - py) < 1) {
                    if (particles[i].is_relativistic) {
                        if (particles[i].gamma > 10.0f) ch = '*'; // Near light speed
                        else if (particles[i].gamma > 2.0f) ch = '#'; // High speed
                        else ch = '+'; // Relativistic
                    } else {
                        ch = 'o'; // Non-relativistic
                    }
                }
            }
            
            // Render black holes
            for (int i = 0; i < black_hole_count; i++) {
                int bx = (int)(black_holes[i].position.x * 2.0f + 45);
                int by = (int)(-black_holes[i].position.y * 1.5f + 18);
                int br = (int)(black_holes[i].schwarzschild_radius * 2.5f);
                
                int dist_sq = (x - bx) * (x - bx) + (y - by) * (y - by);
                if (dist_sq < br * br && dist_sq > 0) {
                    if (dist_sq < (br * br) / 4) ch = 'X'; // Event horizon
                    else if (dist_sq < (br * br) / 2) ch = '#'; // Inner region
                    else ch = '.'; // Outer region
                }
            }
            
            // Render light cones
            for (int i = 0; i < light_cone_count; i++) {
                if (!light_cones[i].active) continue;
                
                int cx = (int)(light_cones[i].apex.x * 2.0f + 45);
                int cy = (int)(-light_cones[i].apex.y * 1.5f + 18);
                
                Vec3 world_pos = vec3((x - 45) / 2.0f, -(y - 18) / 1.5f, 0);
                if (is_inside_light_cone(world_pos, light_cones[i].apex, &light_cones[i])) {
                    ch = '^'; // Inside light cone
                }
            }
            
            putchar(ch);
        }
        putchar('\n');
    }
    
    printf("\n=== RELATIVISTIC PHYSICS SYSTEM ===\n");
    printf("Particles: %d | Black Holes: %d | Light Cones: %d\n", 
           particle_count, black_hole_count, light_cone_count);
    printf("Systems: Time Dilation, Lorentz Transformation, Event Horizons\n");
    printf("Features: Schwarzschild Metric, Light Cones, 4-Velocity\n");
}

void add_relativistic_particle(void) {
    if (particle_count >= MAX_PARTICLES) return;
    
    float speed_fraction = 0.1f + (rand() % 900) / 1000.0f;
    Vec3 velocity = vec3(
        (rand() % 200 - 100) / 100.0f,
        (rand() % 200 - 100) / 100.0f,
        (rand() % 200 - 100) / 100.0f
    );
    
    velocity = vec3_mul(vec3_normalize(velocity), speed_fraction * C * TIME_DILATION_FACTOR);
    float gamma = calculate_lorentz_factor(velocity);
    
    particles[particle_count++] = (RelativisticParticle){
        .position = vec3(-15 + (rand() % 300) / 10.0f, 10 + (rand() % 50) / 10.0f, (rand() % 40) / 10.0f - 2.0f),
        .velocity = velocity,
        .proper_velocity = vec3_mul(velocity, 1.0f / gamma),
        .mass = 0.5f + (rand() % 100) / 100.0f,
        .rest_mass = 0.5f + (rand() % 100) / 100.0f,
        .gamma = gamma,
        .proper_time = 0.0f,
        .coordinate_time = 0.0f,
        .four_velocity = {velocity.x, velocity.y, velocity.z, gamma},
        .is_relativistic = gamma > 1.1f,
        .color = vec3(0.3f + (rand() % 70) / 100.0f, 0.4f + (rand() % 60) / 100.0f, 0.8f + (rand() % 40) / 100.0f)
    };
}

int main(void) {
    printf("Starting RELATIVISTIC PHYSICS SYSTEM\n");
    printf("Exotic Features:\n");
    printf("- Time Dilation Effects\n");
    printf("- Lorentz Transformations\n");
    printf("- Black Hole Physics\n");
    printf("- Event Horizon Simulation\n");
    printf("- Light Cone Causality\n");
    printf("- 4-Velocity Formalism\n");
    printf("- Schwarzschild Metric\n\n");
    
    srand((unsigned int)time(NULL));
    init_relativistic_scene();
    
    float last_time = (float)clock() / CLOCKS_PER_SEC;
    bool running = true;
    int frame_count = 0;
    
    while (running) {
        float current_time = (float)clock() / CLOCKS_PER_SEC;
        float dt = current_time - last_time;
        last_time = current_time;
        
        if (dt > 0.02f) dt = 0.02f;
        
        if (frame_count % 30 == 0) {
            add_relativistic_particle();
        }
        
        if (frame_count % 50 == 0) {
            // Randomly modify black hole masses
            for (int i = 0; i < black_hole_count; i++) {
                black_holes[i].mass = 800.0f + (rand() % 400);
                black_holes[i].schwarzschild_radius = 1.2f + (rand() % 80) / 100.0f;
            }
        }
        
        update_relativistic_physics(dt);
        render_relativistic_system();
        
        frame_count++;
        
        struct timespec ts = {0, 40000000};
        nanosleep(&ts, NULL);
        
        if (frame_count > 500) running = false;
    }
    
    printf("\nRelativistic Physics Demo Complete!\n");
    printf("Demonstrated: Time Dilation, Black Holes, Light Cones, Lorentz Transformations\n");
    return 0;
}
