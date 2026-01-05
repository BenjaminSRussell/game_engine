/**
 * COMPREHENSIVE PHYSICS TESTS - PART 4: Specialized Systems
 * Tests for: SPH, Aerodynamics, Ballistics, Climbing, Fracture, Broadphase
 */

#include "physics_test.h"
#include <physics/advanced/final_systems_batch.c>

// ============================================================================
// SPH FLUID TESTS
// ============================================================================

void test_sph_kernel_poly6() {
    float h = SPH_H;
    
    // At r=0: maximum
    float w0 = sph_kernel_poly6(0.0f);
    ASSERT_TRUE(w0 > 0.0f);
    
    // At r=h: zero
    float wh = sph_kernel_poly6(h);
    ASSERT_FLOAT_EQ(wh, 0.0f, 0.001f);
    
    // At r > h: zero
    float w_far = sph_kernel_poly6(2.0f * h);
    ASSERT_FLOAT_EQ(w_far, 0.0f, 0.001f);
}

void test_sph_density_computation() {
    SPHParticle particles[3];
    
    // Setup particles in a line
    particles[0].position[0] = 0.0f; particles[0].position[1] = 0.0f; particles[0].position[2] = 0.0f;
    particles[1].position[0] = 0.5f; particles[1].position[1] = 0.0f; particles[1].position[2] = 0.0f;
    particles[2].position[0] = 1.0f; particles[2].position[1] = 0.0f; particles[2].position[2] = 0.0f;
    
    sph_compute_density_pressure(particles, 3, 1000.0f, 1000.0f);
    
    // Density should be positive
    ASSERT_TRUE(particles[0].density > 0.0f);
    ASSERT_TRUE(particles[1].density > 0.0f);
    ASSERT_TRUE(particles[2].density > 0.0f);
    
    // Middle particle should have highest density (neighbors on both sides)
    ASSERT_TRUE(particles[1].density >= particles[0].density);
}

void test_sph_pressure_forces() {
    SPHParticle particles[2];
    
    // Two particles close together
    particles[0].position[0] = 0.0f; particles[0].position[1] = 0.0f; particles[0].position[2] = 0.0f;
    particles[0].velocity[0] = 0.0f; particles[0].velocity[1] = 0.0f; particles[0].velocity[2] = 0.0f;
    particles[0].density = 1200.0f; particles[0].pressure = 100.0f;
    
    particles[1].position[0] = 0.3f; particles[1].position[1] = 0.0f; particles[1].position[2] = 0.0f;
    particles[1].velocity[0] = 0.0f; particles[1].velocity[1] = 0.0f; particles[1].velocity[2] = 0.0f;
    particles[1].density = 1200.0f; particles[1].pressure = 100.0f;
    
    // Apply forces
    sph_compute_forces(particles, 2, 0.01f, 0.016f);
    
    // Particles should repel (move apart)
    ASSERT_TRUE(particles[0].velocity[0] < 0.0f || particles[1].velocity[0] > 0.0f);
}

// ============================================================================
// AERODYNAMICS TESTS
// ============================================================================

void test_aerodynamic_drag() {
    AerodynamicsConfig config = {
        .drag_coefficient = 0.5f,
        .lift_coefficient = 0.3f,
        .wing_area = 2.0f,
        .air_density = 1.225f
    };
    
    float velocity[3] = {10.0f, 0.0f, 0.0f}; // 10 m/s forward
    float orientation[3] = {0.0f, 0.1f, 0.0f};
    float force[3], torque[3];
    
    calculate_aerodynamic_forces(&config, velocity, orientation, force, torque);
    
    // Drag should oppose motion (negative X)
    ASSERT_TRUE(force[0] < 0.0f);
    
    // Lift should be upward (positive Y)
    ASSERT_TRUE(force[1] > 0.0f);
}

void test_wind_field_sampling() {
    float wind_vel[3];
    float speed = wind_field_sample(10.0f, 5.0f, 3.0f, 0.0f, wind_vel);
    
    ASSERT_TRUE(speed > 0.0f);
    
    // Wind velocity should be non-zero
    ASSERT_TRUE(wind_vel[0] != 0.0f || wind_vel[1] != 0.0f || wind_vel[2] != 0.0f);
}

// ============================================================================
// BALLISTICS TESTS
// ============================================================================

void test_projectile_gravity() {
    Projectile p = {
        .position = {0, 100, 0},
        .velocity = {0, 0, 0},
        .mass = 1.0f,
        .drag_coeff = 0.0f,
        .cross_section_area = 0.01f
    };
    
    // Update for 1 second with no drag
    projectile_update(&p, 1.0f);
    
    // Should fall: v = gt
    ASSERT_FLOAT_EQ(p.velocity[1], -9.81f, 0.1f);
    
    // Position: y = y0 - 0.5*g*t²
    ASSERT_FLOAT_EQ(p.position[1], 100.0f - 0.5f * 9.81f, 0.5f);
}

void test_projectile_drag() {
    Projectile p = {
        .position = {0, 0, 0},
        .velocity = {100, 0, 0}, // Fast horizontal
        .mass = 1.0f,
        .drag_coeff = 0.47f, // Sphere
        .cross_section_area = 0.01f
    };
    
    float initial_speed = 100.0f;
    
    // Update for 1 second
    projectile_update(&p, 1.0f);
    
    // Speed should decrease due to drag
    float final_speed = sqrtf(p.velocity[0]*p.velocity[0] + p.velocity[1]*p.velocity[1] + p.velocity[2]*p.velocity[2]);
    ASSERT_TRUE(final_speed < initial_speed);
}

