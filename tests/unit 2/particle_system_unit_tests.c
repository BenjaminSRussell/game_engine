// Particle System Unit Tests
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

// Include particle system headers
#include "effects/particles/particle_types.h"
#include "effects/particles/particle_simulation.h"
#include "effects/svg_particles/svg_particle_system.h"
#include "include/math/math.h"
#include "include/core/logger.h"

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("FAIL: %s\n", message); \
            return false; \
        } \
    } while(0)

#define TEST_ASSERT_FLOAT_EQ(a, b, tolerance, message) \
    do { \
        if (fabsf((a) - (b)) > (tolerance)) { \
            printf("FAIL: %s (expected %.6f, got %.6f)\n", message, (b), (a)); \
            return false; \
        } \
    } while(0)

typedef struct {
    const char* test_name;
    bool (*test_func)(void);
    bool passed;
    const char* error_message;
} ParticleTest;

static ParticleTest g_tests[32];
static u32 g_test_count = 0;
static u32 g_tests_passed = 0;

void add_particle_test(const char* name, bool (*test_func)(void)) {
    if (g_test_count < 32) {
        g_tests[g_test_count].test_name = name;
        g_tests[g_test_count].test_func = test_func;
        g_tests[g_test_count].passed = false;
        g_tests[g_test_count].error_message = NULL;
        g_test_count++;
    }
}

// Test 1: Particle Structure Initialization
bool test_particle_structure(void) {
    printf("Testing particle structure initialization...\n");
    
    particle_t particle = {0};
    
    // Test initial values
    TEST_ASSERT_FLOAT_EQ(particle.x, 0.0f, 0.001f, "Particle X initial value");
    TEST_ASSERT_FLOAT_EQ(particle.y, 0.0f, 0.001f, "Particle Y initial value");
    TEST_ASSERT_FLOAT_EQ(particle.z, 0.0f, 0.001f, "Particle Z initial value");
    TEST_ASSERT_FLOAT_EQ(particle.age, 0.0f, 0.001f, "Particle age initial value");
    
    TEST_ASSERT_FLOAT_EQ(particle.vx, 0.0f, 0.001f, "Particle VX initial value");
    TEST_ASSERT_FLOAT_EQ(particle.vy, 0.0f, 0.001f, "Particle VY initial value");
    TEST_ASSERT_FLOAT_EQ(particle.vz, 0.0f, 0.001f, "Particle VZ initial value");
    TEST_ASSERT_FLOAT_EQ(particle.lifetime, 0.0f, 0.001f, "Particle lifetime initial value");
    
    TEST_ASSERT_FLOAT_EQ(particle.color.r, 0.0f, 0.001f, "Particle color R initial value");
    TEST_ASSERT_FLOAT_EQ(particle.color.g, 0.0f, 0.001f, "Particle color G initial value");
    TEST_ASSERT_FLOAT_EQ(particle.color.b, 0.0f, 0.001f, "Particle color B initial value");
    TEST_ASSERT_FLOAT_EQ(particle.color.a, 0.0f, 0.001f, "Particle color A initial value");
    
    TEST_ASSERT_FLOAT_EQ(particle.size, 0.0f, 0.001f, "Particle size initial value");
    TEST_ASSERT_FLOAT_EQ(particle.rotation, 0.0f, 0.001f, "Particle rotation initial value");
    TEST_ASSERT_FLOAT_EQ(particle.mass, 0.0f, 0.001f, "Particle mass initial value");
    TEST_ASSERT_FLOAT_EQ(particle.flags, 0.0f, 0.001f, "Particle flags initial value");
    
    // Test setting values
    particle.x = 1.5f;
    particle.y = 2.5f;
    particle.z = 3.5f;
    particle.age = 0.5f;
    
    particle.vx = -1.0f;
    particle.vy = 2.0f;
    particle.vz = -3.0f;
    particle.lifetime = 5.0f;
    
    particle.color = (color_t){1.0f, 0.5f, 0.25f, 0.8f};
    particle.size = 2.0f;
    particle.rotation = 45.0f;
    particle.mass = 1.5f;
    particle.flags = 1.0f;
    
    // Verify set values
    TEST_ASSERT_FLOAT_EQ(particle.x, 1.5f, 0.001f, "Particle X set value");
    TEST_ASSERT_FLOAT_EQ(particle.y, 2.5f, 0.001f, "Particle Y set value");
    TEST_ASSERT_FLOAT_EQ(particle.z, 3.5f, 0.001f, "Particle Z set value");
    TEST_ASSERT_FLOAT_EQ(particle.age, 0.5f, 0.001f, "Particle age set value");
    
    TEST_ASSERT_FLOAT_EQ(particle.vx, -1.0f, 0.001f, "Particle VX set value");
    TEST_ASSERT_FLOAT_EQ(particle.vy, 2.0f, 0.001f, "Particle VY set value");
    TEST_ASSERT_FLOAT_EQ(particle.vz, -3.0f, 0.001f, "Particle VZ set value");
    TEST_ASSERT_FLOAT_EQ(particle.lifetime, 5.0f, 0.001f, "Particle lifetime set value");
    
    TEST_ASSERT_FLOAT_EQ(particle.color.r, 1.0f, 0.001f, "Particle color R set value");
    TEST_ASSERT_FLOAT_EQ(particle.color.g, 0.5f, 0.001f, "Particle color G set value");
    TEST_ASSERT_FLOAT_EQ(particle.color.b, 0.25f, 0.001f, "Particle color B set value");
    TEST_ASSERT_FLOAT_EQ(particle.color.a, 0.8f, 0.001f, "Particle color A set value");
    
    TEST_ASSERT_FLOAT_EQ(particle.size, 2.0f, 0.001f, "Particle size set value");
    TEST_ASSERT_FLOAT_EQ(particle.rotation, 45.0f, 0.001f, "Particle rotation set value");
    TEST_ASSERT_FLOAT_EQ(particle.mass, 1.5f, 0.001f, "Particle mass set value");
    TEST_ASSERT_FLOAT_EQ(particle.flags, 1.0f, 0.001f, "Particle flags set value");
    
    return true;
}

