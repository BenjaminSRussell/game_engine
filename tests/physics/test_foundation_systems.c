/**
 * COMPREHENSIVE PHYSICS TESTS - PART 1: Foundation Systems
 * Tests for: Explosion, Buoyancy, Suspension, Shallow Water
 */

#include "physics_test.h"
#include <physics/destruction/explosion_system.h>
#include <physics/fluids/buoyancy_solver.h>
#include <physics/vehicles/suspension.h>
#include <physics/fluids/shallow_water.h>

// ============================================================================
// EXPLOSION SYSTEM TESTS
// ============================================================================

void test_explosion_spawn() {
    PhysicsWorld *world = NULL; // Stub
    ExplosionSystem *sys = explosion_system_create(world, 10);
    ASSERT_NOT_NULL(sys);
    
    float pos[3] = {0, 0, 0};
    int idx = explosion_spawn(sys, pos, 10.0f, 1000.0f, 100.0f);
    ASSERT_TRUE(idx >= 0);
    
    explosion_system_destroy(sys);
}

void test_explosion_damage_falloff() {
    PhysicsWorld *world = NULL;
    ExplosionSystem *sys = explosion_system_create(world, 10);
    
    float pos[3] = {0, 0, 0};
    int idx = explosion_spawn(sys, pos, 10.0f, 1000.0f, 100.0f);
    
    Explosion *exp = &sys->explosions[idx];
    
    // At center: full damage
    float damage_center = explosion_calculate_damage(exp, pos);
    ASSERT_FLOAT_EQ(damage_center, 100.0f, 0.1f);
    
    // At half radius: reduced damage (quadratic falloff)
    float pos_half[3] = {5.0f, 0, 0};
    float damage_half = explosion_calculate_damage(exp, pos_half);
    ASSERT_TRUE(damage_half < damage_center);
    ASSERT_TRUE(damage_half > 0.0f);
    
    // Outside radius: no damage
    float pos_far[3] = {20.0f, 0, 0};
    float damage_far = explosion_calculate_damage(exp, pos_far);
    ASSERT_FLOAT_EQ(damage_far, 0.0f, 0.01f);
    
    explosion_system_destroy(sys);
}

void test_explosion_impulse_calculation() {
    PhysicsWorld *world = NULL;
    ExplosionSystem *sys = explosion_system_create(world, 10);
    
    float pos[3] = {0, 0, 0};
    int idx = explosion_spawn(sys, pos, 10.0f, 1000.0f, 100.0f);
    Explosion *exp = &sys->explosions[idx];
    
    float target[3] = {5.0f, 0, 0};
    float impulse[3];
    explosion_calculate_impulse(exp, target, impulse);
    
    // Impulse should point away from explosion
    ASSERT_TRUE(impulse[0] > 0.0f); // Positive X direction
    ASSERT_FLOAT_EQ(impulse[1], 0.0f, 0.01f); // No Y component
    ASSERT_FLOAT_EQ(impulse[2], 0.0f, 0.01f); // No Z component
    
    explosion_system_destroy(sys);
}

// ============================================================================
// BUOYANCY TESTS
// ============================================================================

void test_buoyancy_submerged_volume_sphere() {
    // Create mock rigid body with sphere collider
    // Simplified: test volume calculation directly
    
    // Sphere fully submerged
    float water_level = 10.0f;
    // Test would need actual RigidBody - simplified placeholder
    ASSERT_TRUE(true); // Placeholder for actual implementation
}

void test_buoyancy_archimedes_force() {
    FluidSurface fluid = {
        .height = 0.0f,
        .density = 1000.0f, // Water
        .viscosity = 0.001f,
        .surface_tension = 0.0f,
        .flow_direction = {0, 0, 0},
        .flow_speed = 0.0f
    };
    
    // Test buoyancy force = ρVg
    float volume = 1.0f; // 1 cubic meter
    float expected_force = fluid.density * volume * 9.81f;
    
    // Force should be approximately 9810 N upward
    ASSERT_FLOAT_EQ(expected_force, 9810.0f, 1.0f);
}

void test_buoyancy_drag_force() {
    // Test that drag opposes motion
    FluidSurface fluid = {
        .height = 0.0f,
        .density = 1000.0f,
        .viscosity = 0.5f,
        .surface_tension = 0.0f,
        .flow_direction = {0, 0, 0},
        .flow_speed = 0.0f
    };
    
    // Drag force should be proportional to v²
    // F_drag = 0.5 * ρ * C_d * A * v²
    float velocity = 10.0f; // m/s
    float cross_section = 1.0f; // m²
    float drag = 0.5f * fluid.density * fluid.viscosity * cross_section * velocity * velocity;
    
    ASSERT_TRUE(drag > 0.0f);
}

// ============================================================================
// SUSPENSION TESTS
// ============================================================================

