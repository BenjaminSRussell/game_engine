#include "include/animation/animation_system.h"
#include <common.h>
#include "include/math/mat4.h"
#include "include/math/math_all.h"
#include "include/math/quat.h"
#include "include/math/vec3.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef clampf
#define clampf(val, min_val, max_val) fmaxf(min_val, fminf(val, max_val))
#endif

// Helper: Quat to Axis Angle
static void quat_to_axis_angle(Quat q, Vec3 *axis, f32 *angle) {
  if (q.w > 1.0f)
    q = quat_normalize(q);
  *angle = 2.0f * acosf(q.w);
  f32 s = sqrtf(1.0f - q.w * q.w);
  if (s < 0.001f) {
    *axis = vec3(1.0f, 0.0f, 0.0f);
  } else {
    *axis = vec3(q.x / s, q.y / s, q.z / s);
  }
}

// Helper: Quat from Matrix
static Quat quat_from_mat4(Mat4 m) {
  f32 trace = m.data[0][0] + m.data[1][1] + m.data[2][2];
  if (trace > 0.0f) {
    f32 s = 0.5f / sqrtf(trace + 1.0f);
    return quat(0.25f / s, (m.data[2][1] - m.data[1][2]) * s,
                (m.data[0][2] - m.data[2][0]) * s,
                (m.data[1][0] - m.data[0][1]) * s);
  } else {
    if (m.data[0][0] > m.data[1][1] && m.data[0][0] > m.data[2][2]) {
      f32 s = 2.0f * sqrtf(1.0f + m.data[0][0] - m.data[1][1] - m.data[2][2]);
      return quat((m.data[2][1] - m.data[1][2]) / s, 0.25f * s,
                  (m.data[0][1] + m.data[1][0]) / s,
                  (m.data[0][2] + m.data[2][0]) / s);
    } else if (m.data[1][1] > m.data[2][2]) {
      f32 s = 2.0f * sqrtf(1.0f + m.data[1][1] - m.data[0][0] - m.data[2][2]);
      return quat((m.data[0][2] - m.data[2][0]) / s,
                  (m.data[0][1] + m.data[1][0]) / s, 0.25f * s,
                  (m.data[1][2] + m.data[2][1]) / s);
    } else {
      f32 s = 2.0f * sqrtf(1.0f + m.data[2][2] - m.data[0][0] - m.data[1][1]);
      return quat((m.data[1][0] - m.data[0][1]) / s,
                  (m.data[0][2] + m.data[2][0]) / s,
                  (m.data[1][2] + m.data[2][1]) / s, 0.25f * s);
    }
  }
}

// Helper: Mat4 from Transform (T, R, S)
static Mat4 mat4_trs(Vec3 t, Quat r, Vec3 s) {
  Mat4 translation = mat4_translate(t);
  Mat4 rotation = quat_to_mat4(r);
  Mat4 scale = mat4_scale(s);

  // T * R * S
  Mat4 rs = mat4_mul(rotation, scale);
  return mat4_mul(translation, rs);
}

// Helper: Get Translation from Mat4
static Vec3 mat4_get_translation(Mat4 m) {
  return vec3(m.data[3][0], m.data[3][1], m.data[3][2]);
}

// Helper: Get Rotation from Mat4
static Quat mat4_get_rotation(Mat4 m) {
  // Determine scale
  Vec3 scale;
  scale.x = vec3_length(vec3(m.data[0][0], m.data[0][1], m.data[0][2]));
  scale.y = vec3_length(vec3(m.data[1][0], m.data[1][1], m.data[1][2]));
  scale.z = vec3_length(vec3(m.data[2][0], m.data[2][1], m.data[2][2]));

  // Remove scale
  Mat4 unscaled = m;
  if (scale.x > 0.0001f) {
    unscaled.data[0][0] /= scale.x;
    unscaled.data[0][1] /= scale.x;
    unscaled.data[0][2] /= scale.x;
  }
  if (scale.y > 0.0001f) {
    unscaled.data[1][0] /= scale.y;
    unscaled.data[1][1] /= scale.y;
    unscaled.data[1][2] /= scale.y;
  }
  if (scale.z > 0.0001f) {
    unscaled.data[2][0] /= scale.z;
    unscaled.data[2][1] /= scale.z;
    unscaled.data[2][2] /= scale.z;
  }

  return quat_from_mat4(unscaled);
}

