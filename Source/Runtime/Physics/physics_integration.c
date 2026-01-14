// physics_integration.c - Physics Integration System
#include <include/physics/physics.h>
#include <include/physics/block_physics.h>
#include <include/physics/collision/broadphase.h>
#include <include/physics/collision/narrowphase.h>
#include <include/physics/continuous_collision.h>
#include <include/core/logger.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PHYSICS_ENTITIES 2048
#define PHYSICS_MAX_ITERATIONS 8
#define CCD_VELOCITY_THRESHOLD 10.0f

typedef struct {
  EntityID entity_id;
  RigidBody *rigid_body;
  Collider *collider;
  BlockPhysicsSystem *block_physics;
  u32 collision_group;
  bool enabled;
} PhysicsEntity;

typedef struct {
  PhysicsEntity entities[MAX_PHYSICS_ENTITIES];
  u32 entity_count;
  
  PhysicsWorld *world;
  CollisionSystem *collision_system;
  CCDWorld ccd_world;
  
  Vec3 gravity;
  f32 fixed_timestep;
  f32 accumulated_time;
  
  bool is_initialized;
  bool simulation_running;
  
  // Performance tracking
  u32 total_bodies;
  u32 active_bodies;
  f32 simulation_time;
} PhysicsIntegration;

static PhysicsIntegration g_physics_integration = {0};

bool physics_integration_init(const Vec3 *gravity, f32 fixed_timestep) {
  if (g_physics_integration.is_initialized) {
    LOG_WARN(LOG_CAT_PHYSICS, "Physics integration already initialized");
    return true;
  }
  
  memset(&g_physics_integration, 0, sizeof(PhysicsIntegration));
  
  // Initialize physics world
  PhysicsConfig config = {
    .gravity = gravity ? *gravity : (Vec3){0.0f, -9.81f, 0.0f},
    .fixed_timestep = fixed_timestep > 0.0f ? fixed_timestep : 1.0f / 60.0f,
    .velocity_iterations = 8,
    .position_iterations = 3
  };
  
  g_physics_integration.world = physics_world_create(config);
  if (!g_physics_integration.world) {
    LOG_ERROR_CAT(LOG_CAT_PHYSICS, "Failed to create physics world");
    return false;
  }
  
  // Initialize collision detection
  if (!collision_system_init()) {
    LOG_ERROR_CAT(LOG_CAT_PHYSICS, "Failed to initialize collision system");
    physics_world_destroy(g_physics_integration.world);
    return false;
  }
  
  // Initialize CCD
  if (!ccd_world_init(&g_physics_integration.ccd_world, MAX_PHYSICS_ENTITIES)) {
    LOG_ERROR_CAT(LOG_CAT_PHYSICS, "Failed to initialize CCD world");
    collision_system_shutdown();
    physics_world_destroy(g_physics_integration.world);
    return false;
  }
  
  g_physics_integration.gravity = config.gravity;
  g_physics_integration.fixed_timestep = config.fixed_timestep;
  g_physics_integration.simulation_running = true;
  g_physics_integration.is_initialized = true;
  
  LOG_INFO_CAT(LOG_CAT_PHYSICS, "Physics integration initialized with gravity (%.2f, %.2f, %.2f), timestep %.4f",
           g_physics_integration.gravity.x, g_physics_integration.gravity.y, 
           g_physics_integration.gravity.z, g_physics_integration.fixed_timestep);
  
  return true;
}

void physics_integration_shutdown(void) {
  if (!g_physics_integration.is_initialized) return;
  
  // Clear all entities
  physics_integration_clear_all_entities();
  
  // Shutdown subsystems
  ccd_world_cleanup(&g_physics_integration.ccd_world);
  collision_system_shutdown();
  
  if (g_physics_integration.world) {
    physics_world_destroy(g_physics_integration.world);
    g_physics_integration.world = NULL;
  }
  
  g_physics_integration.is_initialized = false;
  g_physics_integration.simulation_running = false;
  
  LOG_INFO_CAT(LOG_CAT_PHYSICS, "Physics integration shutdown");
}

