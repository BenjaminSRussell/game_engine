// renderer/systems/particle_system_gpu.h
//
// Purpose: High-performance GPU-driven particle system for massive particle counts.
// Uses compute shaders for simulation and indirect rendering for optimal performance.
//
// Key Features:
// - GPU-side particle simulation using compute shaders
// - SSBO-based particle data storage for GPU access
// - Atomic counters for efficient particle lifecycle management
// - Indirect dispatch and rendering for minimal CPU overhead
// - Support for millions of particles with proper culling
//
// Architecture:
// - Particle data stored in GPU SSBOs (Position, Velocity, Color, Age, etc.)
// - Dead/Alive lists managed with atomic counters
// - Compute shaders handle physics simulation
// - Indirect draw calls for rendering
//
#ifndef PARTICLE_SYSTEM_GPU_H
#define PARTICLE_SYSTEM_GPU_H

#include "../../include/common.h"
#include "../../include/math/vec3.h"
#include "../../include/math/vec4.h"
#include "../../include/render/vulkan.h"

#define GPU_MAX_PARTICLES 1048576  // 1M particles
#define GPU_MAX_EMITTERS 1024
#define GPU_PARTICLE_WORKGROUP_SIZE 256

// GPU Particle data structure (matched to compute shader layout)
typedef struct {
    Vec3 position;
    f32 age;                    // Normalized 0-1 lifetime
    Vec3 velocity;
    u32 emitter_id;             // Which emitter created this particle
    Vec4 color;
    f32 size;
    f32 rotation;
    u32 texture_id;
    u32 padding;                // 16-byte alignment
} GPUParticle; // Used for API interaction; GPU uses SoA layout

// Curve for animating values over time (max 8 keyframes)
typedef struct {
    f32 times[8];      // Normalized time 0-1
    f32 values[8];     // Value at each time
    u32 count;         // Number of keyframes
    u32 padding;
} ParticleCurve;

// Emitter data for GPU
typedef struct {
    Vec3 position;
    f32 emission_rate;          // Particles per second
    Vec3 direction;             // Emission direction
    f32 particle_lifetime;
    f32 duration;               // Emitter duration (-1 = infinite)
    Vec3 velocity_min;
    f32 time_alive;             // Emitter age
    Vec3 velocity_max;
    u32 particles_to_emit;      // Calculated each frame
    Vec4 color_start;
    Vec4 color_end;
    f32 size_start;
    f32 size_end;
    
    // Enhanced emission control (Phase 2)
    f32 lifetime_min;           // Min lifetime (particle_lifetime is max)
    f32 lifetime_randomness;    // 0-1 randomization factor
    bool use_burst_mode;
    u32 burst_count;            // Particles per burst
    f32 burst_interval;         // Time between bursts
    f32 burst_timer;            // Internal burst timing
    
    // Velocity control
    f32 velocity_randomness;    // 0-1 cone spread
    f32 velocity_inheritance;   // Inherit from emitter movement
    
    // Rotation control
    f32 rotation_initial_min;
    f32 rotation_initial_max;
    f32 rotation_speed;
    f32 padding_rotation;
    
    u32 emitter_type;           // Point, sphere, box, etc.
    u32 shape_params[2];        // Radius, height, etc.
    f32 spawn_timer;
    s32 active;                  // -1 = inactive, >=0 = active
    u32 curve_set_id;           // Index to curve set (separate buffer)
    u32 padding_[1];
} GPUEmitter;

// Atomic counter buffer
typedef struct {
    u32 alive_count;            // Number of alive particles
    u32 dead_count;             // Number of dead particles (free slots)
    u32 emit_count;             // Particles to emit this frame
    u32 dispatch_count;         // Compute shader dispatch count
    u32 max_particles;          // Capacity (for validation)
    u32 padding[3];
} GPUAtomicCounters;

// Indirect dispatch arguments
typedef struct {
    u32 workgroup_count_x;
    u32 workgroup_count_y;
    u32 workgroup_count_z;
    u32 padding;
} GPUDispatchIndirect;

// Indirect draw arguments
typedef struct {
    u32 vertex_count;
    u32 instance_count;
    u32 first_vertex;
    u32 first_instance;
} GPUDrawIndirect;

