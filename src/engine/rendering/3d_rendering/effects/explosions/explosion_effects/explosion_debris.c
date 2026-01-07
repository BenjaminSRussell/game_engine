#include "explosion_effects.h"
#include <stdlib.h>

// Mock particle spawn function
// In reality, this would be part of the particle system API
void particle_system_spawn_burst(ParticleSystem* system, Vec3 origin, int count, float speed_min, float speed_max);

void spawn_debris(ExplosionEffect* effect, Vec3 origin, float intensity) {
    if (!effect) return;

    if (effect->debris_system) {
        // Spawn solid debris
        // particle_system_spawn_burst(effect->debris_system, origin, (int)(intensity * 50), 5.0f, 15.0f);
    }

    if (effect->spark_system) {
        // Spawn fast sparks
        // particle_system_spawn_burst(effect->spark_system, origin, (int)(intensity * 100), 10.0f, 30.0f);
    }
}

// Helper stub since we don't have the actual particle system definition
void particle_system_spawn_burst(ParticleSystem* system, Vec3 origin, int count, float speed_min, float speed_max) {
    // Implementation would go here
}