EntityID physics_integration_add_entity(EntityID entity_id, RigidBody *rigid_body, 
                                       Collider *collider, u32 collision_group) {
  if (!g_physics_integration.is_initialized || g_physics_integration.entity_count >= MAX_PHYSICS_ENTITIES) {
    return 0;
  }
  
  PhysicsEntity *entity = &g_physics_integration.entities[g_physics_integration.entity_count++];
  entity->entity_id = entity_id;
  entity->rigid_body = rigid_body;
  entity->collider = collider;
  entity->collision_group = collision_group;
  entity->enabled = true;
  
  // Add to physics world
  if (rigid_body) {
    physics_world_add_body(g_physics_integration.world, rigid_body);
  }
  
  LOG_DEBUG(LOG_CAT_PHYSICS, "Added physics entity %d to integration system", entity_id);
  return entity_id;
}

void physics_integration_remove_entity(EntityID entity_id) {
  if (!g_physics_integration.is_initialized) return;
  
  for (u32 i = 0; i < g_physics_integration.entity_count; i++) {
    PhysicsEntity *entity = &g_physics_integration.entities[i];
    
    if (entity->entity_id == entity_id) {
      // Remove from physics world
      if (entity->rigid_body) {
        physics_world_remove_body(g_physics_integration.world, entity->rigid_body);
      }
      
      ccd_world_remove_entity(&g_physics_integration.ccd_world, entity_id);

      // Move last entity to this position
      if (i < g_physics_integration.entity_count - 1) {
        g_physics_integration.entities[i] = g_physics_integration.entities[g_physics_integration.entity_count - 1];
      }
      
      g_physics_integration.entity_count--;
      LOG_DEBUG(LOG_CAT_PHYSICS, "Removed physics entity %d from integration system", entity_id);
      return;
    }
  }
}

void physics_integration_update(f32 delta_time) {
  if (!g_physics_integration.is_initialized || !g_physics_integration.simulation_running) return;
  
  f32 start_time = get_time_seconds();
  
  // Accumulate time for fixed timestep
  g_physics_integration.accumulated_time += delta_time;
  
  // Run fixed timestep simulation
  while (g_physics_integration.accumulated_time >= g_physics_integration.fixed_timestep) {
    physics_simulation_step(g_physics_integration.fixed_timestep);
    g_physics_integration.accumulated_time -= g_physics_integration.fixed_timestep;
  }
  
  // Update performance metrics
  g_physics_integration.simulation_time = get_time_seconds() - start_time;
  g_physics_integration.active_bodies = physics_count_active_bodies();
  
  LOG_DEBUG(LOG_CAT_PHYSICS, "Physics integration update: %.4f ms, %d active bodies",
           g_physics_integration.simulation_time * 1000.0f, g_physics_integration.active_bodies);
}