// Test 2: Particle Physics Simulation
bool test_particle_physics(void) {
    printf("Testing particle physics simulation...\n");
    
    particle_t particle = {
        .x = 0.0f, .y = 10.0f, .z = 0.0f, .age = 0.0f,
        .vx = 1.0f, .vy = 0.0f, .vz = 0.0f, .lifetime = 5.0f,
        .color = {1.0f, 1.0f, 1.0f, 1.0f},
        .size = 1.0f, .rotation = 0.0f, .mass = 1.0f, .flags = 0.0f,
        .ax = 0.0f, .ay = -9.81f, .az = 0.0f, .rotation_speed = 0.0f
    };
    
    // Simulation parameters
    Vec3 gravity = {0.0f, -9.81f, 0.0f};
    float dt = 0.016f; // 60 FPS
    float drag_coefficient = 0.1f;
    
    // Simulate for a few steps
    for (int step = 0; step < 10; step++) {
        // Apply acceleration (gravity + drag)
        Vec3 drag_force = vec3_mul((Vec3){particle.vx, particle.vy, particle.vz}, -drag_coefficient);
        Vec3 acceleration = vec3_add(gravity, vec3_mul(drag_force, 1.0f / particle.mass));
        
        // Update velocity
        particle.vx += acceleration.x * dt;
        particle.vy += acceleration.y * dt;
        particle.vz += acceleration.z * dt;
        
        // Update position
        particle.x += particle.vx * dt;
        particle.y += particle.vy * dt;
        particle.z += particle.vz * dt;
        
        // Update age
        particle.age += dt;
        
        // Update rotation
        particle.rotation += particle.rotation_speed * dt;
    }
    
    // Verify physics results
    TEST_ASSERT(particle.y < 10.0f, "Particle fell due to gravity");
    TEST_ASSERT(particle.vy < 0.0f, "Particle has downward velocity");
    TEST_ASSERT(particle.x > 0.0f, "Particle moved horizontally");
    TEST_ASSERT(particle.age > 0.0f, "Particle aged during simulation");
    
    // Check if particle is still alive
    bool is_alive = particle.age < particle.lifetime;
    TEST_ASSERT(is_alive, "Particle is still alive after simulation");
    
    return true;
}

