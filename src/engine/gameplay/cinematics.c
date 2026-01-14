// cinematics.c - Implementation
#include <common.h>
#include "engine/include/core/logger.h"
#include <math.h>
#include "engine/include/math/math_all.h"
#include <math/vec3.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CINEMATIC_SEQUENCES 64
#define MAX_CINEMATIC_SHOTS 256
#define MAX_CINEMATIC_ACTORS 32
#define MAX_CINEMATIC_TRIGGERS 128

typedef enum {
  CINEMATIC_SHOT_STATIC = 0,
  CINEMATIC_SHOT_PAN,
  CINEMATIC_SHOT_TRACK,
  CINEMATIC_SHOT_DOLLY,
  CINEMATIC_SHOT_ORBIT,
  CINEMATIC_SHOT_FLYBY,
  CINEMATIC_SHOT_FOLLOW
} CinematicShotType;

typedef enum {
  CINEMATIC_TRIGGER_TIME = 0,
  CINEMATIC_TRIGGER_POSITION,
  CINEMATIC_TRIGGER_EVENT,
  CINEMATIC_TRIGGER_INPUT,
  CINEMATIC_TRIGGER_CONDITION
} CinematicTriggerType;

typedef enum {
  CINEMATIC_ACTION_CAMERA_MOVE = 0,
  CINEMATIC_ACTION_CAMERA_FOV,
  CINEMATIC_ACTION_ACTOR_MOVE,
  CINEMATIC_ACTION_ACTOR_ANIM,
  CINEMATIC_ACTION_PLAY_SOUND,
  CINEMATIC_ACTION_SHOW_TEXT,
  CINEMATIC_ACTION_FADE,
  CINEMATIC_ACTION_CUSTOM
} CinematicActionType;

typedef struct {
  Vec3 position;
  Vec3 rotation;
  f32 fov;
  f32 duration;
  CinematicShotType type;
  Vec3 target_position;
  Vec3 control_point;
  f32 speed;
  bool look_at_target;
} CinematicShot;

typedef struct {
  u32 entity_id;
  Vec3 start_position;
  Vec3 end_position;
  Vec3 start_rotation;
  Vec3 end_rotation;
  char animation[64];
  f32 start_time;
  f32 duration;
  bool is_active;
} CinematicActor;

typedef struct {
  CinematicTriggerType type;
  f32 trigger_time;
  Vec3 trigger_position;
  u32 trigger_event;
  char condition[128];
  bool triggered;
} CinematicTrigger;

typedef struct {
  CinematicActionType type;
  f32 execution_time;
  u32 target_id;
  Vec3 vector_data;
  f32 float_data;
  char string_data[256];
  bool executed;
} CinematicAction;

typedef struct {
  char name[64];
  CinematicShot shots[MAX_CINEMATIC_SHOTS];
  CinematicActor actors[MAX_CINEMATIC_ACTORS];
  CinematicTrigger triggers[MAX_CINEMATIC_TRIGGERS];
  CinematicAction actions[MAX_CINEMATIC_TRIGGERS];

  u32 shot_count;
  u32 actor_count;
  u32 trigger_count;
  u32 action_count;

  f32 total_duration;
  bool is_playing;
  f32 current_time;
  u32 current_shot;
  bool can_skip;
  bool skippable;
} CinematicSequence;

static CinematicSequence g_sequences[MAX_CINEMATIC_SEQUENCES];
static u32 g_sequence_count = 0;
static CinematicSequence *g_current_sequence = NULL;

// Forward declarations
void cinematic_stop_sequence(void);
void cinematic_update_actor(CinematicActor *actor, f32 delta_time);
void cinematic_check_trigger(CinematicTrigger *trigger);
void cinematic_execute_action(CinematicAction *action);

bool cinematic_init(void) {
  memset(g_sequences, 0, sizeof(g_sequences));
  g_sequence_count = 0;
  g_current_sequence = NULL;

  LOG_INFO("Cinematics system initialized");
  return true;
}

void cinematic_shutdown(void) {
  if (g_current_sequence) {
    cinematic_stop_sequence();
  }

  memset(g_sequences, 0, sizeof(g_sequences));
  g_sequence_count = 0;
  g_current_sequence = NULL;

  LOG_INFO("Cinematics system shutdown");
}

u32 cinematic_create_sequence(const char *name) {
  if (!name || g_sequence_count >= MAX_CINEMATIC_SEQUENCES) {
    return 0;
  }

  CinematicSequence *seq = &g_sequences[g_sequence_count];
  memset(seq, 0, sizeof(CinematicSequence));

  strncpy(seq->name, name, 63);
  seq->name[63] = '\0';
  seq->can_skip = true;
  seq->skippable = true;

  return ++g_sequence_count;
}

