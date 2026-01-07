#ifndef ANIMATION_SYSTEM_H
#define ANIMATION_SYSTEM_H

#include <common.h>
#include "math/vec3.h"
#include "../math/quat.h"

#ifdef __cplusplus
extern "C" {
#endif

// Animation event types
typedef enum {
    ANIMATION_EVENT_SOUND,
    ANIMATION_EVENT_PARTICLE,
    ANIMATION_EVENT_DAMAGE,
    ANIMATION_EVENT_FOOTSTEP,
    ANIMATION_EVENT_COUNT
} AnimationEventType;

// Single animation event
typedef struct {
    float time;                    // When in animation (0-1, normalized)
    AnimationEventType type;
    void (*callback)(void *context, void *data);
    void *data;
} AnimationEvent;

// List of events in an animation
typedef struct {
    AnimationEvent *events;
    int event_count;
    int event_capacity;
} AnimationEventList;

// Single animation clip
typedef struct {
    float current_time;
    float duration;
    AnimationEventList events;
    bool looping;
} Animation;

// Animation blending state - manages smooth transitions between animations
typedef struct {
    Animation *current;
    Animation *next;

    float current_time;            // Time in current animation
    float blend_time;              // How long we've been blending
    float blend_duration;          // Total blend time

    // Callbacks
    void (*on_event)(AnimationEvent *event);
    void *context;
} AnimationBlend;

// Create animation blending state
AnimationBlend *animation_blend_create(void);

// Destroy animation blending state
void animation_blend_destroy(AnimationBlend *blend);

// Set animation immediately (no blending)
void animation_blend_set_animation(AnimationBlend *blend, Animation *animation, bool loop);

// Queue animation to play after current (with blending)
void animation_blend_queue_animation(AnimationBlend *blend, Animation *animation, float blend_duration);

// Update animation blending each frame
void animation_blend_update(AnimationBlend *blend, float dt);

// Get current blended animation
Animation* animation_blend_get_current(AnimationBlend *blend);

// Get blend factor (0.0 = current, 1.0 = next)
float animation_blend_get_blend_factor(AnimationBlend *blend);

// Get bone position for skeletal animation (placeholder)
Vec3 animation_blend_get_position(AnimationBlend *blend, int bone_index);

// Get all bone poses for skeletal animation
void animation_blend_get_all_poses(AnimationBlend *blend, Vec3 *positions, Quat *rotations, int bone_count);

// Get root motion (for character movement)
Vec3 animation_blend_get_root_motion(AnimationBlend *blend, float *dt_out);

// Add event to animation
void animation_add_event(Animation *anim, AnimationEvent *event);

#ifdef __cplusplus
}
#endif

#endif // ANIMATION_SYSTEM_H
