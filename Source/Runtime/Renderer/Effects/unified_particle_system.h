#ifndef UNIFIED_PARTICLE_SYSTEM_H
#define UNIFIED_PARTICLE_SYSTEM_H

/*
 * Unified Particle System
 * Consolidates all particle implementations into a single, comprehensive system
 * Supports CPU, GPU, and hybrid particle rendering with various emitter types
 */

#include "unified_memory.h"
#include "unified_logging.h"
#include "error_handling.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * PARTICLE SYSTEM CONFIGURATION
 * ============================================================================ */

typedef enum {
    PARTICLE_RENDER_CPU = 0,           // CPU-based rendering
    PARTICLE_RENDER_GPU,               // GPU compute shader rendering
    PARTICLE_RENDER_HYBRID,            // Hybrid CPU+GPU rendering
    PARTICLE_RENDER_INSTANCED,        // GPU instanced rendering
    PARTICLE_RENDER_COUNT
} particle_render_mode_t;

typedef enum {
    PARTICLE_BLEND_ALPHA = 0,          // Standard alpha blending
    PARTICLE_BLEND_ADDITIVE,           // Additive blending
    PARTICLE_BLEND_MULTIPLICATIVE,     // Multiplicative blending
    PARTICLE_BLEND_SCREEN,             // Screen blending
    PARTICLE_BLEND_COUNT
} particle_blend_mode_t;

typedef enum {
    PARTICLE_SHAPE_POINT = 0,          // Point particles
    PARTICLE_SHAPE_QUAD,               // Quad particles
    PARTICLE_SHAPE_SPHERE,             // Sphere particles
    PARTICLE_SHAPE_MESH,               // Custom mesh particles
    PARTICLE_SHAPE_LINE,               // Line particles
    PARTICLE_SHAPE_COUNT
} particle_shape_t;

/* ============================================================================
 * PARTICLE DATA STRUCTURES
 * ============================================================================ */

typedef struct {
    float position[3];
    float velocity[3];
    float acceleration[3];
    float size;
    float rotation;
    float rotation_speed;
    float color[4];
    float life;
    float max_life;
    uint32_t texture_id;
    uint32_t mesh_id;
    bool active;
    uint32_t id;
} particle_t;

typedef struct {
    float position[3];
    float direction[3];
    float spread_angle;
    float initial_speed;
    float speed_variation;
    float initial_size;
    float size_variation;
    float initial_life;
    float life_variation;
    float initial_rotation;
    float rotation_variation;
    float rotation_speed;
    float rotation_speed_variation;
    float color[4];
    float color_variation[4];
    float gravity_strength;
    float air_resistance;
    float turbulence_strength;
    uint32_t emission_rate;
    uint32_t max_particles;
    bool burst_mode;
    float burst_duration;
    bool loop;
    uint32_t texture_id;
    uint32_t mesh_id;
    particle_shape_t shape;
    particle_blend_mode_t blend_mode;
} particle_emitter_config_t;

typedef struct {
    particle_emitter_config_t config;
    particle_t* particles;
    uint32_t particle_count;
    uint32_t active_count;
    uint32_t emission_timer;
    uint32_t emission_accumulator;
    float accumulated_time;
    bool enabled;
    uint32_t id;
    void* gpu_data;  // GPU-specific data
} particle_emitter_t;

typedef struct {
    particle_emitter_t* emitters;
    uint32_t emitter_count;
    uint32_t capacity;
    particle_render_mode_t render_mode;
    float global_time_scale;
    bool enabled;
    uint32_t next_particle_id;
    uint32_t next_emitter_id;
    
    // GPU resources
    uint32_t particle_buffer;
    uint32_t compute_shader;
    uint32_t vertex_shader;
    uint32_t fragment_shader;
    uint32_t texture_array;
    
    // Performance tracking
    uint32_t total_particles;
    uint32_t max_particles_reached;
    float average_frame_time;
    uint32_t frame_count;
} particle_system_t;

/* ============================================================================
 * PARTICLE EFFECTS AND BEHAVIORS
 * ============================================================================ */

