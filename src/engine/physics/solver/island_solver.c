#include "physics/island_solver.h"
#include "core/logger.h"
#include "core/memory.h"
#include "math/math.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

// Global island solver instance
static IslandSolver g_island_solver = {0};

// Island type and state names
static const char *ISLAND_TYPE_NAMES[] = {"Static", "Dynamic", "Kinematic",
                                          "Mixed"};

static const char *ISLAND_STATE_NAMES[] = {"Active", "Sleeping", "Merging",
                                           "Splitting"};

bool island_solver_init(IslandSolver *solver, uint32_t max_islands,
                        uint32_t max_bodies) {
  if (!solver || max_islands == 0 || max_bodies == 0)
    return false;

  memset(solver, 0, sizeof(IslandSolver));

  // Allocate islands array
  solver->island_capacity = max_islands;
  solver->islands = (PhysicsIsland *)core_alloc(solver->island_capacity *
                                                sizeof(PhysicsIsland));
  if (!solver->islands)
    return false;
  memset(solver->islands, 0, solver->island_capacity * sizeof(PhysicsIsland));

  // Initialize each island with body capacity
  uint32_t bodies_per_island = max_bodies / max_islands;
  if (bodies_per_island < 4)
    bodies_per_island = 4; // Minimum bodies per island

  for (uint32_t i = 0; i < solver->island_capacity; i++) {
    PhysicsIsland *island = &solver->islands[i];
    island->island_id = i;
    island->state = ISLAND_STATE_ACTIVE;
    island->body_capacity = bodies_per_island;
    island->contact_capacity = bodies_per_island * 8;    // Estimate contacts
    island->constraint_capacity = bodies_per_island * 4; // Estimate constraints

    island->bodies =
        (IslandBody *)core_alloc(island->body_capacity * sizeof(IslandBody));
    island->contacts = (IslandContact *)core_alloc(island->contact_capacity *
                                                   sizeof(IslandContact));
    island->constraints = (IslandConstraint *)core_alloc(
        island->constraint_capacity * sizeof(IslandConstraint));

    if (!island->bodies || !island->contacts || !island->constraints) {
      island_solver_cleanup(solver);
      return false;
    }

    island->sleep.can_sleep = true;
    island->sleep.sleep_threshold = 0.1f;
    island->solver_iterations = 10;
  }

  // Allocate body mapping
  solver->body_mapping.capacity = max_bodies;
  solver->body_mapping.entity_ids =
      (EntityID *)core_alloc(solver->body_mapping.capacity * sizeof(EntityID));
  solver->body_mapping.island_indices =
      (uint32_t *)core_alloc(solver->body_mapping.capacity * sizeof(uint32_t));

  if (!solver->body_mapping.entity_ids ||
      !solver->body_mapping.island_indices) {
    island_solver_cleanup(solver);
    return false;
  }

  // Allocate contact and constraint buffers
  solver->buffers.contact_capacity = max_bodies * 4; // Estimate total contacts
  solver->buffers.constraint_capacity =
      max_bodies * 2; // Estimate total constraints

  solver->buffers.contact_buffer = (IslandContact *)core_alloc(
      solver->buffers.contact_capacity * sizeof(IslandContact));
  solver->buffers.constraint_buffer = (IslandConstraint *)core_alloc(
      solver->buffers.constraint_capacity * sizeof(IslandConstraint));

  if (!solver->buffers.contact_buffer || !solver->buffers.constraint_buffer) {
    island_solver_cleanup(solver);
    return false;
  }

  // Set default configuration
  solver->enable_island_solving = true;
  solver->enable_sleeping = true;
  solver->enable_merging = true;
  solver->enable_splitting = true;
  solver->merge_distance_threshold = 2.0f;
  solver->split_distance_threshold = 10.0f;
  solver->max_solver_iterations = 10;
  solver->min_island_size = 1.0f;
  solver->max_island_size = 50.0f;

  LOG_INFO("Island solver initialized: %u islands, %u bodies", max_islands,
           max_bodies);
  return true;
}