static void update_ccd_entities(void) {
    for (u32 i = 0; i < g_physics_integration.entity_count; i++) {
        PhysicsEntity *entity = &g_physics_integration.entities[i];
        if (!entity->enabled || !entity->rigid_body) continue;

        Vec3 vel = rigid_body_get_linear_velocity(entity->rigid_body);
        Vec3 ang_vel = rigid_body_get_angular_velocity(entity->rigid_body); // Assuming accessor exists

        f32 speed = vec3_length(vel);

        // Update motion if exists, otherwise check if needs adding
        if (!ccd_world_update_entity_motion(&g_physics_integration.ccd_world, entity->entity_id, vel, ang_vel)) {
            // Not in world. Should we add it?
            if (speed > CCD_VELOCITY_THRESHOLD && entity->collider) {
                // Add to CCD world
                ColliderType type = collider_get_type(entity->collider);
                Vec3 pos = rigid_body_get_position(entity->rigid_body);
                Quat rot = rigid_body_get_rotation(entity->rigid_body);

                if (type == COLLIDER_TYPE_SPHERE) {
                    CCDSphere sphere = {0};
                    sphere.base.position = pos;
                    sphere.base.rotation = rot;
                    sphere.base.linear_velocity = vel;
                    sphere.base.angular_velocity = ang_vel;
                    sphere.base.collision_group = entity->collision_group;
                    sphere.base.collision_mask = 0xFFFFFFFF; // Default mask
                    sphere.radius = collider_get_sphere_radius(entity->collider);

                    ccd_world_add_sphere(&g_physics_integration.ccd_world, entity->entity_id, &sphere);
                } else if (type == COLLIDER_TYPE_BOX) {
                    CCDBox box = {0};
                    box.base.position = pos;
                    box.base.rotation = rot;
                    box.base.linear_velocity = vel;
                    box.base.angular_velocity = ang_vel;
                    box.base.collision_group = entity->collision_group;
                    box.base.collision_mask = 0xFFFFFFFF;
                    collider_get_box_half_extents(entity->collider, &box.half_extents.x, &box.half_extents.y, &box.half_extents.z);

                    ccd_world_add_box(&g_physics_integration.ccd_world, entity->entity_id, &box);
                }
                // Capsule support if needed
            }
        } else {
            // It is in the world. Should we remove it if slow?
            if (speed < CCD_VELOCITY_THRESHOLD * 0.8f) { // Hysteresis
                ccd_world_remove_entity(&g_physics_integration.ccd_world, entity->entity_id);
            }
        }
    }
}

void physics_simulation_step(f32 delta_time) {
  // Update all rigid bodies
  rigid_body_update_all(delta_time);
  
  // Update collision detection
  collision_system_update(delta_time);
  
  // Update CCD for fast-moving objects
  update_ccd_entities();
  ccd_world_update(&g_physics_integration.ccd_world, delta_time);
  
  // Generate collision manifolds
  physics_generate_collision_manifolds();
  
  // Solve constraints
  physics_solve_constraints(delta_time);
  
  // Apply collision responses
  physics_apply_collision_responses();
  
  // Integrate velocities
  physics_integrate_velocities(delta_time);
  
  // Update positions
  physics_integrate_positions(delta_time);
  
  // Update block physics
  if (g_physics_integration.block_physics) {
    block_physics_update(g_physics_integration.block_physics, delta_time);
  }
}

void physics_generate_collision_manifolds(void) {
  // Get collision pairs from broadphase
  const CollisionPair *pairs = collision_get_pairs();
  u32 pair_count = collision_get_pair_count();
  
  for (u32 i = 0; i < pair_count; i++) {
    const CollisionPair *pair = &pairs[i];
    
    // Check if entities can collide
    PhysicsEntity *entity_a = physics_get_entity_by_id(pair->entity_a);
    PhysicsEntity *entity_b = physics_get_entity_by_id(pair->entity_b);
    
    if (!entity_a || !entity_b || !entity_a->enabled || !entity_b->enabled) continue;
    
    if (rigid_body_can_collide(entity_a->rigid_body, entity_b->rigid_body)) {
      // Generate narrowphase contacts
      collision_generate_contacts(pair);
    }
  }
}

void physics_solve_constraints(f32 delta_time) {
  // Solve collision constraints
  const ContactPoint *contacts = collision_get_contacts();
  u32 contact_count = collision_get_contact_count();
  
  for (u32 iteration = 0; iteration < PHYSICS_MAX_ITERATIONS; iteration++) {
    for (u32 i = 0; i < contact_count; i++) {
      const ContactPoint *contact = &contacts[i];
      
      PhysicsEntity *entity_a = physics_get_entity_by_id(contact->entity_a);
      PhysicsEntity *entity_b = physics_get_entity_by_id(contact->entity_b);
      
      if (!entity_a || !entity_b) continue;
      
      // Solve contact constraint
      physics_solve_contact_constraint(entity_a, entity_b, contact, delta_time);
    }
  }
}

