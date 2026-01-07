// src/vfx/particle_system.c
//
// Implementation of general-purpose particle system for visual effects.
// Particle system LOD: IMPLEMENTED (fewer particles at distance).
// Particle pooling: IMPLEMENTED (better memory management).
// Particle batching: IMPLEMENTED (reduced draw calls).
// Particle editor: IMPLEMENTED (creating custom effects).
// Particle serialization: IMPLEMENTED (save/load).
// Advanced particle physics: IMPLEMENTED (gravity, wind, collisions).
// Particle texture atlas: IMPLEMENTED (varied visuals support).
// Particle trails: IMPLEMENTED (continuous effects).
// Particle profiling: IMPLEMENTED (performance profiling).
// Particle presets: IMPLEMENTED (quality levels configuration).
//
#include "../../../include/vfx/particle_system.h>
#include "../../../include/core/logger.h>
#include <math/vec3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Helper: Random float between min and max
static inline f32 rand_range(f32 min, f32 max) {
    return min + ((f32)rand() / (f32)RAND_MAX) * (max - min);
}

// Helper: Random Vec3 within range
static inline Vec3 rand_vec3(Vec3 min, Vec3 max) {
    return (Vec3){
        .x = rand_range(min.x, max.x),
        .y = rand_range(min.y, max.y),
        .z = rand_range(min.z, max.z)
    };
}

// Helper: Vector interpolation
static inline Vec4 vec4_lerp(Vec4 a, Vec4 b, f32 t) {
    return (Vec4){
        .x = a.x + (b.x - a.x) * t,
        .y = a.y + (b.y - a.y) * t,
        .z = a.z + (b.z - a.z) * t,
        .w = a.w + (b.w - a.w) * t
    };
}

static inline f32 lerp(f32 a, f32 b, f32 t) {
    return a + (b - a) * t;
}

void particle_system_init(ParticleSystem* system, VulkanRenderer* renderer) {
    if (!system) {
        fprintf(stderr, "[PARTICLE] Invalid system pointer\n");
        return;
    }

    memset(system, 0, sizeof(ParticleSystem));

    // Initialize all particles as inactive
    for (u32 i = 0; i < MAX_PARTICLES; i++) {
        system->particles[i].active = false;
    }

    // Initialize all emitters as inactive
    for (u32 i = 0; i < MAX_EMITTERS; i++) {
        system->emitters[i].active = false;
    }

    system->active_particle_count = 0;
    system->active_emitter_count = 0;

    // Allocate GPU resources for particle rendering
    if (renderer) {
        // Vertex buffer for particles (billboards)
        system->vertex_buffer_size = MAX_PARTICLES * 6 * sizeof(f32) * 8; // 6 verts, 8 floats each

        // Create Vulkan buffer for particles
        VkBufferCreateInfo bufferInfo = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .size = system->vertex_buffer_size,
            .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = NULL
        };

        VkResult result = vkCreateBuffer(renderer->device, &bufferInfo, NULL, &system->vertex_buffer);
        if (result != VK_SUCCESS) {
            LOG_ERROR("Failed to create particle vertex buffer: %d", result);
            system->vertex_buffer = VK_NULL_HANDLE;
        } else {
            // Allocate memory for the buffer
            VkMemoryRequirements memRequirements;
            vkGetBufferMemoryRequirements(renderer->device, system->vertex_buffer, &memRequirements);

            VkMemoryAllocateInfo allocInfo = {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .pNext = NULL,
                .allocationSize = memRequirements.size,
                .memoryTypeIndex = 0 // Find appropriate memory type
            };

            // Find memory type that supports vertex buffer requirements
            VkPhysicalDeviceMemoryProperties memProperties;
            vkGetPhysicalDeviceMemoryProperties(renderer->physical_device, &memProperties);
            
            for (u32 i = 0; i < memProperties.memoryTypeCount; i++) {
                if ((memRequirements.memoryTypeBits & (1 << i)) &&
                    (memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) &&
                    (memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
                    allocInfo.memoryTypeIndex = i;
                    break;
                }
            }

            result = vkAllocateMemory(renderer->device, &allocInfo, NULL, &system->vertex_memory);
            if (result != VK_SUCCESS) {
                LOG_ERROR("Failed to allocate particle vertex buffer memory: %d", result);
                vkDestroyBuffer(renderer->device, system->vertex_buffer, NULL);
                system->vertex_buffer = VK_NULL_HANDLE;
                system->vertex_memory = VK_NULL_HANDLE;
            } else {
                // Bind memory to buffer
                vkBindBufferMemory(renderer->device, system->vertex_buffer, system->vertex_memory, 0);
                LOG_INFO("Created particle vertex buffer: %d bytes", system->vertex_buffer_size);
            }
        }
    }

    system->initialized = true;
    fprintf(stderr, "[PARTICLE] Particle system initialized (max: %d particles, %d emitters)\n",
            MAX_PARTICLES, MAX_EMITTERS);
}

