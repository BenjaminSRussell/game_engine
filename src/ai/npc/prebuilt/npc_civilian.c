/**
 * =================================================================================================
 *                          NPC CIVILIAN BEHAVIOR
 * =================================================================================================
 *
 * Ambient crowd NPCs with daily routines.
 *
 * =================================================================================================
 */

#include <include/math/math.h>
#include <ai/npc/prebuilt/npc_civilian.h>
#include <stdbool.h>
#include <stdlib.h>

typedef enum {
  ACTIVITY_SLEEP,
  ACTIVITY_EAT,
  ACTIVITY_WORK,
  ACTIVITY_SOCIALIZE,
  ACTIVITY_WANDER
} ActivityState;

typedef struct {
  float position[3];
  float velocity[3];
  ActivityState current_activity;
  float activity_timer;
  bool is_fleeing;
  float flee_timer;

  // Appearance
  int clothing_variant;
  int hair_variant;
  float height_scale;
} CivilianNPC;

// Activity State Machine
void civilian_update_activity(CivilianNPC *npc, float time_of_day, float dt) {
  npc->activity_timer -= dt;

  if (npc->activity_timer <= 0.0f) {
    // Transition based on time of day
    if (time_of_day < 6.0f || time_of_day > 22.0f) {
      npc->current_activity = ACTIVITY_SLEEP;
      npc->activity_timer = 300.0f; // 5 minutes
    } else if (time_of_day >= 7.0f && time_of_day < 8.0f) {
      npc->current_activity = ACTIVITY_EAT;
      npc->activity_timer = 60.0f;
    } else if (time_of_day >= 9.0f && time_of_day < 17.0f) {
      npc->current_activity = ACTIVITY_WORK;
      npc->activity_timer = 120.0f;
    } else {
      // Evening - socialize or wander
      if (rand() % 2 == 0) {
        npc->current_activity = ACTIVITY_SOCIALIZE;
        npc->activity_timer = 90.0f;
      } else {
        npc->current_activity = ACTIVITY_WANDER;
        npc->activity_timer = 45.0f;
      }
    }
  }
}

// Fleeing Behavior
void civilian_flee(CivilianNPC *npc, float *danger_pos, float dt) {
  if (!npc->is_fleeing)
    return;

  // Run away from danger
  float dx = npc->position[0] - danger_pos[0];
  float dz = npc->position[2] - danger_pos[2];
  float dist = sqrtf(dx * dx + dz * dz);

  if (dist < 20.0f) {
    float flee_speed = 6.0f;
    npc->velocity[0] = (dx / dist) * flee_speed;
    npc->velocity[2] = (dz / dist) * flee_speed;

    // Find cover or exit
    // navmesh_find_nearest_cover(npc->position);
  } else {
    // Safe distance reached
    npc->flee_timer -= dt;
    if (npc->flee_timer <= 0.0f) {
      npc->is_fleeing = false;
    }
  }
}

// Crowd Avoidance (Boids)
void civilian_avoid_crowd(CivilianNPC *npc, CivilianNPC *others, int count,
                          float dt) {
  float separation[3] = {0, 0, 0};
  float alignment[3] = {0, 0, 0};
  float cohesion[3] = {0, 0, 0};
  int neighbors = 0;

  for (int i = 0; i < count; i++) {
    if (&others[i] == npc)
      continue;

    float dx = npc->position[0] - others[i].position[0];
    float dz = npc->position[2] - others[i].position[2];
    float dist = sqrtf(dx * dx + dz * dz);

    if (dist < 2.0f) { // Personal space
      // Separation - avoid getting too close
      separation[0] += dx / (dist + 0.001f);
      separation[2] += dz / (dist + 0.001f);
      neighbors++;
    }

    if (dist < 5.0f) { // Neighborhood
      // Alignment - match velocity
      alignment[0] += others[i].velocity[0];
      alignment[2] += others[i].velocity[2];

      // Cohesion - move towards center
      cohesion[0] += others[i].position[0];
      cohesion[2] += others[i].position[2];
    }
  }

  if (neighbors > 0) {
    // Apply boid forces
    npc->velocity[0] += separation[0] * 2.0f * dt;
    npc->velocity[2] += separation[2] * 2.0f * dt;
  }
}

// Random Appearance Generation
void civilian_randomize_appearance(CivilianNPC *npc) {
  // Mix and match clothing/hair parts
  npc->clothing_variant = rand() % 10; // 10 clothing sets
  npc->hair_variant = rand() % 8;      // 8 hairstyles

  // Variation in height/scale (90% to 110%)
  npc->height_scale = 0.9f + ((float)(rand() % 20) / 100.0f);
}

void civilian_init(CivilianNPC *npc) {
  civilian_randomize_appearance(npc);
  npc->current_activity = ACTIVITY_WANDER;
  npc->activity_timer = 30.0f;
  npc->is_fleeing = false;
}

void civilian_update(CivilianNPC *npc, float time_of_day, float dt) {
  civilian_update_activity(npc, time_of_day, dt);

  // Apply velocity
  npc->position[0] += npc->velocity[0] * dt;
  npc->position[1] += npc->velocity[1] * dt;
  npc->position[2] += npc->velocity[2] * dt;
}