typedef enum {
    PARTICLE_EFFECT_NONE = 0,
    PARTICLE_EFFECT_GRAVITY = (1 << 0),
    PARTICLE_EFFECT_WIND = (1 << 1),
    PARTICLE_EFFECT_TURBULENCE = (1 << 2),
    PARTICLE_EFFECT_COLLISION = (1 << 3),
    PARTICLE_EFFECT_ATTRACTION = (1 << 4),
    PARTICLE_EFFECT_REPULSION = (1 << 5),
    PARTICLE_EFFECT_VORTEX = (1 << 6),
    PARTICLE_EFFECT_MAGNETIC = (1 << 7),
    PARTICLE_EFFECT_COUNT
} particle_effect_flags_t;

typedef struct {
    particle_effect_flags_t effects;
    float gravity[3];
    float wind[3];
    float turbulence_scale;
    float turbulence_frequency;
    float collision_radius;
    float attraction_strength;
    float attraction_point[3];
    float repulsion_strength;
    float repulsion_point[3];
    float vortex_center[3];
    float vortex_strength;
    float vortex_radius;
    float magnetic_field[3];
} particle_effects_t;

/* ============================================================================
 * WEATHER AND ENVIRONMENTAL PARTICLES
 * ============================================================================ */

typedef enum {
    WEATHER_PARTICLE_NONE = 0,
    WEATHER_PARTICLE_RAIN = (1 << 0),
    WEATHER_PARTICLE_SNOW = (1 << 1),
    WEATHER_PARTICLE_DUST = (1 << 2),
    WEATHER_PARTICLE_SMOKE = (1 << 3),
    WEATHER_PARTICLE_FOG = (1 << 4),
    WEATHER_PARTICLE_LEAVES = (1 << 5),
    WEATHER_PARTICLE_COUNT
} weather_particle_type_t;

typedef struct {
    weather_particle_type_t type;
    float intensity;           // 0.0 to 1.0
    float coverage;           // 0.0 to 1.0
    float wind_strength[3];
    float particle_size;
    float fall_speed;
    float spread;
    float color[4];
    bool animated;
    float animation_speed;
} weather_system_t;

/* ============================================================================
 * UNIFIED PARTICLE SYSTEM API
 * ============================================================================ */

// System lifecycle
particle_system_t* particle_system_create(particle_render_mode_t mode, uint32_t max_emitters);
void particle_system_destroy(particle_system_t* system);
void particle_system_update(particle_system_t* system, float dt);
void particle_system_render(particle_system_t* system, const float view_matrix[16], 
                           const float projection_matrix[16]);

// Configuration
void particle_system_set_render_mode(particle_system_t* system, particle_render_mode_t mode);
void particle_system_set_time_scale(particle_system_t* system, float time_scale);
void particle_system_enable(particle_system_t* system, bool enabled);

// Emitter management
uint32_t particle_system_add_emitter(particle_system_t* system, const particle_emitter_config_t* config);
void particle_system_remove_emitter(particle_system_t* system, uint32_t emitter_id);
particle_emitter_t* particle_system_get_emitter(particle_system_t* system, uint32_t emitter_id);

void particle_system_start_emitter(particle_system_t* system, uint32_t emitter_id);
void particle_system_stop_emitter(particle_system_t* system, uint32_t emitter_id);
void particle_system_restart_emitter(particle_system_t* system, uint32_t emitter_id);

// Particle effects
void particle_system_set_effects(particle_system_t* system, const particle_effects_t* effects);
void particle_system_apply_effect(particle_system_t* system, particle_effect_flags_t effect, 
                                 const void* effect_data);

// Weather system
weather_system_t* weather_system_create(void);
void weather_system_destroy(weather_system_t* weather);
void weather_system_update(weather_system_t* weather, float dt);
void weather_system_render(weather_system_t* weather, const float view_matrix[16],
                          const float projection_matrix[16]);

void weather_system_set_type(weather_system_t* weather, weather_particle_type_t type);
void weather_system_set_intensity(weather_system_t* weather, float intensity);
void weather_system_set_wind(weather_system_t* weather, const float wind[3]);

// Utility functions
uint32_t particle_system_get_total_particles(particle_system_t* system);
uint32_t particle_system_get_active_emitters(particle_system_t* system);
float particle_system_get_performance_metrics(particle_system_t* system);

void particle_system_clear_all(particle_system_t* system);
void particle_system_clear_emitter(particle_system_t* system, uint32_t emitter_id);

/* ============================================================================
 * EMITTER CONFIGURATION API
 * ============================================================================ */

