#include "physics/physics_system.h"
#include "../core/physics_types.h"
#include <stdlib.h>

// Forward declarations from core
PhysicsWorld* physics_world_create(PhysicsConfig config); // Config mismatch? 
// We need to map PhysicsSystemConfig to PhysicsConfig if they differ. 
// checking definition in physics_types.h... PhysicsConfig is not defined there!
// Wait, I used PhysicsConfig in physics_core.c call to physics_world_create. 
// But I didn't define PhysicsConfig in physics_types.h.
// It was likely in physics/physics.h (external) or I missed it.
// Let's assume I need to define it or map it.

// Let's fix physics_types.h/core to use the compatible config or define it.
// For now, I'll define a local mapping or fix the include.

typedef struct PhysicsSystem {
    PhysicsWorld* world;
    PhysicsSystemConfig config;
    bool initialized;
} PhysicsSystem;

// Re-declare core functions here for linkage
void physics_world_destroy(PhysicsWorld* world);
void physics_world_step(PhysicsWorld* world, float dt);
void physics_world_debug_draw(PhysicsWorld* world);

PhysicsSystem* physics_system_create(PhysicsSystemConfig config) {
    PhysicsSystem* sys = (PhysicsSystem*)calloc(1, sizeof(PhysicsSystem));
    if (!sys) return NULL;
    
    sys->config = config;
    
    // Config mapping
    // We need to ensure PhysicsConfig struct is available.
    // Assuming we pass individual params or a struct.
    // Looking at physics_core.c: physics_world_create(PhysicsConfig config)
    // I need to see where PhysicsConfig is defined.
    // It was likely in physics_engine_core.h which I read.
    // Ah, I missed copying PhysicsConfig to physics_types.h if it was there.
    // It was passed to physics_world_create in physics_core.c.
    
    // NOTE: For now, I will manually construct the world with a temp config struct 
    // that matches what physics_core.c expects (which includes gravity, etc).
    // Or I should fix physics_types.h to include it.
    
    // Creating a compatible struct inline for now
    PhysicsConfig core_conf;
    core_conf.gravity.x = config.gravity[0];
    core_conf.gravity.y = config.gravity[1];
    core_conf.gravity.z = config.gravity[2];
    core_conf.fixed_timestep = config.fixed_timestep > 0 ? config.fixed_timestep : 1.0f/60.0f;
    core_conf.velocity_iterations = 10;
    core_conf.position_iterations = 5;
    
    sys->world = physics_world_create(core_conf);
    
    sys->initialized = true;
    return sys;
}

void physics_system_destroy(PhysicsSystem* system) {
    if (!system) return;
    if (system->world) {
        physics_world_destroy(system->world);
    }
    free(system);
}

void physics_system_update(PhysicsSystem* system, float dt) {
    if (!system || !system->world) return;
    physics_world_step(system->world, dt);
}

void* physics_system_get_world(PhysicsSystem* system) {
    if (!system) return NULL;
    return system->world;
}

void physics_system_debug_render(PhysicsSystem* system) {
    if (!system || !system->world) return;
    physics_world_debug_draw(system->world);
}