typedef struct {
    // GPU Buffers - Double Buffered for SoA
    // Layout: [Position(Vec4)*N][Velocity(Vec4)*N][Color(Vec4)*N][Attributes(Vec4)*N]
    VkBuffer particle_buffer[2];           // Main particle data SSBOs (Read/Write)
    VkDeviceMemory particle_memory[2];
    u32 current_buffer_index;              // Index of the buffer currently simulating FROM
    
    // Metadata Buffer (Flags, Type, Collision Info)
    VkBuffer particle_metadata_buffer;
    VkDeviceMemory particle_metadata_memory;
    
    VkBuffer emitter_buffer;            // Emitter data SSBO
    VkDeviceMemory emitter_memory;
    
    VkBuffer atomic_counter_buffer;     // Atomic counters
    VkDeviceMemory atomic_counter_memory;
    
    VkBuffer dead_list_buffer;          // Indices of dead particles
    VkDeviceMemory dead_list_memory;
    
    VkBuffer alive_list_buffer;         // Indices of alive particles
    VkDeviceMemory alive_list_memory;
    
    VkBuffer new_particle_buffer;       // New particles to add
    VkDeviceMemory new_particle_memory;
    
    VkBuffer dispatch_buffer;           // Indirect dispatch args
    VkDeviceMemory dispatch_memory;
    
    VkBuffer draw_buffer;               // Indirect draw args
    VkDeviceMemory draw_memory;
    
    // Compute shader resources - Simulation
    VkPipeline simulation_pipeline;
    VkPipelineLayout simulation_layout;
    VkDescriptorSetLayout simulation_descriptor_layout;
    VkDescriptorSet simulation_descriptor_set[2]; // One per buffer set
    
    // Compute shader resources - Emission (Phase 3)
    VkPipeline emission_pipeline;
    VkPipelineLayout emission_layout; // Likely same as simulation, but separate for safety
    VkDescriptorSetLayout emission_descriptor_layout;
    VkDescriptorSet emission_descriptor_set; // Shared or one per buffer? Emission writes to "current" buffer. 
                                            // We can just bind the active buffer. 
                                            // Since we have ping-pong, we might need 2 sets if we toggle destination.
                                            // Actually, emission happens before simulation, so it writes to Input (buffer[current]).
                                            // Simulation reads Input -> Output.
                                            // So emission target swaps.
    VkDescriptorSet emission_descriptor_set_array[2];
    
    // Rendering resources
    VkPipeline render_pipeline;
    VkPipelineLayout render_layout;
    VkDescriptorSetLayout render_descriptor_layout;
    VkDescriptorSet render_descriptor_set[2]; // One per buffer set
    
    // Buffer sizes
    u32 max_particles;
    u32 max_emitters;
    size_t particle_buffer_size;     // Size of ONE double buffer
    size_t emitter_buffer_size;
    size_t atomic_buffer_size;
    size_t list_buffer_size;
    
    // SoA Offsets (bytes)
    size_t offset_position;
    size_t offset_velocity;
    size_t offset_color;
    size_t offset_attributes;   // x=age, y=size, z=rotation, w=padding
    
    // Mapping for CPU access
    GPUEmitter* mapped_emitters;
    GPUAtomicCounters* mapped_counters;
    u32* mapped_dead_list;
    u32* mapped_alive_list;
    
    // State
    bool initialized;
    VkDevice device;
    VkPhysicalDevice physical_device;
    VkCommandPool command_pool;
    VkQueue compute_queue;
    VkQueue graphics_queue;
    
    // Simulation configuration (TASK_650)
    ParticleSimulationConfig simulation_config;
    
    // Force fields (TASK_654)
    ParticleForceField force_fields[64];
    u32 force_field_count;
    
    // Rendering configuration (TASK_660)
    ParticleRenderingConfig rendering_config;
    
    // Life curves (TASK_665)
    ParticleLifeCurves life_curves;
} GPUParticleSystem;

// Lifecycle management
bool gpu_particle_system_init(GPUParticleSystem* system, VkDevice device,
                             VkPhysicalDevice physical_device, VkCommandPool command_pool,
                             VkQueue compute_queue, VkQueue graphics_queue);
void gpu_particle_system_shutdown(GPUParticleSystem* system);