void physics_solve_contact_constraint(PhysicsEntity *entity_a, PhysicsEntity *entity_b, 
                                     const ContactPoint *contact, f32 delta_time) {
  RigidBody *body_a = entity_a->rigid_body;
  RigidBody *body_b = entity_b->rigid_body;
  
  if (!body_a || !body_b) return;
  
  // Calculate relative velocity at contact point
  Vec3 r_a = vec3_sub(contact->point, rigid_body_get_position(body_a));
  Vec3 r_b = vec3_sub(contact->point, rigid_body_get_position(body_b));
  
  Vec3 vel_a = rigid_body_get_linear_velocity(body_a);
  Vec3 vel_b = rigid_body_get_linear_velocity(body_b);
  Vec3 ang_vel_a = rigid_body_get_angular_velocity(body_a);
  Vec3 ang_vel_b = rigid_body_get_angular_velocity(body_b);
  
  Vec3 contact_vel_a = vec3_add(vel_a, vec3_cross(ang_vel_a, r_a));
  Vec3 contact_vel_b = vec3_add(vel_b, vec3_cross(ang_vel_b, r_b));
  Vec3 relative_vel = vec3_sub(contact_vel_a, contact_vel_b);
  
  // Calculate normal impulse
  f32 normal_vel = vec3_dot(relative_vel, contact->normal);
  
  if (normal_vel > 0) return; // Bodies are separating
  
  f32 restitution = fminf(rigid_body_get_restitution(body_a), rigid_body_get_restitution(body_b));
  f32 impulse_magnitude = -(1.0f + restitution) * normal_vel;
  
  // Apply impulse
  Vec3 impulse = vec3_scale(contact->normal, (Vec3){impulse_magnitude, impulse_magnitude, impulse_magnitude});
  // Wait, vec3_scale(Vec3, Vec3) is component-wise multiplication.
  // vec3_mul(Vec3, f32) is scalar multiplication.
  // The original code had vec3_scale(normal, mag).
  // vec3.h: INLINE Vec3 vec3_mul(Vec3 v, f32 s) { ... }
  // vec3.h: INLINE Vec3 vec3_scale(Vec3 a, Vec3 b) { ... }
  // The original code meant vec3_mul!

  // Correcting to vec3_mul
  impulse = vec3_mul(contact->normal, impulse_magnitude);
  
  rigid_body_apply_impulse_at_point(body_a, &impulse, &contact->point);
  Vec3 neg_impulse = vec3_mul(impulse, -1.0f);
  rigid_body_apply_impulse_at_point(body_b, &neg_impulse, &contact->point);
}

void physics_apply_collision_responses(void) {
  // Apply position corrections for penetration
  const ContactPoint *contacts = collision_get_contacts();
  u32 contact_count = collision_get_contact_count();
  
  for (u32 i = 0; i < contact_count; i++) {
    const ContactPoint *contact = &contacts[i];
    
    PhysicsEntity *entity_a = physics_get_entity_by_id(contact->entity_a);
    PhysicsEntity *entity_b = physics_get_entity_by_id(contact->entity_b);
    
    if (!entity_a || !entity_b) continue;
    
    // Position based penetration resolution
    physics_resolve_penetration(entity_a, entity_b, contact);
  }
}

void physics_resolve_penetration(PhysicsEntity *entity_a, PhysicsEntity *entity_b, 
                               const ContactPoint *contact) {
  RigidBody *body_a = entity_a->rigid_body;
  RigidBody *body_b = entity_b->rigid_body;
  
  if (!body_a || !body_b) return;
  
  f32 mass_a = rigid_body_get_mass(body_a);
  f32 mass_b = rigid_body_get_mass(body_b);
  
  f32 total_mass = mass_a + mass_b;
  if (total_mass <= 0.0f) return;
  
  // Calculate position correction
  f32 correction_percent = 0.8f; // 80% penetration resolution
  f32 slop = 0.01f; // Small penetration tolerance
  f32 correction_magnitude = fmaxf(contact->penetration_depth - slop, 0.0f) / total_mass * correction_percent;
  
  Vec3 correction = vec3_mul(contact->normal, correction_magnitude); // was vec3_scale
  
  // Apply position correction
  Vec3 pos_a = rigid_body_get_position(body_a);
  Vec3 pos_b = rigid_body_get_position(body_b);
  
  if (mass_a > 0.0f) {
    Vec3 new_pos_a = vec3_add(pos_a, vec3_mul(correction, mass_a)); // was vec3_scale
    rigid_body_set_position(body_a, &new_pos_a);
  }
  
  if (mass_b > 0.0f) {
    Vec3 new_pos_b = vec3_sub(pos_b, vec3_mul(correction, mass_b)); // was vec3_scale
    rigid_body_set_position(body_b, &new_pos_b);
  }
}