typedef struct RecoilData {
  Vec3 kick_translation;
  Vec3 kick_rotation;
  f32 snap_duration;
  f32 return_duration;
  f32 randomization;
  f32 fire_rate;
  f32 intensity;
} RecoilData;

typedef struct SpringMassSystem {
  Vec3 current_pos;
  Vec3 target_pos;
  Vec3 velocity;
  Vec3 acceleration;

  Quat current_rot;
  Quat target_rot;
  Vec3 angular_velocity;
  Vec3 angular_acceleration;

  f32 spring_constant;
  f32 damping_constant;
  f32 mass;

  bool is_active;
  f32 time_since_fire;
} SpringMassSystem;

typedef struct ProceduralRecoil {
  RecoilData data;
  SpringMassSystem spring;

  Vec3 rest_position;
  Quat rest_rotation;

  Vec3 accumulated_offset;
  Vec3 climb_direction;
  f32 climb_rate;

  bool is_firing;
  f32 burst_time;
  f32 last_fire_time;

  i32 bone_index;
  Skeleton *target_skeleton;
} ProceduralRecoil;

//  COMPLETED: Define Recoil Data [Difficulty: 1] [Atomic Steps: 5]
ProceduralRecoil *procedural_recoil_create(i32 bone_index,
                                           Skeleton *target_skeleton) {
  if (!target_skeleton || bone_index < 0 ||
      bone_index >= (i32)target_skeleton->bone_count) {
    return NULL;
  }

  ProceduralRecoil *recoil =
      (ProceduralRecoil *)malloc(sizeof(ProceduralRecoil));
  if (!recoil)
    return NULL;

  memset(recoil, 0, sizeof(ProceduralRecoil));

  // Initialize recoil data with default values
  recoil->data.kick_translation = vec3(0.0f, -0.05f, 0.1f);
  recoil->data.kick_rotation = vec3(-0.1f, 0.05f, 0.0f);
  recoil->data.snap_duration = 0.05f;
  recoil->data.return_duration = 0.3f;
  recoil->data.randomization = 0.2f;
  recoil->data.fire_rate = 600.0f; // RPM
  recoil->data.intensity = 1.0f;

  // Initialize spring-mass system
  recoil->spring.current_pos = vec3(0.0f, 0.0f, 0.0f);
  recoil->spring.target_pos = vec3(0.0f, 0.0f, 0.0f);
  recoil->spring.velocity = vec3(0.0f, 0.0f, 0.0f);
  recoil->spring.acceleration = vec3(0.0f, 0.0f, 0.0f);

  recoil->spring.current_rot = quat_identity();
  recoil->spring.target_rot = quat_identity();
  recoil->spring.angular_velocity = vec3(0.0f, 0.0f, 0.0f);
  recoil->spring.angular_acceleration = vec3(0.0f, 0.0f, 0.0f);

  recoil->spring.spring_constant = 50.0f;
  recoil->spring.damping_constant = 8.0f;
  recoil->spring.mass = 1.0f;
  recoil->spring.is_active = false;

  // Initialize procedural patterns
  recoil->rest_position = vec3(0.0f, 0.0f, 0.0f);
  recoil->rest_rotation = quat_identity();
  recoil->accumulated_offset = vec3(0.0f, 0.0f, 0.0f);
  recoil->climb_direction = vec3(0.0f, 1.0f, 0.0f);
  recoil->climb_rate = 0.01f;

  recoil->bone_index = bone_index;
  recoil->target_skeleton = target_skeleton;

  return recoil;
}

void procedural_recoil_destroy(ProceduralRecoil *recoil) {
  if (recoil) {
    free(recoil);
  }
}

void procedural_recoil_set_parameters(ProceduralRecoil *recoil,
                                      Vec3 kick_translation, Vec3 kick_rotation,
                                      f32 snap_duration, f32 return_duration,
                                      f32 randomization) {
  if (!recoil)
    return;

  recoil->data.kick_translation = kick_translation;
  recoil->data.kick_rotation = kick_rotation;
  recoil->data.snap_duration = snap_duration;
  recoil->data.return_duration = return_duration;
  recoil->data.randomization = randomization;
}

