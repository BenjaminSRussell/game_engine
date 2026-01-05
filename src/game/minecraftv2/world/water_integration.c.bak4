// Water integration implementation
#include <core/logger.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <world/water_integration.h>

void water_integration_init(WaterIntegration *integration,
                            WaterPhysicsSystem *physics, WaterSystem *system,
                            ChunkManager *chunks) {
  if (!integration)
    return;

  memset(integration, 0, sizeof(WaterIntegration));

  integration->physics = physics;
  integration->system = system;
  integration->chunk_manager = chunks;

  // Allocate entity interactions
  integration->max_interactions = 256;
  integration->interactions = (WaterEntityInteraction *)malloc(
      integration->max_interactions * sizeof(WaterEntityInteraction));

  if (!integration->interactions) {
    LOG_ERROR("Failed to allocate water entity interactions");
    return;
  }

  integration->interaction_count = 0;

  // Allocate flow vectors
  integration->flow_vector_count = 0;
  integration->flow_vectors = (Vec3 *)malloc(1024 * sizeof(Vec3));

  if (!integration->flow_vectors) {
    LOG_ERROR("Failed to allocate flow vectors");
    free(integration->interactions);
    return;
  }

  integration->enable_spreading = true;
  integration->spreading_rate = 1.0f;
  integration->spreading_timer = 0.0f;
  integration->enable_flow_detection = true;
  integration->initialized = true;

  LOG_INFO("Water integration system initialized");
}

void water_integration_free(WaterIntegration *integration) {
  if (!integration)
    return;

  if (integration->interactions) {
    free(integration->interactions);
    integration->interactions = NULL;
  }

  if (integration->flow_vectors) {
    free(integration->flow_vectors);
    integration->flow_vectors = NULL;
  }

  memset(integration, 0, sizeof(WaterIntegration));
}

void water_integration_register_entity(WaterIntegration *integration,
                                       Entity *entity,
                                       WaterEntityInteractionType type,
                                       const BuoyancyData *buoyancy) {
  if (!integration || !entity)
    return;

  if (integration->interaction_count >= integration->max_interactions) {
    LOG_WARN("Water entity interaction pool full");
    return;
  }

  WaterEntityInteraction *interaction =
      &integration->interactions[integration->interaction_count];

  interaction->entity = entity;
  interaction->interaction_type = type;
  if (buoyancy) {
    interaction->buoyancy = *buoyancy;
  }
  interaction->is_submerged = false;
  interaction->immersion_depth = 0.0f;
  interaction->applied_force = vec3(0.0f, 0.0f, 0.0f);

  integration->interaction_count++;

  LOG_TRACE("Registered entity for water interaction (type: %u)", type);
}

void water_integration_unregister_entity(WaterIntegration *integration,
                                         Entity *entity) {
  if (!integration || !entity)
    return;

  for (u32 i = 0; i < integration->interaction_count; i++) {
    if (integration->interactions[i].entity == entity) {
      // Shift remaining interactions
      memmove(&integration->interactions[i], &integration->interactions[i + 1],
              (integration->interaction_count - i - 1) *
                  sizeof(WaterEntityInteraction));

      integration->interaction_count--;
      LOG_TRACE("Unregistered entity from water interaction");
      return;
    }
  }
}

void water_integration_apply_buoyancy(WaterIntegration *integration,
                                      Entity *entity, Vec3 *out_force) {
  if (!integration || !entity || !out_force)
    return;

  // Find entity interaction
  WaterEntityInteraction *interaction = NULL;
  for (u32 i = 0; i < integration->interaction_count; i++) {
    if (integration->interactions[i].entity == entity) {
      interaction = &integration->interactions[i];
      break;
    }
  }

  if (!interaction)
    return;

  // Buoyancy = ρ * V * g
  // where ρ = fluid density, V = displaced volume, g = gravity
  f32 gravity = 9.81f;
  f32 water_density = 1000.0f;

  Vec3 buoyancy_force =
      vec3(0.0f, water_density * interaction->buoyancy.volume * gravity, 0.0f);

  *out_force = buoyancy_force;
}

void water_integration_apply_current(WaterIntegration *integration,
                                     Entity *entity, Vec3 *out_velocity) {
  if (!integration || !entity || !out_velocity)
    return;

  // Get position from entity (would need entity structure definition)
  // For now, use a placeholder
  Vec3 entity_pos = vec3(0.0f, 0.0f, 0.0f);

  // Query current at entity position
  if (water_physics_get_current_at(integration->physics, entity_pos,
                                   out_velocity)) {
    LOG_TRACE("Applied water current to entity");
  }
}

void water_integration_apply_pressure(WaterIntegration *integration,
                                      Entity *entity, Vec3 *out_force) {
  if (!integration || !entity || !out_force)
    return;

  // Placeholder for pressure force application
  *out_force = vec3(0.0f, 0.0f, 0.0f);
}

void water_integration_spread_water(WaterIntegration *integration,
                                    Chunk *source_chunk, Vec3 position) {
  if (!integration || !source_chunk)
    return;

  // Placeholder for water spreading simulation
  LOG_TRACE("Water spreading from position (%.1f, %.1f, %.1f)", position.x,
            position.y, position.z);
}