bool cinematic_add_shot(u32 sequence_id, const CinematicShot *shot) {
  if (sequence_id == 0 || sequence_id > g_sequence_count || !shot) {
    return false;
  }

  CinematicSequence *seq = &g_sequences[sequence_id - 1];
  if (seq->shot_count >= MAX_CINEMATIC_SHOTS) {
    return false;
  }

  seq->shots[seq->shot_count] = *shot;
  seq->total_duration += shot->duration;
  seq->shot_count++;

  return true;
}

bool cinematic_add_actor(u32 sequence_id, u32 entity_id, const Vec3 *start_pos,
                         const Vec3 *end_pos, const char *animation,
                         f32 duration) {
  if (sequence_id == 0 || sequence_id > g_sequence_count) {
    return false;
  }

  CinematicSequence *seq = &g_sequences[sequence_id - 1];
  if (seq->actor_count >= MAX_CINEMATIC_ACTORS) {
    return false;
  }

  CinematicActor *actor = &seq->actors[seq->actor_count];
  memset(actor, 0, sizeof(CinematicActor));

  actor->entity_id = entity_id;
  if (start_pos)
    actor->start_position = *start_pos;
  if (end_pos)
    actor->end_position = *end_pos;
  if (animation)
    strncpy(actor->animation, animation, 63);
  actor->duration = duration;
  actor->is_active = false;

  seq->actor_count++;
  return true;
}

bool cinematic_add_trigger(u32 sequence_id, CinematicTriggerType type,
                           f32 trigger_time, const Vec3 *position) {
  if (sequence_id == 0 || sequence_id > g_sequence_count) {
    return false;
  }

  CinematicSequence *seq = &g_sequences[sequence_id - 1];
  if (seq->trigger_count >= MAX_CINEMATIC_TRIGGERS) {
    return false;
  }

  CinematicTrigger *trigger = &seq->triggers[seq->trigger_count];
  memset(trigger, 0, sizeof(CinematicTrigger));

  trigger->type = type;
  trigger->trigger_time = trigger_time;
  if (position)
    trigger->trigger_position = *position;
  trigger->triggered = false;

  seq->trigger_count++;
  return true;
}

bool cinematic_add_action(u32 sequence_id, CinematicActionType type,
                          f32 execution_time, u32 target_id) {
  if (sequence_id == 0 || sequence_id > g_sequence_count) {
    return false;
  }

  CinematicSequence *seq = &g_sequences[sequence_id - 1];
  if (seq->action_count >= MAX_CINEMATIC_TRIGGERS) {
    return false;
  }

  CinematicAction *action = &seq->actions[seq->action_count];
  memset(action, 0, sizeof(CinematicAction));

  action->type = type;
  action->execution_time = execution_time;
  action->target_id = target_id;
  action->executed = false;

  seq->action_count++;
  return true;
}

bool cinematic_play_sequence(u32 sequence_id) {
  if (sequence_id == 0 || sequence_id > g_sequence_count) {
    LOG_ERROR("Invalid cinematic sequence ID: %d", sequence_id);
    return false;
  }

  if (g_current_sequence) {
    cinematic_stop_sequence();
  }

  g_current_sequence = &g_sequences[sequence_id - 1];
  g_current_sequence->is_playing = true;
  g_current_sequence->current_time = 0.0f;
  g_current_sequence->current_shot = 0;

  // Initialize actors
  for (u32 i = 0; i < g_current_sequence->actor_count; i++) {
    g_current_sequence->actors[i].is_active = true;
    g_current_sequence->actors[i].start_time = 0.0f;
  }

  // Reset triggers and actions
  for (u32 i = 0; i < g_current_sequence->trigger_count; i++) {
    g_current_sequence->triggers[i].triggered = false;
  }

  for (u32 i = 0; i < g_current_sequence->action_count; i++) {
    g_current_sequence->actions[i].executed = false;
  }

  LOG_INFO("Started cinematic sequence: %s", g_current_sequence->name);
  return true;
}

void cinematic_stop_sequence(void) {
  if (!g_current_sequence)
    return;

  g_current_sequence->is_playing = false;
  g_current_sequence->current_time = 0.0f;
  g_current_sequence->current_shot = 0;

  // Deactivate actors
  for (u32 i = 0; i < g_current_sequence->actor_count; i++) {
    g_current_sequence->actors[i].is_active = false;
  }

  LOG_INFO("Stopped cinematic sequence: %s", g_current_sequence->name);
  g_current_sequence = NULL;
}

bool cinematic_is_playing(void) {
  return g_current_sequence && g_current_sequence->is_playing;
}

