/**
 * UNREAL ENGINE PARITY TESTS - NIAGARA PARTICLES
 * Test-Driven Development for VFX Particle System
 */

#include "../test_framework_unified.h"
#include "ue_parity_stubs.h"

// =============================================================================
// NIAGARA SYSTEM TESTS
// =============================================================================

static TestResult test_niagara_system_creation(void) {
    NiagaraSystem* sys = niagara_system_create("ExplosionFX");
    TEST_ASSERT_NOT_NULL(sys, "Niagara system should be created");
    
    niagara_system_destroy(sys);
    return TEST_PASS;
}

static TestResult test_niagara_emitter_config(void) {
    NiagaraSystem* sys = niagara_system_create("TestFX");
    
    NiagaraEmitterConfig config = {
        .spawn_mode = NIAGARA_SPAWN_RATE,
        .spawn_rate = 100.0f,
        .max_particles = 10000,
        .gpu_simulation = true,
        .deterministic = false
    };
    
    NiagaraEmitter* emitter = niagara_add_emitter(sys, &config);
    TEST_ASSERT_NOT_NULL(emitter, "Emitter should be added");
    
    niagara_system_destroy(sys);
    return TEST_PASS;
}

static TestResult test_niagara_burst_spawn(void) {
    NiagaraSystem* sys = niagara_system_create("BurstFX");
    
    NiagaraEmitterConfig config = {
        .spawn_mode = NIAGARA_SPAWN_BURST,
        .spawn_rate = 50.0f, // 50 particles per burst
        .max_particles = 1000
    };
    
    niagara_add_emitter(sys, &config);
    niagara_system_activate(sys, (Vec3){0, 0, 0});
    niagara_system_update(sys, 0.016f);
    
    uint32_t count = niagara_get_particle_count(sys);
    TEST_ASSERT_TRUE(count > 0, "Should have spawned particles");
    
    niagara_system_destroy(sys);
    return TEST_PASS;
}

static TestResult test_niagara_gpu_simulation(void) {
    NiagaraSystem* sys = niagara_system_create("GPUParticles");
    
    NiagaraEmitterConfig config = {
        .spawn_mode = NIAGARA_SPAWN_RATE,
        .spawn_rate = 10000.0f, // High spawn rate for GPU
        .max_particles = 1000000,
        .gpu_simulation = true
    };
    
    NiagaraEmitter* emitter = niagara_add_emitter(sys, &config);
    TEST_ASSERT_NOT_NULL(emitter, "GPU emitter created");
    
    niagara_system_destroy(sys);
    return TEST_PASS;
}

static TestResult test_niagara_data_interfaces(void) {
    // Test mesh sampling DI
    NiagaraDataInterface* mesh_di = niagara_di_create_mesh_sample(NULL);
    TEST_ASSERT_NOT_NULL(mesh_di, "Mesh sample DI should be created");
    
    // Test collision query DI
    NiagaraDataInterface* collision_di = niagara_di_create_collision_query(NULL);
    TEST_ASSERT_NOT_NULL(collision_di, "Collision query DI should be created");
    
    // Test audio spectrum DI
    NiagaraDataInterface* audio_di = niagara_di_create_audio_spectrum();
    TEST_ASSERT_NOT_NULL(audio_di, "Audio spectrum DI should be created");
    
    return TEST_PASS;
}

// =============================================================================
// REGISTRATION
// =============================================================================

void register_ue_niagara_tests(void) {
    TEST_REGISTER("UE:Niagara", "System creation", test_niagara_system_creation);
    TEST_REGISTER("UE:Niagara", "Emitter configuration", test_niagara_emitter_config);
    TEST_REGISTER("UE:Niagara", "Burst spawn mode", test_niagara_burst_spawn);
    TEST_REGISTER("UE:Niagara", "GPU simulation", test_niagara_gpu_simulation);
    TEST_REGISTER("UE:Niagara", "Data interfaces", test_niagara_data_interfaces);
}
