/**
 * Performance Benchmark: AoS vs SoA Physics Integration
 * 
 * Compares traditional Array of Structures against Structure of Arrays
 * to demonstrate cache efficiency and SIMD benefits.
 */

#include "../src/engine/physics/integration/physics_integration.h"
#include "../src/engine/core/containers/soa_rigidbody.h"
#include "../src/engine/core/simd/simd_math.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

// ========================================
// Legacy AoS RigidBody (for comparison)
// ========================================

typedef struct RigidBodyAoS {
    v4f position;
    v4f velocity;
    v4f force;
    float inv_mass;
    uint8_t flags;
    // ... other fields interleaved ...
} RigidBodyAoS;

// Legacy integration (cache-inefficient)
void integrate_aos(RigidBodyAoS *bodies, uint32_t count, float dt) {
    for (uint32_t i = 0; i < count; i++) {
        if (bodies[i].inv_mass == 0.0f) continue;
        
        v4f acceleration;
        v4f_scale(&acceleration, &bodies[i].force, bodies[i].inv_mass);
        
        v4f dv;
        v4f_scale(&dv, &acceleration, dt);
        v4f_add(&bodies[i].velocity, &bodies[i].velocity, &dv);
        
        v4f dp;
        v4f_scale(&dp, &bodies[i].velocity, dt);
        v4f_add(&bodies[i].position, &bodies[i].position, &dp);
    }
}

// ========================================
// Benchmark Harness
// ========================================

double get_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

void benchmark_aos_vs_soa(uint32_t body_count, uint32_t iterations) {
    printf("\n=== AoS vs SoA Performance Benchmark ===\n");
    printf("Bodies: %u\n", body_count);
    printf("Iterations: %u\n", iterations);
    printf("Platform: ");
#if defined(SIMD_AVX2)
    printf("x86-64 AVX2\n");
#elif defined(SIMD_SSE2)
    printf("x86-64 SSE2\n");
#elif defined(SIMD_NEON)
    printf("ARM NEON\n");
#else
    printf("Scalar\n");
#endif
    
    float dt = 1.0f / 60.0f;
    
    // Setup AoS
    RigidBodyAoS *aos_bodies = (RigidBodyAoS*)malloc(sizeof(RigidBodyAoS) * body_count);
    for (uint32_t i = 0; i < body_count; i++) {
        aos_bodies[i].position = V4F_VECTOR(0.0f, 0.0f, 0.0f);
        aos_bodies[i].velocity = V4F_VECTOR(0.0f, 0.0f, 0.0f);
        aos_bodies[i].force = V4F_VECTOR(10.0f, 0.0f, 0.0f);
        aos_bodies[i].inv_mass = 1.0f;
        aos_bodies[i].flags = RIGIDBODY_FLAG_ACTIVE;
    }
    
    // Setup SoA
    RigidBodySoA *soa = soa_rigidbody_create(body_count);
    for (uint32_t i = 0; i < body_count; i++) {
        uint32_t idx = soa_rigidbody_add(soa);
        soa->forces[idx] = V4F_VECTOR(10.0f, 0.0f, 0.0f);
        soa->inv_masses[idx] = 1.0f;
        soa->flags[idx] = RIGIDBODY_FLAG_ACTIVE;
    }
    
    // Warm-up
    integrate_aos(aos_bodies, body_count, dt);
    physics_integrate_velocities(soa, dt);
    
    // Benchmark AoS
    printf("\n[AoS] Running...\n");
    double aos_start = get_time_ms();
    for (uint32_t iter = 0; iter < iterations; iter++) {
        integrate_aos(aos_bodies, body_count, dt);
    }
    double aos_end = get_time_ms();
    double aos_time = aos_end - aos_start;
    
    // Benchmark SoA
    printf("[SoA] Running...\n");
    double soa_start = get_time_ms();
    for (uint32_t iter = 0; iter < iterations; iter++) {
        physics_integrate_velocities(soa, dt);
    }
    double soa_end = get_time_ms();
    double soa_time = soa_end - soa_start;
    
    // Results
    printf("\n--- Results ---\n");
    printf("AoS Time: %.2f ms\n", aos_time);
    printf("SoA Time: %.2f ms\n", soa_time);
    printf("Speedup:  %.2fx\n", aos_time / soa_time);
    
    double aos_bodies_per_sec = (body_count * iterations) / (aos_time / 1000.0);
    double soa_bodies_per_sec = (body_count * iterations) / (soa_time / 1000.0);
    
    printf("\nThroughput:\n");
    printf("  AoS: %.0f bodies/sec\n", aos_bodies_per_sec);
    printf("  SoA: %.0f bodies/sec\n", soa_bodies_per_sec);
    
    // Cleanup
    free(aos_bodies);
    soa_rigidbody_destroy(soa);
}

