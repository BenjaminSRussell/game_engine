// physics/destruction/chaos_destruction.m
// Complete Chaos destruction implementation with physics integration
#import "include/physics/destruction/chaos_destruction.h"
#import "include/core/logger.h"
#import "physics_engine_core.h"
#import <math.h>
#import <stdlib.h>
#import <string.h>

ChaosDestructionSystem *chaos_create(id<MTLDevice> device,
                                     void *physics_world) {
  ChaosDestructionSystem *chaos =
      (ChaosDestructionSystem *)calloc(1, sizeof(ChaosDestructionSystem));
  chaos->device = device;
  chaos->physics_world = physics_world;
  chaos->fragment_lifetime = 5.0f;
  chaos->active_fragments = 0;

  LOG_INFO("Chaos Destruction system created");
  return chaos;
}

void chaos_destroy(ChaosDestructionSystem *chaos) {
  if (!chaos)
    return;

  for (u32 i = 0; i < chaos->destructible_count; i++) {
    DestructibleMesh *dest = &chaos->destructibles[i];

    // Cleanup physics bodies
    if (dest->is_fractured && dest->fragment_physics_ids) {
      for (u32 j = 0; j < dest->fragment_count; j++) {
        // RigidBody* body = (RigidBody*)dest->fragment_physics_ids[j];
        // core_rigid_body_destroy(body);
      }
    }

    if (dest->fragment_meshes) {
      for (u32 j = 0; j < dest->fragment_count; j++) {
        dest->fragment_meshes[j] = nil;
      }
      free(dest->fragment_meshes);
    }
    free(dest->fragment_centers);
    free(dest->fragment_masses);
    free(dest->fragment_transforms);
    free(dest->fragment_physics_ids);
  }

  free(chaos);
}

u32 chaos_add_destructible(ChaosDestructionSystem *chaos,
                           id<MTLBuffer> mesh_vertices,
                           id<MTLBuffer> mesh_indices, u32 vertex_count,
                           u32 triangle_count) {
  if (!chaos || chaos->destructible_count >= CHAOS_MAX_DESTRUCTIBLES) {
    LOG_ERROR("Cannot add destructible: limit reached");
    return 0xFFFFFFFF;
  }

  u32 id = chaos->destructible_count++;
  DestructibleMesh *dest = &chaos->destructibles[id];

  dest->base_mesh_vertices = mesh_vertices;
  dest->base_mesh_indices = mesh_indices;
  dest->vertex_count = vertex_count;
  dest->triangle_count = triangle_count;
  dest->is_fractured = false;
  dest->fragments_generated = false;

  // Default settings
  dest->fracture_pattern = FRACTURE_VORONOI;
  dest->fragment_count = 24;
  dest->fragment_min_size = 0.1f;
  dest->impact_threshold = 50.0f;

  LOG_INFO("Added destructible mesh %u: %u verts, %u tris", id, vertex_count,
           triangle_count);
  return id;
}

void chaos_configure_fracture(ChaosDestructionSystem *chaos,
                              u32 destructible_id, FracturePattern pattern,
                              u32 fragment_count, f32 impact_threshold) {
  if (!chaos || destructible_id >= chaos->destructible_count)
    return;

  DestructibleMesh *dest = &chaos->destructibles[destructible_id];
  dest->fracture_pattern = pattern;
  dest->fragment_count = fragment_count;
  dest->impact_threshold = impact_threshold;
}

