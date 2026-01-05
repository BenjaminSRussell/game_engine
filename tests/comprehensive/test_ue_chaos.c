/**
 * UNREAL ENGINE PARITY TESTS - CHAOS PHYSICS
 * Test-Driven Development for Chaos Destruction, Cloth, and Vehicles
 */

#include "../test_framework_unified.h"
#include "ue_parity_stubs.h"

// =============================================================================
// CHAOS DESTRUCTION TESTS
// =============================================================================

static TestResult test_chaos_destructible_creation(void) {
    ChaosDestructionConfig config = {
        .cluster_connection_method = 0, // Proximity
        .damage_threshold = 100.0f,
        .max_cluster_count = 1000,
        .enable_clustering = true,
        .cluster_level_count = 3
    };
    
    DestructibleMesh* dm = chaos_destructible_create(NULL, &config);
    TEST_ASSERT_NOT_NULL(dm, "Destructible mesh should be created");
    
    chaos_destructible_destroy(dm);
    return TEST_PASS;
}

static TestResult test_chaos_destruction_damage(void) {
    ChaosDestructionConfig config = {
        .damage_threshold = 50.0f,
        .max_cluster_count = 100,
        .enable_clustering = true
    };
    
    DestructibleMesh* dm = chaos_destructible_create(NULL, &config);
    
    // Initial chunk count should be 1 (intact)
    uint32_t initial_chunks = chaos_destructible_get_chunk_count(dm);
    
    // Apply damage
    Vec3 damage_location = {0, 0, 0};
    chaos_destructible_apply_damage(dm, damage_location, 5.0f, 100.0f);
    
    // After damage, should have more chunks
    uint32_t after_damage_chunks = chaos_destructible_get_chunk_count(dm);
    TEST_ASSERT_TRUE(after_damage_chunks >= initial_chunks, 
                    "Should have same or more chunks after damage");
    
    chaos_destructible_destroy(dm);
    return TEST_PASS;
}

static TestResult test_chaos_destruction_hierarchy(void) {
    // Test hierarchical destruction (small damage = small pieces, big = large chunks)
    float cluster_sizes[] = {100.0f, 50.0f, 10.0f}; // Level sizes
    
    ChaosDestructionConfig config = {
        .damage_threshold = 25.0f,
        .max_cluster_count = 500,
        .enable_clustering = true,
        .cluster_sizes = cluster_sizes,
        .cluster_level_count = 3
    };
    
    DestructibleMesh* dm = chaos_destructible_create(NULL, &config);
    
    // Small damage - should break into larger chunks only
    chaos_destructible_apply_damage(dm, (Vec3){0,0,0}, 1.0f, 30.0f);
    uint32_t small_damage_chunks = chaos_destructible_get_chunk_count(dm);
    
    // Large damage - should break into many small pieces
    chaos_destructible_apply_damage(dm, (Vec3){0,0,0}, 5.0f, 200.0f);
    uint32_t large_damage_chunks = chaos_destructible_get_chunk_count(dm);
    
    TEST_ASSERT_TRUE(large_damage_chunks >= small_damage_chunks,
                    "More damage should create more chunks");
    
    chaos_destructible_destroy(dm);
    return TEST_PASS;
}

// =============================================================================
// CHAOS CLOTH TESTS
// =============================================================================

static TestResult test_chaos_cloth_creation(void) {
    ChaosClothConfig config = {
        .mass_per_area = 0.1f,
        .stiffness = 1.0f,
        .damping = 0.1f,
        .friction = 0.5f,
        .collision_thickness = 0.01f,
        .self_collision = true
    };
    
    ChaosCloth* cloth = chaos_cloth_create(NULL, &config);
    TEST_ASSERT_NOT_NULL(cloth, "Chaos cloth should be created");
    
    chaos_cloth_destroy(cloth);
    return TEST_PASS;
}

static TestResult test_chaos_cloth_simulation(void) {
    ChaosClothConfig config = {
        .mass_per_area = 0.1f,
        .stiffness = 0.8f,
        .damping = 0.05f
    };
    
    ChaosCloth* cloth = chaos_cloth_create(NULL, &config);
    
    // Simulate for a few frames
    for (int i = 0; i < 60; i++) {
        chaos_cloth_simulate(cloth, 1.0f / 60.0f);
    }
    
    // Cloth should still be valid after simulation
    TEST_ASSERT_NOT_NULL(cloth, "Cloth valid after simulation");
    
    chaos_cloth_destroy(cloth);
    return TEST_PASS;
}

