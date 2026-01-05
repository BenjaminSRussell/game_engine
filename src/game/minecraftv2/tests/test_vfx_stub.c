#include "../include/vfx/particle_system.h>
#include <string.h>

void particle_system_init(ParticleSystem* system, VulkanRenderer* renderer) {
    if (!system) return;
    memset(system, 0, sizeof(ParticleSystem));
    system->initialized = true;
    system->active_particle_count = 0;
}

u32 particle_emit(ParticleSystem* system, ParticleType type, Vec3 position,
                 Vec3 velocity, f32 lifetime) {
    if (!system) return 0;
    u32 id = system->active_particle_count;
    system->active_particle_count++;
    if (system->active_particle_count > MAX_PARTICLES) system->active_particle_count = MAX_PARTICLES;
    return id;
}

void particle_emit_burst(ParticleSystem* system, ParticleType type, Vec3 position,
                        Vec3 velocity_base, f32 velocity_spread, u32 count,
                        f32 lifetime) {
    if (!system) return;
    system->active_particle_count += count;
    if (system->active_particle_count > MAX_PARTICLES) system->active_particle_count = MAX_PARTICLES;
}

u32 particle_get_active_count(ParticleSystem* system) {
    if (!system) return 0;
    return system->active_particle_count;
}

void particle_clear_all(ParticleSystem* system) {
    if (!system) return;
    system->active_particle_count = 0;
}

/* Minimal stubs for external dependencies to allow unit testing of VFX helpers */
#include <stdarg.h>

u32 audio_play_sound_2d(void *sys, int sound, f32 volume, int category) {
    (void)sys; (void)sound; (void)volume; (void)category; return 0;
}

void *chunk_manager_get(void *manager, void *pos) {
    (void)manager; (void)pos; return NULL;
}

int chunk_get_block(void *chunk, int x, int y, int z) {
    (void)chunk; (void)x; (void)y; (void)z; return 0; // BLOCK_AIR
}

void *item_registry_get(void *registry, u32 id) {
    (void)registry; (void)id; return NULL;
}

void logger_log(int level, const char *category, const char *format, ...) {
    (void)level; (void)category;
    va_list ap; va_start(ap, format); va_end(ap);
}
