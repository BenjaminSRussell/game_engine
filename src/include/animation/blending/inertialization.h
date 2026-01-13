/**
 * @file inertialization.h
 * @brief Smooth animation transition using inertialization.
 */
#ifndef ANIMATION_BLENDING_INERTIALIZATION_H
#define ANIMATION_BLENDING_INERTIALIZATION_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize inertialization system.
 */
void inertialization_init(void);

/**
 * Start a smooth transition from one pose to another.
 */
void inertialization_start_transition(void *from_pose, void *to_pose);

/**
 * Update inertialization and output blended pose.
 */
void inertialization_update(float dt, void *output_pose);

#ifdef __cplusplus
}
#endif

#endif // ANIMATION_BLENDING_INERTIALIZATION_H
