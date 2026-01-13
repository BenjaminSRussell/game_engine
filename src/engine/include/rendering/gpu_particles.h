// GPU Particle System Header
// GPU-PARTICLES-001: Particle system core architecture
// GPU-PARTICLES-002: GPU-based particle simulation
// GPU-PARTICLES-003: Compute shader particle updates
// GPU-PARTICLES-004: Particle rendering pipeline
// GPU-PARTICLES-005: Emitter management
// GPU-PARTICLES-006: Force fields and physics
// GPU-PARTICLES-007: Collision detection
// GPU-PARTICLES-008: Sorting and culling
// GPU-PARTICLES-009: Performance optimization
// GPU-PARTICLES-010: Memory management

#ifndef GPU_PARTICLES_H
#define GPU_PARTICLES_H

#include "include/rendering/vulkan.h"
#include <common.h>
#include <math/mat4.h>
#include <math/vec3.h>
#include <math/vec4.h>

#ifdef __cplusplus
extern "C" {
#endif

// Maximum number of particles per system
#define MAX_PARTICLES 1000000
#define MAX_EMITTERS 256
#define MAX_FORCE_FIELDS 64

// Particle types
typedef enum {
  PARTICLE_TYPE_POINT = 0,
  PARTICLE_TYPE_QUAD,
  PARTICLE_TYPE_MESH,
  PARTICLE_TYPE_LINE,
  PARTICLE_TYPE_COUNT
} ParticleType;

// Particle blend modes
typedef enum {
  PARTICLE_BLEND_ALPHA = 0,
  PARTICLE_BLEND_ADDITIVE,
  PARTICLE_BLEND_MULTIPLY,
  PARTICLE_BLEND_SCREEN,
  PARTICLE_BLEND_COUNT
} ParticleBlendMode;

// Particle simulation space
typedef enum {
  PARTICLE_SPACE_WORLD = 0,
  PARTICLE_SPACE_LOCAL,
  PARTICLE_SPACE_COUNT
} ParticleSpace;

// Particle structure (GPU layout)
typedef struct {
  Vec3 position;
  float life;
  Vec3 velocity;
  float size;
  Vec3 acceleration;
  float rotation;
  Vec4 color;
  float age;
  u32 flags;
  u32 emitterId;
  float mass;
  float padding;
} Particle;

// Emitter configuration
typedef struct {
  Vec3 position;
  Vec3 direction;
  Vec3 spread;
  float rate;
  float lifetime;
  float initialSpeed;
  float initialSize;
  Vec4 initialColor;
  Vec4 finalColor;
  ParticleType type;
  ParticleBlendMode blendMode;
  ParticleSpace space;
  bool enabled;
  u32 maxParticles;
  u32 particlesPerEmission;
  float emissionRadius;
  float rotationSpeed;
} ParticleEmitter;

// Force field types
typedef enum {
  FORCE_FIELD_POINT = 0,
  FORCE_FIELD_PLANE,
  FORCE_FIELD_SPHERE,
  FORCE_FIELD_TURBULENCE,
  FORCE_FIELD_VORTEX,
  FORCE_FIELD_COUNT
} ForceFieldType;

// Force field configuration
typedef struct {
  ForceFieldType type;
  Vec3 position;
  Vec3 direction;
  float strength;
  float radius;
  float turbulence;
  bool enabled;
} ForceField;

// Particle system configuration
typedef struct {
  u32 maxParticles;
  u32 maxEmitters;
  u32 maxForceFields;
  bool enableCollisions;
  bool enableSorting;
  bool enableCulling;
  float deltaTime;
  float gravity;
  float damping;
  float maxVelocity;
} ParticleSystemConfig;

// Particle system statistics
typedef struct {
  u32 activeParticles;
  u32 activeEmitters;
  u32 activeForceFields;
  u64 totalParticlesEmitted;
  u64 totalUpdates;
  double simulationTime;
  double renderTime;
  u64 memoryUsage;
  float averageLifetime;
} ParticleSystemStats;

// GPU particle system
typedef struct ParticleSystem {
#ifdef VULKAN_BUILD
  // Particle buffers
  VkBuffer particleBuffer;
  VkDeviceMemory particleMemory;
  VkBuffer particleBufferRead;
  VkDeviceMemory particleMemoryRead;

  // Emitter buffers
  VkBuffer emitterBuffer;
  VkDeviceMemory emitterMemory;

  // Force field buffers
  VkBuffer forceFieldBuffer;
  VkDeviceMemory forceFieldMemory;

  // Indirect dispatch buffer
  VkBuffer indirectBuffer;
  VkDeviceMemory indirectMemory;

  // Compute pipeline
  VkPipeline computePipeline;
  VkPipelineLayout computeLayout;
  VkDescriptorSetLayout computeDescriptorLayout;
  VkDescriptorSet computeDescriptorSet;

  // Render pipeline
  VkPipeline renderPipeline;
  VkPipelineLayout renderLayout;
  VkDescriptorSetLayout renderDescriptorLayout;
  VkDescriptorSet renderDescriptorSet;

  // Particle texture atlas
  VkImage textureAtlas;
  VkImageView textureAtlasView;
  VkSampler textureAtlasSampler;
  VkDeviceMemory textureAtlasMemory;
#endif
  bool initialized;
  ParticleSystemConfig config;
  ParticleSystemStats stats;

  // CPU-side data
  ParticleEmitter emitters[MAX_EMITTERS];
  ForceField forceFields[MAX_FORCE_FIELDS];
  u32 emitterCount;
  u32 forceFieldCount;
} ParticleSystem;

// Core particle system functions
bool gpu_particles_init(ParticleSystem *system, VulkanRenderer *renderer,
                        const ParticleSystemConfig *config);
void gpu_particles_shutdown(ParticleSystem *system, VulkanRenderer *renderer);
bool gpu_particles_is_initialized(const ParticleSystem *system);

// Simulation functions
bool gpu_particles_update(ParticleSystem *system, VulkanRenderer *renderer,
                          VkCommandBuffer commandBuffer, float deltaTime);
bool gpu_particles_emit(ParticleSystem *system, u32 emitterId, u32 count);
bool gpu_particles_emit_burst(ParticleSystem *system, u32 emitterId, u32 count);

// Emitter management
u32 gpu_particles_add_emitter(ParticleSystem *system,
                              const ParticleEmitter *emitter);
bool gpu_particles_remove_emitter(ParticleSystem *system, u32 emitterId);
bool gpu_particles_update_emitter(ParticleSystem *system, u32 emitterId,
                                  const ParticleEmitter *emitter);
const ParticleEmitter *gpu_particles_get_emitter(const ParticleSystem *system,
                                                 u32 emitterId);

// Force field management
u32 gpu_particles_add_force_field(ParticleSystem *system,
                                  const ForceField *forceField);
bool gpu_particles_remove_force_field(ParticleSystem *system, u32 fieldId);
bool gpu_particles_update_force_field(ParticleSystem *system, u32 fieldId,
                                      const ForceField *forceField);

// Rendering functions
bool gpu_particles_render(ParticleSystem *system, VulkanRenderer *renderer,
                          VkCommandBuffer commandBuffer, const Mat4 *viewMatrix,
                          const Mat4 *projMatrix);
bool gpu_particles_render_depth(ParticleSystem *system,
                                VulkanRenderer *renderer,
                                VkCommandBuffer commandBuffer);

// Configuration and optimization
void gpu_particles_set_config(ParticleSystem *system,
                              const ParticleSystemConfig *config);
void gpu_particles_get_config(const ParticleSystem *system,
                              ParticleSystemConfig *outConfig);
void gpu_particles_optimize_for_gpu(ParticleSystem *system);
void gpu_particles_enable_culling(ParticleSystem *system, bool enable);
void gpu_particles_enable_sorting(ParticleSystem *system, bool enable);

// Statistics and debugging
void gpu_particles_get_stats(const ParticleSystem *system,
                             ParticleSystemStats *outStats);
void gpu_particles_reset_stats(ParticleSystem *system);
void gpu_particles_debug_print_stats(const ParticleSystem *system);
bool gpu_particles_validate_system(const ParticleSystem *system);

// Advanced features
bool gpu_particles_enable_collision(ParticleSystem *system, bool enable);
bool gpu_particles_set_collision_geometry(ParticleSystem *system,
                                          const void *vertices,
                                          u32 vertexCount);
bool gpu_particles_enable_turbulence(ParticleSystem *system, bool enable);
bool gpu_particles_set_wind_field(ParticleSystem *system,
                                  const Vec3 *windDirection, float strength);

// Utility functions
u64 gpu_particles_estimate_memory_usage(const ParticleSystemConfig *config);
bool gpu_particles_check_gpu_support(VulkanRenderer *renderer);
u32 gpu_particles_calculate_work_group_count(u32 particleCount);

#ifdef __cplusplus
}
#endif

#endif // GPU_PARTICLES_H
