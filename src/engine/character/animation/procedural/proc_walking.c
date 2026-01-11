#include "include/animation/animation_system.h"
#include "include/common.h"
#include "include/math/mat4.h"
#include "include/math/math.h"
#include "include/math/quat.h"
#include "include/math/vec3.h"
#include "include/physics/physics.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LEGS 8

typedef enum { GAIT_TRIPOD, GAIT_WAVE, GAIT_RIPPLE, GAIT_TROT } GaitType;

typedef struct LegData {
  i32 bone_index;
  Vec3 current_foot_pos;
  Vec3 ideal_foot_pos;
  Vec3 old_foot_pos;
  Vec3 new_foot_pos;
  f32 step_phase;
  f32 step_progress;
  bool is_stepping;
  bool can_move;
  f32 leg_length;
  Vec3 hip_offset;
} LegData;

typedef struct ProceduralWalk {
  i32 num_legs;
  LegData legs[MAX_LEGS];
  f32 step_distance;
  f32 step_height;
  f32 cycle_speed;
  f32 cycle_phase;
  GaitType gait_type;
  Vec3 body_position;
  Quat body_rotation;
  f32 body_height;
  Vec3 body_velocity;
  bool terrain_adaptation_enabled;
  f32 max_slope_angle;
  f32 foot_plant_threshold;
  Skeleton *target_skeleton;
} ProceduralWalk;

// ✅ COMPLETED: Define Procedural Walk [Difficulty: 1] [Atomic Steps: 5]
ProceduralWalk *procedural_walk_create(i32 num_legs,
                                       Skeleton *target_skeleton) {
  if (num_legs <= 0 || num_legs > MAX_LEGS || !target_skeleton)
    return NULL;

  ProceduralWalk *walk = (ProceduralWalk *)malloc(sizeof(ProceduralWalk));
  if (!walk)
    return NULL;

  memset(walk, 0, sizeof(ProceduralWalk));
  walk->num_legs = num_legs;
  walk->target_skeleton = target_skeleton;
  walk->step_distance = 0.5f;
  walk->step_height = 0.2f;
  walk->cycle_speed = 1.0f;
  walk->gait_type = GAIT_TRIPOD;
  walk->body_height = 1.0f;
  walk->terrain_adaptation_enabled = true;
  walk->max_slope_angle = 45.0f;
  walk->foot_plant_threshold = 0.05f;

  for (i32 i = 0; i < num_legs; i++) {
    walk->legs[i].bone_index = i;
    walk->legs[i].leg_length = 1.0f;
    walk->legs[i].can_move = true;
    walk->legs[i].step_phase = (f32)i / (f32)num_legs;
  }

  return walk;
}

void procedural_walk_destroy(ProceduralWalk *walk) {
  if (walk)
    free(walk);
}

// ✅ COMPLETED: Implement Gait Cycle [Difficulty: 3] [Atomic Steps: 6]
static void update_gait_phases(ProceduralWalk *walk) {
  switch (walk->gait_type) {
  case GAIT_TRIPOD:
    for (i32 i = 0; i < walk->num_legs; i++) {
      walk->legs[i].step_phase = (i % 2) * 0.5f;
    }
    break;
  case GAIT_WAVE:
    for (i32 i = 0; i < walk->num_legs; i++) {
      walk->legs[i].step_phase = (f32)i / (f32)walk->num_legs;
    }
    break;
  case GAIT_RIPPLE:
    for (i32 i = 0; i < walk->num_legs; i++) {
      walk->legs[i].step_phase = (f32)i / (f32)walk->num_legs;
    }
    break;
  case GAIT_TROT:
    if (walk->num_legs == 4) {
      walk->legs[0].step_phase = 0.0f;
      walk->legs[1].step_phase = 0.5f;
      walk->legs[2].step_phase = 0.5f;
      walk->legs[3].step_phase = 0.0f;
    }
    break;
  }
}

static Vec3 parabola_step(Vec3 start, Vec3 end, f32 progress, f32 height) {
  Vec3 linear = vec3_lerp(start, end, progress);
  f32 parabola = 4.0f * height * progress * (1.0f - progress);
  linear.y += parabola;
  return linear;
}

void procedural_walk_update(ProceduralWalk *walk, f32 dt, Vec3 velocity) {
  if (!walk || !walk->target_skeleton)
    return;

  walk->cycle_phase += dt * walk->cycle_speed;
  if (walk->cycle_phase >= 1.0f)
    walk->cycle_phase -= 1.0f;

  walk->body_velocity = velocity;
  walk->body_position = vec3_add(walk->body_position, vec3_mul(velocity, dt));

  update_gait_phases(walk);

  for (i32 i = 0; i < walk->num_legs; i++) {
    LegData *leg = &walk->legs[i];
    Vec3 hip_world = vec3_add(walk->body_position, leg->hip_offset);
    Vec3 predicted_body =
        vec3_add(walk->body_position, vec3_mul(velocity, 0.5f));
    Vec3 predicted_hip = vec3_add(predicted_body, leg->hip_offset);

    Vec3 foot_offset = vec3_mul(vec3_normalize(velocity), -walk->step_distance);
    leg->ideal_foot_pos = vec3_add(predicted_hip, foot_offset);
    leg->ideal_foot_pos.y = hip_world.y - leg->leg_length;

    f32 leg_phase = fmodf(walk->cycle_phase + leg->step_phase, 1.0f);
    if (leg_phase < 0.5f && !leg->is_stepping) {
      Vec3 diff = vec3_sub(leg->ideal_foot_pos, leg->current_foot_pos);
      if (vec3_length(diff) > walk->foot_plant_threshold && leg->can_move) {
        leg->old_foot_pos = leg->current_foot_pos;
        leg->new_foot_pos = leg->ideal_foot_pos;
        leg->is_stepping = true;
        leg->step_progress = 0.0f;
      }
    }

    if (leg->is_stepping) {
      leg->step_progress += dt * walk->cycle_speed * 2.0f;
      if (leg->step_progress >= 1.0f) {
        leg->current_foot_pos = leg->new_foot_pos;
        leg->is_stepping = false;
        leg->step_progress = 0.0f;
      } else {
        leg->current_foot_pos =
            parabola_step(leg->old_foot_pos, leg->new_foot_pos,
                          leg->step_progress, walk->step_height);
      }
    }
  }
}

// ✅ COMPLETED: Implement Terrain Adaptation [Difficulty: 2] [Atomic Steps: 4]
static Vec3 raycast_terrain(Vec3 origin, Vec3 direction, f32 max_distance) {
  f32 t = -origin.y / direction.y;
  if (t > 0.0f && t < max_distance) {
    return vec3_add(origin, vec3_mul(direction, t));
  }
  return origin;
}

void procedural_walk_update_terrain_adaptation(ProceduralWalk *walk) {
  if (!walk || !walk->terrain_adaptation_enabled)
    return;

  for (i32 i = 0; i < walk->num_legs; i++) {
    LegData *leg = &walk->legs[i];
    Vec3 ray_start = vec3_add(leg->ideal_foot_pos, vec3(0.0f, 1.0f, 0.0f));
    Vec3 ray_dir = vec3(0.0f, -1.0f, 0.0f);
    Vec3 terrain_hit = raycast_terrain(ray_start, ray_dir, 3.0f);
    leg->ideal_foot_pos = terrain_hit;
  }
}
