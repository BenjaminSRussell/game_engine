# Particle System API Documentation

## Overview

The particle system provides high-performance, GPU-accelerated particle effects for games, including fire, smoke, explosions, magic effects, and environmental phenomena. The system supports both CPU and GPU simulation with advanced features like collision, forces, and custom behaviors.

## Core Particle System API

### System Management

```c
// Initialize particle system
void particle_system_init(void);

// Shutdown particle system
void particle_system_shutdown(void);

// Update all particle systems
void particle_system_update(float dt);

// Render all particle systems
void particle_system_render(const float* view_matrix, const float* projection_matrix);

// Set global particle settings
void particle_system_set_global_settings(const ParticleGlobalSettings* settings);
```

### Particle Emitter Creation and Management

```c
// Create a particle emitter
uint32_t particle_create_emitter(const ParticleEmitterConfig* config);

// Destroy a particle emitter
void particle_destroy_emitter(uint32_t emitter_id);

// Get emitter configuration
void particle_get_emitter_config(uint32_t emitter_id, ParticleEmitterConfig* config);

// Update emitter configuration
void particle_update_emitter_config(uint32_t emitter_id, const ParticleEmitterConfig* config);

// Set emitter position
void particle_set_emitter_position(uint32_t emitter_id, const float* position);

// Set emitter rotation
void particle_set_emitter_rotation(uint32_t emitter_id, const float* rotation);

// Set emitter scale
void particle_set_emitter_scale(uint32_t emitter_id, const float* scale);

// Enable/disable emitter
void particle_set_emitter_enabled(uint32_t emitter_id, bool enabled);

// Reset emitter
void particle_reset_emitter(uint32_t emitter_id);

// Get particle count
uint32_t particle_get_emitter_count(uint32_t emitter_id);

// Get emitter bounds
void particle_get_emitter_bounds(uint32_t emitter_id, float* min_bounds, float* max_bounds);
```

### Particle Control

```c
// Emit particles immediately
void particle_emit_burst(uint32_t emitter_id, uint32_t count);

// Set emission rate
void particle_set_emission_rate(uint32_t emitter_id, float particles_per_second);

// Stop emission
void particle_stop_emission(uint32_t emitter_id);

// Resume emission
void particle_resume_emission(uint32_t emitter_id);

// Clear all particles
void particle_clear_emitter(uint32_t emitter_id);

// Apply force to particles
void particle_apply_force(uint32_t emitter_id, const float* force);

// Apply impulse to particles
void particle_apply_impulse(uint32_t emitter_id, const float* impulse, const float* position);
```

## Data Structures

### Emitter Configuration

```c
typedef struct {
    // Emitter properties
    float position[3];
    float rotation[3];
    float scale[3];
    EmitterShape shape;
    
    // Emission properties
    float emission_rate;
    uint32_t max_particles;
    float particle_lifetime;
    float lifetime_variation;
    
    // Particle properties
    float start_size[2];
    float end_size[2];
    float start_color[4];
    float end_color[4];
    float start_velocity[3];
    float velocity_variation[3];
    
    // Physics properties
    float mass;
    float drag;
    float gravity_scale;
    bool collision_enabled;
    
    // Rendering properties
    BlendMode blend_mode;
    TextureID texture;
    bool soft_particles;
    bool face_camera;
    
    // Advanced properties
    NoiseType noise_type;
    float noise_strength;
    float noise_frequency;
    bool turbulence_enabled;
} ParticleEmitterConfig;
```

### Emitter Shapes

```c
typedef enum {
    EMITTER_SHAPE_POINT,
    EMITTER_SHAPE_SPHERE,
    EMITTER_SHAPE_BOX,
    EMITTER_SHAPE_CYLINDER,
    EMITTER_SHAPE_CONE,
    EMITTER_SHAPE_RING,
    EMITTER_SHAPE_MESH,
    EMITTER_SHAPE_LINE
} EmitterShape;

typedef struct {
    float radius;
} SphereEmitterShape;

typedef struct {
    float width, height, depth;
} BoxEmitterShape;

typedef struct {
    float radius, height;
} CylinderEmitterShape;

typedef struct {
    float radius, angle;
} ConeEmitterShape;
```

### Particle Data

```c
typedef struct {
    float position[3];
    float velocity[3];
    float size;
    float color[4];
    float lifetime;
    float age;
    float rotation;
    float rotation_speed;
    uint32_t texture_index;
    bool alive;
} Particle;
```

### Blend Modes

```c
typedef enum {
    BLEND_MODE_ALPHA,
    BLEND_MODE_ADDITIVE,
    BLEND_MODE_MULTIPLICATIVE,
    BLEND_MODE_SCREEN,
    BLEND_MODE_SUBTRACTIVE
} BlendMode;
```

## Advanced Features

### GPU Particle Simulation