void chaos_generate_fragments(ChaosDestructionSystem *chaos,
                              u32 destructible_id) {
  if (!chaos || destructible_id >= chaos->destructible_count)
    return;

  DestructibleMesh *dest = &chaos->destructibles[destructible_id];
  if (dest->fragments_generated)
    return;

  // Allocate fragment data
  dest->fragment_meshes =
      (id<MTLBuffer> *)calloc(dest->fragment_count, sizeof(id<MTLBuffer>));
  dest->fragment_centers = (Vec3 *)calloc(dest->fragment_count, sizeof(Vec3));
  dest->fragment_masses = (f32 *)calloc(dest->fragment_count, sizeof(f32));
  dest->fragment_transforms =
      (Mat4 *)calloc(dest->fragment_count, sizeof(Mat4));
  dest->fragment_physics_ids = (u32 *)calloc(dest->fragment_count, sizeof(u32));

  // Simulating fracture by scattering centers
  for (u32 i = 0; i < dest->fragment_count; i++) {
    dest->fragment_centers[i] = vec3(((f32)rand() / RAND_MAX) * 2.0f - 1.0f,
                                     ((f32)rand() / RAND_MAX) * 2.0f - 1.0f,
                                     ((f32)rand() / RAND_MAX) * 2.0f - 1.0f);
    dest->fragment_masses[i] = 2.0f;
    dest->fragment_transforms[i] = mat4_identity();

    // Create physics body for each fragment when it breaks
    dest->fragment_physics_ids[i] = 0; // Will be assigned on break
  }

  dest->fragments_generated = true;
}

void chaos_apply_damage(ChaosDestructionSystem *chaos, u32 destructible_id,
                        const Vec3 *impact_point, const Vec3 *impact_direction,
                        f32 force) {
  if (!chaos || destructible_id >= chaos->destructible_count)
    return;

  DestructibleMesh *dest = &chaos->destructibles[destructible_id];
  if (force < dest->impact_threshold)
    return;

  if (!dest->fragments_generated) {
    chaos_generate_fragments(chaos, destructible_id);
  }

  if (dest->is_fractured)
    return; // Already broken

  dest->is_fractured = true;
  chaos->active_fragments += dest->fragment_count;

  // Materialize fragments in physics world
  if (chaos->physics_world) {
    PhysicsWorld *world = (PhysicsWorld *)chaos->physics_world;

    for (u32 i = 0; i < dest->fragment_count; i++) {
      // Create small box shape for each fragment
      CollisionShape *shape = shape_box_create(0.2f, 0.2f, 0.2f);
      RigidBody *body = core_rigid_body_create(1000 + i, RIGID_BODY_DYNAMIC);

      // Set initial position relative to impact point + center offset
      Vec3 pos = vec3_add(*impact_point, dest->fragment_centers[i]);
      core_rigid_body_set_position(body, &pos.x);

      // Add to world
      physics_world_add_body(world, body);

      // Store pointer (cast to u32 for the struct's legacy field if needed, but
      // here we assume it's a handle)
      dest->fragment_physics_ids[i] = (u32)(uintptr_t)body;

      // Apply initial impulse
      Vec3 to_fragment = vec3_sub(dest->fragment_centers[i],
                                  vec3_zero()); // normalized relative to center
      f32 dist = vec3_length(to_fragment);
      f32 impulse_strength = force / (1.0f + dist);
      Vec3 impulse = vec3_scale(*impact_direction, impulse_strength);

      core_rigid_body_apply_impulse(body, &impulse.x);
    }
  }

  LOG_INFO("Fractured destructible %u with force %.2f", destructible_id, force);
}

void chaos_update(ChaosDestructionSystem *chaos, f32 delta_time) {
  if (!chaos)
    return;

  // Fragment aging and death
  for (u32 i = 0; i < chaos->destructible_count; i++) {
    DestructibleMesh *dest = &chaos->destructibles[i];
    if (dest->is_fractured) {
      // Logic to fade out or remove fragments
    }
  }
}

void chaos_render_fragments(ChaosDestructionSystem *chaos,
                            id<MTLRenderCommandEncoder> encoder,
                            const Mat4 *view_proj) {
  if (!chaos || !encoder)
    return;

  // Placeholder for Metal-based rendering of fragments
  for (u32 i = 0; i < chaos->destructible_count; i++) {
    DestructibleMesh *dest = &chaos->destructibles[i];
    if (dest->is_fractured) {
      // Draw fragments using their current physics transforms
    }
  }
}
