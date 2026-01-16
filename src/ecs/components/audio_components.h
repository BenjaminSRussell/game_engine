#ifndef AUDIO_COMPONENTS_H
#define AUDIO_COMPONENTS_H

#include "core/types.h"
#include "engine/include/math/math_all.h"

/**
 * ECS Audio Components
 *
 * - AudioSourceComponent: Emits sound (3D or 2D)
 * - AudioListenerComponent: Receives sound (usually on Camera)
 */

// ============================================================================
// AUDIO SOURCE COMPONENT
// ============================================================================

typedef struct {
  u32 sound_id; // Resource ID

  f32 volume; // 0.0 to 1.0
  f32 pitch;  // 1.0 = normal speed

  bool is_3d;         // 3D positioning enabled?
  bool loop;          // Loop indefinitely?
  bool play_on_awake; // Auto-start?

  // Spatial parameters
  f32 min_distance;   // Full volume distance
  f32 max_distance;   // Silence distance
  f32 rolloff_factor; // Attenuation curve

  // State
  bool is_playing;
  bool is_paused;
  u32 voice_id; // Internal voice handle
} AudioSourceComponent;

// ============================================================================
// AUDIO LISTENER COMPONENT
// ============================================================================

typedef struct {
  f32 master_volume; // 0.0 to 1.0
  bool enabled;      // Listener active?
  u32 listener_id;   // Internal listener handle
} AudioListenerComponent;

// ============================================================================
// COMPONENT REGISTRATION
// ============================================================================

extern u32 g_audio_source_component_id;
extern u32 g_audio_listener_component_id;

/**
 * Register audio components with ECS world
 */
void register_audio_components(void *world);

/**
 * Helper: Create 2D audio source
 */
AudioSourceComponent audio_source_create_2d(u32 sound_id, f32 volume,
                                            bool loop);

/**
 * Helper: Create 3D audio source
 */
AudioSourceComponent audio_source_create_3d(u32 sound_id, f32 volume,
                                            f32 min_dist, f32 max_dist);

/**
 * Helper: Create audio listener
 */
AudioListenerComponent audio_listener_create(void);

#endif // AUDIO_COMPONENTS_H
