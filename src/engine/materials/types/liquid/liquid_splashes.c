#include "materials/types/liquid/liquid_effects.h"
#include <stdlib.h>

extern void particle_system_spawn_burst(ParticleSystem* system, Vec3 origin, int count, float speed_min, float speed_max);

// Global splash system (would be part of a larger context in reality)
static ParticleSystem* g_SplashSystem = NULL;

void spawn_liquid_splash(Vec3 position, float force) {
    if (!g_SplashSystem) {
        // g_SplashSystem = particle_system_get("SplashParticles");
        return; 
    }

    int particle_count = (int)(force * 10.0f);
    if (particle_count > 100) particle_count = 100;
    if (particle_count < 5) particle_count = 5;

    // Upsward spray
    // particle_system_spawn_burst(g_SplashSystem, position, particle_count, 2.0f, 8.0f);
}