```c
// Enable GPU simulation
void particle_enable_gpu_simulation(uint32_t emitter_id, bool enabled);

// Set GPU simulation parameters
void particle_set_gpu_simulation_params(uint32_t emitter_id, const GPUSimulationParams* params);

typedef struct {
    uint32_t compute_shader;
    uint32_t particle_buffer;
    uint32_t counter_buffer;
    bool use_compute_shaders;
    uint32_t threads_per_group;
} GPUSimulationParams;
```

### Collision Detection

```c
// Enable particle collision
void particle_enable_collision(uint32_t emitter_id, bool enabled);

// Set collision mask
void particle_set_collision_mask(uint32_t emitter_id, uint32_t collision_mask);

// Set collision response
void particle_set_collision_response(uint32_t emitter_id, CollisionResponse response);

typedef enum {
    COLLISION_RESPONSE_NONE,
    COLLISION_RESPONSE_BOUNCE,
    COLLISION_RESPONSE_STICK,
    COLLISION_RESPONSE_SLIDE
} CollisionResponse;
```

### Forces and Fields

```c
// Create force field
uint32_t particle_create_force_field(const ForceFieldConfig* config);

// Destroy force field
void particle_destroy_force_field(uint32_t field_id);

// Update force field
void particle_update_force_field(uint32_t field_id, const ForceFieldConfig* config);

typedef struct {
    ForceFieldType type;
    float position[3];
    float strength;
    float radius;
    float direction[3];
    bool affected_by_mass;
} ForceFieldConfig;

typedef enum {
    FORCE_FIELD_POINT,
    FORCE_FIELD_DIRECTIONAL,
    FORCE_FIELD_VORTEX,
    FORCE_FIELD_TURBULENCE,
    FORCE_FIELD_MAGNETIC
} ForceFieldType;
```

### Noise and Turbulence

```c
// Set noise parameters
void particle_set_noise_params(uint32_t emitter_id, const NoiseParams* params);

typedef struct {
    NoiseType type;
    float frequency;
    float amplitude;
    float octaves;
    float persistence;
    float lacunarity;
    float speed;
} NoiseParams;

typedef enum {
    NOISE_TYPE_PERLIN,
    NOISE_TYPE_SIMPLEX,
    NOISE_TYPE_WORLEY,
    NOISE_TYPE_FRACTAL,
    NOISE_TYPE_TURBULENCE
} NoiseType;
```

## Performance Optimization

### Level of Detail (LOD)

```c
// Set LOD settings
void particle_set_lod_settings(uint32_t emitter_id, const LODSettings* settings);

typedef struct {
    float lod_distances[4];
    float emission_rates[4];
    float particle_sizes[4];
    bool enable_lod;
} LODSettings;
```

### Culling

```c
// Enable frustum culling
void particle_enable_frustum_culling(uint32_t emitter_id, bool enabled);

// Enable distance culling
void particle_enable_distance_culling(uint32_t emitter_id, bool enabled, float max_distance);

// Set culling bounds
void particle_set_culling_bounds(uint32_t emitter_id, const float* min_bounds, const float* max_bounds);
```

### Memory Management

```c
// Get memory usage
size_t particle_get_memory_usage(uint32_t emitter_id);

// Set memory budget
void particle_set_memory_budget(size_t budget_bytes);

// Get performance statistics
void particle_get_performance_stats(uint32_t emitter_id, ParticlePerformanceStats* stats);

typedef struct {
    uint32_t active_particles;
    uint32_t emitted_this_frame;
    float simulation_time;
    float rendering_time;
    size_t memory_usage;
    float gpu_utilization;
} ParticlePerformanceStats;
```

## Integration Examples

### Basic Fire Effect

```c
// Create fire emitter
ParticleEmitterConfig fire_config = {
    .position = {0.0f, 0.0f, 0.0f},
    .shape = EMITTER_SHAPE_POINT,
    .emission_rate = 100.0f,
    .max_particles = 500,
    .particle_lifetime = 2.0f,
    .lifetime_variation = 0.5f,
    .start_size = {0.1f, 0.2f},
    .end_size = {0.3f, 0.5f},
    .start_color = {1.0f, 0.8f, 0.2f, 1.0f},
    .end_color = {0.2f, 0.0f, 0.0f, 0.0f},
    .start_velocity = {0.0f, 2.0f, 0.0f},
    .velocity_variation = {0.5f, 0.5f, 0.5f},
    .mass = 0.1f,
    .drag = 0.1f,
    .gravity_scale = -0.5f,
    .blend_mode = BLEND_MODE_ADDITIVE,
    .texture = fire_texture,
    .face_camera = true
};

uint32_t fire_emitter = particle_create_emitter(&fire_config);
```

### Smoke Effect with Turbulence