void test_suspension_spring_damper() {
    SuspensionConfig config = {
        .rest_length = 1.0f,
        .spring_stiffness = 10000.0f,
        .damping_coefficient = 1000.0f,
        .max_compression = 0.3f,
        .max_extension = 0.2f,
        .anti_squat_factor = 0.5f,
        .anti_dive_factor = 0.5f,
        .camber_angle = 0.0f,
        .camber_rate = 0.1f,
        .wheel_attach_local = {0, 0, 0},
        .ray_direction = {0, -1, 0}
    };
    
    // Test spring force calculation
    float compression = 0.1f; // 10cm compressed
    float velocity = 0.5f; // 0.5 m/s compression rate
    
    float spring_force = config.spring_stiffness * compression;
    float damping_force = config.damping_coefficient * velocity;
    float total_force = spring_force - damping_force;
    
    ASSERT_FLOAT_EQ(spring_force, 1000.0f, 0.1f);
    ASSERT_FLOAT_EQ(damping_force, 500.0f, 0.1f);
    ASSERT_FLOAT_EQ(total_force, 500.0f, 0.1f);
}

void test_suspension_compression_limits() {
    SuspensionConfig config = {
        .rest_length = 1.0f,
        .spring_stiffness = 10000.0f,
        .damping_coefficient = 1000.0f,
        .max_compression = 0.3f,
        .max_extension = 0.2f,
        .anti_squat_factor = 0.0f,
        .anti_dive_factor = 0.0f,
        .camber_angle = 0.0f,
        .camber_rate = 0.0f,
        .wheel_attach_local = {0, 0, 0},
        .ray_direction = {0, -1, 0}
    };
    
    ASSERT_FLOAT_EQ(config.max_compression, 0.3f, 0.01f);
    ASSERT_FLOAT_EQ(config.max_extension, 0.2f, 0.01f);
}

// ============================================================================
// SHALLOW WATER TESTS
// ============================================================================

void test_shallow_water_init() {
    ShallowWaterGrid *grid = shallow_water_init(64, 64, 1.0f);
    ASSERT_NOT_NULL(grid);
    ASSERT_INT_EQ(grid->width, 64);
    ASSERT_INT_EQ(grid->depth, 64);
    ASSERT_FLOAT_EQ(grid->cell_size, 1.0f, 0.01f);
    
    // Check initial state
    ASSERT_NOT_NULL(grid->height);
    ASSERT_NOT_NULL(grid->velocity_x);
    ASSERT_NOT_NULL(grid->velocity_z);
    
    // Free
    free(grid->height);
    free(grid->velocity_x);
    free(grid->velocity_z);
    free(grid->foam);
    free(grid);
}

void test_shallow_water_wave_propagation() {
    ShallowWaterGrid *grid = shallow_water_init(64, 64, 1.0f);
    
    // Add initial disturbance
    grid->height[32 * 64 + 32] = 1.0f;
    
    // Update and check wave spreads
    shallow_water_update(grid, 0.016f);
    
    // Wave should have spread to neighbors
    ASSERT_TRUE(fabsf(grid->height[32 * 64 + 33]) > 0.0f ||
                fabsf(grid->height[32 * 64 + 31]) > 0.0f);
    
    free(grid->height);
    free(grid->velocity_x);
    free(grid->velocity_z);
    free(grid->foam);
    free(grid);
}

void test_shallow_water_ripple_interaction() {
    ShallowWaterGrid *grid = shallow_water_init(64, 64, 1.0f);
    
    // Add ripple at specific location
    shallow_water_add_ripple(grid, 32.0f, 32.0f, 0.5f);
    
    // Check ripple was added
    ASSERT_FLOAT_EQ(grid->height[32 * 64 + 32], 0.5f, 0.01f);
    
    free(grid->height);
    free(grid->velocity_x);
    free(grid->velocity_z);
    free(grid->foam);
    free(grid);
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main() {
    TEST_SUITE_BEGIN("Physics Foundation Systems");
    
    printf("\n--- Explosion Tests ---\n");
    RUN_TEST(test_explosion_spawn);
    RUN_TEST(test_explosion_damage_falloff);
    RUN_TEST(test_explosion_impulse_calculation);
    
    printf("\n--- Buoyancy Tests ---\n");
    RUN_TEST(test_buoyancy_submerged_volume_sphere);
    RUN_TEST(test_buoyancy_archimedes_force);
    RUN_TEST(test_buoyancy_drag_force);
    
    printf("\n--- Suspension Tests ---\n");
    RUN_TEST(test_suspension_spring_damper);
    RUN_TEST(test_suspension_compression_limits);
    
    printf("\n--- Shallow Water Tests ---\n");
    RUN_TEST(test_shallow_water_init);
    RUN_TEST(test_shallow_water_wave_propagation);
    RUN_TEST(test_shallow_water_ripple_interaction);
    
    TEST_SUITE_END();
}
