#include "ecs/components/physics_components.h"
#include "ecs/ecs.h"
#include "engine/include/core/logger.h"
#include <string.h>

u32 g_rigidbody_component_id = 0;
u32 g_collider_component_id = 0;

void register_physics_components(void *world) {
  if (!world) {
    LOG_ERROR("[Physics] Cannot register components: null world");
    return;
  }

  // Register Rigidbody
  ComponentInfo rb_info = {.name = "Rigidbody",
                           .size = sizeof(RigidbodyComponent),
                           .alignment = alignof(RigidbodyComponent)};
  g_rigidbody_component_id = ecs_register_component(world, &rb_info);

  // Register Collider
  ComponentInfo col_info = {.name = "Collider",
                            .size = sizeof(ColliderComponent),
                            .alignment = alignof(ColliderComponent)};
  g_collider_component_id = ecs_register_component(world, &col_info);

  LOG_INFO("[Physics] Registered components:");
  LOG_INFO("  Rigidbody: ID %u", g_rigidbody_component_id);
  LOG_INFO("  Collider: ID %u", g_collider_component_id);
}

RigidbodyComponent rigidbody_create(f32 mass) {
  RigidbodyComponent rb = {0};
  rb.mass = mass;
  rb.drag = 0.0f;
  rb.angular_drag = 0.05f;
  rb.use_gravity = true;
  rb.is_kinematic = false;
  rb.velocity = (Vec3){0, 0, 0};
  rb.angular_velocity = (Vec3){0, 0, 0};
  rb.internal_body = NULL;
  return rb;
}

ColliderComponent collider_create_box(Vec3 size) {
  ColliderComponent col = {0};
  col.shape_type = COLLIDER_SHAPE_BOX;
  col.box.size = size;
  col.center = (Vec3){0, 0, 0};
  col.friction = 0.5f;
  col.restitution = 0.0f;
  return col;
}

ColliderComponent collider_create_sphere(f32 radius) {
  ColliderComponent col = {0};
  col.shape_type = COLLIDER_SHAPE_SPHERE;
  col.sphere.radius = radius;
  col.center = (Vec3){0, 0, 0};
  col.friction = 0.5f;
  col.restitution = 0.0f;
  return col;
}

ColliderComponent collider_create_capsule(f32 radius, f32 height) {
  ColliderComponent col = {0};
  col.shape_type = COLLIDER_SHAPE_CAPSULE;
  col.capsule.radius = radius;
  col.capsule.height = height;
  col.center = (Vec3){0, 0, 0};
  col.friction = 0.5f;
  col.restitution = 0.0f;
  return col;
}