// Test 3: Particle Emission
bool test_particle_emission(void) {
    printf("Testing particle emission...\n");
    
    // Simulate an emitter
    typedef struct {
        Vec3 position;
        Vec3 direction;
        Vec3 spread;
        float emission_rate;
        float particle_lifetime;
        float initial_speed;
        u32 max_particles;
        u32 active_count;
        particle_t particles[100];
    } ParticleEmitter;
    
    ParticleEmitter emitter = {
        .position = {0.0f, 0.0f, 0.0f},
        .direction = {0.0f, 1.0f, 0.0f}, // Emit upward
        .spread = {0.5f, 0.5f, 0.5f},
        .emission_rate = 60.0f, // 60 particles per second
        .particle_lifetime = 3.0f,
        .initial_speed = 5.0f,
        .max_particles = 100,
        .active_count = 0
    };
    
    // Simulate emission for 1 second at 60 FPS
    float dt = 1.0f / 60.0f;
    float emission_accumulator = 0.0f;
    
    for (int frame = 0; frame < 60; frame++) {
        // Calculate how many particles to emit this frame
        emission_accumulator += emitter.emission_rate * dt;
        
        u32 particles_to_emit = (u32)emission_accumulator;
        if (particles_to_emit > 0 && emitter.active_count < emitter.max_particles) {
            // Emit particles
            for (u32 i = 0; i < particles_to_emit && emitter.active_count < emitter.max_particles; i++) {
                particle_t* particle = &emitter.particles[emitter.active_count];
                
                // Random position within spread
                float spread_x = (float)(rand() % 1000) / 1000.0f * 2.0f - 1.0f;
                float spread_y = (float)(rand() % 1000) / 1000.0f * 2.0f - 1.0f;
                float spread_z = (float)(rand() % 1000) / 1000.0f * 2.0f - 1.0f;
                
                particle->x = emitter.position.x + spread_x * emitter.spread.x;
                particle->y = emitter.position.y + spread_y * emitter.spread.y;
                particle->z = emitter.position.z + spread_z * emitter.spread.z;
                particle->age = 0.0f;
                particle->lifetime = emitter.particle_lifetime;
                
                // Random velocity in emission direction
                Vec3 base_velocity = vec3_mul(emitter.direction, emitter.initial_speed);
                particle->vx = base_velocity.x + spread_x * 2.0f;
                particle->vy = base_velocity.y + spread_y * 2.0f;
                particle->vz = base_velocity.z + spread_z * 2.0f;
                
                // Random color
                particle->color = (color_t){
                    0.5f + (float)(rand() % 1000) / 2000.0f,
                    0.5f + (float)(rand() % 1000) / 2000.0f,
                    0.5f + (float)(rand() % 1000) / 2000.0f,
                    1.0f
                };
                
                // Random size
                particle->size = 0.5f + (float)(rand() % 1000) / 1000.0f;
                particle->mass = particle->size * particle->size * particle->size; // Mass proportional to volume
                particle->rotation = (float)(rand() % 360);
                particle->rotation_speed = (float)(rand() % 360) - 180.0f;
                
                emitter.active_count++;
            }
            
            emission_accumulator -= particles_to_emit;
        }
        
        // Update existing particles
        for (u32 i = 0; i < emitter.active_count; i++) {
            particle_t* particle = &emitter.particles[i];
            
            // Apply gravity
            particle->vy -= 9.81f * dt;
            
            // Update position
            particle->x += particle->vx * dt;
            particle->y += particle->vy * dt;
            particle->z += particle->vz * dt;
            
            // Update age
            particle->age += dt;
            
            // Update rotation
            particle->rotation += particle->rotation_speed * dt;
            
            // Remove dead particles
            if (particle->age >= particle->lifetime) {
                // Move last particle to this position
                if (i < emitter.active_count - 1) {
                    emitter.particles[i] = emitter.particles[emitter.active_count - 1];
                }
                emitter.active_count--;
                i--; // Re-check this position
            }
        }
    }
    
    // Verify emission results
    TEST_ASSERT(emitter.active_count > 0, "Particles were emitted");
    TEST_ASSERT(emitter.active_count <= emitter.max_particles, "Particle count doesn't exceed maximum");
    
    // Check that particles have properties
    if (emitter.active_count > 0) {
        particle_t* first_particle = &emitter.particles[0];
        TEST_ASSERT(first_particle->age > 0.0f, "Emitted particle has aged");
        TEST_ASSERT(first_particle->age < first_particle->lifetime, "Emitted particle is still alive");
        TEST_ASSERT(first_particle->size > 0.0f, "Emitted particle has size");
        TEST_ASSERT(first_particle->color.a > 0.0f, "Emitted particle has alpha");
    }
    
    return true;
}