// Configuration builders
void particle_emitter_config_init(particle_emitter_config_t* config);
void particle_emitter_config_set_position(particle_emitter_config_t* config, const float position[3]);
void particle_emitter_config_set_direction(particle_emitter_config_t* config, const float direction[3]);
void particle_emitter_config_set_emission_rate(particle_emitter_config_t* config, uint32_t rate);
void particle_emitter_config_set_max_particles(particle_emitter_config_t* config, uint32_t max_particles);
void particle_emitter_config_set_particle_life(particle_emitter_config_t* config, float life, float variation);
void particle_emitter_config_set_particle_size(particle_emitter_config_t* config, float size, float variation);
void particle_emitter_config_set_particle_speed(particle_emitter_config_t* config, float speed, float variation);
void particle_emitter_config_set_particle_color(particle_emitter_config_t* config, const float color[4]);
void particle_emitter_config_set_particle_color_variation(particle_emitter_config_t* config, const float variation[4]);
void particle_emitter_config_set_gravity(particle_emitter_config_t* config, float strength);
void particle_emitter_config_set_texture(particle_emitter_config_t* config, uint32_t texture_id);
void particle_emitter_config_set_shape(particle_emitter_config_t* config, particle_shape_t shape);
void particle_emitter_config_set_blend_mode(particle_emitter_config_t* config, particle_blend_mode_t blend);

// Preset configurations
void particle_emitter_config_fire(particle_emitter_config_t* config);
void particle_emitter_config_smoke(particle_emitter_config_t* config);
void particle_emitter_config_explosion(particle_emitter_config_t* config);
void particle_emitter_config_fountain(particle_emitter_config_t* config);
void particle_emitter_config_rain(particle_emitter_config_t* config);
void particle_emitter_config_snow(particle_emitter_config_t* config);
void particle_emitter_config_dust(particle_emitter_config_t* config);
void particle_emitter_config_sparkles(particle_emitter_config_t* config);
void particle_emitter_config_magic(particle_emitter_config_t* config);

/* ============================================================================
 * GPU PARTICLE SYSTEM API
 * ============================================================================ */

// GPU-specific functions
bool particle_system_init_gpu_resources(particle_system_t* system);
void particle_system_destroy_gpu_resources(particle_system_t* system);
void particle_system_update_gpu(particle_system_t* system, float dt);
void particle_system_render_gpu(particle_system_t* system, const float view_matrix[16],
                                const float projection_matrix[16]);

// Compute shader operations
void particle_system_upload_to_gpu(particle_system_t* system);
void particle_system_download_from_gpu(particle_system_t* system);
void particle_system_dispatch_compute(particle_system_t* system);

/* ============================================================================
 * PERFORMANCE OPTIMIZATION
 * ============================================================================ */

// Culling and optimization
void particle_system_cull_offscreen(particle_system_t* system, const float view_matrix[16],
                                   const float projection_matrix[16]);
void particle_system_sort_by_depth(particle_system_t* system, const float camera_position[3]);
void particle_system_batch_similar_particles(particle_system_t* system);

// Memory management
void particle_system_optimize_memory(particle_system_t* system);
void particle_system_compact_particles(particle_system_t* system);
void particle_system_preallocate_particles(particle_system_t* system, uint32_t count);

/* ============================================================================
 * DEBUG AND PROFILING
 * ============================================================================ */

#ifdef DEBUG_BUILD
#define PARTICLE_DEBUG_ENABLED 1
#else
#define PARTICLE_DEBUG_ENABLED 0
#endif

#if PARTICLE_DEBUG_ENABLED
void particle_system_debug_render(particle_system_t* system);
void particle_system_debug_print_stats(particle_system_t* system);
void particle_system_debug_validate(particle_system_t* system);
#else
#define particle_system_debug_render(system) ((void)0)
#define particle_system_debug_print_stats(system) ((void)0)
#define particle_system_debug_validate(system) ((void)0)
#endif

// Profiling macros
#define PARTICLE_PROFILE_START(name) \
    uint64_t particle_start_##name = get_time_ns()