// ========================================
// Functional Tests
// ========================================

void test_gravity_application() {
    printf("\nTesting gravity application... ");
    
    RigidBodySoA *soa = soa_rigidbody_create(10);
    
    // Add bodies
    for (int i = 0; i < 5; i++) {
        uint32_t idx = soa_rigidbody_add(soa);
        soa_set_mass(soa, idx, 1.0f);  // 1 kg
    }
    
    // Apply gravity
    v4f gravity = V4F_VECTOR(0.0f, -9.81f, 0.0f);
    physics_apply_gravity(soa, &gravity);
    
    // Verify: F = m * g = 1.0 * (-9.81) = (0, -9.81, 0)
    for (uint32_t i = 0; i < soa->count; i++) {
        assert(fabsf(soa->forces[i].x - 0.0f) < 1e-4f);
        assert(fabsf(soa->forces[i].y - (-9.81f)) < 1e-4f);
        assert(fabsf(soa->forces[i].z - 0.0f) < 1e-4f);
    }
    
    soa_rigidbody_destroy(soa);
    printf("PASSED\n");
}

void test_integration_correctness() {
    printf("Testing integration correctness... ");
    
    RigidBodySoA *soa = soa_rigidbody_create(10);
    uint32_t idx = soa_rigidbody_add(soa);
    
    // Setup: position = (0,0,0), velocity = (0,0,0), force = (10,0,0), mass = 1kg
    soa_set_position(soa, idx, &V4F_ZERO);
    soa_set_velocity(soa, idx, &V4F_ZERO);
    soa->forces[idx] = V4F_VECTOR(10.0f, 0.0f, 0.0f);
    soa_set_mass(soa, idx, 1.0f);
    
    float dt = 1.0f / 60.0f;
    
    // Integrate
    physics_integrate_velocities(soa, dt);
    
    // Expected: a = F/m = 10/1 = 10 m/s²
    //           v' = 0 + 10 * (1/60) = 0.1667 m/s
    //           p' = 0 + 0.1667 * (1/60) = 0.00278 m
    float expected_vel = 10.0f * dt;
    float expected_pos = expected_vel * dt;
    
    assert(fabsf(soa->velocities[idx].x - expected_vel) < 1e-4f);
    assert(fabsf(soa->positions[idx].x - expected_pos) < 1e-5f);
    
    soa_rigidbody_destroy(soa);
    printf("PASSED\n");
}

void test_damping() {
    printf("Testing damping... ");
    
    RigidBodySoA *soa = soa_rigidbody_create(10);
    uint32_t idx = soa_rigidbody_add(soa);
    
    soa_set_velocity(soa, idx, &((v4f){10.0f, 0.0f, 0.0f, 0.0f}));
    soa_set_mass(soa, idx, 1.0f);
    soa->linear_damping[idx] = 0.1f;  // 10% per second
    
    float dt = 1.0f;
    physics_apply_damping(soa, dt);
    
    // Expected: v' = 10 * (1 - 0.1 * 1.0) = 10 * 0.9 = 9.0
    assert(fabsf(soa->velocities[idx].x - 9.0f) < 1e-4f);
    
    soa_rigidbody_destroy(soa);
    printf("PASSED\n");
}

// ========================================
// Main
// ========================================

int main() {
    printf("=== Physics Integration Tests & Benchmarks ===\n");
    
    // Functional tests
    test_gravity_application();
    test_integration_correctness();
    test_damping();
    
    // Performance benchmarks
    benchmark_aos_vs_soa(1000, 10000);   // 1K bodies, 10K iterations
    benchmark_aos_vs_soa(5000, 1000);    // 5K bodies, 1K iterations
    
    printf("\n=== ALL TESTS PASSED ===\n");
    return 0;
}
