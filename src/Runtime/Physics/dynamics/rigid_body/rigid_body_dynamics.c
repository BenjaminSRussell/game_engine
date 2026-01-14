#include <math.h>
#include <physics/core/physics_types.h>
#include <physics/physics.h>

// External from collision system (could be header)
u32 collision_system_get_contacts_for_body(u32 body_id, void *out_contacts,
                                           u32 max_count);

void rigid_body_add_force(RigidBody *body, Vec3 force) {
  if (body) {
    body->accumulated_force[0] += force.x;
    body->accumulated_force[1] += force.y;
    body->accumulated_force[2] += force.z;
  }
}

void rigid_body_add_impulse(RigidBody *body, Vec3 impulse) {
  if (body && body->inv_mass > 0.0f) {
    body->velocity[0] += impulse.x * body->inv_mass;
    body->velocity[1] += impulse.y * body->inv_mass;
    body->velocity[2] += impulse.z * body->inv_mass;
  }
}

void rigid_body_clear_forces(RigidBody *body) {
  if (body) {
    body->accumulated_force[0] = 0.0f;
    body->accumulated_force[1] = 0.0f;
    body->accumulated_force[2] = 0.0f;
    body->accumulated_torque[0] = 0.0f;
    body->accumulated_torque[1] = 0.0f;
    body->accumulated_torque[2] = 0.0f;
  }
}

// Internal function exposed to physics world or made non-static
void integrate_body(RigidBody *body, f32 dt, const Vec3 gravity) {
  if (!body || body->type == RIGID_BODY_STATIC || !body->is_active) {
    return;
  }

  // 1. Core Force Integration (Linear)
  if (body->type == RIGID_BODY_DYNAMIC) {
    body->velocity[0] += gravity.x * dt;
    body->velocity[1] += gravity.y * dt;
    body->velocity[2] += gravity.z * dt;
  }

  if (body->type == RIGID_BODY_DYNAMIC && body->inv_mass > 0.0f) {
    body->velocity[0] += (body->accumulated_force[0] * body->inv_mass) * dt;
    body->velocity[1] += (body->accumulated_force[1] * body->inv_mass) * dt;
    body->velocity[2] += (body->accumulated_force[2] * body->inv_mass) * dt;
  }

  // 2. Core Torque Integration (Angular)
  if (body->type == RIGID_BODY_DYNAMIC) {
    body->angular_velocity[0] += body->accumulated_torque[0] * dt;
    body->angular_velocity[1] += body->accumulated_torque[1] * dt;
    body->angular_velocity[2] += body->accumulated_torque[2] * dt;
  }

  // Clear forces and torques
  rigid_body_clear_forces(body);

  // 3. Apply Damping
  f32 l_damping = 1.0f - (body->linear_damping * dt);
  if (l_damping < 0.0f)
    l_damping = 0.0f;
  body->velocity[0] *= l_damping;
  body->velocity[1] *= l_damping;
  body->velocity[2] *= l_damping;

  f32 a_damping = 1.0f - (body->angular_damping * dt);
  if (a_damping < 0.0f)
    a_damping = 0.0f;
  body->angular_velocity[0] *= a_damping;
  body->angular_velocity[1] *= a_damping;
  body->angular_velocity[2] *= a_damping;

  // 4. Retrieve contacts resolved by collision system
  ContactPoint contacts[16];
  u32 contact_count =
      collision_system_get_contacts_for_body(body->id, contacts, 16);

  // Apply collision response
  for (u32 i = 0; i < contact_count; i++) {
    ContactPoint *contact = &contacts[i];
    float velocity_dot_normal = body->velocity[0] * contact->normal[0] +
                                body->velocity[1] * contact->normal[1] +
                                body->velocity[2] * contact->normal[2];

    if (velocity_dot_normal < 0.0f) {
      // Bounce
      float j = -(1.0f + body->restitution) * velocity_dot_normal;
      body->velocity[0] += j * contact->normal[0];
      body->velocity[1] += j * contact->normal[1];
      body->velocity[2] += j * contact->normal[2];

      // Friction
      body->velocity[0] *= (1.0f - body->friction);
      body->velocity[2] *= (1.0f - body->friction);

      // Positional correction
      body->position[0] +=
          contact->normal[0] * contact->penetration_depth * 0.8f;
      body->position[1] +=
          contact->normal[1] * contact->penetration_depth * 0.8f;
      body->position[2] +=
          contact->normal[2] * contact->penetration_depth * 0.8f;
    }
  }

  // 5. Integrate Position and Orientation
  body->position[0] += body->velocity[0] * dt;
  body->position[1] += body->velocity[1] * dt;
  body->position[2] += body->velocity[2] * dt;

  // Rotation integration
  Vec3 angular_vel = {body->angular_velocity[0], body->angular_velocity[1],
                      body->angular_velocity[2]};
  f32 omega_sq = vec3_length_sq(angular_vel);
  if (omega_sq > 1e-6f) {
    f32 omega = sqrtf(omega_sq);
    f32 angle = omega * dt;
    Vec3 axis = vec3_div(angular_vel, omega);

    Quat rotation_change = quat_from_axis_angle(axis, angle);
    // Assuming XYZW layout for body->rotation (w at index 3)
    Quat current_rot = {body->rotation[0], body->rotation[1], body->rotation[2],
                        body->rotation[3]};

    // quat_mul expects Quats. quat_normalize returns Quat.
    Quat new_rot = quat_normalize(quat_mul(rotation_change, current_rot));

    body->rotation[0] = new_rot.x;
    body->rotation[1] = new_rot.y;
    body->rotation[2] = new_rot.z;
    body->rotation[3] = new_rot.w;
  }
}