void test_trajectory_prediction() {
    Projectile start = {
        .position = {0, 0, 0},
        .velocity = {10, 20, 0},
        .mass = 1.0f,
        .drag_coeff = 0.0f,
        .cross_section_area = 0.01f
    };
    
    float positions[100 * 3];
    trajectory_predict(&start, 3.0f, 0.1f, positions, 100);
    
    // First point should be near start
    ASSERT_FLOAT_EQ(positions[0], 0.0f, 1.0f);
    ASSERT_FLOAT_EQ(positions[1], 0.0f, 1.0f);
    
    // Later points should show parabolic trajectory
    ASSERT_TRUE(positions[30] > 0.0f); // Moved forward
    ASSERT_TRUE(positions[31] < positions[1]); // Fell down
}

// ============================================================================
// CHARACTER CLIMBING TESTS
// ============================================================================

void test_climbing_stamina_drain() {
    ClimbingState state = {
        .is_climbing = true,
        .climb_speed = 2.0f,
        .wall_normal = {0, 0, -1},
        .grip_strength = 1.0f,
        .stamina = 100.0f
    };
    
    float input[3] = {0, 1, 0}; // Climb up
    
    // Update for 10 seconds
    for (int i = 0; i < 600; i++) {
        climbing_update(&state, input, 0.016f);
    }
    
    // Stamina should have decreased
    ASSERT_TRUE(state.stamina < 100.0f);
}

void test_climbing_wall_detection() {
    float pos[3] = {0, 0, 0};
    float dir[3] = {1, 0, 0}; // Looking right
    float normal[3];
    
    bool climbable = check_climbable_surface(pos, dir, normal);
    
    // Should detect wall (simplified)
    ASSERT_TRUE(climbable);
    
    // Normal should point back
    ASSERT_FLOAT_EQ(normal[0], -1.0f, 0.1f);
}

// ============================================================================
// VORONOI FRACTURE TESTS
// ============================================================================

void test_voronoi_fragment_generation() {
    float center[3] = {0, 0, 0};
    float impact[3] = {1, 0, 0};
    VoronoiFragment fragments[10];
    
    voronoi_shatter(center, impact, 10, fragments);
    
    // Should generate fragments
    for (int i = 0; i < 10; i++) {
        // Each fragment should have a centroid
        ASSERT_TRUE(fragments[i].centroid[0] != 0.0f ||
                   fragments[i].centroid[1] != 0.0f ||
                   fragments[i].centroid[2] != 0.0f ||
                   i == 0); // First might be at origin
    }
}

// ============================================================================
// BROADPHASE TESTS
// ============================================================================

void test_gpu_broadphase_creation() {
    GPUBroadphase *bp = gpu_broadphase_create(32);
    ASSERT_NOT_NULL(bp);
    ASSERT_INT_EQ(bp->grid_size, 32);
    
    free(bp->cell_start);
    free(bp->cell_count);
    free(bp);
}

void test_spatial_hash_consistency() {
    uint32_t grid_size = 16;
    
    // Same position should give same hash
    uint32_t hash1 = spatial_hash(5.0f, 10.0f, 3.0f, grid_size);
    uint32_t hash2 = spatial_hash(5.0f, 10.0f, 3.0f, grid_size);
    ASSERT_INT_EQ(hash1, hash2);
    
    // Different position should (probably) give different hash
    uint32_t hash3 = spatial_hash(15.0f, 10.0f, 3.0f, grid_size);
    // Note: collisions possible, so just check valid range
    ASSERT_TRUE(hash3 < grid_size * grid_size * grid_size);
}

void test_sap_sort_and_sweep() {
    SAPEndpoint endpoints[4] = {
        {0, 0.0f, true},  // Object 0 min
        {0, 5.0f, false}, // Object 0 max
        {1, 3.0f, true},  // Object 1 min
        {1, 8.0f, false}  // Object 1 max
    };
    
    uint32_t pairs[10 * 2];
    uint32_t pair_count;
    
    sap_sort_and_sweep(endpoints, 4, pairs, &pair_count);
    
    // Objects overlap, should generate 1 pair
    ASSERT_INT_EQ(pair_count, 1);
    ASSERT_TRUE((pairs[0] == 0 && pairs[1] == 1) ||
               (pairs[0] == 1 && pairs[1] == 0));
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main() {
    TEST_SUITE_BEGIN("Physics Specialized Systems");
    
    printf("\n--- SPH Fluid Tests ---\n");
    RUN_TEST(test_sph_kernel_poly6);
    RUN_TEST(test_sph_density_computation);
    RUN_TEST(test_sph_pressure_forces);
    
    printf("\n--- Aerodynamics Tests ---\n");
    RUN_TEST(test_aerodynamic_drag);
    RUN_TEST(test_wind_field_sampling);
    
    printf("\n--- Ballistics Tests ---\n");
    RUN_TEST(test_projectile_gravity);
    RUN_TEST(test_projectile_drag);
    RUN_TEST(test_trajectory_prediction);
    
    printf("\n--- Climbing Tests ---\n");
    RUN_TEST(test_climbing_stamina_drain);
    RUN_TEST(test_climbing_wall_detection);
    
    printf("\n--- Fracture Tests ---\n");
    RUN_TEST(test_voronoi_fragment_generation);
    
    printf("\n--- Broadphase Tests ---\n");
    RUN_TEST(test_gpu_broadphase_creation);
    RUN_TEST(test_spatial_hash_consistency);
    RUN_TEST(test_sap_sort_and_sweep);
    
    TEST_SUITE_END();
}
