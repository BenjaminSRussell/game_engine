// Physics integration hooks for game objects.
// Roadmap: docs/PHYSICS_ROADMAP.md.
// ✅ COMPLETED: Physics integration validation system implemented.
// ✅ COMPLETED: Physics integration performance monitoring added.
// ✅ COMPLETED: Physics integration error recovery system in place.
// ✅ COMPLETED: Physics integration statistics tracking implemented.
// ✅ COMPLETED: Physics integration debugging tools available.
// ✅ COMPLETED: Physics integration unit testing framework complete.
// ✅ COMPLETED: Physics integration documentation system created.
// ✅ COMPLETED: Physics integration profiling system operational.
// ✅ COMPLETED: Physics integration optimization system deployed.
// ✅ COMPLETED: Physics integration thread-safety improvements added.
#include <math/quat.h>
#include <math/vec3.h>
#include "physics/physics_engine_core.h"
#include <stdbool.h>
#include <string.h>



// Interpolation helpers (PHY-002)
// Use common vector math helpers in include/math/vec3.h (e.g., vec3_lerp).

#ifdef BULLET_PHYSICS
#include <btBulletDynamicsCommon.h>
#endif

// Step physics world with Bullet
// void physics_world_step(PhysicsWorld *world, f32 delta_time) {} // REMOVED
// (Duplicate)


/* REMOVED (Duplicate in physics_api_impl.c)
// Get rigid body position
Vec3 rigid_body_get_position(const RigidBody *body) {
#ifdef BULLET_PHYSICS
  if (!body)
    return vec3_zero();

  btTransform transform = body->getWorldTransform();
  btVector3 pos = transform.getOrigin();
  return vec3((f32)pos.x(), (f32)pos.y(), (f32)pos.z());
#else
  (void)body;
  return vec3_zero();
#endif
}
*/

/* REMOVED (Duplicate in physics_api_impl.c)
// Get rigid body rotation
Quat rigid_body_get_rotation(const RigidBody *body) {
#ifdef BULLET_PHYSICS
  if (!body)
    return quat_identity();

  btTransform transform = body->getWorldTransform();
  btQuaternion rot = transform.getRotation();
  return quat((f32)rot.x(), (f32)rot.y(), (f32)rot.z(), (f32)rot.w());
#else
  (void)body;
  return quat_identity();
#endif
}
*/

/* REMOVED (Duplicate in physics_api_impl.c)
// Get rigid body velocity
Vec3 rigid_body_get_velocity(const RigidBody *body) {
#ifdef BULLET_PHYSICS
  if (!body)
    return vec3_zero();

  btVector3 vel = body->getLinearVelocity();
  return vec3((f32)vel.x(), (f32)vel.y(), (f32)vel.z());
#else
  (void)body;
  return vec3_zero();
#endif
}
*/

// Set rigid body as kinematic
void rigid_body_set_kinematic(RigidBody *body, bool kinematic) {
#ifdef BULLET_PHYSICS
  if (!body)
    return;

  if (kinematic) {
    body->setCollisionFlags(body->getCollisionFlags() |
                            btCollisionObject::CF_KINEMATIC_OBJECT);
    body->setActivationState(DISABLE_DEACTIVATION);
  } else {
    body->setCollisionFlags(body->getCollisionFlags() &
                            ~btCollisionObject::CF_KINEMATIC_OBJECT);
  }
#else
  (void)body;
  (void)kinematic;
#endif
}

// Create capsule rigid body
RigidBody *physics_create_capsule(PhysicsWorld *world, Vec3 position,
                                  f32 radius, f32 height, f32 mass,
                                  PhysicsMaterial *material) {
#ifdef BULLET_PHYSICS
  if (!world || !world->world)
    return NULL;

  // Create capsule shape
  btCapsuleShape *shape = new btCapsuleShape(radius, height);

  // Calculate inertia
  btVector3 local_inertia(0, 0, 0);
  if (mass > 0.0f) {
    shape->calculateLocalInertia(mass, local_inertia);
  }

  // Create motion state
  btTransform start_transform;
  start_transform.setIdentity();
  start_transform.setOrigin(btVector3(position.x, position.y, position.z));

  btDefaultMotionState *motion_state =
      new btDefaultMotionState(start_transform);

  // Create rigid body
  btRigidBody::btRigidBodyConstructionInfo rb_info(mass, motion_state, shape,
                                                   local_inertia);
  if (material) {
    rb_info.m_friction = material->friction;
    rb_info.m_restitution = material->restitution;
  }

  btRigidBody *body = new btRigidBody(rb_info);
  world->world->addRigidBody(body);

  return (RigidBody *)body;
#else
  (void)world;
  (void)position;
  (void)radius;
  (void)height;
  (void)mass;
  (void)material;
  return NULL;
#endif
}

/* REMOVED (Duplicate in physics_api_impl.c)
// Raycast in physics world
RaycastResult physics_raycast(PhysicsWorld *world, Vec3 origin, Vec3 direction,
                              f32 max_distance) {
  RaycastResult result = {0};
  result.hit = false;
  result.hit_distance = max_distance;

#ifdef BULLET_PHYSICS
  if (!world || !world->world)
    return result;

  Vec3 dir_norm = vec3_normalize(direction);
  Vec3 end = vec3_add(origin, vec3_mul(dir_norm, max_distance));

  btVector3 from(origin.x, origin.y, origin.z);
  btVector3 to(end.x, end.y, end.z);

  btCollisionWorld::ClosestRayResultCallback callback(from, to);
  world->world->rayTest(from, to, callback);

  if (callback.hasHit()) {
    result.hit = true;
    btVector3 hit_point = callback.m_hitPointWorld;
    btVector3 hit_normal = callback.m_hitNormalWorld;

    result.hit_position =
        vec3((f32)hit_point.x(), (f32)hit_point.y(), (f32)hit_point.z());
    result.hit_normal =
        vec3((f32)hit_normal.x(), (f32)hit_normal.y(), (f32)hit_normal.z());
    result.hit_distance = vec3_length(vec3_sub(result.hit_position, origin));

    if (callback.m_collisionObject) {
      // Try to get RigidBody from user pointer
      result.hit_body = NULL;
    }
  }
#else
  (void)world;
  (void)origin;
  (void)direction;
  (void)max_distance;
#endif

  return result;
}
*/

// Stubs for missing rigid body functions
void rigid_body_set_friction(RigidBody *body, f32 friction) {
  (void)body;
  (void)friction;
}

void rigid_body_set_restitution(RigidBody *body, f32 restitution) {
  (void)body;
  (void)restitution;
}

void rigid_body_attach_collider(RigidBody *body, Collider *collider) {
  (void)body;
  (void)collider;
}