void island_solver_cleanup(IslandSolver *solver) {
  if (!solver)
    return;

  // Cleanup islands
  if (solver->islands) {
    for (uint32_t i = 0; i < solver->island_capacity; i++) {
      PhysicsIsland *island = &solver->islands[i];
      if (island->bodies)
        core_free(island->bodies);
      if (island->contacts)
        core_free(island->contacts);
      if (island->constraints)
        core_free(island->constraints);
    }
    core_free(solver->islands);
    solver->islands = NULL;
  }

  // Cleanup body mapping
  if (solver->body_mapping.entity_ids)
    core_free(solver->body_mapping.entity_ids);
  if (solver->body_mapping.island_indices)
    core_free(solver->body_mapping.island_indices);

  // Cleanup buffers
  if (solver->buffers.contact_buffer)
    core_free(solver->buffers.contact_buffer);
  if (solver->buffers.constraint_buffer)
    core_free(solver->buffers.constraint_buffer);

  memset(solver, 0, sizeof(IslandSolver));
  LOG_INFO("Island solver cleaned up");
}

void island_solver_update(IslandSolver *solver, PhysicsWorld *world,
                          float time_step) {
  if (!solver || !world || !solver->enable_island_solving)
    return;

  float start_time = get_time();

  // Reset statistics
  solver->stats.total_islands = solver->island_count;
  solver->stats.active_islands = 0;
  solver->stats.sleeping_islands = 0;
  solver->stats.merged_islands = 0;
  solver->stats.split_islands = 0;

  // Build islands from current physics world state
  float build_start = get_time();
  island_solver_build_islands(solver, world);
  solver->stats.island_build_time = get_time() - build_start;

  // Update island properties
  for (uint32_t i = 0; i < solver->island_count; i++) {
    island_solver_update_island_properties(solver, i);
  }

  // Check for merging opportunities
  if (solver->enable_merging) {
    island_solver_check_merging_opportunities(solver);
  }

  // Check for splitting opportunities
  if (solver->enable_splitting) {
    island_solver_check_splitting_opportunities(solver);
  }

  // Update sleep states
  if (solver->enable_sleeping) {
    island_solver_update_sleep_states(solver, time_step);
  }

  // Solve islands
  float solve_start = get_time();
  island_solver_solve_islands(solver, time_step);
  solver->stats.island_solve_time = get_time() - solve_start;

  // Update statistics
  for (uint32_t i = 0; i < solver->island_count; i++) {
    const PhysicsIsland *island = &solver->islands[i];

    if (island->state == ISLAND_STATE_ACTIVE) {
      solver->stats.active_islands++;
    } else if (island->state == ISLAND_STATE_SLEEPING) {
      solver->stats.sleeping_islands++;
    }

    solver->stats.average_island_size += island->body_count;
    if (island->body_count > solver->stats.largest_island_size) {
      solver->stats.largest_island_size = island->body_count;
    }
  }

  if (solver->island_count > 0) {
    solver->stats.average_island_size /= solver->island_count;
  }

  solver->stats.total_time = get_time() - start_time;
}

void island_solver_build_islands(IslandSolver *solver, PhysicsWorld *world) {
  if (!solver || !world)
    return;

  // Clear existing islands
  for (uint32_t i = 0; i < solver->island_count; i++) {
    PhysicsIsland *island = &solver->islands[i];
    island->body_count = 0;
    island->contact_count = 0;
    island->constraint_count = 0;
    island->state = ISLAND_STATE_ACTIVE;
  }

  solver->island_count = 0;
  solver->body_mapping.count = 0;
  solver->buffers.contact_count = 0;
  solver->buffers.constraint_count = 0;

  // This is a simplified implementation
  // In a real system, we would:
  // 1. Iterate through all physics bodies
  // 2. Build connectivity graph based on contacts and constraints
  // 3. Find connected components using union-find or similar algorithm
  // 4. Create islands for each connected component

  // For now, we'll create a simple example island
  if (solver->island_count < solver->island_capacity) {
    uint32_t island_id =
        island_solver_create_island(solver, ISLAND_TYPE_DYNAMIC);

    // Add some example bodies
    for (int i = 0; i < 5 && solver->island_count < solver->island_capacity;
         i++) {
      Vec3 position = {(float)i, 0.0f, 0.0f};
      Vec3 velocity = {0.0f, 0.0f, 0.0f};
      float mass = 1.0f;

      island_solver_add_body_to_island(solver, island_id, (EntityID)(i + 1),
                                       position, velocity, mass, false, false);
    }

    // Add some example contacts
    for (int i = 0; i < 4; i++) {
      Vec3 contact_point = {(float)i + 0.5f, 0.0f, 0.0f};
      Vec3 normal = {1.0f, 0.0f, 0.0f};

      island_solver_add_contact(solver, island_id, (EntityID)(i + 1),
                                (EntityID)(i + 2), contact_point, normal, 0.01f,
                                0.5f, 0.5f);
    }
  }

  LOG_DEBUG("Built %u islands from physics world", solver->island_count);
}