void water_integration_flow_to_neighbors(WaterIntegration *integration,
                                         Vec3 position) {
  if (!integration)
    return;

  // Check all neighbor positions and create currents
  const Vec3 neighbors[] = {vec3(1.0f, 0.0f, 0.0f), vec3(-1.0f, 0.0f, 0.0f),
                            vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, 0.0f, -1.0f),
                            vec3(0.0f, -1.0f, 0.0f)};

  for (u32 i = 0; i < 5; i++) {
    Vec3 neighbor_pos = vec3_add(position, vec3_mul(neighbors[i], 16.0f));
    // Create flow current towards neighbor
    water_physics_create_current(integration->physics, neighbor_pos,
                                 vec3_mul(neighbors[i], -1.0f), 0.5f, 10.0f);
  }
}

void water_integration_simulate_waterfalls(WaterIntegration *integration) {
  if (!integration)
    return;

  // Scan for waterfall sources and simulate water falling
  LOG_TRACE("Simulating waterfalls");
}

bool water_integration_is_position_in_water(WaterIntegration *integration,
                                            Vec3 position) {
  if (!integration)
    return false;

  // Query water system to check if position is underwater
  // This would require water level queries from chunk system
  return false;
}

Vec3 water_integration_get_water_flow_at(WaterIntegration *integration,
                                         Vec3 position) {
  if (!integration)
    return vec3(0.0f, 0.0f, 0.0f);

  return water_physics_get_velocity_field(integration->physics, position);
}

f32 water_integration_get_water_depth_at(WaterIntegration *integration,
                                         Vec3 position) {
  if (!integration)
    return 0.0f;

  return water_physics_get_depth(integration->physics, position);
}

void water_integration_apply_underwater_effects(WaterIntegration *integration,
                                                Entity *entity) {
  if (!integration || !entity)
    return;

  // Apply slowdown, bubbles, etc.
  LOG_TRACE("Applied underwater effects to entity");
}

void water_integration_create_splash(WaterIntegration *integration,
                                     Vec3 position, f32 intensity) {
  if (!integration)
    return;

  // Create splash effect (particles + sound)
  // Create wave in water physics
  water_physics_create_wave(integration->physics, position, intensity * 0.1f,
                            5.0f);

  LOG_TRACE("Created splash at (%.1f, %.1f, %.1f) with intensity %.2f",
            position.x, position.y, position.z, intensity);
}

void water_integration_create_ripple(WaterIntegration *integration,
                                     Vec3 position, f32 amplitude) {
  if (!integration)
    return;

  // Create water ripple wave
  water_physics_create_wave(integration->physics, position, amplitude, 2.0f);

  LOG_TRACE("Created ripple at (%.1f, %.1f, %.1f) with amplitude %.2f",
            position.x, position.y, position.z, amplitude);
}

void water_integration_set_config(WaterIntegration *integration,
                                  const WaterIntegrationConfig *config) {
  if (!integration || !config)
    return;

  // Apply configuration
  LOG_TRACE("Water integration configuration updated");
}

void water_integration_update(WaterIntegration *integration, f32 delta_time) {
  if (!integration || !integration->initialized)
    return;

  // Update water spreading
  if (integration->enable_spreading) {
    integration->spreading_timer += delta_time;

    if (integration->spreading_timer >= integration->spreading_rate) {
      integration->spreading_timer = 0.0f;
      // Perform water spreading step
      LOG_TRACE("Water spreading update");
    }
  }

  // Update entity interactions
  for (u32 i = 0; i < integration->interaction_count; i++) {
    WaterEntityInteraction *interaction = &integration->interactions[i];

    if (!interaction->entity)
      continue;

    // Apply buoyancy force
    Vec3 buoyancy_force;
    water_integration_apply_buoyancy(integration, interaction->entity,
                                     &buoyancy_force);

    // Apply current velocity
    Vec3 current_velocity;
    water_integration_apply_current(integration, interaction->entity,
                                    &current_velocity);

    interaction->applied_force = buoyancy_force;

    // Apply underwater effects
    water_integration_apply_underwater_effects(integration,
                                               interaction->entity);
  }

  // Simulate waterfalls
  if (integration->enable_flow_detection) {
    water_integration_simulate_waterfalls(integration);
  }
}

WaterIntegrationStats
water_integration_get_stats(WaterIntegration *integration) {
  WaterIntegrationStats stats = {0};

  if (!integration)
    return stats;

  stats.entities_in_water = integration->interaction_count;
  stats.active_currents =
      integration->physics ? integration->physics->active_currents : 0;
  stats.active_waterfalls = 0;     // Would be tracked separately
  stats.water_blocks_affected = 0; // Would be tracked during spreading
  stats.total_water_force = 0.0f;

  // Calculate total water force on entities
  for (u32 i = 0; i < integration->interaction_count; i++) {
    Vec3 force = integration->interactions[i].applied_force;
    stats.total_water_force += vec3_length(force);
  }

  return stats;
}
