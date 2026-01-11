// Water physics integration - connects water physics to world systems
// Handles water spreading, object interaction, and environmental effects
#ifndef WATER_INTEGRATION_H
#define WATER_INTEGRATION_H

#include "../game_common.h"
#include <math/vec3.h>
#include "water_physics.h"
#include "water_system.h"

// Forward declarations
typedef struct Entity Entity;
typedef struct ChunkManager ChunkManager;
typedef struct Chunk Chunk;

// Water entity interaction types
typedef enum {
  WATER_ENTITY_NONE = 0,
  WATER_ENTITY_FLOAT = 1,
  WATER_ENTITY_SINK = 2,
  WATER_ENTITY_BUOYANT = 3,
  WATER_ENTITY_AFFECTED_BY_CURRENT = 4
} WaterEntityInteractionType;

// Buoyancy data
typedef struct {
  f32 mass;
  f32 volume;
  f32 density;
  f32 drag_coefficient;
} BuoyancyData;

// Water-entity interaction
typedef struct {
  Entity *entity;
  WaterEntityInteractionType interaction_type;
  BuoyancyData buoyancy;
  f32 immersion_depth;
  bool is_submerged;
  Vec3 applied_force;
} WaterEntityInteraction;

// Water integration system
typedef struct {
  WaterPhysicsSystem *physics;
  WaterSystem *system;
  ChunkManager *chunk_manager;

  // Entity interactions
  WaterEntityInteraction *interactions;
  u32 interaction_count;
  u32 max_interactions;

  // Water spreading
  bool enable_spreading;
  f32 spreading_rate;
  f32 spreading_timer;

  // Flow detection
  bool enable_flow_detection;
  Vec3 *flow_vectors;
  u32 flow_vector_count;

  bool initialized;
} WaterIntegration;

// Initialization
void water_integration_init(WaterIntegration *integration,
                           WaterPhysicsSystem *physics,
                           WaterSystem *system,
                           ChunkManager *chunks);
void water_integration_free(WaterIntegration *integration);

// Entity interaction
void water_integration_register_entity(WaterIntegration *integration,
                                     Entity *entity,
                                     WaterEntityInteractionType type,
                                     const BuoyancyData *buoyancy);
void water_integration_unregister_entity(WaterIntegration *integration,
                                       Entity *entity);

// Physics application
void water_integration_apply_buoyancy(WaterIntegration *integration,
                                    Entity *entity, Vec3 *out_force);
void water_integration_apply_current(WaterIntegration *integration,
                                   Entity *entity, Vec3 *out_velocity);
void water_integration_apply_pressure(WaterIntegration *integration,
                                    Entity *entity, Vec3 *out_force);

// Update loop
void water_integration_update(WaterIntegration *integration, f32 delta_time);

// Water spreading
void water_integration_spread_water(WaterIntegration *integration,
                                  Chunk *source_chunk, Vec3 position);
void water_integration_flow_to_neighbors(WaterIntegration *integration,
                                        Vec3 position);
void water_integration_simulate_waterfalls(WaterIntegration *integration);

// Flow queries
bool water_integration_is_position_in_water(WaterIntegration *integration,
                                           Vec3 position);
Vec3 water_integration_get_water_flow_at(WaterIntegration *integration,
                                        Vec3 position);
f32 water_integration_get_water_depth_at(WaterIntegration *integration,
                                        Vec3 position);

// Underwater effects
void water_integration_apply_underwater_effects(WaterIntegration *integration,
                                               Entity *entity);
void water_integration_create_splash(WaterIntegration *integration,
                                   Vec3 position, f32 intensity);
void water_integration_create_ripple(WaterIntegration *integration,
                                   Vec3 position, f32 amplitude);

// Configuration
typedef struct {
  f32 water_density;
  f32 gravity;
  f32 damping;
  f32 spreading_speed;
  bool enable_buoyancy;
  bool enable_currents;
  bool enable_pressure;
} WaterIntegrationConfig;

void water_integration_set_config(WaterIntegration *integration,
                                 const WaterIntegrationConfig *config);

// Statistics
typedef struct {
  u32 entities_in_water;
  u32 active_currents;
  u32 active_waterfalls;
  u32 water_blocks_affected;
  f32 total_water_force;
} WaterIntegrationStats;

WaterIntegrationStats water_integration_get_stats(
    WaterIntegration *integration);

#endif // WATER_INTEGRATION_H
