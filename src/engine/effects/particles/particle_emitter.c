/*
 * particle_emitter.c
 * Particle emitter implementation for CPU and GPU particle systems
 */

#include "effects/particles/particle_emitter.h"
#include "effects/particles/particle_types.h"
#include "engine/include/core/logger.h"
#include "core/memory.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Create a new particle emitter
ParticleEmitter* particle_emitter_create(const emitter_params_t* params) {
    if (!params) {
        LOG_ERROR("Cannot create emitter with null parameters");
        return NULL;
    }
    
    ParticleEmitter* emitter = (ParticleEmitter*)malloc(sizeof(ParticleEmitter));
    if (!emitter) {
        LOG_ERROR("Failed to allocate memory for particle emitter");
        return NULL;
    }
    
    // Initialize emitter
    memset(emitter, 0, sizeof(ParticleEmitter));
    emitter->params = *params;
    emitter->time_since_last_emit = 0.0f;
    emitter->active = true;
    emitter->particle_count = 0;
    
    // Allocate particle array
    emitter->max_particles = params->flags & 0xFF; // Use low byte of flags for max particles
    if (emitter->max_particles == 0) {
        emitter->max_particles = 1000; // Default
    }
    
    emitter->particles = (particle_t*)malloc(sizeof(particle_t) * emitter->max_particles);
    if (!emitter->particles) {
        LOG_ERROR("Failed to allocate particle array");
        free(emitter);
        return NULL;
    }
    
    // Initialize all particles as dead
    memset(emitter->particles, 0, sizeof(particle_t) * emitter->max_particles);
    for (u32 i = 0; i < emitter->max_particles; i++) {
        emitter->particles[i].flags = 0; // Dead
    }
    
    LOG_INFO("Created particle emitter with max %u particles", emitter->max_particles);
    return emitter;
}

// Update particle emitter
void particle_emitter_update(ParticleEmitter* emitter, float delta_time) {
    if (!emitter || !emitter->active) {
        return;
    }
    
    emitter->time_since_last_emit += delta_time;
    
    // Emit new particles
    if (emitter->params.emission_rate > 0.0f) {
        float emit_interval = 1.0f / emitter->params.emission_rate;
        while (emitter->time_since_last_emit >= emit_interval) {
            particle_emitter_emit(emitter, 1);
            emitter->time_since_last_emit -= emit_interval;
        }
    }
    
    // Update existing particles
    for (u32 i = 0; i < emitter->max_particles; i++) {
        particle_t* particle = &emitter->particles[i];
        
        if (!(particle->flags & PARTICLE_FLAG_ALIVE)) {
            continue;
        }
        
        // Update age
        particle->age += delta_time;
        
        // Kill old particles
        if (particle->age >= particle->lifetime) {
            particle->flags &= ~PARTICLE_FLAG_ALIVE;
            particle->flags |= PARTICLE_FLAG_DEAD;
            emitter->particle_count--;
            continue;
        }
        
        // Update physics
        particle->position.x += particle->velocity.x * delta_time;
        particle->position.y += particle->velocity.y * delta_time;
        particle->position.z += particle->velocity.z * delta_time;
        
        particle->velocity.x += particle->acceleration.x * delta_time;
        particle->velocity.y += particle->acceleration.y * delta_time;
        particle->velocity.z += particle->acceleration.z * delta_time;
        
        // Update rotation
        particle->rotation += particle->rotation_speed * delta_time;
        
        // Update size (lerp between start and end)
        float t = particle->age / particle->lifetime;
        particle->size = emitter->params.size_start + 
                       (emitter->params.size_end - emitter->params.size_start) * t;
        
        // Update color (lerp)
        particle->color.r = emitter->params.color_start.r + 
                         (emitter->params.color_end.r - emitter->params.color_start.r) * t;
        particle->color.g = emitter->params.color_start.g + 
                         (emitter->params.color_end.g - emitter->params.color_start.g) * t;
        particle->color.b = emitter->params.color_start.b + 
                         (emitter->params.color_end.b - emitter->params.color_start.b) * t;
        particle->color.a = emitter->params.color_start.a + 
                         (emitter->params.color_end.a - emitter->params.color_start.a) * t;
    }
}

// Emit particles from emitter
void particle_emitter_emit(ParticleEmitter* emitter, u32 count) {
    if (!emitter || !emitter->active || count == 0) {
        return;
    }
    
    u32 emitted = 0;
    for (u32 i = 0; i < emitter->max_particles && emitted < count; i++) {
        particle_t* particle = &emitter->particles[i];
        
        if (particle->flags & PARTICLE_FLAG_ALIVE) {
            continue; // Skip alive particles
        }
        
        // Initialize particle
        particle->position = emitter->params.position;
        particle->velocity = emitter->params.velocity;
        particle->acceleration = emitter->params.acceleration;
        
        particle->size = emitter->params.size_start;
        particle->rotation = emitter->params.rotation_start;
        particle->rotation_speed = emitter->params.rotation_speed;
        particle->mass = emitter->params.mass;
        
        particle->color = emitter->params.color_start;
        particle->age = 0.0f;
        particle->lifetime = emitter->params.lifetime;
        
        particle->flags = PARTICLE_FLAG_ALIVE;
        if (emitter->params.flags & PARTICLE_FLAG_COLLIDES) {
            particle->flags |= PARTICLE_FLAG_COLLIDES;
        }
        if (emitter->params.flags & PARTICLE_FLAG_EMITS_LIGHT) {
            particle->flags |= PARTICLE_FLAG_EMITS_LIGHT;
        }
        if (emitter->params.flags & PARTICLE_FLAG_TRAIL) {
            particle->flags |= PARTICLE_FLAG_TRAIL;
        }
        
        emitter->particle_count++;
        emitted++;
    }
}

// Destroy particle emitter
void particle_emitter_destroy(ParticleEmitter* emitter) {
    if (!emitter) {
        return;
    }
    
    if (emitter->particles) {
        free(emitter->particles);
    }
    
    free(emitter);
    LOG_INFO("Destroyed particle emitter");
}
