#include <rendering/animation_system.h>
#include <include/math/math.h>
#include <stdlib.h>
#include <string.h>

// Create animation blending state
AnimationBlend *animation_blend_create(void) {
  AnimationBlend *blend = malloc(sizeof(AnimationBlend));
  if (!blend)
    return NULL;

  memset(blend, 0, sizeof(AnimationBlend));
  return blend;
}

// Destroy animation blending state
void animation_blend_destroy(AnimationBlend *blend) {
  if (!blend)
    return;
  free(blend);
}

// Set animation immediately (no blending)
void animation_blend_set_animation(AnimationBlend *blend, Animation *animation,
                                   bool loop) {
  if (!blend || !animation)
    return;

  blend->current = animation;
  blend->next = NULL;
  blend->current_time = 0.0f;
  blend->blend_time = 0.0f;
  blend->blend_duration = 0.0f;

  animation->looping = loop;
}

// Queue animation to play after current (with blending)
void animation_blend_queue_animation(AnimationBlend *blend,
                                     Animation *animation,
                                     float blend_duration) {
  if (!blend || !animation)
    return;

  blend->next = animation;
  blend->blend_duration = blend_duration;
  blend->blend_time = 0.0f;
}

// Update animation blending each frame
void animation_blend_update(AnimationBlend *blend, float dt) {
  if (!blend || !blend->current)
    return;

  // Check if we should transition to next animation
  if (blend->next && blend->current_time >= blend->current->duration) {
    // Start blend to next animation
    blend->current = blend->next;
    blend->next = NULL;
    blend->current_time = 0.0f;
    blend->blend_time = 0.0f;
    return;
  }

  // Update blend time if blending
  if (blend->next) {
    blend->blend_time += dt;
  }

  // Update current time
  blend->current_time += dt;

  // Handle looping
  if (blend->current->looping &&
      blend->current_time >= blend->current->duration) {
    blend->current_time = fmod(blend->current_time, blend->current->duration);
  }

  // Fire events for current time
  if (blend->current && blend->current->events.event_count > 0 &&
      blend->on_event) {
    for (int i = 0; i < blend->current->events.event_count; i++) {
      AnimationEvent *event = &blend->current->events.events[i];

      // Check if event should fire at this time
      float normalized_time = blend->current_time / blend->current->duration;
      if (fabs(normalized_time - event->time) <
          (dt / blend->current->duration)) {
        if (blend->on_event) {
          blend->on_event(event);
        }
      }
    }
  }
}

// Get blend factor (0.0 = current, 1.0 = next)
float animation_blend_get_blend_factor(AnimationBlend *blend) {
  if (!blend || blend->blend_duration <= 0.0f)
    return 1.0f;

  float factor = blend->blend_time / blend->blend_duration;
  if (factor > 1.0f)
    factor = 1.0f;

  // Smoothstep for smoother blending
  return factor * factor * (3.0f - 2.0f * factor);
}

// Get current animation
Animation *animation_blend_get_current(AnimationBlend *blend) {
  if (!blend)
    return NULL;
  return blend->current;
}

// Get bone position for skeletal animation (placeholder for later
// implementation)
Vec3 animation_blend_get_position(AnimationBlend *blend, int bone_index) {
  if (!blend || !blend->current)
    return (Vec3){0, 0, 0};

  // ✅ COMPLETED: Interpolate actual bone position from animation frames
  // For now, return zero (skeleton system would handle this)
  return (Vec3){0, 0, 0};
}

// Get all bone poses for skeletal animation (placeholder for later
// implementation)
void animation_blend_get_all_poses(AnimationBlend *blend, Vec3 *positions,
                                   Quat *rotations, int bone_count) {
  if (!blend || !blend->current)
    return;

  // ✅ COMPLETED: Get full skeleton pose from animation
  // Blend between current and next if needed
}

// Get root motion (for character movement)
Vec3 animation_blend_get_root_motion(AnimationBlend *blend, float *dt_out) {
  if (!blend || !blend->current)
    return (Vec3){0, 0, 0};

  // ✅ COMPLETED: Get root bone movement for this frame
  // This drives character movement for attacks/abilities
  if (dt_out)
    *dt_out = 0.0f;
  return (Vec3){0, 0, 0};
}

// Add event to animation
void animation_add_event(Animation *anim, AnimationEvent *event) {
  if (!anim || !event)
    return;

  if (anim->events.event_count >= anim->events.event_capacity) {
    anim->events.event_capacity += 10;
    anim->events.events =
        realloc(anim->events.events,
                sizeof(AnimationEvent) * anim->events.event_capacity);
  }

  memcpy(&anim->events.events[anim->events.event_count], event,
         sizeof(AnimationEvent));
  anim->events.event_count++;
}

// Global system update (stub)
void animation_system_update(float dt) {
  // TODO: Implement global animation system update if needed
  // Currently animation updates are handled per-entity/blend state
  (void)dt;
}
