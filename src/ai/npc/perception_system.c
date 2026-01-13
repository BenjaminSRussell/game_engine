#include <ai/npc/perception_system.h>
#include <ai/npc_advanced/memory_system.h>
#include <common.h>
#include "engine/include/core/memory.h"
#include <core/multithreading.h>
#include <core/threading/mutex.h>
#include <core/time_system.h>
#include <ecs/ecs.h>
#include "engine/include/math/math.h"
#include <math/vec3.h>
#include <physics/physics.h>

//  COMPLETED: Implement Perception System with Visual, Auditory,
// Olfactory, Tactile stimuli

// Internal structures
struct PerceptionSystem {
  PerceptionSystemConfig config;

  // Agent management
  PerceptualAgent *agents;
  u32 agent_count;
  u32 agent_capacity;
  EntityID *entity_to_agent_index;
  u32 index_capacity;

  // Stimulus processing
  SensoryInput *stimulus_queue;
  u32 stimulus_count;
  u32 stimulus_capacity;

  // Spatial partitioning for optimization
  struct {
    f32 grid_size;
    u32 grid_width;
    u32 grid_height;
    u32 **cell_agents;      // 2D grid of agent indices
    u32 *cell_agent_counts; // Count of agents per cell
  } spatial_grid;

  // Timing and maintenance
  f64 current_time;
  f64 last_update_time;

  // Thread safety
  Mutex *system_mutex;

  // Debug and statistics
  bool debug_mode;
  u32 stimuli_processed;
  u32 raycasts_performed;
  u32 entities_detected;

  // Memory system bridge
  void *memory_system; // Pointer to MemorySystem
};

struct PerceptualAgent {
  EntityID entity_id;
  SensoryConfig config;

  // Current sensory state
  struct {
    Vec3 position;
    Vec3 forward;
    Vec3 up;
    f32 alertness_level;
    f32 attention_focus;
    f32 fatigue_level;
  } state;

  // Perceived entities
  PerceivedEntity *perceived_entities;
  u32 perceived_count;
  u32 perceived_capacity;

  // Memory and learning
  struct {
    PerceivedEntity *memory;
    u32 memory_count;
    u32 memory_capacity;
    f64 last_decay_time;
  } perception_memory;

  // Performance tracking
  u32 stimuli_processed;
  f64 total_processing_time;
};

// Internal helper functions
static u32 get_spatial_grid_coords(const PerceptionSystem *system,
                                   const Vec3 *position, u32 *x, u32 *y) {
  if (!system || !position || !x || !y)
    return 0;

  // Convert world position to grid coordinates
  *x = (u32)((position->x + system->config.spatial_grid_size * 0.5f) /
             system->config.spatial_grid_size);
  *y = (u32)((position->z + system->config.spatial_grid_size * 0.5f) /
             system->config.spatial_grid_size);

  // Clamp to grid bounds
  *x = (*x < system->spatial_grid.grid_width)
           ? *x
           : system->spatial_grid.grid_width - 1;
  *y = (*y < system->spatial_grid.grid_height)
           ? *y
           : system->spatial_grid.grid_height - 1;

  return *y * system->spatial_grid.grid_width + *x;
}

static void add_agent_to_spatial_cell(PerceptionSystem *system,
                                      PerceptualAgent *agent) {
  u32 x, y;
  u32 cell_index =
      get_spatial_grid_coords(system, &agent->state.position, &x, &y);

  // Add agent to cell
  u32 *cell = system->spatial_grid.cell_agents[cell_index];
  if (system->spatial_grid.cell_agent_counts[cell_index] <
      32) { // Max 32 agents per cell
    cell[system->spatial_grid.cell_agent_counts[cell_index]++] =
        agent - system->agents;
  }
}

static void remove_agent_from_spatial_cell(PerceptionSystem *system,
                                           PerceptualAgent *agent) {
  u32 x, y;
  u32 cell_index =
      get_spatial_grid_coords(system, &agent->state.position, &x, &y);

  // Remove agent from cell
  u32 *cell = system->spatial_grid.cell_agents[cell_index];
  u32 agent_index = agent - system->agents;

  for (u32 i = 0; i < system->spatial_grid.cell_agent_counts[cell_index]; i++) {
    if (cell[i] == agent_index) {
      // Shift remaining agents
      for (u32 j = i;
           j < system->spatial_grid.cell_agent_counts[cell_index] - 1; j++) {
        cell[j] = cell[j + 1];
      }
      system->spatial_grid.cell_agent_counts[cell_index]--;
      break;
    }
  }
}