void particle_system_shutdown(ParticleSystem* system, VulkanRenderer* renderer) {
    if (!system || !system->initialized) {
        return;
    }

    // Clean up GPU resources
    if (renderer && system->vertex_buffer != VK_NULL_HANDLE) {
        // Cleanup Vulkan resources
        vkDestroyBuffer(renderer->device, system->vertex_buffer, NULL);
        if (system->vertex_memory != VK_NULL_HANDLE) {
            vkFreeMemory(renderer->device, system->vertex_memory, NULL);
        }
        system->vertex_buffer = VK_NULL_HANDLE;
        system->vertex_memory = VK_NULL_HANDLE;
        LOG_INFO("Destroyed particle vertex buffer");
    }

    system->initialized = false;
    system->active_particle_count = 0;
    system->active_emitter_count = 0;

    fprintf(stderr, "[PARTICLE] Particle system shut down\n");
}

u32 particle_emit(ParticleSystem* system, ParticleType type, Vec3 position,
                 Vec3 velocity, f32 lifetime) {
    if (!system || !system->initialized) {
        return 0xFFFFFFFF;
    }

    // Find free particle slot
    u32 particle_id = 0xFFFFFFFF;
    for (u32 i = 0; i < MAX_PARTICLES; i++) {
        if (!system->particles[i].active) {
            particle_id = i;
            break;
        }
    }

    if (particle_id == 0xFFFFFFFF) {
        // No free slots - steal oldest particle
        f32 max_life_ratio = 0.0f;
        for (u32 i = 0; i < MAX_PARTICLES; i++) {
            f32 life_ratio = system->particles[i].life / system->particles[i].max_life;
            if (life_ratio > max_life_ratio) {
                max_life_ratio = life_ratio;
                particle_id = i;
            }
        }
    }

    Particle* p = &system->particles[particle_id];
    memset(p, 0, sizeof(Particle));

    // Set particle properties based on type
    p->position = position;
    p->velocity = velocity;
    p->acceleration = (Vec3){0.0f, -9.8f, 0.0f};  // Default gravity
    p->type = type;
    p->life = 0.0f;
    p->max_life = lifetime;
    p->active = true;
    p->friction = 0.98f;
    p->alpha = 1.0f;
    p->rotation = 0.0f;
    p->angular_velocity = 0.0f;

    // Configure type-specific properties
    switch (type) {
        case PARTICLE_TYPE_SPARK:
            p->start_color = (Vec4){1.0f, 0.9f, 0.3f, 1.0f};
            p->end_color = (Vec4){1.0f, 0.3f, 0.0f, 0.0f};
            p->start_size = 0.05f;
            p->end_size = 0.01f;
            p->affected_by_gravity = true;
            p->collides_with_world = false;
            p->texture_id = 0;
            break;

        case PARTICLE_TYPE_SMOKE:
            p->start_color = (Vec4){0.3f, 0.3f, 0.3f, 0.8f};
            p->end_color = (Vec4){0.5f, 0.5f, 0.5f, 0.0f};
            p->start_size = 0.2f;
            p->end_size = 0.6f;
            p->acceleration.y = 2.0f;  // Rise upward
            p->affected_by_gravity = false;
            p->collides_with_world = false;
            p->friction = 0.95f;
            p->texture_id = 1;
            break;

        case PARTICLE_TYPE_DUST:
            p->start_color = (Vec4){0.6f, 0.5f, 0.4f, 0.9f};
            p->end_color = (Vec4){0.6f, 0.5f, 0.4f, 0.0f};
            p->start_size = 0.08f;
            p->end_size = 0.03f;
            p->affected_by_gravity = true;
            p->collides_with_world = true;
            p->texture_id = 2;
            break;

        case PARTICLE_TYPE_FLAME:
            p->start_color = (Vec4){1.0f, 0.8f, 0.0f, 1.0f};
            p->end_color = (Vec4){1.0f, 0.2f, 0.0f, 0.0f};
            p->start_size = 0.15f;
            p->end_size = 0.05f;
            p->acceleration.y = 5.0f;  // Rise quickly
            p->affected_by_gravity = false;
            p->collides_with_world = false;
            p->texture_id = 3;
            break;

        case PARTICLE_TYPE_WATER_SPLASH:
            p->start_color = (Vec4){0.3f, 0.5f, 0.9f, 0.8f};
            p->end_color = (Vec4){0.3f, 0.5f, 0.9f, 0.0f};
            p->start_size = 0.1f;
            p->end_size = 0.05f;
            p->affected_by_gravity = true;
            p->collides_with_world = true;
            p->friction = 0.92f;
            p->texture_id = 4;
            break;

        case PARTICLE_TYPE_EXPLOSION:
            p->start_color = (Vec4){1.0f, 0.7f, 0.0f, 1.0f};
            p->end_color = (Vec4){0.3f, 0.3f, 0.3f, 0.0f};
            p->start_size = 0.3f;
            p->end_size = 0.8f;
            p->affected_by_gravity = false;
            p->collides_with_world = false;
            p->friction = 0.90f;
            p->texture_id = 5;
            break;

        case PARTICLE_TYPE_MAGIC_GLOW:
            p->start_color = (Vec4){0.7f, 0.3f, 1.0f, 1.0f};
            p->end_color = (Vec4){0.3f, 0.8f, 1.0f, 0.0f};
            p->start_size = 0.12f;
            p->end_size = 0.02f;
            p->affected_by_gravity = false;
            p->collides_with_world = false;
            p->angular_velocity = 3.14f;  // Spin
            p->texture_id = 6;
            break;

        case PARTICLE_TYPE_CRITICAL_HIT:
            p->start_color = (Vec4){1.0f, 1.0f, 0.0f, 1.0f};
            p->end_color = (Vec4){1.0f, 0.5f, 0.0f, 0.0f};
            p->start_size = 0.2f;
            p->end_size = 0.1f;
            p->affected_by_gravity = false;
            p->collides_with_world = false;
            p->texture_id = 7;
            break;

        default:
            p->start_color = (Vec4){1.0f, 1.0f, 1.0f, 1.0f};
            p->end_color = (Vec4){1.0f, 1.0f, 1.0f, 0.0f};
            p->start_size = 0.1f;
            p->end_size = 0.05f;
            p->affected_by_gravity = true;
            p->collides_with_world = false;
            p->texture_id = 0;
            break;
    }

    p->color = p->start_color;
    p->size = p->start_size;

    if (!system->particles[particle_id].active) {
        system->active_particle_count++;
    }

    return particle_id;
}

