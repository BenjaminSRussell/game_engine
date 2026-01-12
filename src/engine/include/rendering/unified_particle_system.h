#ifndef UNIFIED_PARTICLE_SYSTEM_H
#define UNIFIED_PARTICLE_SYSTEM_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __APPLE__
#include <Metal/Metal.h>
#endif

// ============================================================================
// Unified Particle System - Consolidates all particle rendering functionality
// ============================================================================

// Forward declarations
typedef struct UnifiedParticleSystem UnifiedParticleSystem;
typedef struct ParticleRenderer ParticleRenderer;

// Particle types
typedef enum {
    PARTICLE_TYPE_POLL = 0,
    PARTICLE_TYPE_SPORE,
    PARTICLE_TYPE_SEED,
    PARTICLE_TYPE_PETAL,
    PARTICLE_TYPE_RAIN,
    PARTICLE_TYPE_SNOW,
    PARTICLE_TYPE_DUST,
    PARTICLE_TYPE_BLOOM,
    PARTICLE_TYPE_GENERIC,
    PARTICLE_TYPE_SVG,
    PARTICLE_TYPE_COUNT
} ParticleType;

// Particle vertex structure
typedef struct {
    float x, y, z;        // Position
    float u, v;          // Texture coordinates
    float size;          // Particle size
    float rotation;      // Rotation angle
    uint8_t r, g, b, a;  // Color
    uint8_t type;        // Particle type
    float life;          // Life remaining (0-1)
} ParticleVertex;

// Particle system configuration
typedef struct {
    bool enable_instancing;
    bool enable_sorting;
    bool enable_soft_particles;
    bool enable_depth_fade;
    bool enable_gpu_simulation;
    bool enable_svg_support;
    uint32_t max_particles_per_batch;
    float particle_size_scale;
    float brightness_scale;
    bool enable_glow;
    float glow_intensity;
} ParticleSystemConfig;

// Particle emitter properties
typedef struct {
    ParticleType type;
    float position[3];
    float velocity[3];
    float acceleration[3];
    float emission_rate;
    float particle_lifetime;
    float size_min, size_max;
    uint8_t color_min[4], color_max[4];
    bool enabled;
    uint32_t max_particles;
} ParticleEmitter;

// Render statistics
typedef struct {
    uint32_t active_particles;
    uint32_t draw_calls;
    uint32_t gpu_updates;
    float frame_time_ms;
    float simulation_time_ms;
    float rendering_time_ms;
} ParticleRenderStats;

// ============================================================================
// Core System Functions
// ============================================================================

// System lifecycle
UnifiedParticleSystem* unified_particle_system_create(const ParticleSystemConfig* config);
void unified_particle_system_destroy(UnifiedParticleSystem* system);

// Configuration
void unified_particle_system_set_config(UnifiedParticleSystem* system, 
                                       const ParticleSystemConfig* config);
void unified_particle_system_get_config(const UnifiedParticleSystem* system, 
                                       ParticleSystemConfig* out_config);

// ============================================================================
// Emitter Management
// ============================================================================

// Emitter lifecycle
uint32_t unified_particle_system_create_emitter(UnifiedParticleSystem* system,
                                               const ParticleEmitter* emitter);
void unified_particle_system_destroy_emitter(UnifiedParticleSystem* system, 
                                            uint32_t emitter_id);
void unified_particle_system_update_emitter(UnifiedParticleSystem* system,
                                           uint32_t emitter_id,
                                           const ParticleEmitter* emitter);

// Emitter control
void unified_particle_system_start_emitter(UnifiedParticleSystem* system, 
                                          uint32_t emitter_id);
void unified_particle_system_stop_emitter(UnifiedParticleSystem* system, 
                                         uint32_t emitter_id);
void unified_particle_system_set_emitter_position(UnifiedParticleSystem* system,
                                                 uint32_t emitter_id,
                                                 const float position[3]);

// ============================================================================
// Simulation
// ============================================================================

// Update simulation
void unified_particle_system_update(UnifiedParticleSystem* system, 
                                  float delta_time,
                                  const float camera_position[3],
                                  const float view_matrix[16]);

// Force fields
void unified_particle_system_add_gravity(UnifiedParticleSystem* system, 
                                        const float gravity[3]);
void unified_particle_system_add_wind(UnifiedParticleSystem* system, 
                                      const float wind[3]);
void unified_particle_system_add_turbulence(UnifiedParticleSystem* system, 
                                           float strength);

// ============================================================================
// Rendering
// ============================================================================

// Rendering interface
void unified_particle_system_render(UnifiedParticleSystem* system,
                                   void* command_encoder,
                                   const float view_projection_matrix[16]);

// Texture management
void unified_particle_system_load_textures(UnifiedParticleSystem* system);
void unified_particle_system_set_svg_texture(UnifiedParticleSystem* system,
                                            uint32_t texture_id);

// ============================================================================
// Statistics and Debugging
// ============================================================================

// Performance monitoring
void unified_particle_system_get_stats(const UnifiedParticleSystem* system,
                                      ParticleRenderStats* out_stats);
void unified_particle_system_reset_stats(UnifiedParticleSystem* system);

// Debug utilities
void unified_particle_system_debug_render_bounds(UnifiedParticleSystem* system);
void unified_particle_system_debug_render_emitters(UnifiedParticleSystem* system);

// ============================================================================
// Platform-specific Interface
// ============================================================================

#ifdef __APPLE__
// Metal-specific functions
void unified_particle_system_metal_update_buffers(UnifiedParticleSystem* system,
                                                 id<MTLCommandBuffer> command_buffer);
void unified_particle_system_metal_render(UnifiedParticleSystem* system,
                                         id<MTLRenderCommandEncoder> encoder);
#endif

#endif // UNIFIED_PARTICLE_SYSTEM_H