// Forward declarations for internal functions used in process_frame
void perception_system_update_spatial_partition(PerceptionSystem *system);
static PerceivedEntity *
perception_system_get_perceived_entity_by_id(PerceptualAgent *agent,
                                             EntityID entity_id);
static PerceivedEntity *
perception_system_create_perceived_entity(PerceptualAgent *agent,
                                          EntityID entity_id);
f32 perception_system_calculate_audibility(const Vec3 *observer_pos,
                                           const Vec3 *source_pos,
                                           f32 intensity, f32 max_distance);
static void perception_system_process_visual_stimuli(PerceptionSystem *system,
                                                     PerceptualAgent *agent,
                                                     f64 delta_time);
static void perception_system_process_auditory_stimuli(PerceptionSystem *system,
                                                       PerceptualAgent *agent,
                                                       f64 delta_time);
static void perception_system_process_olfactory_stimuli(
    PerceptionSystem *system, PerceptualAgent *agent, f64 delta_time);
static void perception_system_process_tactile_stimuli(PerceptionSystem *system,
                                                      PerceptualAgent *agent,
                                                      f64 delta_time);
static void perception_system_update_perceived_entities(
    PerceptionSystem *system, PerceptualAgent *agent, f64 delta_time);

// Main Perception System implementation
PerceptionSystem *
perception_system_create(const PerceptionSystemConfig *config) {
  PerceptionSystem *system =
      MALLOC_TAGGED(sizeof(PerceptionSystem), MEMORY_TAG_AI);
  if (!system)
    return NULL;

  memset(system, 0, sizeof(PerceptionSystem));

  if (config) {
    system->config = *config;
  } else {
    // Default configuration
    system->config.max_agents = 1000;
    system->config.max_stimuli_per_frame = 100;
    system->config.max_perceived_entities = 50;
    system->config.spatial_grid_size = 10.0f;
    system->config.enable_occlusion = true;
    system->config.enable_doppler = true;
    system->config.enable_wind_effects = true;
    system->config.memory_decay_time = 300.0; // 5 minutes
    system->config.debug_mode = false;
  }

  // Allocate agent storage
  system->agent_capacity = system->config.max_agents;
  system->agents = MALLOC_TAGGED(
      sizeof(PerceptualAgent) * system->agent_capacity, MEMORY_TAG_AI);
  if (!system->agents) {
    FREE(system);
    return NULL;
  }
  memset(system->agents, 0, sizeof(PerceptualAgent) * system->agent_capacity);
  
  // Allocate stimulus queue
  system->stimulus_capacity = system->config.max_stimuli_per_frame;
  system->stimulus_queue = MALLOC_TAGGED(
      sizeof(SensoryInput) * system->stimulus_capacity, MEMORY_TAG_AI);
  if (!system->stimulus_queue) {
    if (system->agents) FREE(system->agents);
    FREE(system);
    return NULL;
  }

  // Allocate spatial partitioning grid
  system->spatial_grid.grid_width =
      (u32)(100.0f / system->config.spatial_grid_size) + 1;
  system->spatial_grid.grid_height =
      (u32)(100.0f / system->config.spatial_grid_size) + 1;
  u32 total_cells =
      system->spatial_grid.grid_width * system->spatial_grid.grid_height;

  system->spatial_grid.cell_agents =
      MALLOC_TAGGED(sizeof(u32 *) * total_cells, MEMORY_TAG_AI);
  system->spatial_grid.cell_agent_counts =
      MALLOC_TAGGED(sizeof(u32) * total_cells, MEMORY_TAG_AI);

  if (!system->spatial_grid.cell_agents ||
      !system->spatial_grid.cell_agent_counts) {
    if (system->agents) FREE(system->agents);
    if (system->stimulus_queue) FREE(system->stimulus_queue);
    if (system->spatial_grid.cell_agents) FREE(system->spatial_grid.cell_agents);
    FREE(system);
    return NULL;
  }

  // Initialize spatial grid
  for (u32 i = 0; i < total_cells; i++) {
    system->spatial_grid.cell_agents[i] =
        MALLOC_TAGGED(sizeof(u32) * 32, MEMORY_TAG_AI);
    system->spatial_grid.cell_agent_counts[i] = 0;
  }

  // g_item_db.name_map = hashmap_create(1024, sizeof(char*), sizeof(u32),
  // hash_string, equals_string);
  system->system_mutex = mutex_create(false, "PerceptionSystem");
  system->current_time = time_get_high_res_time();

  return system;
}