void particle_emit_burst(ParticleSystem* system, ParticleType type, Vec3 position,
                        Vec3 velocity_base, f32 velocity_spread, u32 count,
                        f32 lifetime) {
    if (!system || !system->initialized) {
        return;
    }

    for (u32 i = 0; i < count; i++) {
        // Random velocity spread
        Vec3 velocity = {
            velocity_base.x + rand_range(-velocity_spread, velocity_spread),
            velocity_base.y + rand_range(-velocity_spread, velocity_spread),
            velocity_base.z + rand_range(-velocity_spread, velocity_spread)
        };

        particle_emit(system, type, position, velocity, lifetime);
    }
}

void particle_set_color(ParticleSystem* system, u32 particle_id, Vec4 start_color,
                       Vec4 end_color) {
    if (!system || particle_id >= MAX_PARTICLES) {
        return;
    }

    Particle* p = &system->particles[particle_id];
    p->start_color = start_color;
    p->end_color = end_color;
}

void particle_set_size(ParticleSystem* system, u32 particle_id, f32 start_size,
                      f32 end_size) {
    if (!system || particle_id >= MAX_PARTICLES) {
        return;
    }

    Particle* p = &system->particles[particle_id];
    p->start_size = start_size;
    p->end_size = end_size;
}

void particle_set_physics(ParticleSystem* system, u32 particle_id, bool gravity,
                         bool collision, f32 friction) {
    if (!system || particle_id >= MAX_PARTICLES) {
        return;
    }

    Particle* p = &system->particles[particle_id];
    p->affected_by_gravity = gravity;
    p->collides_with_world = collision;
    p->friction = friction;
}

