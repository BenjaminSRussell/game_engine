// Spirit 3D Model Integration Stubs
// This file implements stubs for the 'spirit' 3D model integration without
// breaking the build. Roadmap: docs/SPIRIT_MODEL_ROADMAP.md.

// TODO: Implement spirit model loading system.
// TODO: Implement spirit model texture system.
// TODO: Implement spirit model optimization system.
// TODO: Add spirit model validation system.
// TODO: Implement spirit model statistics tracking.
// TODO: Add spirit model debugging visualization.
// TODO: Implement spirit model unit testing framework.
// TODO: Add spirit model documentation system.
#include <player/spirit_model.h>
#include <string.h>

void spirit_model_init(SpiritModelComponent *spirit) {
  if (!spirit)
    return;
  memset(spirit, 0, sizeof(SpiritModelComponent));

  // ASSET LOCATION: Spirit Character Model
  // Download from: https://www.mixamo.com
  // Steps:
  //   1. Create free Adobe account
  //   2. Search for "Y Bot" character
  //   3. Download as GLTF 2.0 format (with T-Pose)
  //   4. Place at: assets/models/characters/spirit.gltf
  //   5. Ensure .bin file is also placed: assets/models/characters/spirit.bin
  //
  // ASSET LOCATION: Spirit Texture
  //   - Texture is included in GLTF download
  //   - Place at: assets/textures/characters/spirit.png
  //   - Or use YBot_diffuse.png from Mixamo download
  //
  // Scale: Mixamo models are typically large, scale down to 0.01
  // See: docs/QUICK_START_ASSETS.md for detailed instructions

  spirit->model_path = "assets/models/characters/spirit.gltf";
  spirit->texture_path = "assets/textures/characters/spirit.png";
  spirit->scale = vec3(0.01f, 0.01f, 0.01f); // Scale down Mixamo models
  spirit->visible = true;
  spirit->current_anim = SPIRIT_ANIM_IDLE;
  spirit->anim_loop = true;
  spirit->anim_playing = true;
  spirit->anim_speed = 1.0f;
  spirit->anim_durations[SPIRIT_ANIM_IDLE] = 2.0f;
  spirit->anim_durations[SPIRIT_ANIM_WALK] = 1.0f;
  spirit->anim_durations[SPIRIT_ANIM_RUN] = 0.8f;
  spirit->anim_durations[SPIRIT_ANIM_JUMP] = 0.5f;
  spirit->anim_durations[SPIRIT_ANIM_FALL] = 1.0f;
  spirit->anim_durations[SPIRIT_ANIM_LAND] = 0.4f;
  spirit->anim_durations[SPIRIT_ANIM_INTERACT] = 0.6f;

  spirit->lod_near_distance = 12.0f;
  spirit->lod_far_distance = 30.0f;
  spirit->lod_level = 0;
}

void spirit_model_update_animation(SpiritModelComponent *spirit, f32 delta_time,
                                   SpiritAnimState state) {
  if (!spirit)
    return;

  if (spirit->current_anim != state) {
    spirit->current_anim = state;
    spirit->anim_time = 0.0f;
    spirit->anim_playing = true;
    switch (state) {
    case SPIRIT_ANIM_JUMP:
    case SPIRIT_ANIM_LAND:
    case SPIRIT_ANIM_INTERACT:
      spirit->anim_loop = false;
      break;
    default:
      spirit->anim_loop = true;
      break;
    }
  }

  if (!spirit->anim_playing) {
    return;
  }

  f32 duration = spirit->anim_durations[state];
  if (duration <= 0.0f) {
    duration = 1.0f;
  }

  spirit->anim_time += delta_time * spirit->anim_speed;
  if (spirit->anim_loop) {
    while (spirit->anim_time > duration) {
      spirit->anim_time -= duration;
    }
  } else if (spirit->anim_time >= duration) {
    spirit->anim_time = duration;
    spirit->anim_playing = false;
  }
}

void spirit_model_set_visible(SpiritModelComponent *spirit, bool visible) {
  if (!spirit)
    return;
  spirit->visible = visible;
}

void spirit_model_update_lod(SpiritModelComponent *spirit, f32 distance) {
  if (!spirit)
    return;

  if (distance > spirit->lod_far_distance) {
    spirit->lod_level = 2;
  } else if (distance > spirit->lod_near_distance) {
    spirit->lod_level = 1;
  } else {
    spirit->lod_level = 0;
  }
}

void spirit_model_cleanup(SpiritModelComponent *spirit) {
  if (!spirit)
    return;
  memset(spirit, 0, sizeof(SpiritModelComponent));
}