void perception_system_destroy(PerceptionSystem *system) {
  if (!system)
    return;

  // Cleanup agents
  for (u32 i = 0; i < system->agent_count; i++) {
    if (system->agents[i].perceived_entities) {
      FREE(system->agents[i].perceived_entities);
    }
    if (system->agents[i].perception_memory.memory) {
      FREE(system->agents[i].perception_memory.memory);
    }
  }

  // Cleanup spatial grid
  u32 total_cells =
      system->spatial_grid.grid_width * system->spatial_grid.grid_height;
  if (system->spatial_grid.cell_agents) {
    for (u32 i = 0; i < total_cells; i++) {
      if (system->spatial_grid.cell_agents[i]) {
        FREE(system->spatial_grid.cell_agents[i]);
      }
    }
  }

  // Free allocated memory
  if (system->agents)
    FREE(system->agents);
  if (system->stimulus_queue)
    FREE(system->stimulus_queue);
  if (system->spatial_grid.cell_agents)
    FREE(system->spatial_grid.cell_agents);
  if (system->spatial_grid.cell_agent_counts)
    FREE(system->spatial_grid.cell_agent_counts);
  if (system->entity_to_agent_index)
    FREE(system->entity_to_agent_index);

  if (system->system_mutex)
    mutex_destroy(system->system_mutex);

  FREE(system);
}

bool perception_system_initialize(PerceptionSystem *system) {
  if (!system)
    return false;

  mutex_lock(system->system_mutex);

  // Initialize agents
  for (u32 i = 0; i < system->agent_count; i++) {
    PerceptualAgent *agent = &system->agents[i];

    agent->perceived_entities = MALLOC_TAGGED(
        sizeof(PerceivedEntity) * system->config.max_perceived_entities,
        MEMORY_TAG_AI);
    agent->perceived_capacity = system->config.max_perceived_entities;
    agent->perceived_count = 0;

    agent->perception_memory.memory = MALLOC_TAGGED(
        sizeof(PerceivedEntity) * system->config.max_perceived_entities,
        MEMORY_TAG_AI);
    agent->perception_memory.memory_capacity =
        system->config.max_perceived_entities;
    agent->perception_memory.memory_count = 0;
    agent->perception_memory.last_decay_time = system->current_time;

    agent->stimuli_processed = 0;
    agent->total_processing_time = 0.0;
  }

  // Clear stimulus queue
  system->stimulus_count = 0;

  system->current_time = time_get_high_res_time();
  system->last_update_time = system->current_time;

  mutex_unlock(system->system_mutex);
  return true;
}

PerceptualAgent *perception_system_add_agent(PerceptionSystem *system,
                                             EntityID entity_id,
                                             const SensoryConfig *config) {
  if (!system || !config)
    return NULL;

  mutex_lock(system->system_mutex);

  if (system->agent_count >= system->agent_capacity) {
    mutex_unlock(system->system_mutex);
    return NULL;
  }

  // Create new agent
  PerceptualAgent *agent = &system->agents[system->agent_count];
  memset(agent, 0, sizeof(PerceptualAgent));

  agent->entity_id = entity_id;
  agent->config = *config;
  agent->state.position = vec3_zero();
  agent->state.forward = vec3(0, 0, 1);
  agent->state.up = vec3(0, 1, 0);
  agent->state.alertness_level = 0.5f;
  agent->state.attention_focus = 0.5f;
  agent->state.fatigue_level = 0.0f;

  system->agent_count++;

  mutex_unlock(system->system_mutex);
  return agent;
}