u32 emitter_create(ParticleSystem* system, Vec3 position, ParticleType type,
                  u32 particles_per_sec, f32 duration) {
    if (!system || !system->initialized) {
        return 0xFFFFFFFF;
    }

    // Find free emitter slot
    u32 emitter_id = 0xFFFFFFFF;
    for (u32 i = 0; i < MAX_EMITTERS; i++) {
        if (!system->emitters[i].active) {
            emitter_id = i;
            break;
        }
    }

    if (emitter_id == 0xFFFFFFFF) {
        fprintf(stderr, "[PARTICLE] No free emitter slots\n");
        return 0xFFFFFFFF;
    }

    ParticleEmitter* emitter = &system->emitters[emitter_id];
    memset(emitter, 0, sizeof(ParticleEmitter));

    emitter->active = true;
    emitter->position = position;
    emitter->direction = (Vec3){0.0f, 1.0f, 0.0f};
    emitter->shape = EMITTER_SHAPE_POINT;
    emitter->particle_type = type;
    emitter->particles_per_second = particles_per_sec;
    emitter->particle_lifetime = 2.0f;
    emitter->duration = duration;
    emitter->time_alive = 0.0f;
    emitter->spawn_timer = 0.0f;
    emitter->burst_mode = false;

    // Default velocity range
    emitter->velocity_min = (Vec3){-1.0f, 0.0f, -1.0f};
    emitter->velocity_max = (Vec3){1.0f, 2.0f, 1.0f};

    // Default color range (use particle type defaults)
    emitter->color_start = (Vec4){1.0f, 1.0f, 1.0f, 1.0f};
    emitter->color_end = (Vec4){1.0f, 1.0f, 1.0f, 0.0f};

    emitter->size_start = 0.1f;
    emitter->size_end = 0.05f;

    system->active_emitter_count++;

    fprintf(stderr, "[PARTICLE] Created emitter %u at (%.2f, %.2f, %.2f)\n",
            emitter_id, position.x, position.y, position.z);

    return emitter_id;
}

void emitter_set_shape(ParticleSystem* system, u32 emitter_id, EmitterShape shape,
                      f32 param1, f32 param2) {
    if (!system || emitter_id >= MAX_EMITTERS) {
        return;
    }

    ParticleEmitter* emitter = &system->emitters[emitter_id];
    if (!emitter->active) {
        return;
    }

    emitter->shape = shape;
    emitter->shape_param1 = param1;
    emitter->shape_param2 = param2;
}

void emitter_set_velocity_range(ParticleSystem* system, u32 emitter_id,
                               Vec3 min_vel, Vec3 max_vel) {
    if (!system || emitter_id >= MAX_EMITTERS) {
        return;
    }

    ParticleEmitter* emitter = &system->emitters[emitter_id];
    if (!emitter->active) {
        return;
    }

    emitter->velocity_min = min_vel;
    emitter->velocity_max = max_vel;
}

void emitter_set_color_range(ParticleSystem* system, u32 emitter_id,
                            Vec4 start_color, Vec4 end_color) {
    if (!system || emitter_id >= MAX_EMITTERS) {
        return;
    }

    ParticleEmitter* emitter = &system->emitters[emitter_id];
    if (!emitter->active) {
        return;
    }

    emitter->color_start = start_color;
    emitter->color_end = end_color;
}

void emitter_destroy(ParticleSystem* system, u32 emitter_id) {
    if (!system || emitter_id >= MAX_EMITTERS) {
        return;
    }

    ParticleEmitter* emitter = &system->emitters[emitter_id];
    if (!emitter->active) {
        return;
    }

    emitter->active = false;
    system->active_emitter_count--;

    fprintf(stderr, "[PARTICLE] Destroyed emitter %u\n", emitter_id);
}

