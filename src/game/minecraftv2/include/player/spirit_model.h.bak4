// Spirit 3D Model Integration Plan
// This file defines the minimal integration plan for the 'spirit' 3D model
// without breaking the build.
//
// ASSET LOCATION: Spirit Character Animations
// ===========================================
// Animation files must be downloaded separately from Mixamo.
//
// Required Animation Files (GLTF 2.0 format):
//   assets/models/characters/spirit_idle.gltf    - Idle standing animation
//   assets/models/characters/spirit_walk.gltf    - Walking animation
//   assets/models/characters/spirit_run.gltf     - Running animation
//   assets/models/characters/spirit_jump.gltf    - Jumping animation
//   assets/models/characters/spirit_fall.gltf    - Falling animation
//   assets/models/characters/spirit_land.gltf    - Landing animation
//   assets/models/characters/spirit_attack.gltf  - Attack/interact animation
//
// How to Download Animations from Mixamo:
//   1. Go to: https://www.mixamo.com
//   2. Click "Animations" tab
//   3. Search for animation (e.g., "Idle", "Walking", "Running")
//   4. Select the "Y Bot" character (same as your base model)
//   5. Configure settings:
//      - Format: GLTF 2.0
//      - Skin: With Skin
//      - Frames per second: 30
//      - Keyframe Reduction: None (for quality) or Uniform (for smaller files)
//   6. Click "Download"
//   7. Extract and place in assets/models/characters/
//
// Animation Details:
//   - Idle: "Idle" or "Breathing Idle" (loops continuously)
//   - Walk: "Walking" (forward motion, loops)
//   - Run: "Running" (fast motion, loops)
//   - Jump: "Jumping Up" (one-shot)
//   - Fall: "Falling Idle" (loops while airborne)
//   - Land: "Hard Landing" (one-shot)
//   - Attack: "Sword Slash" or "Punch" (one-shot)
//
// See: docs/QUICK_START_ASSETS.md Part 1 (Step 1) for detailed tutorial
//

#ifndef SPIRIT_MODEL_H
#define SPIRIT_MODEL_H

#include "../game_common.h"
#include <math/quat.h>
#include <math/vec3.h>

// Spirit animation states (placeholder enum)
typedef enum {
  SPIRIT_ANIM_IDLE = 0,
  SPIRIT_ANIM_WALK,
  SPIRIT_ANIM_RUN,
  SPIRIT_ANIM_JUMP,
  SPIRIT_ANIM_FALL,
  SPIRIT_ANIM_LAND,
  SPIRIT_ANIM_INTERACT,
  SPIRIT_ANIM_COUNT
} SpiritAnimState;

// Spirit model component (placeholder for future 3D model rendering)
typedef struct {
  // Asset pipeline placeholder
  const char *model_path;   // Path to .gltf file (base character model)
  const char *texture_path; // Path to albedo/diffuse texture (included in GLTF)
  const char *material_path; // Path to material definition (future)

  // Rendering hook placeholder
  u32 mesh_id;    // Mesh ID in renderer (when implemented)
  u32 texture_id; // Texture ID in renderer (when implemented)
  bool visible;   // Whether to render the model
  Vec3 scale;     // Model scale factor

  // Basic animation state machine placeholder
  u32 current_anim;                      // Current animation clip ID
  f32 anim_time;                         // Current time in animation
  bool anim_playing;                     // Whether animation is playing
  bool anim_loop;                        // Whether to loop animation
  f32 anim_speed;                        // Playback speed multiplier
  f32 anim_durations[SPIRIT_ANIM_COUNT]; // Per-state durations (seconds)

  // Reserved for future skeletal animation
  void *skeleton;             // Placeholder for skeleton data
  void *animation_controller; // Placeholder for animation controller

  // LOD control (simple distance-based switching)
  f32 lod_near_distance;
  f32 lod_far_distance;
  u32 lod_level; // 0 = full, 1 = medium, 2 = far
} SpiritModelComponent;

// Initialize spirit model component (stub)
void spirit_model_init(SpiritModelComponent *spirit);

// Update spirit animation (stub)
void spirit_model_update_animation(SpiritModelComponent *spirit, f32 delta_time,
                                   SpiritAnimState state);

// Set spirit visibility (stub)
void spirit_model_set_visible(SpiritModelComponent *spirit, bool visible);

// Update LOD state based on camera distance
void spirit_model_update_lod(SpiritModelComponent *spirit, f32 distance);

// Cleanup spirit model resources (stub)
void spirit_model_cleanup(SpiritModelComponent *spirit);

#endif // SPIRIT_MODEL_H
