#include "ecs/components/ai_components.h"
#include "ecs/ecs.h"
#include "engine/include/core/logger.h"
#include <string.h>

u32 g_agent_component_id = 0;
u32 g_pathfinding_component_id = 0;
u32 g_perception_component_id = 0;

void register_ai_components(void *world) {
  if (!world) {
    LOG_ERROR("[AI] Cannot register components: null world");
    return;
  }

  // Register Agent
  ComponentInfo agent_info = {.name = "Agent",
                              .size = sizeof(AgentComponent),
                              .alignment = alignof(AgentComponent)};
  g_agent_component_id = ecs_register_component(world, &agent_info);

  // Register Pathfinding
  ComponentInfo path_info = {.name = "Pathfinding",
                             .size = sizeof(PathfindingComponent),
                             .alignment = alignof(PathfindingComponent)};
  g_pathfinding_component_id = ecs_register_component(world, &path_info);

  // Register Perception
  ComponentInfo perc_info = {.name = "Perception",
                             .size = sizeof(PerceptionComponent),
                             .alignment = alignof(PerceptionComponent)};
  g_perception_component_id = ecs_register_component(world, &perc_info);

  LOG_INFO("[AI] Registered components:");
  LOG_INFO("  Agent: ID %u", g_agent_component_id);
  LOG_INFO("  Pathfinding: ID %u", g_pathfinding_component_id);
  LOG_INFO("  Perception: ID %u", g_perception_component_id);
}

AgentComponent agent_create(u32 type, f32 speed, f32 aggression) {
  AgentComponent agent = {0};
  agent.type_id = type;
  agent.speed = speed;
  agent.aggression = aggression;
  agent.state = AGENT_STATE_IDLE;
  agent.attack_range = 2.0f;
  return agent;
}

PathfindingComponent pathfinding_create(void) {
  PathfindingComponent path = {0};
  path.has_path = false;
  path.path_pending = false;
  return path;
}

PerceptionComponent perception_create(f32 view_dist, f32 fov) {
  PerceptionComponent perc = {0};
  perc.view_distance = view_dist;
  perc.fov_angle = fov;
  perc.hearing_range = view_dist * 0.5f;
  return perc;
}