void perception_system_process_frame(PerceptionSystem *system, f64 delta_time) {
  if (!system)
    return;

  f64 start_time = time_get_high_res_time();

  mutex_lock(system->system_mutex);

  system->current_time += delta_time;

  // Update spatial partitioning
  perception_system_update_spatial_partition(system);

  // Process all agents
  for (u32 i = 0; i < system->agent_count; i++) {
    PerceptualAgent *agent = &system->agents[i];

    // Decay agent's internal perception memory
    f64 time_since_decay =
        system->current_time - agent->perception_memory.last_decay_time;
    if (time_since_decay > 1.0) { // Decay once per second
      for (u32 j = 0; j < agent->perceived_count; j++) {
        PerceivedEntity *entity = &agent->perceived_entities[j];
        entity->overall_confidence *= 0.9f; // Simple decay
        if (entity->overall_confidence < 0.1f) {
          entity->is_detectable = false;
        }
      }
      agent->perception_memory.last_decay_time = system->current_time;
    }

    // Process different stimulus types
    perception_system_process_visual_stimuli(system, agent, delta_time);
    perception_system_process_auditory_stimuli(system, agent, delta_time);
    perception_system_process_olfactory_stimuli(system, agent, delta_time);
    perception_system_process_tactile_stimuli(system, agent, delta_time);

    // Bridge to advanced memory system if available
    if (system->memory_system) {
      memory_sync_with_perception((MemoryAgent *)agent,
                                  (PerceivedEntity *)agent->perceived_entities,
                                  agent->perceived_count, system->current_time);
    }

    // Update perceived entities (e.g., remove low confidence ones, update
    // memory)
    perception_system_update_perceived_entities(system, agent, delta_time);

    agent->stimuli_processed++;
  }

  // Clear processed stimuli
  system->stimulus_count = 0;

  system->last_update_time = system->current_time;
  system->entities_detected += system->agent_count; // Simplified counting

  mutex_unlock(system->system_mutex);

  f64 processing_time = time_get_high_res_time() - start_time;
  for (u32 i = 0; i < system->agent_count; i++) {
    system->agents[i].total_processing_time += processing_time;
  }
}

static void perception_system_process_visual_stimuli(PerceptionSystem *system,
                                                     PerceptualAgent *agent,
                                                     f64 delta_time) {
  // Get nearby agents from spatial partitioning
  u32 nearby_count = 0;
  PerceivedEntity *nearby_entities[32];

  u32 x, y;
  get_spatial_grid_coords(system, &agent->state.position, &x, &y);

  // Check surrounding cells
  for (int dx = -1; dx <= 1; dx++) {
    for (int dy = -1; dy <= 1; dy++) {
      u32 check_x = x + dx;
      u32 check_y = y + dy;

      if (check_x < system->spatial_grid.grid_width &&
          check_y < system->spatial_grid.grid_height) {
        u32 cell_index = check_y * system->spatial_grid.grid_width + check_x;
        u32 *cell_agents = system->spatial_grid.cell_agents[cell_index];

        for (u32 j = 0;
             j < system->spatial_grid.cell_agent_counts[cell_index] &&
             nearby_count < 32;
             j++) {
          PerceptualAgent *nearby_agent = &system->agents[cell_agents[j]];
          if (nearby_agent->entity_id != agent->entity_id) {
            // Check if entity is already perceived
            PerceivedEntity *perceived =
                perception_system_get_perceived_entity_by_id(
                    agent, nearby_agent->entity_id);
            if (!perceived) {
              // Create new perceived entity
              if (agent->perceived_count < agent->perceived_capacity) {
                PerceivedEntity *new_entity =
                    &agent->perceived_entities[agent->perceived_count++];
                memset(new_entity, 0, sizeof(PerceivedEntity));
                new_entity->entity_id = nearby_agent->entity_id;
                new_entity->last_known_position = nearby_agent->state.position;
                new_entity->last_seen_time = system->current_time;
                new_entity->visual_confidence = 0.8f; // Initial confidence
              }
            }
          }
        }
      }
    }
  }

  // Update visual confidence based on visibility
  for (u32 i = 0; i < agent->perceived_count; i++) {
    PerceivedEntity *entity = &agent->perceived_entities[i];

    f32 visibility = perception_system_calculate_visibility(
        &agent->state.position, &entity->last_known_position,
        &agent->state.forward, agent->config.visual.fov_horizontal,
        agent->config.visual.max_distance);

    entity->visual_confidence *= visibility;
    entity->is_visible = entity->visual_confidence > 0.3f;
    entity->overall_confidence =
        fmaxf(entity->overall_confidence, entity->visual_confidence);
    entity->last_update_time = system->current_time;
  }
}