#define PARTICLE_PROFILE_END(name) \
    do { \
        uint64_t particle_end_##name = get_time_ns(); \
        LOG_CORE_DEBUG("Particle Profile [%s]: %.3f ms", #name, \
                       (particle_end_##name - particle_start_##name) / 1000000.0f); \
    } while(0)

/* ============================================================================
 * COMPATIBILITY LAYER - Legacy system integration
 * ============================================================================ */

// For existing particle systems
typedef particle_system_t legacy_particle_system_t;
typedef particle_emitter_t legacy_particle_emitter_t;
typedef particle_t legacy_particle_t;

// Legacy function mappings
#define legacy_particle_system_create particle_system_create
#define legacy_particle_system_destroy particle_system_destroy
#define legacy_particle_system_update particle_system_update
#define legacy_particle_system_render particle_system_render

#define legacy_particle_system_add_emitter particle_system_add_emitter
#define legacy_particle_system_remove_emitter particle_system_remove_emitter
#define legacy_particle_system_get_emitter particle_system_get_emitter

// Migration helpers
void particle_system_migrate_legacy(particle_system_t* unified_system, void* legacy_system);
void particle_system_import_legacy_config(particle_emitter_config_t* unified_config, 
                                         const void* legacy_config);

/* ============================================================================
 * CONSTANTS AND LIMITS
 * ============================================================================ */

#define UNIFIED_PARTICLE_MAX_EMITTERS 1024
#define UNIFIED_PARTICLE_MAX_PARTICLES_PER_EMITTER 10000
#define UNIFIED_PARTICLE_MAX_TOTAL_PARTICLES 100000
#define UNIFIED_PARTICLE_MAX_TEXTURES 256
#define UNIFIED_PARTICLE_MAX_MESHES 64

#define UNIFIED_PARTICLE_MIN_SIZE 0.1f
#define UNIFIED_PARTICLE_MAX_SIZE 1000.0f
#define UNIFIED_PARTICLE_MIN_LIFE 0.01f
#define UNIFIED_PARTICLE_MAX_LIFE 100.0f
#define UNIFIED_PARTICLE_MIN_SPEED 0.0f
#define UNIFIED_PARTICLE_MAX_SPEED 1000.0f

/* ============================================================================
 * ERROR CODES
 * ============================================================================ */

typedef enum {
    PARTICLE_ERROR_NONE = 0,
    PARTICLE_ERROR_INVALID_PARAMETER,
    PARTICLE_ERROR_OUT_OF_MEMORY,
    PARTICLE_ERROR_GPU_INIT_FAILED,
    PARTICLE_ERROR_SHADER_COMPILATION_FAILED,
    PARTICLE_ERROR_TEXTURE_LOAD_FAILED,
    PARTICLE_ERROR_MESH_LOAD_FAILED,
    PARTICLE_ERROR_BUFFER_CREATION_FAILED,
    PARTICLE_ERROR_COMPUTE_DISPATCH_FAILED,
    PARTICLE_ERROR_SYSTEM_NOT_INITIALIZED,
    PARTICLE_ERROR_COUNT
} particle_error_t;

const char* particle_error_string(particle_error_t error);

/* ============================================================================
 * MACROS AND CONVENIENCE FUNCTIONS
 * ============================================================================ */

// Common emitter creation macros
#define CREATE_FIRE_EMITTER(system, pos) \
    particle_system_add_emitter_with_preset(system, pos, PARTICLE_PRESET_FIRE)

#define CREATE_SMOKE_EMITTER(system, pos) \
    particle_system_add_emitter_with_preset(system, pos, PARTICLE_PRESET_SMOKE)

#define CREATE_EXPLOSION_EMITTER(system, pos) \
    particle_system_add_emitter_with_preset(system, pos, PARTICLE_PRESET_EXPLOSION)

// Weather macros
#define ENABLE_RAIN(weather, intensity) \
    weather_system_set_type(weather, WEATHER_PARTICLE_RAIN); \
    weather_system_set_intensity(weather, intensity)

#define ENABLE_SNOW(weather, intensity) \
    weather_system_set_type(weather, WEATHER_PARTICLE_SNOW); \
    weather_system_set_intensity(weather, intensity)

// Performance macros
#define PARTICLE_BEGIN_PROFILE(system) PARTICLE_PROFILE_START(particle_update)
#define PARTICLE_END_PROFILE(system) PARTICLE_PROFILE_END(particle_update)

#ifdef __cplusplus
}
#endif

#endif /* UNIFIED_PARTICLE_SYSTEM_H */
