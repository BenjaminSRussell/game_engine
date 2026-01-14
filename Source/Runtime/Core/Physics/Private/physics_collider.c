#include "../Public/unified_physics.h"
#include "physics_types.h"
#include <stdlib.h>
#include <string.h>
#include <unified_logger.h>
#include <unified_memory.h>

Collider *collider_create_sphere(f32 radius) {
  Collider *collider = UNIFIED_ALLOC(sizeof(Collider));
  if (!collider) {
    LOG_ERROR(LOG_CAT_PHYSICS, "Failed to allocate sphere collider");
    return NULL;
  }

  memset(collider, 0, sizeof(Collider));
  collider->type = COLLIDER_TYPE_SPHERE;
  collider->shape.sphere.radius = radius;

  // Default material
  collider->material.friction = 0.5f;
  collider->material.restitution = 0.5f;
  collider->material.density = 1.0f;

  LOG_INFO(LOG_CAT_PHYSICS, "Sphere collider created with radius: %.2f",
           radius);
  return collider;
}

Collider *collider_create_box(Vec3 half_extents) {
  Collider *collider = UNIFIED_ALLOC(sizeof(Collider));
  if (!collider) {
    LOG_ERROR(LOG_CAT_PHYSICS, "Failed to allocate box collider");
    return NULL;
  }

  memset(collider, 0, sizeof(Collider));
  collider->type = COLLIDER_TYPE_BOX;
  collider->shape.box.half_extents = half_extents;

  // Default material
  collider->material.friction = 0.5f;
  collider->material.restitution = 0.5f;
  collider->material.density = 1.0f;

  LOG_INFO(LOG_CAT_PHYSICS,
           "Box collider created with half extents: (%.2f, %.2f, %.2f)",
           half_extents.x, half_extents.y, half_extents.z);
  return collider;
}

void collider_destroy(Collider *collider) {
  if (!collider)
    return;

  if (collider->type == COLLIDER_TYPE_MESH && collider->shape.mesh.mesh_data) {
    // Cleanup mesh data if needed
    UNIFIED_FREE(collider->shape.mesh.mesh_data);
  }

  UNIFIED_FREE(collider);
  LOG_INFO(LOG_CAT_PHYSICS, "Collider destroyed");
}

ColliderType collider_get_type(const Collider *collider) {
  if (!collider)
    return COLLIDER_TYPE_SPHERE;
  return collider->type;
}

void collider_get_box_half_extents(const Collider *collider, f32 *out_x,
                                   f32 *out_y, f32 *out_z) {
  if (!collider || collider->type != COLLIDER_TYPE_BOX) {
    if (out_x)
      *out_x = 0.0f;
    if (out_y)
      *out_y = 0.0f;
    if (out_z)
      *out_z = 0.0f;
    return;
  }

  if (out_x)
    *out_x = collider->shape.box.half_extents.x;
  if (out_y)
    *out_y = collider->shape.box.half_extents.y;
  if (out_z)
    *out_z = collider->shape.box.half_extents.z;
}

f32 collider_get_sphere_radius(const Collider *collider) {
  if (!collider || collider->type != COLLIDER_TYPE_SPHERE)
    return 0.0f;
  return collider->shape.sphere.radius;
}

// Helper
RigidBody *physics_create_box(PhysicsWorld *world, Vec3 pos, Vec3 size,
                              f32 mass, PhysicsMaterial *mat) {
  if (!world)
    return NULL;

  RigidBody *body = rigid_body_create(BODY_TYPE_DYNAMIC, pos);
  if (!body)
    return NULL;

  rigid_body_set_mass(body, mass);

  Collider *collider =
      collider_create_box((Vec3){size.x * 0.5f, size.y * 0.5f, size.z * 0.5f});
  if (!collider) {
    rigid_body_destroy(body);
    return NULL;
  }

  if (mat) {
    collider->material = *mat;
  }

  rigid_body_attach_collider(body, collider);
  physics_world_add_body(world, body);

  return body;
}
