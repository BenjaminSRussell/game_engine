#include "effects/particles/particle_system.h"
#include "core/logger.h"
#include "core/memory.h"
#include "rendering/render_types.h"
#include <stdlib.h>
#include <string.h>

// Internal particle system state
struct ParticleSystem {
    gpu_particle_system_t* simulation;
    ParticleEmitter** emitters;
    u32 emitter_count;
    u32 max_emitters;
    bool initialized;
};

// Global instance
static ParticleSystem g_particle_system = {0};

bool particle_system_init(const particle_system_config_t* config) {
    if (g_particle_system.initialized) {
        LOG_WARN("Particle system already initialized");
        return true;
    }

    if (!config) {
        LOG_ERROR("Cannot initialize particle system with null config");
        return false;
    }

    LOG_INFO("Initializing particle system (Max particles: %u, GPU: %s)",
             config->max_particles, config->use_gpu ? "Yes" : "No");

    // Initialize simulation
    g_particle_system.simulation = particle_simulation_create(config->max_particles);
    if (!g_particle_system.simulation) {
        LOG_ERROR("Failed to create particle simulation");
        return false;
    }

    // Set GPU mode if requested
    if (config->use_gpu) {
        particle_simulation_set_gpu_mode(g_particle_system.simulation, true);
    }

    // Initialize emitter storage
    g_particle_system.max_emitters = 1024; // Default max emitters
    g_particle_system.emitters = (ParticleEmitter**)calloc(g_particle_system.max_emitters, sizeof(ParticleEmitter*));
    if (!g_particle_system.emitters) {
        LOG_ERROR("Failed to allocate emitter storage");
        particle_simulation_destroy(g_particle_system.simulation);
        return false;
    }

    g_particle_system.emitter_count = 0;
    g_particle_system.initialized = true;

    LOG_INFO("Particle system initialized successfully");
    return true;
}

void particle_system_shutdown(void) {
    if (!g_particle_system.initialized) {
        return;
    }

    LOG_INFO("Shutting down particle system");

    // Destroy all emitters
    for (u32 i = 0; i < g_particle_system.max_emitters; i++) {
        if (g_particle_system.emitters[i]) {
            particle_emitter_destroy(g_particle_system.emitters[i]);
            g_particle_system.emitters[i] = NULL;
        }
    }

    free(g_particle_system.emitters);
    g_particle_system.emitters = NULL;

    // Destroy simulation
    if (g_particle_system.simulation) {
        particle_simulation_destroy(g_particle_system.simulation);
        g_particle_system.simulation = NULL;
    }

    g_particle_system.initialized = false;
}

ParticleSystem* particle_system_get(void) {
    if (!g_particle_system.initialized) {
        return NULL;
    }
    return &g_particle_system;
}