// Test 4: SVG Particle System
bool test_svg_particle_system(void) {
    printf("Testing SVG particle system...\n");
    
    // Create SVG particle system
    SVGParticleSystem* system = svg_particle_system_create(16);
    TEST_ASSERT(system != NULL, "SVG particle system created successfully");
    
    if (!system) return false;
    
    // Create emitter configuration
    SVGParticleEmitterConfig config = {
        .position = {0.0f, 0.0f, 0.0f},
        .emission_rate = 30.0f,
        .particle_lifetime = 2.0f,
        .initial_velocity = {0.0f, 5.0f, 0.0f},
        .velocity_variation = {2.0f, 2.0f, 2.0f},
        .start_color = {1.0f, 0.8f, 0.2f, 1.0f},
        .end_color = {1.0f, 0.2f, 0.1f, 0.0f},
        .start_size = 0.5f,
        .end_size = 0.1f,
        .particle_count = 50
    };
    
    // Create emitter
    SVGParticleEmitter* emitter = svg_particle_emitter_create(config);
    TEST_ASSERT(emitter != NULL, "SVG particle emitter created successfully");
    
    if (emitter) {
        // Add emitter to system
        bool added = svg_particle_system_add_emitter(system, emitter);
        TEST_ASSERT(added, "Emitter added to system successfully");
        
        // Set emitter position
        svg_particle_emitter_set_position(emitter, 1.0f, 2.0f, 3.0f);
        
        // Start emission
        svg_particle_emitter_start(emitter);
        
        // Update system for a few frames
        for (int frame = 0; frame < 10; frame++) {
            svg_particle_system_update(system, 0.016f); // 60 FPS
        }
        
        // Check if particles were created
        u32 active_count = svg_particle_emitter_get_active_count(emitter);
        TEST_ASSERT(active_count > 0, "SVG particles were emitted");
        
        // Get particles for rendering
        u32 particle_count;
        const SVGParticle* particles = svg_particle_emitter_get_particles(emitter, &particle_count);
        TEST_ASSERT(particles != NULL, "SVG particles retrieved successfully");
        TEST_ASSERT(particle_count > 0, "SVG particle count is valid");
        
        // Stop emission
        svg_particle_emitter_stop(emitter);
        
        // Clean up emitter
        svg_particle_emitter_destroy(emitter);
    }
    
    // Clean up system
    svg_particle_system_destroy(system);
    
    return true;
}

