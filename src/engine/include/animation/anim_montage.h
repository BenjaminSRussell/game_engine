// Animation/Animation Montage System
#ifndef ANIM_MONTAGE_H
#define ANIM_MONTAGE_H

#include <common.h>
#import <Foundation/Foundation.h>

#define MAX_MONTAGE_SECTIONS 32
#define MAX_MONTAGE_NOTIFIES 64

// Animation Notifies (Events during animation)
typedef enum {
  NOTIFY_SOUND,
  NOTIFY_PARTICLE,
  NOTIFY_SCRIPT_EVENT,
  NOTIFY_STATE_CHANGE
} AnimNotifyType;

typedef struct {
  char name[64];
  f32 time;     // Time in seconds from start of montage
  f32 duration; // If > 0, it's a NotifyState
  AnimNotifyType type;

  // Payload
  char asset_path[128];
  u32 int_param;
  f32 float_param;

  bool triggered; // Runtime flag
} AnimNotify;

// Montage Section (Named part of animation)
typedef struct {
  char name[64];
  f32 start_time;
  f32 end_time;
  char next_section[64]; // Default next section, or empty for stop
} MontageSection;

// The Montage Definition
typedef struct {
  char name[128];
  f32 total_duration;
  f32 blend_in_time;
  f32 blend_out_time;

  // Underlying animations (sequence of clips)
  // Simplified: assuming single track for now
  void *animation_sequence;

  MontageSection sections[MAX_MONTAGE_SECTIONS];
  u32 section_count;

  AnimNotify notifies[MAX_MONTAGE_NOTIFIES];
  u32 notify_count;

  // Slot name (e.g., "FullBody", "UpperBody") for layering
  char slot_name[32];

} AnimMontage;

// Runtime Instance
typedef struct {
  AnimMontage *montage;
  bool is_playing;
  f32 current_time;
  f32 play_rate;
  f32 blend_weight;

  char current_section[64];

  // Callback function pointer for events
  void (*on_notify)(AnimNotify *notify, void *user_data);
  void *user_data;

} MontageInstance;

#ifdef __cplusplus
extern "C" {
#endif

AnimMontage *montage_create(const char *name);
void montage_destroy(AnimMontage *montage);

// Authoring
int montage_add_section(AnimMontage *montage, const char *name, f32 start,
                        f32 end);
void montage_set_next_section(AnimMontage *montage, const char *current,
                              const char *next);
int montage_add_notify(AnimMontage *montage, f32 time, AnimNotifyType type);

// Runtime
MontageInstance *montage_play(AnimMontage *montage, f32 in_play_rate);
void montage_stop(MontageInstance *instance, f32 blend_out);
void montage_update(MontageInstance *instance, f32 delta_time);
void montage_jump_to_section(MontageInstance *instance,
                             const char *section_name);

bool montage_is_playing(MontageInstance *instance);
const char *montage_get_current_section(MontageInstance *instance);

#ifdef __cplusplus
}
#endif

#endif // ANIM_MONTAGE_H
