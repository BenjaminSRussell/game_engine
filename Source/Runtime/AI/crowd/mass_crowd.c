#include "ai/crowd/mass_crowd.h"
#include "ecs/mass_ecb.h"
#include "engine/include/core/logger.h"
#include <ecs/component_ids.h>
#include <math.h>
#include <stdlib.h>

ECSComponentID MASS_AGENT_COMPONENT_ID = 0;

void mass_crowd_init(World *world) {
  if (!world)
    return;

  // Register Component
  ComponentInfo info = {0};
  info.name = "MassAgentComponent";
  info.size = sizeof(MassAgentComponent);
  info.alignment = 16;
  MASS_AGENT_COMPONENT_ID = ecs_register_component(world, &info);

  LOG_INFO("Mass Crowd System Initialized (Component ID: %d)",
           MASS_AGENT_COMPONENT_ID);
}

void mass_crowd_spawn_agents(World *world, u32 count, Vec3 center, f32 radius) {
  if (!world)
    return;

  // Using ECB for batch spawn
  MassCommandBuffer *ecb = mass_ecb_create(world);

  for (u32 i = 0; i < count; i++) {
    Entity e = mass_ecb_create_entity(ecb);

    MassAgentComponent agent;
    agent.velocity = (Vec3){((f32)rand() / RAND_MAX - 0.5f) * 2.0f, 0.0f,
                            ((f32)rand() / RAND_MAX - 0.5f) * 2.0f};
    agent.target_position = center;
    agent.speed = 4.0f + ((f32)rand() / RAND_MAX);
    agent.separation_radius = 1.0f;
    agent.alignment_weight = 0.5f;
    agent.cohesion_weight = 0.3f;

    // Spawn around center
    Vec3 pos = center;
    pos.x += ((f32)rand() / RAND_MAX - 0.5f) * radius * 2.0f;
    pos.z += ((f32)rand() / RAND_MAX - 0.5f) * radius * 2.0f;

    // Add Agent Component
    mass_ecb_add_component(ecb, e, MASS_AGENT_COMPONENT_ID, &agent);

    // Add Position/Transform (Assuming core TransformComponent exists)
    // TransformComponent trans = { pos, (Quat){0,0,0,1}, {1,1,1} };
    // mass_ecb_add_component(ecb, e, TRANSFORM_COMPONENT_ID, &trans);
    // Note: We skip Transform for now as we don't have its definition here,
    // but normally we'd include it.
  }

  mass_ecb_playback(ecb);
  mass_ecb_destroy(ecb);

  LOG_INFO("Spawned %d Mass Agents", count);
}

// Processor Function
static void mass_crowd_processor(SystemContext *ctx) {
  // Iterate entities with MassAgentComponent
  // For now simple single-threaded iteration inside system

  Entity entity;
  MassAgentComponent *agent;
  // Assuming query includes MassAgentComponent (Component 0 in query)

  while (ecs_query_next(ctx->query, &entity, (void **)&agent)) {
    // Simple movement logic
    agent->velocity.x +=
        (agent->target_position.x - agent->velocity.x) * 0.01f; // Stub

    // If we had transform, we'd update it here.
    // e.g. transform->position += agent->velocity * ctx->delta_time;
  }
}

void mass_crowd_update(World *world, f32 delta_time) {
  // In a real system, we'd register this as a System.
  // Here we just manually run a query for demo purposes.

  QueryDesc desc = ECS_QUERY_ALL(MASS_AGENT_COMPONENT_ID);
  Query *query = ecs_query_create(world, &desc);

  SystemContext ctx = {world, query, delta_time, NULL};

  // Manual execution (simulate system)
  ecs_query_reset(query);
  Entity e;
  MassAgentComponent *agent;

  // We need to fetch components manually since ecs_query_next API in ecs.h
  // expects void** array for components matching query order.
  void *components[1];

  while (ecs_query_next(query, &e, components)) {
    agent = (MassAgentComponent *)components[0];

    // Update physics
    // Stub: Just integrate position if we had it.
    // agent->velocity...
  }

  ecs_query_destroy(world, query);
}