// Buffer management (TASK_630)
bool gpu_particle_create_buffers(GPUParticleSystem* system);
void gpu_particle_destroy_buffers(GPUParticleSystem* system);
bool gpu_particle_map_buffers(GPUParticleSystem* system);
void gpu_particle_unmap_buffers(GPUParticleSystem* system);
bool gpu_particle_resize_buffers(GPUParticleSystem* system, u32 new_max_particles);

// Atomic counter management (TASK_631)
bool gpu_particle_init_atomic_counters(GPUParticleSystem* system);
void gpu_particle_reset_counters(GPUParticleSystem* system);
u32 gpu_particle_get_alive_count(GPUParticleSystem* system);
u32 gpu_particle_get_dead_count(GPUParticleSystem* system);

// Dead/Alive list management (TASK_631)
void gpu_particle_init_dead_list(GPUParticleSystem* system);
u32 gpu_particle_alloc_particle_slot(GPUParticleSystem* system);
void gpu_particle_free_particle_slot(GPUParticleSystem* system, u32 slot);

// New particle buffer (TASK_632)
bool gpu_particle_init_new_particle_buffer(GPUParticleSystem* system);
void gpu_particle_add_new_particles(GPUParticleSystem* system, const GPUParticle* particles, u32 count);

// Indirect dispatch (TASK_633)
void gpu_particle_update_dispatch_args(GPUParticleSystem* system);
void gpu_particle_update_draw_args(GPUParticleSystem* system);

// Compute shader operations
void gpu_particle_dispatch_simulation(GPUParticleSystem* system, VkCommandBuffer cmd_buffer, f32 delta_time);

// Emitter management
u32 gpu_particle_create_emitter(GPUParticleSystem* system, const GPUEmitter* emitter);
void gpu_particle_update_emitter(GPUParticleSystem* system, u32 emitter_id, const GPUEmitter* emitter);
void gpu_particle_destroy_emitter(GPUParticleSystem* system, u32 emitter_id);
void gpu_particle_update_emitters_cpu(GPUParticleSystem* system, f32 delta_time);

// Pipeline and Descriptor management (Phase 3)
bool gpu_particle_create_pipelines(GPUParticleSystem* system);
void gpu_particle_destroy_pipelines(GPUParticleSystem* system);
bool gpu_particle_create_descriptor_sets(GPUParticleSystem* system);
void gpu_particle_update_descriptor_sets(GPUParticleSystem* system);

// Curve evaluation and emission control (Phase 2)
f32 gpu_particle_evaluate_curve(const ParticleCurve* curve, f32 t);
void gpu_particle_enable_burst_mode(GPUParticleSystem* system, u32 emitter_id, bool enabled, u32 count, f32 interval);
void gpu_particle_trigger_burst(GPUParticleSystem* system, u32 emitter_id);
void gpu_particle_set_lifetime_range(GPUParticleSystem* system, u32 emitter_id, f32 min, f32 max);
void gpu_particle_set_rotation_range(GPUParticleSystem* system, u32 emitter_id, f32 initial_min, f32 initial_max, f32 speed);
void gpu_particle_set_velocity_randomness(GPUParticleSystem* system, u32 emitter_id, f32 randomness);
void gpu_particle_set_velocity_inheritance(GPUParticleSystem* system, u32 emitter_id, f32 factor);

// Point emitter system (TASK_640)
typedef struct {
    Vec3 position;
    Vec3 direction;
    f32 spread_angle;        // Radians
    f32 speed_min;
    f32 speed_max;
    Vec4 color_start;
    Vec4 color_end;
    f32 size_start;
    f32 size_end;
    f32 lifetime;
    u32 particle_type;
    u32 texture_id;
} PointEmitterConfig;

u32 gpu_particle_create_point_emitter(GPUParticleSystem* system, const PointEmitterConfig* config);
void gpu_particle_set_point_emitter_rate(GPUParticleSystem* system, u32 emitter_id, f32 particles_per_second);
void gpu_particle_set_point_emitter_direction(GPUParticleSystem* system, u32 emitter_id, Vec3 direction, f32 spread_angle);
void gpu_particle_set_point_emitter_speed(GPUParticleSystem* system, u32 emitter_id, f32 speed_min, f32 speed_max);

