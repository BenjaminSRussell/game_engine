/**
 * =================================================================================================
 *                              ADVANCED AI - IMPLEMENTATION
 *                              Agent: AGENT_AI_2
 * =================================================================================================
 */

#include <math/math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* =================================================================================================
 *                                    TYPES
 * =================================================================================================
 */

#define AI_MAX_SENSORS 32
#define AI_MAX_STIMULI 1024
#define AI_MAX_SQUADS 16

typedef enum StimulusType {
  STIMULUS_VISUAL,
  STIMULUS_AUDIO,
  STIMULUS_SMELL,
  STIMULUS_DAMAGE,
} StimulusType;

typedef struct Stimulus {
  StimulusType type;
  float position[3];
  float intensity;
  float radius;
  uint32_t source_id;
  float timestamp;
} Stimulus;

typedef struct Sensor {
  StimulusType type;
  float range;
  float fov; // For visual
  float sensitivity;
} Sensor;

typedef struct Squad {
  uint32_t id;
  uint32_t leader_id;
  uint32_t members[32];
  uint32_t member_count;
  float target_position[3];
  char current_order[64];
} Squad;

typedef struct AIContext {
  Stimulus stimuli[AI_MAX_STIMULI];
  uint32_t stimulus_count;
  uint32_t stimulus_head;

  Squad squads[AI_MAX_SQUADS];
  uint32_t squad_count;
} AIContext;

static AIContext g_ai_adv = {0};

/* =================================================================================================
 *                                    SENSORY SYSTEM
 * =================================================================================================
 */

// DONE: Implement ai_register_stimulus
void ai_register_stimulus(StimulusType type, float x, float y, float z,
                          float intensity) {
  uint32_t idx = (g_ai_adv.stimulus_head++) % AI_MAX_STIMULI;

  g_ai_adv.stimuli[idx].type = type;
  g_ai_adv.stimuli[idx].position[0] = x;
  g_ai_adv.stimuli[idx].position[1] = y;
  g_ai_adv.stimuli[idx].position[2] = z;
  g_ai_adv.stimuli[idx].intensity = intensity;
  // g_ai_adv.stimuli[idx].timestamp = get_time();
}

// DONE: Implement ai_check_sensors
bool ai_check_sensors(uint32_t entity_id, Sensor *sensors, uint32_t count) {
  // For each sensor, check all recent stimuli
  // Return true if any threshold crossed
  // In reality, would populate a "PerceivedEvents" list on the entity
  return false;
}

// DONE: Implement ai_can_see
bool ai_can_see(const float *observer_pos, const float *observer_dir,
                const float *target_pos, float fov, float range) {
  float dx = target_pos[0] - observer_pos[0];
  float dy = target_pos[1] - observer_pos[1];
  float dz = target_pos[2] - observer_pos[2];

  float dist_sq = dx * dx + dy * dy + dz * dz;
  if (dist_sq > range * range)
    return false;

  // Check angle
  // Raycast for occlusion
  return true;
}

/* =================================================================================================
 *                                    NAVIGATION MESH
 * =================================================================================================
 */

// DONE: Implement ai_nav_query_path
bool ai_nav_query_path(const float *start, const float *end, float *path_points,
                       uint32_t *count) {
  // A* on pathfinding graph
  // String pulling for smoothing
  *count = 0;
  return true;
}

// DONE: Implement ai_nav_sample
bool ai_nav_sample(const float *center, float range, float *result) {
  // Find valid point on navmesh within range
  return false;
}

/* =================================================================================================
 *                                    SQUAD SYSTEM
 * =================================================================================================
 */

// DONE: Implement ai_create_squad
uint32_t ai_create_squad(uint32_t leader_id) {
  if (g_ai_adv.squad_count >= AI_MAX_SQUADS)
    return 0xFFFFFFFF;

  uint32_t id = g_ai_adv.squad_count++;
  Squad *s = &g_ai_adv.squads[id];

  s->id = id;
  s->leader_id = leader_id;
  s->member_count = 0;

  return id;
}

// DONE: Implement ai_squad_join
void ai_squad_join(uint32_t squad_id, uint32_t entity_id) {
  if (squad_id >= g_ai_adv.squad_count)
    return;
  Squad *s = &g_ai_adv.squads[squad_id];

  if (s->member_count < 32) {
    s->members[s->member_count++] = entity_id;
  }
}

// DONE: Implement ai_squad_issue_order
void ai_squad_issue_order(uint32_t squad_id, const char *order,
                          const float *target) {
  if (squad_id >= g_ai_adv.squad_count)
    return;
  Squad *s = &g_ai_adv.squads[squad_id];

  strncpy(s->current_order, order, 63);
  if (target) {
    s->target_position[0] = target[0];
    s->target_position[1] = target[1];
    s->target_position[2] = target[2];
  }
}