uint32_t island_solver_create_island(IslandSolver *solver, IslandType type) {
  if (!solver || solver->island_count >= solver->island_capacity)
    return UINT32_MAX;

  PhysicsIsland *island = &solver->islands[solver->island_count];
  island->island_id = solver->island_count;
  island->type = type;
  island->state = ISLAND_STATE_ACTIVE;
  island->body_count = 0;
  island->contact_count = 0;
  island->constraint_count = 0;
  island->active = true;
  island->needs_update = true;
  island->center_of_mass = vec3_zero();
  island->total_mass = 0.0f;
  island->linear_velocity = vec3_zero();
  island->angular_velocity = vec3_zero();
  island->total_force = vec3_zero();
  island->total_torque = vec3_zero();
  island->aabb.dirty = true;
  island->sleep.can_sleep = true;
  island->sleep.sleep_timer = 0.0f;
  island->sleep.sleep_frames = 0;

  return solver->island_count++;
}

bool island_solver_add_body_to_island(IslandSolver *solver, uint32_t island_id,
                                      EntityID entity_id, Vec3 position,
                                      Vec3 velocity, float mass, bool is_static,
                                      bool is_kinematic) {
  if (!solver || island_id >= solver->island_count)
    return false;

  PhysicsIsland *island = &solver->islands[island_id];
  if (island->body_count >= island->body_capacity)
    return false;

  IslandBody *body = &island->bodies[island->body_count];
  body->entity_id = entity_id;
  body->island_index = island_id;
  body->is_static = is_static;
  body->is_kinematic = is_kinematic;
  body->is_sleeping = false;
  body->position = position;
  body->velocity = velocity;
  body->force = vec3_zero();
  body->mass = mass;
  body->inv_mass = (mass > 0.0f) ? 1.0f / mass : 0.0f;

  // Update body mapping
  if (solver->body_mapping.count < solver->body_mapping.capacity) {
    solver->body_mapping.entity_ids[solver->body_mapping.count] = entity_id;
    solver->body_mapping.island_indices[solver->body_mapping.count] = island_id;
    solver->body_mapping.count++;
  }

  island->body_count++;
  island->needs_update = true;

  // Update island type based on body composition
  if (is_static && island->type != ISLAND_TYPE_MIXED) {
    island->type = ISLAND_TYPE_STATIC;
  } else if (is_kinematic && island->type != ISLAND_TYPE_MIXED) {
    island->type = ISLAND_TYPE_KINEMATIC;
  } else if (!is_static && !is_kinematic && island->type != ISLAND_TYPE_MIXED) {
    island->type = ISLAND_TYPE_DYNAMIC;
  } else {
    island->type = ISLAND_TYPE_MIXED;
  }

  return true;
}

bool island_solver_add_contact(IslandSolver *solver, uint32_t island_id,
                               EntityID entity_a, EntityID entity_b,
                               Vec3 contact_point, Vec3 normal,
                               float penetration, float restitution,
                               float friction) {
  if (!solver || island_id >= solver->island_count)
    return false;

  PhysicsIsland *island = &solver->islands[island_id];
  if (island->contact_count >= island->contact_capacity)
    return false;

  IslandContact *contact = &island->contacts[island->contact_count];
  contact->body_a_index = UINT32_MAX; // Would need to find body indices
  contact->body_b_index = UINT32_MAX;
  contact->contact_point = contact_point;
  contact->contact_normal = normal;
  contact->penetration_depth = penetration;
  contact->restitution = restitution;
  contact->friction = friction;
  contact->enabled = true;

  island->contact_count++;
  island->needs_update = true;

  return true;
}

