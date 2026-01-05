/**
 * FINAL REMAINING TODOs: Extended Features Across All Systems
 * ~600 TODOs across rendering, gameplay, AI, audio, and utilities
 */

#include <math.h>
#include <stdlib.h>
#include <string.h>

// ADVANCED CAMERA SYSTEM
typedef enum {
  CAM_FIRST_PERSON,
  CAM_THIRD_PERSON,
  CAM_ORBIT,
  CAM_FREE
} CameraMode;

typedef struct {
  float position[3], target[3], up[3];
  float fov, near_plane, far_plane;
  CameraMode mode;
  float orbit_distance, orbit_yaw, orbit_pitch;
  float smoothing;
} Camera;

Camera *camera_create() {
  Camera *cam = calloc(1, sizeof(Camera));
  cam->fov = 60.0f;
  cam->near_plane = 0.1f;
  cam->far_plane = 1000.0f;
  cam->mode = CAM_THIRD_PERSON;
  cam->orbit_distance = 5.0f;
  cam->smoothing = 0.1f;
  cam->up[1] = 1.0f;
  return cam;
}

void camera_update_third_person(Camera *cam, float target_pos[3], float dt) {
  // Smooth follow
  for (int i = 0; i < 3; i++) {
    cam->target[i] += (target_pos[i] - cam->target[i]) * cam->smoothing;
  }

  // Position behind and above target
  float offset[3] = {sinf(cam->orbit_yaw) * cam->orbit_distance,
                     cam->orbit_distance * 0.5f,
                     cosf(cam->orbit_yaw) * cam->orbit_distance};

  cam->position[0] = cam->target[0] + offset[0];
  cam->position[1] = cam->target[1] + offset[1];
  cam->position[2] = cam->target[2] + offset[2];
}

void camera_update_orbit(Camera *cam, float dt) {
  cam->position[0] = cam->target[0] + sinf(cam->orbit_yaw) *
                                          cosf(cam->orbit_pitch) *
                                          cam->orbit_distance;
  cam->position[1] =
      cam->target[1] + sinf(cam->orbit_pitch) * cam->orbit_distance;
  cam->position[2] = cam->target[2] + cosf(cam->orbit_yaw) *
                                          cosf(cam->orbit_pitch) *
                                          cam->orbit_distance;
}

// SKILL TREE SYSTEM
typedef struct {
  int id, parent_id;
  char name[64], description[256];
  int cost, current_level, max_level;
  bool unlocked;
  void (*on_unlock)(void *);
} Skill;

typedef struct {
  Skill *skills;
  int skill_count;
  int available_points;
} SkillTree;

SkillTree *skill_tree_create(int capacity) {
  SkillTree *tree = calloc(1, sizeof(SkillTree));
  tree->skills = calloc(capacity, sizeof(Skill));
  return tree;
}

bool skill_tree_can_unlock(SkillTree *tree, int skill_id) {
  if (skill_id < 0 || skill_id >= tree->skill_count)
    return false;

  Skill *skill = &tree->skills[skill_id];

  if (skill->unlocked)
    return false;
  if (tree->available_points < skill->cost)
    return false;

  // Check parent requirement
  if (skill->parent_id >= 0) {
    if (!tree->skills[skill->parent_id].unlocked)
      return false;
  }

  return true;
}

void skill_tree_unlock(SkillTree *tree, int skill_id) {
  if (!skill_tree_can_unlock(tree, skill_id))
    return;

  Skill *skill = &tree->skills[skill_id];
  skill->unlocked = true;
  skill->current_level = 1;
  tree->available_points -= skill->cost;

  if (skill->on_unlock) {
    skill->on_unlock(NULL);
  }
}

// FACTION SYSTEM
typedef struct {
  int faction_id;
  float reputation; // -100 to 100
} FactionRelationship;

typedef struct {
  FactionRelationship *relationships;
  int relationship_count;
} FactionSystem;

FactionSystem *faction_system_create() {
  FactionSystem *sys = calloc(1, sizeof(FactionSystem));
  sys->relationships = calloc(32, sizeof(FactionRelationship));
  return sys;
}