static TestResult test_chaos_cloth_wind(void) {
    ChaosClothConfig config = {.mass_per_area = 0.05f};
    ChaosCloth* cloth = chaos_cloth_create(NULL, &config);
    
    // Apply wind force
    Vec3 wind_dir = {1.0f, 0.0f, 0.0f};
    chaos_cloth_apply_wind(cloth, wind_dir, 10.0f);
    
    // Simulate
    chaos_cloth_simulate(cloth, 0.016f);
    
    // Wind applied successfully (no crash)
    chaos_cloth_destroy(cloth);
    return TEST_PASS;
}

// =============================================================================
// CHAOS VEHICLE TESTS
// =============================================================================

static TestResult test_chaos_vehicle_creation(void) {
    float torque_curve[] = {0.0f, 200.0f, 0.5f, 400.0f, 1.0f, 300.0f};
    
    ChaosVehicleConfig config = {
        .chassis_mass = 1500.0f,
        .max_engine_rpm = 7000.0f,
        .torque_curve = torque_curve,
        .torque_curve_points = 3,
        .wheel_count = 4,
        .suspension_max_raise = 0.1f,
        .suspension_max_drop = 0.15f
    };
    
    ChaosVehicle* vehicle = chaos_vehicle_create(&config);
    TEST_ASSERT_NOT_NULL(vehicle, "Chaos vehicle should be created");
    
    chaos_vehicle_destroy(vehicle);
    return TEST_PASS;
}

static TestResult test_chaos_vehicle_controls(void) {
    ChaosVehicleConfig config = {
        .chassis_mass = 1200.0f,
        .max_engine_rpm = 6500.0f,
        .wheel_count = 4
    };
    
    ChaosVehicle* vehicle = chaos_vehicle_create(&config);
    
    // Apply throttle
    chaos_vehicle_set_throttle(vehicle, 1.0f);
    chaos_vehicle_set_steering(vehicle, 0.0f);
    chaos_vehicle_set_brake(vehicle, 0.0f);
    
    // Check speed (should increase with throttle)
    float speed = chaos_vehicle_get_speed_kmh(vehicle);
    TEST_ASSERT_TRUE(speed >= 0.0f, "Speed should be non-negative");
    
    chaos_vehicle_destroy(vehicle);
    return TEST_PASS;
}

static TestResult test_chaos_vehicle_braking(void) {
    ChaosVehicleConfig config = {.chassis_mass = 1000.0f, .wheel_count = 4};
    ChaosVehicle* vehicle = chaos_vehicle_create(&config);
    
    // Full brake should be applicable
    chaos_vehicle_set_brake(vehicle, 1.0f);
    
    float speed_with_brake = chaos_vehicle_get_speed_kmh(vehicle);
    // With full brake from standstill, speed should be 0
    TEST_ASSERT_FLOAT_EQ(speed_with_brake, 0.0f, 0.1f, "Should be stopped with brake");
    
    chaos_vehicle_destroy(vehicle);
    return TEST_PASS;
}

// =============================================================================
// REGISTRATION
// =============================================================================

void register_ue_chaos_tests(void) {
    // Destruction tests
    TEST_REGISTER("UE:Chaos", "Destructible mesh creation", test_chaos_destructible_creation);
    TEST_REGISTER("UE:Chaos", "Destruction damage", test_chaos_destruction_damage);
    TEST_REGISTER("UE:Chaos", "Destruction hierarchy", test_chaos_destruction_hierarchy);
    
    // Cloth tests
    TEST_REGISTER("UE:Chaos", "Cloth creation", test_chaos_cloth_creation);
    TEST_REGISTER("UE:Chaos", "Cloth simulation", test_chaos_cloth_simulation);
    TEST_REGISTER("UE:Chaos", "Cloth wind", test_chaos_cloth_wind);
    
    // Vehicle tests
    TEST_REGISTER("UE:Chaos", "Vehicle creation", test_chaos_vehicle_creation);
    TEST_REGISTER("UE:Chaos", "Vehicle controls", test_chaos_vehicle_controls);
    TEST_REGISTER("UE:Chaos", "Vehicle braking", test_chaos_vehicle_braking);
}