// Volume emitter system (TASK_641)
typedef struct {
    Vec3 center;
    Vec3 size;              // Width, height, depth for box
    Vec3 direction;         // Main emission direction
    f32 speed_min;
    f32 speed_max;
    Vec4 color_start;
    Vec4 color_end;
    f32 size_start;
    f32 size_end;
    f32 lifetime;
    u32 particle_type;
    u32 texture_id;
    bool emit_from_surface;  // true=surface only, false=entire volume
} BoxEmitterConfig;

typedef struct {
    Vec3 center;
    f32 radius;
    Vec3 direction;         // Main emission direction
    f32 speed_min;
    f32 speed_max;
    Vec4 color_start;
    Vec4 color_end;
    f32 size_start;
    f32 size_end;
    f32 lifetime;
    u32 particle_type;
    u32 texture_id;
    bool emit_from_surface;  // true=surface only, false=entire volume
} SphereEmitterConfig;

u32 gpu_particle_create_box_emitter(GPUParticleSystem* system, const BoxEmitterConfig* config);
u32 gpu_particle_create_sphere_emitter(GPUParticleSystem* system, const SphereEmitterConfig* config);
void gpu_particle_set_box_emitter_size(GPUParticleSystem* system, u32 emitter_id, Vec3 size);
void gpu_particle_set_sphere_emitter_radius(GPUParticleSystem* system, u32 emitter_id, f32 radius);
void gpu_particle_set_volume_emitter_surface_only(GPUParticleSystem* system, u32 emitter_id, bool surface_only);

// Mesh emitter system (TASK_642)
typedef struct {
    u32 mesh_id;            // Mesh resource ID
    Vec3 position;         // World position
    Vec3 rotation;         // Rotation (Euler angles in radians)
    Vec3 scale;            // Scale factors
    Vec3 direction;        // Main emission direction
    f32 speed_min;
    f32 speed_max;
    Vec4 color_start;
    Vec4 color_end;
    f32 size_start;
    f32 size_end;
    f32 lifetime;
    u32 particle_type;
    u32 texture_id;
    bool emit_from_vertices; // true=vertices, false=triangles
    bool use_vertex_colors;  // Sample vertex colors for particle colors
} MeshEmitterConfig;

u32 gpu_particle_create_mesh_emitter(GPUParticleSystem* system, const MeshEmitterConfig* config);
void gpu_particle_set_mesh_emitter_transform(GPUParticleSystem* system, u32 emitter_id, 
                                           Vec3 position, Vec3 rotation, Vec3 scale);
void gpu_particle_set_mesh_emitter_mesh(GPUParticleSystem* system, u32 emitter_id, u32 mesh_id);
void gpu_particle_set_mesh_emitter_vertex_mode(GPUParticleSystem* system, u32 emitter_id, bool from_vertices);

// Vertex color sampling from emitter meshes (TASK_643)
typedef struct {
    Vec4* vertex_colors;       // Vertex color array
    u32 vertex_color_count;    // Number of vertex colors
    bool use_vertex_colors;     // Enable vertex color sampling
    bool multiply_with_base;    // Multiply with base particle color
    f32 color_variation;        // Random variation in color (0.0-1.0)
} MeshVertexColors;

void gpu_particle_set_mesh_vertex_colors(GPUParticleSystem* system, u32 emitter_id, 
                                        const MeshVertexColors* colors);
void gpu_particle_enable_vertex_color_sampling(GPUParticleSystem* system, u32 emitter_id, 
                                             bool enabled, f32 variation);

// Particle simulation system (TASK_650)
typedef struct {
    Vec3 gravity;             // Global gravity vector
    f32 air_resistance;       // Air drag coefficient (0.0-1.0)
    f32 time_scale;           // Time scale for simulation
    bool enable_collision;     // Enable particle collision
    bool enable_wind;          // Enable wind effects
    Vec3 wind_velocity;        // Global wind velocity
} ParticleSimulationConfig;

void gpu_particle_set_simulation_config(GPUParticleSystem* system, const ParticleSimulationConfig* config);
void gpu_particle_set_gravity(GPUParticleSystem* system, Vec3 gravity);
void gpu_particle_set_air_resistance(GPUParticleSystem* system, f32 resistance);
void gpu_particle_set_wind(GPUParticleSystem* system, Vec3 wind_velocity);
void gpu_particle_enable_collision(GPUParticleSystem* system, bool enabled);
void gpu_particle_enable_wind(GPUParticleSystem* system, bool enabled);