void particle_system_update(float delta_time) {
    if (!g_particle_system.initialized) {
        return;
    }

    // 1. Update all emitters (they generate new particles)
    for (u32 i = 0; i < g_particle_system.max_emitters; i++) {
        ParticleEmitter* emitter = g_particle_system.emitters[i];
        if (emitter && emitter->active) {
            particle_emitter_update(emitter, delta_time);

            // Transfer new particles from emitter to global simulation
            // Note: In a real implementation, we might want to avoid copying
            // and instead have emitters write directly to simulation buffers,
            // or have the simulation pull from emitters.
            // For now, we assume emitters manage their own particles locally,
            // but we might want to consolidate them for rendering.

            // Current `particle_emitter.c` manages its own particle array.
            // `particle_simulation.c` also manages a particle array.
            // We need to decide if emitters are just generators or containers.
            // Based on `particle_emitter.c`, it contains particles.
            // Based on `particle_simulation.c`, it contains particles.

            // Strategy: Emitters are high-level logic that can spawn particles into the simulation.
            // But `particle_emitter_update` in `particle_emitter.c` simulates particles itself!
            // This is a duplication of logic.
            // Ideally, `particle_emitter` should just emit, and `particle_simulation` should update.

            // For this implementation, I will treat `particle_emitter` as a standalone system for now
            // since rewriting it completely might be out of scope or break existing behavior if used elsewhere.
            // However, the task prompt asks to "Implement particle update loop (physics integration, lifetime)"
            // inside `particle_system.c` context.

            // If `particle_emitter` is already updating particles, we don't need to do it twice.
            // But `particle_simulation` is the "GPU accelerated" one.

            // Let's assume for now that `particle_simulation` is the main system,
            // and we should move particles from emitters to simulation if we want centralized rendering/physics.

            // However, `particle_emitter.c` as implemented does everything itself.
            // I will leave it as is for now to satisfy "emitters spawn particles".

            // If we want to use `particle_simulation`, we should probably change how emitters work.
            // But let's stick to updating what we have.
        }
    }

    // 2. Update global simulation (if used for non-emitter particles or if we bridge them)
    simulation_params_t params = {
        .gravity = {0.0f, -9.8f, 0.0f},
        .wind_velocity = {0.0f, 0.0f, 0.0f},
        .delta_time = delta_time,
        .drag_coefficient = 0.5f,
        .bounce_damping = 0.5f,
        .collision_enabled = true
    };

    if (g_particle_system.simulation) {
        // We can optionally check for specific particles that need global simulation
        // For now, just update the system
        if (g_particle_system.simulation->use_gpu_simulation) {
             particle_simulation_update_gpu(g_particle_system.simulation, &params);
        } else {
             particle_simulation_update_cpu(g_particle_system.simulation, &params);
        }
    }
}

void particle_system_render(const void* view_matrix, const void* proj_matrix) {
    if (!g_particle_system.initialized) {
        return;
    }

    // TODO: Implement actual rendering
    // This would involve:
    // 1. Collecting all active particles from emitters and simulation
    // 2. Building a vertex buffer (billboards or instanced meshes)
    // 3. Submitting draw calls to the renderer

    // Since we don't have direct access to the renderer submit queue here easily without more context,
    // and RENDER-001 is a separate task, we will leave this as a placeholder.
    // However, we can log some debug info if needed.
    // LOG_DEBUG("Rendering particles...");
}

ParticleEmitter* particle_system_create_emitter(const emitter_params_t* params) {
    if (!g_particle_system.initialized) {
        return NULL;
    }

    // Find free slot
    for (u32 i = 0; i < g_particle_system.max_emitters; i++) {
        if (g_particle_system.emitters[i] == NULL) {
            ParticleEmitter* emitter = particle_emitter_create(params);
            if (emitter) {
                g_particle_system.emitters[i] = emitter;
                g_particle_system.emitter_count++;
                return emitter;
            }
            return NULL;
        }
    }

    LOG_WARN("Max emitters reached (%u), cannot create new emitter", g_particle_system.max_emitters);
    return NULL;
}

void particle_system_destroy_emitter(ParticleEmitter* emitter) {
    if (!g_particle_system.initialized || !emitter) {
        return;
    }

    for (u32 i = 0; i < g_particle_system.max_emitters; i++) {
        if (g_particle_system.emitters[i] == emitter) {
            particle_emitter_destroy(emitter);
            g_particle_system.emitters[i] = NULL;
            g_particle_system.emitter_count--;
            return;
        }
    }
}

particle_stats_t particle_system_get_stats(void) {
    if (!g_particle_system.initialized || !g_particle_system.simulation) {
        particle_stats_t empty = {0};
        return empty;
    }

    particle_stats_t stats = particle_simulation_get_stats(g_particle_system.simulation);

    // Add emitter stats
    for (u32 i = 0; i < g_particle_system.max_emitters; i++) {
        if (g_particle_system.emitters[i]) {
            stats.active_particles += g_particle_system.emitters[i]->particle_count;
        }
    }

    return stats;
}