void island_solver_update_island_properties(IslandSolver *solver,
                                            uint32_t island_id) {
  if (!solver || island_id >= solver->island_count)
    return;

  PhysicsIsland *island = &solver->islands[island_id];
  if (!island->needs_update || island->body_count == 0)
    return;

  // Calculate center of mass
  island->center_of_mass = vec3_zero();
  island->total_mass = 0.0f;
  island->linear_velocity = vec3_zero();
  island->total_force = vec3_zero();

  for (uint32_t i = 0; i < island->body_count; i++) {
    IslandBody *body = &island->bodies[i];
    island->center_of_mass =
        vec3_add(island->center_of_mass, vec3_mul(body->position, body->mass));
    island->total_mass += body->mass;
    island->linear_velocity =
        vec3_add(island->linear_velocity, vec3_mul(body->velocity, body->mass));
    island->total_force = vec3_add(island->total_force, body->force);
  }

  if (island->total_mass > 0.0f) {
    island->center_of_mass =
        vec3_div(island->center_of_mass, island->total_mass);
    island->linear_velocity =
        vec3_div(island->linear_velocity, island->total_mass);
  }

  // Update AABB
  island_solver_update_island_bounds(solver, island_id);

  island->needs_update = false;
}

void island_solver_update_island_bounds(IslandSolver *solver,
                                        uint32_t island_id) {
  if (!solver || island_id >= solver->island_count)
    return;

  PhysicsIsland *island = &solver->islands[island_id];
  if (island->body_count == 0)
    return;

  Vec3 min_bounds = island->bodies[0].position;
  Vec3 max_bounds = island->bodies[0].position;

  for (uint32_t i = 1; i < island->body_count; i++) {
    Vec3 pos = island->bodies[i].position;

    if (pos.x < min_bounds.x)
      min_bounds.x = pos.x;
    if (pos.y < min_bounds.y)
      min_bounds.y = pos.y;
    if (pos.z < min_bounds.z)
      min_bounds.z = pos.z;

    if (pos.x > max_bounds.x)
      max_bounds.x = pos.x;
    if (pos.y > max_bounds.y)
      max_bounds.y = pos.y;
    if (pos.z > max_bounds.z)
      max_bounds.z = pos.z;
  }

  island->aabb.bounds_min = min_bounds;
  island->aabb.bounds_max = max_bounds;
  island->aabb.dirty = false;
}

void island_solver_solve_islands(IslandSolver *solver, float time_step) {
  if (!solver)
    return;

  for (uint32_t i = 0; i < solver->island_count; i++) {
    PhysicsIsland *island = &solver->islands[i];

    if (island->state != ISLAND_STATE_ACTIVE || island->body_count == 0)
      continue;

    island_solver_solve_single_island(solver, i, time_step);
  }
}

void island_solver_solve_single_island(IslandSolver *solver, uint32_t island_id,
                                       float time_step) {
  if (!solver || island_id >= solver->island_count)
    return;

  PhysicsIsland *island = &solver->islands[island_id];
  float start_time = get_time();

  // Reset statistics
  island->stats.bodies_solved = 0;
  island->stats.contacts_solved = 0;
  island->stats.constraints_solved = 0;

  // Integrate forces
  for (uint32_t i = 0; i < island->body_count; i++) {
    IslandBody *body = &island->bodies[i];

    if (!body->is_static && !body->is_kinematic) {
      // Simple Euler integration
      Vec3 acceleration = vec3_mul(body->force, body->inv_mass);
      body->velocity =
          vec3_add(body->velocity, vec3_mul(acceleration, time_step));
      body->position =
          vec3_add(body->position, vec3_mul(body->velocity, time_step));

      island->stats.bodies_solved++;
    }

    // Reset force for next frame
    body->force = vec3_zero();
  }

  // Solve contacts (simplified)
  for (int iter = 0; iter < island->solver_iterations; iter++) {
    for (uint32_t i = 0; i < island->contact_count; i++) {
      IslandContact *contact = &island->contacts[i];
      if (!contact->enabled)
        continue;

      // Simplified contact resolution
      // In a real implementation, this would use proper constraint solving
      island->stats.contacts_solved++;
    }
  }

  // Solve constraints (simplified)
  for (int iter = 0; iter < island->solver_iterations; iter++) {
    for (uint32_t i = 0; i < island->constraint_count; i++) {
      IslandConstraint *constraint = &island->constraints[i];
      if (!constraint->enabled)
        continue;

      // Simplified constraint resolution
      island->stats.constraints_solved++;
    }
  }

  island->stats.solve_time = get_time() - start_time;
  island->stats.update_count++;
}