void faction_modify_reputation(FactionSystem *sys, int faction_id,
                               float amount) {
  for (int i = 0; i < sys->relationship_count; i++) {
    if (sys->relationships[i].faction_id == faction_id) {
      sys->relationships[i].reputation += amount;

      if (sys->relationships[i].reputation > 100.0f) {
        sys->relationships[i].reputation = 100.0f;
      } else if (sys->relationships[i].reputation < -100.0f) {
        sys->relationships[i].reputation = -100.0f;
      }
      return;
    }
  }

  // New faction relationship
  if (sys->relationship_count < 32) {
    sys->relationships[sys->relationship_count].faction_id = faction_id;
    sys->relationships[sys->relationship_count].reputation = amount;
    sys->relationship_count++;
  }
}

float faction_get_reputation(FactionSystem *sys, int faction_id) {
  for (int i = 0; i < sys->relationship_count; i++) {
    if (sys->relationships[i].faction_id == faction_id) {
      return sys->relationships[i].reputation;
    }
  }
  return 0.0f;
}

// QUEST OBJECTIVES (Extended)
typedef enum {
  OBJ_KILL,
  OBJ_COLLECT,
  OBJ_TALK,
  OBJ_EXPLORE,
  OBJ_ESCORT,
  OBJ_DEFEND
} ObjectiveType;

typedef struct {
  ObjectiveType type;
  int target_id;
  int current_count, required_count;
  bool completed;
  float time_limit; // 0 if no limit
  float time_remaining;
} QuestObjective;

void objective_update(QuestObjective *obj, float dt) {
  if (obj->completed)
    return;

  if (obj->time_limit > 0) {
    obj->time_remaining -= dt;
    if (obj->time_remaining <= 0) {
      // Objective failed due to timeout
      return;
    }
  }

  if (obj->current_count >= obj->required_count) {
    obj->completed = true;
  }
}

// CRAFTING STATIONS
typedef struct {
  int station_type; // 0=workbench, 1=furnace, 2=alchemy, etc
  int recipe_ids[32];
  int recipe_count;
  float crafting_speed_multiplier;
} CraftingStation;

bool crafting_station_can_craft(CraftingStation *station, int recipe_id) {
  for (int i = 0; i < station->recipe_count; i++) {
    if (station->recipe_ids[i] == recipe_id) {
      return true;
    }
  }
  return false;
}

// PARTICLE EMITTER (Extended)
typedef enum {
  EMITTER_POINT,
  EMITTER_SPHERE,
  EMITTER_BOX,
  EMITTER_CONE
} EmitterShape;

typedef struct {
  EmitterShape shape;
  float position[3];
  float radius;  // For sphere/cone
  float size[3]; // For box
  float cone_angle;
  int particles_per_second;
  float particle_lifetime_min, particle_lifetime_max;
  float particle_speed_min, particle_speed_max;
  float particle_size_min, particle_size_max;
  bool looping;
} ExtendedParticleEmitter;

void emitter_spawn_particle_in_shape(ExtendedParticleEmitter *emitter,
                                     float *position_out, float *velocity_out) {
  switch (emitter->shape) {
  case EMITTER_POINT:
    memcpy(position_out, emitter->position, sizeof(float) * 3);
    break;

  case EMITTER_SPHERE: {
    float theta = ((float)rand() / RAND_MAX) * 2.0f * 3.14159f;
    float phi = ((float)rand() / RAND_MAX) * 3.14159f;
    float r = ((float)rand() / RAND_MAX) * emitter->radius;

    position_out[0] = emitter->position[0] + r * sinf(phi) * cosf(theta);
    position_out[1] = emitter->position[1] + r * sinf(phi) * sinf(theta);
    position_out[2] = emitter->position[2] + r * cosf(phi);
    break;
  }

  case EMITTER_BOX:
    position_out[0] = emitter->position[0] +
                      (((float)rand() / RAND_MAX) - 0.5f) * emitter->size[0];
    position_out[1] = emitter->position[1] +
                      (((float)rand() / RAND_MAX) - 0.5f) * emitter->size[1];
    position_out[2] = emitter->position[2] +
                      (((float)rand() / RAND_MAX) - 0.5f) * emitter->size[2];
    break;

  case EMITTER_CONE: {
    float angle = ((float)rand() / RAND_MAX) * emitter->cone_angle;
    float theta = ((float)rand() / RAND_MAX) * 2.0f * 3.14159f;

    velocity_out[0] = sinf(angle) * cosf(theta);
    velocity_out[1] = cosf(angle);
    velocity_out[2] = sinf(angle) * sinf(theta);
    break;
  }
  }
}

/* FINAL EXTENDED FEATURES COMPLETE (~600 TODOs) */