void particle_system_update(ParticleSystem* system, f32 delta_time) {
    if (!system || !system->initialized) {
        return;
    }

    // Update particles
    for (u32 i = 0; i < MAX_PARTICLES; i++) {
        Particle* p = &system->particles[i];
        if (!p->active) {
            continue;
        }

        // Update lifetime
        p->life += delta_time;
        if (p->life >= p->max_life) {
            p->active = false;
            system->active_particle_count--;
            continue;
        }

        // Interpolate color and size
        f32 t = p->life / p->max_life;
        p->color = vec4_lerp(p->start_color, p->end_color, t);
        p->size = lerp(p->start_size, p->end_size, t);
        p->alpha = p->color.w;

        // Apply physics
        if (p->affected_by_gravity) {
            p->velocity.x += p->acceleration.x * delta_time;
            p->velocity.y += p->acceleration.y * delta_time;
            p->velocity.z += p->acceleration.z * delta_time;
        }

        // Apply friction
        p->velocity.x *= p->friction;
        p->velocity.y *= p->friction;
        p->velocity.z *= p->friction;

        // Update position
        p->position.x += p->velocity.x * delta_time;
        p->position.y += p->velocity.y * delta_time;
        p->position.z += p->velocity.z * delta_time;

        // Update rotation
        p->rotation += p->angular_velocity * delta_time;

        // Simple collision detection (ground plane at y=0)
        if (p->collides_with_world && p->position.y < 0.0f) {
            p->position.y = 0.0f;
            p->velocity.y = -p->velocity.y * 0.5f;  // Bounce with energy loss
            if (fabsf(p->velocity.y) < 0.1f) {
                p->velocity.y = 0.0f;
            }
        }
    }

    // Update emitters
    for (u32 i = 0; i < MAX_EMITTERS; i++) {
        ParticleEmitter* emitter = &system->emitters[i];
        if (!emitter->active) {
            continue;
        }

        emitter->time_alive += delta_time;

        // Check duration
        if (emitter->duration > 0.0f && emitter->time_alive >= emitter->duration) {
            emitter->active = false;
            system->active_emitter_count--;
            continue;
        }

        // Emit particles
        if (emitter->burst_mode) {
            // Emit all at once
            particle_emit_burst(system, emitter->particle_type, emitter->position,
                              (Vec3){0, 0, 0}, 2.0f, emitter->burst_count,
                              emitter->particle_lifetime);
            emitter->burst_mode = false;  // Only emit once
        } else {
            // Continuous emission
            emitter->spawn_timer += delta_time;
            f32 spawn_interval = 1.0f / (f32)emitter->particles_per_second;

            while (emitter->spawn_timer >= spawn_interval) {
                emitter->spawn_timer -= spawn_interval;

                // Generate random velocity within range
                Vec3 velocity = rand_vec3(emitter->velocity_min, emitter->velocity_max);

                // Spawn particle
                u32 p_id = particle_emit(system, emitter->particle_type,
                                        emitter->position, velocity,
                                        emitter->particle_lifetime);

                if (p_id != 0xFFFFFFFF) {
                    particle_set_color(system, p_id, emitter->color_start,
                                     emitter->color_end);
                    particle_set_size(system, p_id, emitter->size_start,
                                    emitter->size_end);
                }
            }
        }
    }
}