void cinematic_update(f32 delta_time) {
  if (!g_current_sequence || !g_current_sequence->is_playing) {
    return;
  }

  g_current_sequence->current_time += delta_time;

  // Check if sequence is complete
  if (g_current_sequence->current_time >= g_current_sequence->total_duration) {
    cinematic_stop_sequence();
    return;
  }

  // Update current shot
  f32 shot_time = 0.0f;
  for (u32 i = 0; i < g_current_sequence->shot_count; i++) {
    if (shot_time + g_current_sequence->shots[i].duration >
        g_current_sequence->current_time) {
      g_current_sequence->current_shot = i;
      break;
    }
    shot_time += g_current_sequence->shots[i].duration;
  }

  // Update actors
  for (u32 i = 0; i < g_current_sequence->actor_count; i++) {
    cinematic_update_actor(&g_current_sequence->actors[i], delta_time);
  }

  // Check triggers
  for (u32 i = 0; i < g_current_sequence->trigger_count; i++) {
    cinematic_check_trigger(&g_current_sequence->triggers[i]);
  }

  // Execute actions
  for (u32 i = 0; i < g_current_sequence->action_count; i++) {
    cinematic_execute_action(&g_current_sequence->actions[i]);
  }
}

void cinematic_update_actor(CinematicActor *actor, f32 delta_time) {
  if (!actor || !actor->is_active)
    return;

  f32 elapsed = g_current_sequence->current_time - actor->start_time;
  f32 t = elapsed / actor->duration;

  if (t >= 1.0f) {
    actor->is_active = false;
    return;
  }

  // Smooth interpolation
  t = t * t * (3.0f - 2.0f * t);

  // Update position
  Vec3 current_pos;
  current_pos.x = actor->start_position.x +
                  (actor->end_position.x - actor->start_position.x) * t;
  current_pos.y = actor->start_position.y +
                  (actor->end_position.y - actor->start_position.y) * t;
  current_pos.z = actor->start_position.z +
                  (actor->end_position.z - actor->start_position.z) * t;

  // Update rotation
  Vec3 current_rot;
  current_rot.x = actor->start_rotation.x +
                  (actor->end_rotation.x - actor->start_rotation.x) * t;
  current_rot.y = actor->start_rotation.y +
                  (actor->end_rotation.y - actor->start_rotation.y) * t;
  current_rot.z = actor->start_rotation.z +
                  (actor->end_rotation.z - actor->start_rotation.z) * t;

  // Apply to entity (would integrate with entity system)
  LOG_DEBUG("Cinematic actor %d: pos(%.2f, %.2f, %.2f) rot(%.2f, %.2f, %.2f)",
            actor->entity_id, current_pos.x, current_pos.y, current_pos.z,
            current_rot.x, current_rot.y, current_rot.z);
}

void cinematic_check_trigger(CinematicTrigger *trigger) {
  if (!trigger || trigger->triggered)
    return;

  bool should_trigger = false;

  switch (trigger->type) {
  case CINEMATIC_TRIGGER_TIME:
    if (g_current_sequence->current_time >= trigger->trigger_time) {
      should_trigger = true;
    }
    break;

  case CINEMATIC_TRIGGER_POSITION:
    // Check player position (would integrate with player system)
    break;

  case CINEMATIC_TRIGGER_EVENT:
    // Check for specific event
    break;

  case CINEMATIC_TRIGGER_INPUT:
    // Check for input (would integrate with input system)
    break;

  case CINEMATIC_TRIGGER_CONDITION:
    // Evaluate condition
    break;

  default:
    break;
  }

  if (should_trigger) {
    trigger->triggered = true;
    LOG_DEBUG("Cinematic trigger activated at time %.2f",
              g_current_sequence->current_time);
  }
}

void cinematic_execute_action(CinematicAction *action) {
  if (!action || action->executed)
    return;

  if (g_current_sequence->current_time < action->execution_time) {
    return;
  }

  switch (action->type) {
  case CINEMATIC_ACTION_CAMERA_MOVE:
    LOG_DEBUG("Cinematic camera move to (%.2f, %.2f, %.2f)",
              action->vector_data.x, action->vector_data.y,
              action->vector_data.z);
    break;

  case CINEMATIC_ACTION_CAMERA_FOV:
    LOG_DEBUG("Cinematic camera FOV: %.2f", action->float_data);
    break;

  case CINEMATIC_ACTION_ACTOR_MOVE:
    LOG_DEBUG("Cinematic actor %d move to (%.2f, %.2f, %.2f)",
              action->target_id, action->vector_data.x, action->vector_data.y,
              action->vector_data.z);
    break;

  case CINEMATIC_ACTION_ACTOR_ANIM:
    LOG_DEBUG("Cinematic actor %d play animation: %s", action->target_id,
              action->string_data);
    break;

  case CINEMATIC_ACTION_PLAY_SOUND:
    LOG_DEBUG("Cinematic play sound: %s", action->string_data);
    break;

  case CINEMATIC_ACTION_SHOW_TEXT:
    LOG_DEBUG("Cinematic show text: %s", action->string_data);
    break;

  case CINEMATIC_ACTION_FADE:
    LOG_DEBUG("Cinematic fade: %.2f", action->float_data);
    break;

  case CINEMATIC_ACTION_CUSTOM:
    LOG_DEBUG("Cinematic custom action: %s", action->string_data);
    break;

  default:
    break;
  }

  action->executed = true;
}