static void perception_system_process_auditory_stimuli(PerceptionSystem *system,
                                                       PerceptualAgent *agent,
                                                       f64 delta_time) {
  // Process auditory stimuli from stimulus queue
  for (u32 i = 0; i < system->stimulus_count; i++) {
    SensoryInput *stimulus = &system->stimulus_queue[i];

    if (stimulus->type == STIMULUS_AUDITORY) {
      f32 distance =
          vec3_length(vec3_sub(stimulus->position, agent->state.position));

      if (distance <= agent->config.auditory.max_distance) {
        f32 audibility = perception_system_calculate_audibility(
            &agent->state.position, &stimulus->position,
            stimulus->base_intensity, agent->config.auditory.max_distance);

        if (audibility > agent->config.auditory.min_volume) {
          // Find or create perceived entity
          PerceivedEntity *entity =
              perception_system_get_perceived_entity_by_id(
                  agent, stimulus->source_entity);
          if (!entity) {
            entity = perception_system_create_perceived_entity(
                agent, stimulus->source_entity);
          }

          entity->last_heard_time = system->current_time;
          entity->auditory_confidence = audibility;
          entity->is_audible = true;
          entity->overall_confidence =
              fmaxf(entity->overall_confidence, entity->auditory_confidence);
          entity->last_update_time = system->current_time;
        }
      }
    }
  }
}

static void perception_system_process_olfactory_stimuli(
    PerceptionSystem *system, PerceptualAgent *agent, f64 delta_time) {
  // Process olfactory stimuli from stimulus queue
  for (u32 i = 0; i < system->stimulus_count; i++) {
    SensoryInput *stimulus = &system->stimulus_queue[i];

    if (stimulus->type == STIMULUS_OLFACTORY) {
      f32 distance =
          vec3_length(vec3_sub(stimulus->position, agent->state.position));

      if (distance <= agent->config.olfactory.max_distance) {
        // Calculate odor concentration at agent location
        f32 concentration =
            stimulus->base_intensity *
            expf(-distance / stimulus->data.olfactory.spread_radius);

        if (concentration >= agent->config.olfactory.min_concentration) {
          // Find or create perceived entity
          PerceivedEntity *entity =
              perception_system_get_perceived_entity_by_id(
                  agent, stimulus->source_entity);
          if (!entity) {
            entity = perception_system_create_perceived_entity(
                agent, stimulus->source_entity);
          }

          entity->last_smelled_time = system->current_time;
          entity->olfactory_confidence = fminf(1.0f, concentration);
          entity->overall_confidence =
              fmaxf(entity->overall_confidence, entity->olfactory_confidence);
          entity->last_update_time = system->current_time;
        }
      }
    }
  }
}

static void perception_system_process_tactile_stimuli(PerceptionSystem *system,
                                                      PerceptualAgent *agent,
                                                      f64 delta_time) {
  // Process tactile stimuli from stimulus queue
  for (u32 i = 0; i < system->stimulus_count; i++) {
    SensoryInput *stimulus = &system->stimulus_queue[i];

    if (stimulus->type == STIMULUS_TACTILE) {
      f32 distance =
          vec3_length(vec3_sub(stimulus->position, agent->state.position));

      if (distance <= agent->config.tactile.max_distance) {
        // Calculate tactile intensity
        f32 intensity = stimulus->base_intensity *
                        (1.0f - distance / agent->config.tactile.max_distance);
        // g_crafting.recipe_map = hashmap_create(1024, sizeof(u32),
        // sizeof(u32), NULL, NULL);

        if (intensity >= agent->config.tactile.min_pressure) {
          // Find or create perceived entity
          PerceivedEntity *entity =
              perception_system_get_perceived_entity_by_id(
                  agent, stimulus->source_entity);
          if (!entity) {
            entity = perception_system_create_perceived_entity(
                agent, stimulus->source_entity);
          }

          entity->last_felt_time = system->current_time;
          entity->tactile_confidence = intensity;
          entity->overall_confidence =
              fmaxf(entity->overall_confidence, entity->tactile_confidence);
          entity->last_update_time = system->current_time;
        }
      }
    }
  }
}