// Test 5: Particle Performance
bool test_particle_performance(void) {
    printf("Testing particle performance...\n");
    
    const int NUM_PARTICLES = 10000;
    const int NUM_FRAMES = 60; // 1 second at 60 FPS
    
    // Create particle array
    particle_t* particles = malloc(NUM_PARTICLES * sizeof(particle_t));
    if (!particles) return false;
    
    // Initialize particles
    for (int i = 0; i < NUM_PARTICLES; i++) {
        particles[i] = (particle_t){
            .x = (float)(rand() % 100 - 50),
            .y = (float)(rand() % 20),
            .z = (float)(rand() % 100 - 50),
            .age = 0.0f,
            .vx = (float)(rand() % 20 - 10) / 10.0f,
            .vy = (float)(rand() % 20 - 10) / 10.0f,
            .vz = (float)(rand() % 20 - 10) / 10.0f,
            .lifetime = 5.0f + (float)(rand() % 100) / 20.0f,
            .color = {
                (float)(rand() % 1000) / 1000.0f,
                (float)(rand() % 1000) / 1000.0f,
                (float)(rand() % 1000) / 1000.0f,
                1.0f
            },
            .size = 0.1f + (float)(rand() % 100) / 100.0f,
            .rotation = (float)(rand() % 360),
            .mass = 1.0f,
            .flags = 0.0f,
            .ax = 0.0f,
            .ay = -9.81f,
            .az = 0.0f,
            .rotation_speed = (float)(rand() % 360) - 180.0f
        };
    }
    
    // Performance test
    clock_t start = clock();
    
    float dt = 1.0f / 60.0f; // 60 FPS
    Vec3 gravity = {0.0f, -9.81f, 0.0f};
    
    for (int frame = 0; frame < NUM_FRAMES; frame++) {
        for (int i = 0; i < NUM_PARTICLES; i++) {
            particle_t* p = &particles[i];
            
            // Simple physics update
            p->vx += gravity.x * dt;
            p->vy += gravity.y * dt;
            p->vz += gravity.z * dt;
            
            p->x += p->vx * dt;
            p->y += p->vy * dt;
            p->z += p->vz * dt;
            
            p->age += dt;
            p->rotation += p->rotation_speed * dt;
            
            // Fade out based on age
            if (p->lifetime > 0.0f) {
                float life_ratio = p->age / p->lifetime;
                p->color.a = 1.0f - life_ratio;
            }
        }
    }
    
    clock_t end = clock();
    double elapsed = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    printf("Performance: %d particles, %d frames in %.3f seconds\n", NUM_PARTICLES, NUM_FRAMES, elapsed);
    printf("Average: %.1f particles per second\n", (NUM_PARTICLES * NUM_FRAMES) / elapsed);
    
    // Performance should be reasonable (at least 100,000 particles/second)
    double particles_per_second = (NUM_PARTICLES * NUM_FRAMES) / elapsed;
    TEST_ASSERT(particles_per_second > 100000.0, "Particle performance meets minimum requirements");
    
    free(particles);
    return true;
}

void run_particle_system_unit_tests(void) {
    printf("=== Particle System Unit Tests ===\n\n");
    
    // Add all tests
    add_particle_test("Particle Structure Initialization", test_particle_structure);
    add_particle_test("Particle Physics Simulation", test_particle_physics);
    add_particle_test("Particle Emission", test_particle_emission);
    add_particle_test("SVG Particle System", test_svg_particle_system);
    add_particle_test("Particle Performance", test_particle_performance);
    
    // Run all tests
    for (u32 i = 0; i < g_test_count; i++) {
        printf("\n--- Test %u: %s ---\n", i + 1, g_tests[i].test_name);
        
        bool passed = g_tests[i].test_func();
        g_tests[i].passed = passed;
        
        if (passed) {
            printf("PASS\n");
            g_tests_passed++;
        } else {
            printf("FAIL\n");
        }
    }
    
    // Print summary
    printf("\n=== Test Summary ===\n");
    printf("Tests passed: %u/%u (%.1f%%)\n", 
           g_tests_passed, g_test_count, 
           (float)g_tests_passed / g_test_count * 100.0f);
    
    if (g_tests_passed == g_test_count) {
        printf("All particle system unit tests PASSED!\n");
    } else {
        printf("Some particle system unit tests FAILED.\n");
    }
}

int main(void) {
    printf("Particle System Unit Test Suite\n");
    printf("===================================\n\n");
    
    run_particle_system_unit_tests();
    
    return (g_tests_passed == g_test_count) ? 0 : 1;
}