```c
// Create smoke emitter
ParticleEmitterConfig smoke_config = {
    .position = {0.0f, 0.0f, 0.0f},
    .shape = EMITTER_SHAPE_SPHERE,
    .emission_rate = 50.0f,
    .max_particles = 200,
    .particle_lifetime = 5.0f,
    .lifetime_variation = 1.0f,
    .start_size = {0.5f, 0.8f},
    .end_size = {2.0f, 3.0f},
    .start_color = {0.3f, 0.3f, 0.3f, 0.8f},
    .end_color = {0.1f, 0.1f, 0.1f, 0.0f},
    .start_velocity = {0.0f, 1.0f, 0.0f},
    .velocity_variation = {0.3f, 0.2f, 0.3f},
    .mass = 0.05f,
    .drag = 0.2f,
    .gravity_scale = 0.1f,
    .blend_mode = BLEND_MODE_ALPHA,
    .texture = smoke_texture,
    .face_camera = true,
    .noise_type = NOISE_TYPE_TURBULENCE,
    .noise_strength = 0.5f,
    .noise_frequency = 0.1f,
    .turbulence_enabled = true
};

uint32_t smoke_emitter = particle_create_emitter(&smoke_config);

// Add turbulence force field
ForceFieldConfig turbulence = {
    .type = FORCE_FIELD_TURBULENCE,
    .position = {0.0f, 2.0f, 0.0f},
    .strength = 1.0f,
    .radius = 5.0f,
    .affected_by_mass = false
};

uint32_t turbulence_field = particle_create_force_field(&turbulence);
```

### Explosion Effect

```c
// Create explosion burst
ParticleEmitterConfig explosion_config = {
    .position = {0.0f, 0.0f, 0.0f},
    .shape = EMITTER_SHAPE_SPHERE,
    .emission_rate = 0.0f, // Manual burst
    .max_particles = 1000,
    .particle_lifetime = 1.5f,
    .lifetime_variation = 0.3f,
    .start_size = {0.2f, 0.4f},
    .end_size = {0.1f, 0.2f},
    .start_color = {1.0f, 1.0f, 0.2f, 1.0f},
    .end_color = {1.0f, 0.2f, 0.0f, 0.0f},
    .start_velocity = {0.0f, 0.0f, 0.0f},
    .velocity_variation = {10.0f, 10.0f, 10.0f},
    .mass = 0.01f,
    .drag = 0.05f,
    .gravity_scale = 0.0f,
    .blend_mode = BLEND_MODE_ADDITIVE,
    .texture = explosion_texture,
    .face_camera = true
};

uint32_t explosion_emitter = particle_create_emitter(&explosion_config);

// Trigger explosion
particle_emit_burst(explosion_emitter, 1000);
```

## Debugging and Tools

### Debug Visualization

```c
// Enable debug rendering
void particle_enable_debug_render(uint32_t emitter_id, bool enabled);

// Set debug mode
void particle_set_debug_mode(uint32_t emitter_id, ParticleDebugMode mode);

typedef enum {
    DEBUG_MODE_NONE,
    DEBUG_MODE_VELOCITY,
    DEBUG_MODE_LIFETIME,
    DEBUG_MODE_SIZE,
    DEBUG_MODE_COLLISION,
    DEBUG_MODE_BOUNDS
} ParticleDebugMode;
```

### Performance Monitoring

```c
// Enable performance profiling
void particle_enable_profiling(uint32_t emitter_id, bool enabled);

// Get profiling data
void particle_get_profiling_data(uint32_t emitter_id, ParticleProfilingData* data);

typedef struct {
    float emit_time;
    float update_time;
    float render_time;
    float collision_time;
    float force_time;
    uint32_t cull_count;
    uint32_t render_count;
} ParticleProfilingData;
```

## Best Practices

1. **Pool Management**: Use particle pools to avoid frequent allocations
2. **LOD Optimization**: Implement distance-based quality reduction
3. **Culling**: Enable frustum and distance culling for large scenes
4. **GPU Simulation**: Use GPU simulation for large particle counts
5. **Texture Atlases**: Use texture atlases to reduce texture switches
6. **Batching**: Group similar particles for efficient rendering
7. **Memory Budgeting**: Set appropriate memory limits for different platforms

## Error Handling

```c
typedef enum {
    PARTICLE_ERROR_NONE = 0,
    PARTICLE_ERROR_INVALID_EMITTER,
    PARTICLE_ERROR_INVALID_CONFIG,
    PARTICLE_ERROR_OUT_OF_MEMORY,
    PARTICLE_ERROR_INVALID_TEXTURE,
    PARTICLE_ERROR_GPU_ERROR
} ParticleError;

// Get last error
ParticleError particle_get_last_error(void);

// Set error callback
void particle_set_error_callback(ParticleErrorCallback callback);
```

This documentation provides comprehensive coverage of the particle system API, including advanced features, optimization techniques, and practical examples for creating various visual effects.