bool cinematic_skip_sequence(void) {
  if (!g_current_sequence || !g_current_sequence->is_playing ||
      !g_current_sequence->skippable) {
    return false;
  }

  LOG_INFO("Skipped cinematic sequence: %s", g_current_sequence->name);
  cinematic_stop_sequence();
  return true;
}

void cinematic_set_skippable(u32 sequence_id, bool skippable) {
  if (sequence_id == 0 || sequence_id > g_sequence_count) {
    return;
  }

  g_sequences[sequence_id - 1].skippable = skippable;
}

CinematicSequence *cinematic_get_current_sequence(void) {
  return g_current_sequence;
}

u32 cinematic_find_sequence(const char *name) {
  if (!name)
    return 0;

  for (u32 i = 0; i < g_sequence_count; i++) {
    if (strcmp(g_sequences[i].name, name) == 0) {
      return i + 1;
    }
  }

  return 0;
}

void cinematic_get_current_camera(Vec3 *out_position, Vec3 *out_rotation,
                                  f32 *out_fov) {
  if (!g_current_sequence || !g_current_sequence->is_playing)
    return;

  if (g_current_sequence->current_shot >= g_current_sequence->shot_count)
    return;

  CinematicShot *shot =
      &g_current_sequence->shots[g_current_sequence->current_shot];

  // Calculate shot progress
  f32 shot_time = 0.0f;
  for (u32 i = 0; i < g_current_sequence->current_shot; i++) {
    shot_time += g_current_sequence->shots[i].duration;
  }

  f32 shot_progress =
      (g_current_sequence->current_time - shot_time) / shot->duration;
  shot_progress = fmaxf(0.0f, fminf(1.0f, shot_progress));

  // Smooth interpolation
  shot_progress = shot_progress * shot_progress * (3.0f - 2.0f * shot_progress);

  // Calculate camera position based on shot type
  Vec3 camera_pos = shot->position;
  Vec3 camera_rot = shot->rotation;

  switch (shot->type) {
  case CINEMATIC_SHOT_PAN:
    // Pan from position to target
    camera_pos.x = shot->position.x +
                   (shot->target_position.x - shot->position.x) * shot_progress;
    camera_pos.y = shot->position.y +
                   (shot->target_position.y - shot->position.y) * shot_progress;
    camera_pos.z = shot->position.z +
                   (shot->target_position.z - shot->position.z) * shot_progress;
    break;

  case CINEMATIC_SHOT_TRACK:
    // Track movement along path
    camera_pos.x = shot->position.x + shot->control_point.x * shot_progress;
    camera_pos.y = shot->position.y + shot->control_point.y * shot_progress;
    camera_pos.z = shot->position.z + shot->control_point.z * shot_progress;
    break;

  case CINEMATIC_SHOT_ORBIT: {
    // Orbit around target
    f32 angle = shot_progress * PI * 2.0f;
    f32 radius = vec3_distance(shot->position, shot->target_position);
    camera_pos.x = shot->target_position.x + cosf(angle) * radius;
    camera_pos.z = shot->target_position.z + sinf(angle) * radius;
    camera_pos.y = shot->position.y;
    break;
  }

  case CINEMATIC_SHOT_FOLLOW:
    // Follow target with offset
    camera_pos = shot->target_position;
    camera_pos.x += shot->position.x;
    camera_pos.y += shot->position.y;
    camera_pos.z += shot->position.z;
    break;

  default:
    break;
  }

  if (out_position)
    *out_position = camera_pos;
  if (out_rotation)
    *out_rotation = camera_rot;
  if (out_fov)
    *out_fov = shot->fov;
}

f32 cinematic_get_progress(void) {
  if (!g_current_sequence || !g_current_sequence->is_playing) {
    return 0.0f;
  }

  return g_current_sequence->current_time / g_current_sequence->total_duration;
}

void cinematic_get_info(char *out_info, u32 max_length) {
  if (!out_info || max_length == 0)
    return;

  if (!g_current_sequence) {
    snprintf(out_info, max_length, "No cinematic playing");
    return;
  }

  f32 progress = cinematic_get_progress() * 100.0f;

  snprintf(out_info, max_length,
           "Cinematic: %s, Shot %d/%d, %.1f%% complete, %.2fs/%.2fs",
           g_current_sequence->name, g_current_sequence->current_shot + 1,
           g_current_sequence->shot_count, progress,
           g_current_sequence->current_time,
           g_current_sequence->total_duration);
}