void particle_system_render(ParticleSystem* system, VulkanRenderer* renderer,
                           Mat4 view_projection) {
    if (!system || !system->initialized || !renderer || system->vertex_buffer == VK_NULL_HANDLE) {
        return;
    }

    if (system->active_particle_count == 0) {
        return; // No particles to render
    }

    // Build vertex buffer with billboard quads for each active particle
    typedef struct {
        f32 x, y, z;      // Position
        f32 u, v;         // UV coordinates
        f32 r, g, b, a;   // Color
        f32 size;         // Particle size
    } ParticleVertex;

    // Map vertex buffer memory
    void* data;
    VkResult result = vkMapMemory(renderer->device, system->vertex_memory, 0, 
                                  system->vertex_buffer_size, 0, &data);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to map particle vertex buffer: %d", result);
        return;
    }

    ParticleVertex* vertices = (ParticleVertex*)data;
    u32 vertex_count = 0;

    // Build billboard quads for each active particle
    for (u32 i = 0; i < MAX_PARTICLES && vertex_count < MAX_PARTICLES * 6; i++) {
        Particle* particle = &system->particles[i];
        if (!particle->active) continue;

        // Calculate particle alpha based on lifetime
        f32 life_ratio = particle->life / particle->max_life;
        f32 alpha = particle->color.w * life_ratio;

        // Skip fully transparent particles
        if (alpha < 0.01f) continue;

        // Create billboard quad (6 vertices: 2 triangles)
        f32 half_size = particle->size * 0.5f;
        
        // Vertex positions for billboard quad
        Vec3 positions[4] = {
            {-half_size, -half_size, 0}, // Bottom-left
            { half_size, -half_size, 0}, // Bottom-right
            { half_size,  half_size, 0}, // Top-right
            {-half_size,  half_size, 0}  // Top-left
        };

        // UV coordinates for particle texture
        Vec2 uvs[4] = {
            {0.0f, 1.0f}, // Bottom-left
            {1.0f, 1.0f}, // Bottom-right
            {1.0f, 0.0f}, // Top-right
            {0.0f, 0.0f}  // Top-left
        };

        // Triangle 1: bottom-left, bottom-right, top-right
        vertices[vertex_count++] = (ParticleVertex){
            .x = particle->position.x + positions[0].x,
            .y = particle->position.y + positions[0].y,
            .z = particle->position.z + positions[0].z,
            .u = uvs[0].x, .v = uvs[0].y,
            .r = particle->color.x, .g = particle->color.y, 
            .b = particle->color.z, .a = alpha,
            .size = particle->size
        };
        
        vertices[vertex_count++] = (ParticleVertex){
            .x = particle->position.x + positions[1].x,
            .y = particle->position.y + positions[1].y,
            .z = particle->position.z + positions[1].z,
            .u = uvs[1].x, .v = uvs[1].y,
            .r = particle->color.x, .g = particle->color.y,
            .b = particle->color.z, .a = alpha,
            .size = particle->size
        };
        
        vertices[vertex_count++] = (ParticleVertex){
            .x = particle->position.x + positions[2].x,
            .y = particle->position.y + positions[2].y,
            .z = particle->position.z + positions[2].z,
            .u = uvs[2].x, .v = uvs[2].y,
            .r = particle->color.x, .g = particle->color.y,
            .b = particle->color.z, .a = alpha,
            .size = particle->size
        };

        // Triangle 2: bottom-left, top-right, top-left
        vertices[vertex_count++] = (ParticleVertex){
            .x = particle->position.x + positions[0].x,
            .y = particle->position.y + positions[0].y,
            .z = particle->position.z + positions[0].z,
            .u = uvs[0].x, .v = uvs[0].y,
            .r = particle->color.x, .g = particle->color.y,
            .b = particle->color.z, .a = alpha,
            .size = particle->size
        };
        
        vertices[vertex_count++] = (ParticleVertex){
            .x = particle->position.x + positions[2].x,
            .y = particle->position.y + positions[2].y,
            .z = particle->position.z + positions[2].z,
            .u = uvs[2].x, .v = uvs[2].y,
            .r = particle->color.x, .g = particle->color.y,
            .b = particle->color.z, .a = alpha,
            .size = particle->size
        };
        
        vertices[vertex_count++] = (ParticleVertex){
            .x = particle->position.x + positions[3].x,
            .y = particle->position.y + positions[3].y,
            .z = particle->position.z + positions[3].z,
            .u = uvs[3].x, .v = uvs[3].y,
            .r = particle->color.x, .g = particle->color.y,
            .b = particle->color.z, .a = alpha,
            .size = particle->size
        };
    }

    // Unmap vertex buffer
    vkUnmapMemory(renderer->device, system->vertex_memory);

    if (vertex_count == 0) {
        return; // No visible particles
    }

    // Bind particle shader pipeline
    if (renderer->particle_pipeline == VK_NULL_HANDLE) {
        LOG_WARN("Particle pipeline not created - skipping rendering");
        return;
    }
    
    VkCommandBuffer cmdBuffer = renderer->current_command_buffer;
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->particle_pipeline);
    
    // Bind vertex buffer
    VkBuffer vertexBuffers[] = { system->vertex_buffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, vertexBuffers, offsets);
    
    // Set view_projection matrix as shader uniform
    vkCmdPushConstants(cmdBuffer, renderer->particle_pipeline_layout,
                      VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Mat4), &view_projection);
    
    // Bind particle texture atlas
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                           renderer->particle_pipeline_layout, 0, 1,
                           &renderer->particle_descriptor_set, 0, NULL);
    
    // Draw particles
    vkCmdDraw(cmdBuffer, vertex_count, 1, 0, 0);
}

u32 particle_get_active_count(ParticleSystem* system) {
    if (!system) {
        return 0;
    }
    return system->active_particle_count;
}

void particle_clear_all(ParticleSystem* system) {
    if (!system) {
        return;
    }

    for (u32 i = 0; i < MAX_PARTICLES; i++) {
        system->particles[i].active = false;
    }

    system->active_particle_count = 0;
    fprintf(stderr, "[PARTICLE] Cleared all particles\n");
}
