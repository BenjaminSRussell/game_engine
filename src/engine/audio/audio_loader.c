// src/audio/audio_loader.c
// Sound file loading implementation for the audio system
#include <audio/audio_system.h>
#include <stdio.h>
#include <string.h>

// Sound file path mapping table
static const char *SOUND_FILE_PATHS[SOUND_COUNT] = {
    [SOUND_FOOTSTEP_GRASS] = "assets/sounds/kenney_rpg/Audio/footstep00.ogg",
    [SOUND_FOOTSTEP_STONE] = "assets/sounds/kenney_rpg/Audio/footstep01.ogg",
    [SOUND_FOOTSTEP_WOOD] = "assets/sounds/kenney_rpg/Audio/footstep02.ogg",
    [SOUND_FOOTSTEP_SAND] = "assets/sounds/kenney_rpg/Audio/footstep03.ogg",
    [SOUND_FOOTSTEP_GRAVEL] = "assets/sounds/kenney_rpg/Audio/footstep04.ogg",
    [SOUND_FOOTSTEP_SNOW] = "assets/sounds/kenney_rpg/Audio/footstep05.ogg",
    [SOUND_BLOCK_BREAK] = "assets/sounds/kenney_rpg/Audio/chop.ogg",
    [SOUND_BLOCK_PLACE] = "assets/sounds/kenney_rpg/Audio/bookPlace1.ogg",
    [SOUND_SWORD_SWING] = "assets/sounds/kenney_rpg/Audio/drawKnife1.ogg",
    [SOUND_SWORD_HIT] = "assets/sounds/kenney_rpg/Audio/knifeSlice2.ogg",
    [SOUND_BOW_DRAW] = "assets/sounds/kenney_rpg/Audio/handleSmallLeather.ogg",
    [SOUND_BOW_FIRE] = "assets/sounds/kenney_digital/Audio/laser1.ogg",
    [SOUND_ITEM_PICKUP] = "assets/sounds/kenney_rpg/Audio/handleCoins.ogg",
    [SOUND_ITEM_DROP] = "assets/sounds/kenney_rpg/Audio/dropLeather.ogg",
    [SOUND_MOB_ZOMBIE_GROAN] = "assets/sounds/kenney_rpg/Audio/creak1.ogg",
    [SOUND_MOB_SKELETON_RATTLE] =
        "assets/sounds/kenney_rpg/Audio/metalClick.ogg",
    [SOUND_MOB_CREEPER_HISS] = "assets/sounds/kenney_digital/Audio/laser7.ogg",
    [SOUND_MOB_SPIDER_SKITTER] = "assets/sounds/kenney_rpg/Audio/clothBelt.ogg",
    [SOUND_MOB_COW_MOO] = "assets/sounds/kenney_rpg/Audio/creak2.ogg",
    [SOUND_MOB_PIG_OINK] = "assets/sounds/kenney_rpg/Audio/creak3.ogg",
    [SOUND_WATER_SPLASH] = "assets/sounds/kenney_rpg/Audio/metalPot1.ogg",
    [SOUND_FIRE_BURN] = "assets/sounds/kenney_digital/Audio/laser4.ogg",
    [SOUND_AMBIENT_WIND] = "assets/sounds/kenney_digital/Audio/laser5.ogg",
    [SOUND_AMBIENT_CAVE] = "assets/sounds/kenney_digital/Audio/laser6.ogg",
    [SOUND_MUSIC_CALM] = "assets/sounds/kenney_rpg/Preview.ogg",
    [SOUND_MUSIC_ACTION] = "assets/sounds/kenney_rpg/Preview.ogg",
    // Weather sounds
    [SOUND_RAIN_LIGHT] = "assets/sounds/kenney_digital/Audio/laser8.ogg",
    [SOUND_RAIN_MODERATE] = "assets/sounds/kenney_digital/Audio/laser9.ogg",
    [SOUND_RAIN_HEAVY] = "assets/sounds/kenney_digital/Audio/laser9.ogg",
    [SOUND_WIND_LIGHT] = "assets/sounds/kenney_digital/Audio/laser5.ogg",
    [SOUND_WIND_MODERATE] = "assets/sounds/kenney_digital/Audio/laser5.ogg",
    [SOUND_WIND_HEAVY] = "assets/sounds/kenney_digital/Audio/laser5.ogg",
    [SOUND_THUNDER_01] = "assets/sounds/kenney_rpg/Audio/metalPot2.ogg",
    [SOUND_THUNDER_02] = "assets/sounds/kenney_rpg/Audio/metalPot3.ogg",
    [SOUND_SNOW_LIGHT] = "assets/sounds/kenney_rpg/Audio/cloth1.ogg",
    [SOUND_SNOW_MODERATE] = "assets/sounds/kenney_rpg/Audio/cloth2.ogg",
    [SOUND_SNOW_HEAVY] = "assets/sounds/kenney_rpg/Audio/cloth3.ogg",
    [SOUND_FOG_LIGHT] = "assets/sounds/kenney_digital/Audio/laser6.ogg",
    [SOUND_FOG_MODERATE] = "assets/sounds/kenney_digital/Audio/laser6.ogg",
    [SOUND_FOG_HEAVY] = "assets/sounds/kenney_digital/Audio/laser6.ogg",
    // Player sounds
    [SOUND_PLAYER_HURT] =
        "assets/sounds/kenney_rpg/Audio/handleSmallLeather2.ogg",
    [SOUND_PLAYER_HEAL] = "assets/sounds/kenney_digital/Audio/powerUp1.ogg",
    [SOUND_EXPLOSION] = "assets/sounds/kenney_rpg/Audio/metalPot2.ogg",
    [SOUND_CRAFTING_SUCCESS] = "assets/sounds/kenney_ui/Audio/click_003.ogg",
};

const char *audio_get_sound_path(SoundType sound) {
  if (sound >= SOUND_COUNT) {
    return NULL;
  }
  return SOUND_FILE_PATHS[sound];
}

void audio_load_all_sounds(AudioSystem *sys) {
  if (!sys || !sys->initialized) {
    fprintf(stderr, "[AUDIO] Cannot load sounds: system not initialized\n");
    return;
  }

  u32 loaded_count = 0;
  u32 failed_count = 0;

  fprintf(stderr, "[AUDIO] Loading sound assets...\n");

  for (u32 i = 0; i < SOUND_COUNT; i++) {
    const char *path = SOUND_FILE_PATHS[i];
    if (path && audio_load_sound_buffer(sys, (SoundType)i, path)) {
      loaded_count++;
    } else if (path) {
      failed_count++;
      fprintf(stderr, "[AUDIO] Failed to load sound %u from: %s\n", i, path);
    }
  }

  fprintf(stderr, "[AUDIO] Loaded %u/%u sounds (%u failed)\n", loaded_count,
          loaded_count + failed_count, failed_count);
}