void perception_system_add_stimulus(PerceptionSystem *system,
                                    const SensoryInput *stimulus) {
  if (!system || !stimulus)
    return;

  mutex_lock(system->system_mutex);

  if (system->stimulus_count < system->stimulus_capacity) {
    system->stimulus_queue[system->stimulus_count++] = *stimulus;
  }

  mutex_unlock(system->system_mutex);
}

// Utility functions
f32 perception_system_calculate_visibility(const Vec3 *observer_pos,
                                           const Vec3 *target_pos,
                                           const Vec3 *observer_forward,
                                           f32 fov, f32 max_range) {
  if (!observer_pos || !target_pos || !observer_forward)
    return 0.0f;

  Vec3 to_target = vec3_sub(*target_pos, *observer_pos);
  f32 distance = vec3_length(to_target);

  if (distance > max_range)
    return 0.0f;

  // Check field of view
  Vec3 to_target_normalized = vec3_normalize(to_target);
  f32 dot_product = vec3_dot(*observer_forward, to_target_normalized);

  f32 fov_rad = fov * 3.14159f / 180.0f;
  f32 cos_fov = cosf(fov_rad * 0.5f);

  if (dot_product < cos_fov)
    return 0.0f;

  // Distance-based visibility falloff
  return fmaxf(0.0f, 1.0f - (distance / max_range));
}

f32 perception_system_calculate_audibility(const Vec3 *observer_pos,
                                           const Vec3 *source_pos, f32 volume,
                                           f32 max_range) {
  if (!observer_pos || !source_pos)
    return 0.0f;

  f32 distance = vec3_length(vec3_sub(*source_pos, *observer_pos));

  if (distance > max_range)
    return 0.0f;

  // Inverse square law for sound attenuation
  f32 attenuation = volume / (1.0f + distance * distance * 0.01f);

  return fmaxf(0.0f, attenuation);
}

bool perception_system_line_of_sight_test(const Vec3 *start, const Vec3 *end) {
  if (!start || !end)
    return false;

  // In a real implementation, this would use the physics raycast system
  // For now, simplified line-of-sight test
  Vec3 direction = vec3_sub(*end, *start);
  f32 distance = vec3_length(direction);

  // Check for obstacles (simplified)
  f32 steps = ceilf(distance / 5.0f); // Check every 5 units
  Vec3 step = vec3_mul(vec3_normalize(direction), distance / steps);

  for (u32 i = 1; i < steps; i++) {
    Vec3 check_point = vec3_add(*start, vec3_mul(step, i));

    // Check if point is inside any obstacle (simplified)
    // In real implementation, would query collision system
    if (check_point.y < 0.0f) { // Ground level check
      return false;
    }
  }

  return true;
}

void perception_system_print_statistics(PerceptionSystem *system) {
  if (!system)
    return;

  mutex_lock(system->system_mutex);

  printf("Perception System Statistics:\n");
  printf("  Agents: %u/%u\n", system->agent_count, system->agent_capacity);
  printf("  Stimuli Processed: %u\n", system->stimuli_processed);
  printf("  Raycasts Performed: %u\n", system->raycasts_performed);
  printf("  Entities Detected: %u\n", system->entities_detected);

  u32 total_perceived = 0;
  f64 avg_processing_time = 0.0;

  for (u32 i = 0; i < system->agent_count; i++) {
    total_perceived += system->agents[i].perceived_count;
    avg_processing_time += system->agents[i].total_processing_time;
  }

  if (system->agent_count > 0) {
    avg_processing_time /= system->agent_count;
  }

  printf("  Total Perceived Entities: %u\n", total_perceived);
  printf("  Average Processing Time: %.3f ms\n", avg_processing_time * 1000.0);

  mutex_unlock(system->system_mutex);
}

// ============================================================================
// INTERNAL FUNCTION IMPLEMENTATIONS
// ============================================================================