//  COMPLETED: Implement Spring Mass System [Difficulty: 3] [Atomic Steps: 6]
static void update_spring_mass(SpringMassSystem *spring, f32 dt) {
  if (!spring->is_active)
    return;

  // Linear spring-mass simulation: acc = (target - current) * k - vel * c
  Vec3 pos_diff = vec3_sub(spring->target_pos, spring->current_pos);
  spring->acceleration =
      vec3_mul(pos_diff, spring->spring_constant / spring->mass);
  spring->acceleration = vec3_sub(
      spring->acceleration,
      vec3_mul(spring->velocity, spring->damping_constant / spring->mass));

  // Semi-implicit Euler integration
  spring->velocity =
      vec3_add(spring->velocity, vec3_mul(spring->acceleration, dt));
  spring->current_pos =
      vec3_add(spring->current_pos, vec3_mul(spring->velocity, dt));

  // Angular spring-mass simulation
  Quat rot_diff =
      quat_mul(quat_inverse(spring->current_rot), spring->target_rot);
  Vec3 rot_axis;
  f32 rot_angle;
  quat_to_axis_angle(rot_diff, &rot_axis, &rot_angle);

  // Clamp rotation to avoid instability
  rot_angle = fmaxf(-M_PI, fminf(M_PI, rot_angle));

  spring->angular_acceleration =
      vec3_mul(rot_axis, (rot_angle * spring->spring_constant) / spring->mass);
  spring->angular_acceleration =
      vec3_sub(spring->angular_acceleration,
               vec3_mul(spring->angular_velocity,
                        spring->damping_constant / spring->mass));

  spring->angular_velocity = vec3_add(
      spring->angular_velocity, vec3_mul(spring->angular_acceleration, dt));

  // Apply angular velocity
  f32 angular_speed = vec3_length(spring->angular_velocity);
  if (angular_speed > 0.001f) {
    Vec3 axis = vec3_normalize(spring->angular_velocity);
    Quat delta_rot = quat_from_axis_angle(axis, angular_speed * dt);
    spring->current_rot = quat_mul(delta_rot, spring->current_rot);
  }

  // Check if spring has settled
  f32 pos_threshold = 0.001f;
  f32 vel_threshold = 0.01f;

  if (vec3_length_sq(pos_diff) < pos_threshold * pos_threshold &&
      vec3_length_sq(spring->velocity) < vel_threshold * vel_threshold) {
    spring->is_active = false;
    spring->current_pos = spring->target_pos;
    spring->velocity = vec3(0.0f, 0.0f, 0.0f);
    spring->current_rot = spring->target_rot;
    spring->angular_velocity = vec3(0.0f, 0.0f, 0.0f);
  }
}

void procedural_recoil_fire(ProceduralRecoil *recoil) {
  if (!recoil)
    return;

  // Add randomization to kick
  f32 random_factor =
      1.0f + (rand() / (f32)RAND_MAX - 0.5f) * recoil->data.randomization;

  Vec3 kick_trans = vec3_mul(recoil->data.kick_translation,
                             recoil->data.intensity * random_factor);
  Vec3 kick_rot = vec3_mul(recoil->data.kick_rotation,
                           recoil->data.intensity * random_factor);

  // Apply impulsive force to velocity
  recoil->spring.velocity =
      vec3_add(recoil->spring.velocity,
               vec3_mul(kick_trans, 1.0f / recoil->data.snap_duration));
  recoil->spring.angular_velocity =
      vec3_add(recoil->spring.angular_velocity,
               vec3_mul(kick_rot, 1.0f / recoil->data.snap_duration));

  // Target is usually (0,0,0) - return to center
  recoil->spring.target_pos = vec3(0.0f, 0.0f, 0.0f);
  recoil->spring.target_rot = quat_identity();

  recoil->spring.is_active = true;
  recoil->spring.time_since_fire = 0.0f;
  recoil->is_firing = true;
  recoil->last_fire_time = 0.0f;
}