void island_solver_update_sleep_states(IslandSolver *solver, float time_step) {
  if (!solver)
    return;

  for (uint32_t i = 0; i < solver->island_count; i++) {
    PhysicsIsland *island = &solver->islands[i];

    if (island->state == ISLAND_STATE_SLEEPING) {
      // Check if island should wake up
      if (!island_solver_should_island_sleep(solver, i)) {
        island_solver_wake_up_island(solver, i);
      }
    } else if (island->state == ISLAND_STATE_ACTIVE &&
               island->sleep.can_sleep) {
      // Check if island should go to sleep
      if (island_solver_should_island_sleep(solver, i)) {
        island->sleep.sleep_timer += time_step;
        if (island->sleep.sleep_timer >
            0.5f) { // Sleep after 0.5 seconds of inactivity
          island_solver_put_island_to_sleep(solver, i);
        }
      } else {
        island->sleep.sleep_timer = 0.0f;
      }
    }
  }
}

bool island_solver_should_island_sleep(const IslandSolver *solver,
                                       uint32_t island_id) {
  if (!solver || island_id >= solver->island_count)
    return false;

  const PhysicsIsland *island = &solver->islands[island_id];

  // Check if all bodies in island are sleeping candidates
  for (uint32_t i = 0; i < island->body_count; i++) {
    const IslandBody *body = &island->bodies[i];

    // Don't sleep if any body is moving significantly
    if (vec3_length_sq(body->velocity) >
        island->sleep.sleep_threshold * island->sleep.sleep_threshold) {
      return false;
    }

    // Don't sleep if any body has significant forces
    if (vec3_length_sq(body->force) > 1.0f) {
      return false;
    }

    // Don't sleep if body is kinematic
    if (body->is_kinematic) {
      return false;
    }
  }

  return true;
}

void island_solver_put_island_to_sleep(IslandSolver *solver,
                                       uint32_t island_id) {
  if (!solver || island_id >= solver->island_count)
    return;

  PhysicsIsland *island = &solver->islands[island_id];
  island->state = ISLAND_STATE_SLEEPING;

  // Put all bodies to sleep
  for (uint32_t i = 0; i < island->body_count; i++) {
    island->bodies[i].is_sleeping = true;
  }

  LOG_DEBUG("Island %d put to sleep", island_id);
}

void island_solver_wake_up_island(IslandSolver *solver, uint32_t island_id) {
  if (!solver || island_id >= solver->island_count)
    return;

  PhysicsIsland *island = &solver->islands[island_id];
  island->state = ISLAND_STATE_ACTIVE;
  island->sleep.sleep_timer = 0.0f;

  // Wake up all bodies
  for (uint32_t i = 0; i < island->body_count; i++) {
    island->bodies[i].is_sleeping = false;
  }

  LOG_DEBUG("Island %d woke up", island_id);
}

// Utility functions
const char *island_type_get_name(IslandType type) {
  if (type >= sizeof(ISLAND_TYPE_NAMES) / sizeof(ISLAND_TYPE_NAMES[0]))
    return "Unknown";
  return ISLAND_TYPE_NAMES[type];
}

const char *island_state_get_name(IslandState state) {
  if (state >= sizeof(ISLAND_STATE_NAMES) / sizeof(ISLAND_STATE_NAMES[0]))
    return "Unknown";
  return ISLAND_STATE_NAMES[state];
}

void island_solver_get_statistics(const IslandSolver *solver,
                                  uint32_t *total_islands,
                                  uint32_t *active_islands,
                                  uint32_t *sleeping_islands,
                                  float *average_size, float *solve_time) {
  if (!solver)
    return;

  if (total_islands)
    *total_islands = solver->stats.total_islands;
  if (active_islands)
    *active_islands = solver->stats.active_islands;
  if (sleeping_islands)
    *sleeping_islands = solver->stats.sleeping_islands;
  if (average_size)
    *average_size = solver->stats.average_island_size;
  if (solve_time)
    *solve_time = solver->stats.island_solve_time;
}

// Global accessor functions
IslandSolver *get_island_solver(void) { return &g_island_solver; }

bool init_island_solver(uint32_t max_islands, uint32_t max_bodies) {
  return island_solver_init(&g_island_solver, max_islands, max_bodies);
}

void cleanup_island_solver(void) { island_solver_cleanup(&g_island_solver); }