// Particle physics forces (TASK_653, TASK_654)
typedef struct {
    Vec3 position;
    f32 strength;
    f32 radius;
    bool is_attractor;        // true=attractor, false=repeller
    bool active;
} ParticleForceField;

u32 gpu_particle_add_force_field(GPUParticleSystem* system, const ParticleForceField* field);
void gpu_particle_remove_force_field(GPUParticleSystem* system, u32 field_id);
void gpu_particle_update_force_field(GPUParticleSystem* system, u32 field_id, const ParticleForceField* field);

// Particle rendering system (TASK_660)
typedef struct {
    VkPipeline billboard_pipeline;
    VkPipelineLayout billboard_layout;
    VkDescriptorSetLayout billboard_descriptor_layout;
    VkSampler particle_sampler;
    
    // Rendering settings
    bool enable_depth_write;
    bool enable_depth_test;
    VkBlendFactor src_blend;
    VkBlendFactor dst_blend;
    VkCullModeFlags cull_mode;
} ParticleRenderingConfig;

void gpu_particle_set_rendering_config(GPUParticleSystem* system, const ParticleRenderingConfig* config);
void gpu_particle_set_blend_mode(GPUParticleSystem* system, VkBlendFactor src, VkBlendFactor dst);
void gpu_particle_set_depth_mode(GPUParticleSystem* system, bool depth_test, bool depth_write);
void gpu_particle_set_cull_mode(GPUParticleSystem* system, VkCullModeFlags cull_mode);

// Color/alpha over life curves (TASK_665)
typedef struct {
    Vec4 color_keys[8];      // Color keys for interpolation
    f32 alpha_keys[8];        // Alpha keys for interpolation
    f32 key_times[8];         // Normalized time for each key (0.0-1.0)
    u32 color_key_count;
    u32 alpha_key_count;
} ParticleLifeCurves;

void gpu_particle_set_life_curves(GPUParticleSystem* system, const ParticleLifeCurves* curves);
void gpu_particle_set_color_curve(GPUParticleSystem* system, const Vec4* colors, const f32* times, u32 count);
// GPU-side sorting for transparency (TASK_662)
typedef struct {
    u32 particle_index;
    f32 depth;
} ParticleDepthEntry;

void gpu_particle_enable_depth_sorting(GPUParticleSystem* system, bool enabled);
void gpu_particle_set_sorting_method(GPUParticleSystem* system, bool front_to_back);

// Texture sheet animation (TASK_663)
typedef struct {
    u32 columns;              // Number of columns in texture sheet
    u32 rows;                 // Number of rows in texture sheet
    u32 total_frames;         // Total frames in animation
    f32 frames_per_second;    // Animation speed
    bool loop_animation;      // Loop the animation
    bool random_start_frame;  // Random starting frame
} TextureSheetAnimation;

void gpu_particle_set_texture_animation(GPUParticleSystem* system, u32 emitter_id, 
                                        const TextureSheetAnimation* animation);

// Soft particles with depth fading (TASK_664)
typedef struct {
    f32 softness;             // Softness factor (0.0-1.0)
    f32 depth_scale;          // Depth scaling factor
    bool enable_depth_fade;   // Enable depth-based fading
    f32 near_fade_distance;   // Near fade distance
    f32 far_fade_distance;    // Far fade distance
} SoftParticleSettings;

void gpu_particle_set_soft_settings(GPUParticleSystem* system, const SoftParticleSettings* settings);

void gpu_particle_set_alpha_curve(GPUParticleSystem* system, const f32* alphas, const f32* times, u32 count);

// Rendering
void gpu_particle_render(GPUParticleSystem* system, VkCommandBuffer cmd_buffer, 
                        VkDescriptorSet camera_descriptor_set);

// Utility functions
void gpu_particle_get_stats(GPUParticleSystem* system, u32* alive_count, u32* dead_count, u32* emitter_count);
bool gpu_particle_is_initialized(GPUParticleSystem* system);

#endif // PARTICLE_SYSTEM_GPU_H