//  COMPLETED: Implement Procedural Patterns [Difficulty: 2] [Atomic Steps: 4]
void procedural_recoil_start_burst(ProceduralRecoil *recoil) {
  if (!recoil)
    return;

  recoil->is_firing = true;
  recoil->burst_time = 0.0f;
  recoil->accumulated_offset = vec3(0.0f, 0.0f, 0.0f);
}

void procedural_recoil_stop_burst(ProceduralRecoil *recoil) {
  if (!recoil)
    return;

  recoil->is_firing = false;

  // Reset target rest position when trigger released
  recoil->spring.target_pos = recoil->rest_position;
  recoil->spring.target_rot = recoil->rest_rotation;
}

static void update_procedural_patterns(ProceduralRecoil *recoil, f32 dt) {
  if (!recoil || !recoil->is_firing)
    return;

  recoil->burst_time += dt;

  // Automatic fire climb - modify target rest position over burst time
  f32 climb_offset = recoil->burst_time * recoil->climb_rate;
  Vec3 climb_offset_vec = vec3_mul(recoil->climb_direction, climb_offset);

  // Add horizontal noise (wander)
  f32 noise_time = recoil->burst_time * 10.0f;
  f32 noise_x = sinf(noise_time) * 0.002f;
  f32 noise_z = cosf(noise_time * 1.3f) * 0.002f;
  Vec3 noise_offset = vec3(noise_x, 0.0f, noise_z);

  // Update accumulated offset
  recoil->accumulated_offset = vec3_add(climb_offset_vec, noise_offset);

  // Apply offset to target position
  recoil->spring.target_pos =
      vec3_add(recoil->rest_position, recoil->accumulated_offset);
}

void procedural_recoil_update(ProceduralRecoil *recoil, f32 dt) {
  if (!recoil || !recoil->target_skeleton)
    return;

  recoil->last_fire_time += dt;

  // Update procedural patterns
  update_procedural_patterns(recoil, dt);

  // Update spring-mass system
  update_spring_mass(&recoil->spring, dt);

  if (recoil->spring.is_active) {
    recoil->spring.time_since_fire += dt;
  }

  // Apply current position/rotation to hand/weapon bone
  if (recoil->bone_index < (i32)recoil->target_skeleton->bone_count) {
    Mat4 current_transform =
        recoil->target_skeleton->global_transforms[recoil->bone_index];

    // Extract current bone position and rotation
    Vec3 bone_pos = mat4_get_translation(current_transform);
    Quat bone_rot = mat4_get_rotation(current_transform);

    // Apply recoil offset
    Vec3 new_pos = vec3_add(bone_pos, recoil->spring.current_pos);
    Quat new_rot = quat_mul(recoil->spring.current_rot, bone_rot);

    // Update bone transform
    Mat4 new_transform = mat4_trs(new_pos, new_rot, vec3(1.0f, 1.0f, 1.0f));
    recoil->target_skeleton->global_transforms[recoil->bone_index] =
        new_transform;
  }
}

void procedural_recoil_set_intensity(ProceduralRecoil *recoil, f32 intensity) {
  if (recoil) {
    recoil->data.intensity = fmaxf(0.0f, intensity);
  }
}

void procedural_recoil_set_climb_pattern(ProceduralRecoil *recoil,
                                         Vec3 climb_direction, f32 climb_rate) {
  if (recoil) {
    recoil->climb_direction = vec3_normalize(climb_direction);
    recoil->climb_rate = climb_rate;
  }
}

bool procedural_recoil_is_active(ProceduralRecoil *recoil) {
  return recoil ? recoil->spring.is_active : false;
}

f32 procedural_recoil_get_settlement_time(ProceduralRecoil *recoil) {
  if (!recoil)
    return 0.0f;

  // Approximate settlement time based on damping ratio
  f32 damping_ratio =
      recoil->spring.damping_constant /
      (2.0f * sqrtf(recoil->spring.spring_constant * recoil->spring.mass));
  f32 natural_frequency =
      sqrtf(recoil->spring.spring_constant / recoil->spring.mass);

  if (damping_ratio >= 1.0f) {
    // Overdamped
    return 3.0f / (damping_ratio * natural_frequency);
  } else {
    // Underdamped - use logarithmic decrement
    return 3.0f / (damping_ratio * natural_frequency);
  }
}