void perception_system_update_spatial_partition(PerceptionSystem *system) {
  if (!system)
    return;

  // Clear current grid
  u32 total_cells =
      system->spatial_grid.grid_width * system->spatial_grid.grid_height;
  for (u32 i = 0; i < total_cells; i++) {
    system->spatial_grid.cell_agent_counts[i] = 0;
  }

  // Optimize: Re-add all agents to cells
  // This is a simple O(N) rebuild of the spatial grid each frame
  for (u32 i = 0; i < system->agent_count; i++) {
    add_agent_to_spatial_cell(system, &system->agents[i]);
  }
}

static PerceivedEntity *
perception_system_get_perceived_entity_by_id(PerceptualAgent *agent,
                                             EntityID entity_id) {
  if (!agent)
    return NULL;

  for (u32 i = 0; i < agent->perceived_count; i++) {
    if (agent->perceived_entities[i].entity_id == entity_id) {
      return &agent->perceived_entities[i];
    }
  }
  return NULL;
}

static PerceivedEntity *
perception_system_create_perceived_entity(PerceptualAgent *agent,
                                          EntityID entity_id) {
  if (!agent)
    return NULL;

  if (agent->perceived_count >= agent->perceived_capacity) {
    // Find entity with lowest confidence to replace
    f32 min_confidence = 100.0f;
    u32 replace_idx = 0;
    
    for (u32 i = 0; i < agent->perceived_count; i++) {
      if (agent->perceived_entities[i].overall_confidence < min_confidence) {
        min_confidence = agent->perceived_entities[i].overall_confidence;
        replace_idx = i;
      }
    }
    
    // If we're not full confidence, replace, otherwise fail
    if (min_confidence < 0.1f) {
      // Reuse this slot
      PerceivedEntity* entity = &agent->perceived_entities[replace_idx];
      memset(entity, 0, sizeof(PerceivedEntity));
      entity->entity_id = entity_id;
      return entity;
    }
    
    return NULL; // Cannot add new entity, memory full of high confidence entities
  }

  PerceivedEntity *entity = &agent->perceived_entities[agent->perceived_count++];
  memset(entity, 0, sizeof(PerceivedEntity));
  entity->entity_id = entity_id;
  return entity;
}

static void perception_system_update_perceived_entities(
    PerceptionSystem *system, PerceptualAgent *agent, f64 delta_time) {
  if (!system || !agent)
    return;

  // Cleanup loop: remove entities that are no longer detectable
  // We use a compaction algorithm
  u32 write_idx = 0;
  for (u32 read_idx = 0; read_idx < agent->perceived_count; read_idx++) {
    PerceivedEntity *entity = &agent->perceived_entities[read_idx];
    
    // Decay confidence over time if not updated
    if (system->current_time - entity->last_update_time > delta_time) {
      // Logic for decay already partly in process_frame but we can add more here if needed
      // Currently process_frame handles memory decay
    }

    // Keep entity if confidence is high enough OR it was updated recently
    bool keep = entity->overall_confidence > 0.05f || 
                (system->current_time - entity->last_update_time) < 5.0f;

    if (keep) {
      if (write_idx != read_idx) {
        agent->perceived_entities[write_idx] = *entity;
      }
      write_idx++;
    }
  }
  agent->perceived_count = write_idx;
}

void perception_system_shutdown(PerceptionSystem *system) {
  perception_system_destroy(system);
}

// Implementation of missing public API function
PerceivedEntity* perception_system_get_perceived_entities(PerceptionSystem* system, 
                                                     EntityID agent_id, u32* count) {
    if (!system || !count) return NULL;
    
    mutex_lock(system->system_mutex);
    
    // Find the agent
    PerceptualAgent* target_agent = NULL;
    for (u32 i = 0; i < system->agent_count; i++) {
        if (system->agents[i].entity_id == agent_id) {
            target_agent = &system->agents[i];
            break;
        }
    }
    
    if (!target_agent) {
        mutex_unlock(system->system_mutex);
        *count = 0;
        return NULL;
    }
    
    *count = target_agent->perceived_count;
    PerceivedEntity* entities = target_agent->perceived_entities;
    
    mutex_unlock(system->system_mutex);
    return entities;
}