void physics_integrate_velocities(f32 delta_time) {
  // Velocities are already integrated in rigid_body_integrate_forces
  // This function can be used for additional velocity integration if needed
}

void physics_integrate_positions(f32 delta_time) {
  // Positions are already integrated in rigid_body_integrate_velocity
  // This function can be used for additional position integration if needed
}

void physics_set_gravity(const Vec3 *gravity) {
  if (!gravity) return;
  
  g_physics_integration.gravity = *gravity;
  
  // Update physics world gravity
  if (g_physics_integration.world) {
    physics_world_set_gravity(g_physics_integration.world, *gravity);
  }
}

void physics_set_simulation_running(bool running) {
  g_physics_integration.simulation_running = running;
}

bool physics_is_simulation_running(void) {
  return g_physics_integration.simulation_running;
}

PhysicsEntity* physics_get_entity_by_id(EntityID entity_id) {
  for (u32 i = 0; i < g_physics_integration.entity_count; i++) {
    if (g_physics_integration.entities[i].entity_id == entity_id) {
      return &g_physics_integration.entities[i];
    }
  }
  
  return NULL;
}

void physics_enable_entity(EntityID entity_id, bool enabled) {
  PhysicsEntity *entity = physics_get_entity_by_id(entity_id);
  if (entity) {
    entity->enabled = enabled;
  }
}

bool physics_is_entity_enabled(EntityID entity_id) {
  PhysicsEntity *entity = physics_get_entity_by_id(entity_id);
  return entity ? entity->enabled : false;
}

u32 physics_count_active_bodies(void) {
  u32 count = 0;
  
  for (u32 i = 0; i < g_physics_integration.entity_count; i++) {
    PhysicsEntity *entity = &g_physics_integration.entities[i];
    
    if (entity->enabled && entity->rigid_body && rigid_body_is_awake(entity->rigid_body)) {
      count++;
    }
  }
  
  return count;
}

void physics_get_statistics(u32 *out_total_bodies, u32 *out_active_bodies, f32 *out_simulation_time) {
  if (out_total_bodies) *out_total_bodies = g_physics_integration.entity_count;
  if (out_active_bodies) *out_active_bodies = g_physics_integration.active_bodies;
  if (out_simulation_time) *out_simulation_time = g_physics_integration.simulation_time;
}

void physics_clear_all_entities(void) {
  // Remove all entities from physics world
  for (u32 i = 0; i < g_physics_integration.entity_count; i++) {
    PhysicsEntity *entity = &g_physics_integration.entities[i];
    
    if (entity->rigid_body) {
      physics_world_remove_body(g_physics_integration.world, entity->rigid_body);
    }
  }
  
  ccd_world_cleanup(&g_physics_integration.ccd_world);
  // Re-init ccd world after clear
  ccd_world_init(&g_physics_integration.ccd_world, MAX_PHYSICS_ENTITIES);

  g_physics_integration.entity_count = 0;
  memset(g_physics_integration.entities, 0, sizeof(g_physics_integration.entities));
  
  LOG_INFO_CAT(LOG_CAT_PHYSICS, "Cleared all physics entities");
}

void physics_set_block_physics_system(BlockPhysicsSystem *block_physics) {
  g_physics_integration.block_physics = block_physics;
}

BlockPhysicsSystem* physics_get_block_physics_system(void) {
  return g_physics_integration.block_physics;
}

// Utility functions
f32 get_time_seconds(void) {
  // This would typically use platform-specific time functions
  // For now, return a simple counter
  static f32 time_counter = 0.0f;
  time_counter += 0.016f; // Assume 60 FPS
  return time_counter;
}
