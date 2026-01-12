#include <audio/audio_loader.h>
#include <audio/audio_system.h>
#include <core/asset_manager.h>
#include <core/logger.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <vendor/miniaudio.h>

// Stubs for Audio Loader

const char *audio_get_sound_path(SoundType type) {
  switch (type) {
  case SOUND_FOOTSTEP_GRASS:
    return "assets/sounds/player/footsteps/grass_01.wav";
  case SOUND_FOOTSTEP_STONE:
    return "assets/sounds/player/footsteps/stone_01.wav";
  case SOUND_FOOTSTEP_WOOD:
    return "assets/sounds/player/footsteps/wood_01.wav";
  case SOUND_FOOTSTEP_SAND:
    return "assets/sounds/player/footsteps/sand_01.wav";
  case SOUND_FOOTSTEP_GRAVEL:
    return "assets/sounds/player/footsteps/stone_02.wav";
  case SOUND_FOOTSTEP_SNOW:
    return "assets/sounds/player/footsteps/sand_02.wav";
  case SOUND_BLOCK_BREAK:
    return "assets/sounds/blocks/stone_break.wav";
  case SOUND_BLOCK_PLACE:
    return "assets/sounds/blocks/stone_place.wav";
  case SOUND_SWORD_SWING:
    return "assets/sounds/player/combat/sword_swing.wav";
  case SOUND_SWORD_HIT:
    return "assets/sounds/player/combat/sword_hit.wav";
  case SOUND_BOW_DRAW:
    return "assets/sounds/player/combat/arrow_shoot.wav"; // Placeholder
  case SOUND_BOW_FIRE:
    return "assets/sounds/player/combat/arrow_shoot.wav";
  case SOUND_ITEM_PICKUP:
    return "assets/sounds/kenney_ui/Audio/click_001.ogg"; // Use UI click as placeholder
  case SOUND_ITEM_DROP:
    return "assets/sounds/kenney_ui/Audio/drop_001.ogg";
  case SOUND_MOB_ZOMBIE_GROAN:
    return "assets/sounds/mobs/zombie_groan_01.mp3";
  case SOUND_MOB_SKELETON_RATTLE:
    return "assets/sounds/mobs/skeleton_rattle_01.mp3";
  case SOUND_MOB_CREEPER_HISS:
    return "assets/sounds/mobs/creeper_hiss.mp3";
  case SOUND_MOB_SPIDER_SKITTER:
    return "assets/sounds/mobs/spider_skitter.mp3";
  case SOUND_MOB_COW_MOO:
    return "assets/sounds/mobs/cow_moo.mp3";
  case SOUND_MOB_PIG_OINK:
    return "assets/sounds/mobs/pig_oink.mp3";
  case SOUND_WATER_SPLASH:
    return "assets/sounds/blocks/water_splash.wav";
  case SOUND_FIRE_BURN:
    return "assets/sounds/placeholder.wav"; // No fire sound found
  case SOUND_AMBIENT_WIND:
    return "assets/sounds/ambient/wind_light.wav";
  case SOUND_AMBIENT_CAVE:
    return "assets/sounds/ambient/cave_01.wav";
  case SOUND_MUSIC_CALM:
    return "assets/sounds/kenney_rpg/Audio/bookFlip1.ogg"; // Placeholder
  case SOUND_MUSIC_ACTION:
    return "assets/sounds/kenney_rpg/Audio/bookFlip2.ogg"; // Placeholder
  case SOUND_RAIN_LIGHT:
    return "assets/sounds/ambient/rain_light.wav";
  case SOUND_RAIN_MODERATE:
    return "assets/sounds/ambient/rain_moderate.mp3";
  case SOUND_RAIN_HEAVY:
    return "assets/sounds/ambient/rain_heavy.wav";
  case SOUND_WIND_LIGHT:
    return "assets/sounds/ambient/wind_light.wav";
  case SOUND_WIND_MODERATE:
    return "assets/sounds/ambient/wind_moderate.mp3";
  case SOUND_WIND_HEAVY:
    return "assets/sounds/ambient/wind_heavy.wav";
  case SOUND_THUNDER_01:
    return "assets/sounds/ambient/thunder_01.wav";
  case SOUND_THUNDER_02:
    return "assets/sounds/ambient/thunder_02.wav";
  case SOUND_SNOW_LIGHT:
    return "assets/sounds/ambient/wind_light.wav"; // Placeholder
  case SOUND_SNOW_MODERATE:
    return "assets/sounds/ambient/wind_light.wav"; // Placeholder
  case SOUND_SNOW_HEAVY:
    return "assets/sounds/ambient/wind_heavy.wav"; // Placeholder
  case SOUND_FOG_LIGHT:
    return "assets/sounds/ambient/wind_light.wav"; // Placeholder
  case SOUND_FOG_MODERATE:
    return "assets/sounds/ambient/wind_light.wav"; // Placeholder
  case SOUND_FOG_HEAVY:
    return "assets/sounds/ambient/wind_heavy.wav"; // Placeholder
  case SOUND_PLAYER_HURT:
    return "assets/sounds/player/combat/hurt_01.wav";
  case SOUND_PLAYER_HEAL:
    return "assets/sounds/kenney_ui/Audio/maximize_001.ogg";
  case SOUND_EXPLOSION:
    return "assets/sounds/placeholder.wav"; // No explosion sound found
  case SOUND_CRAFTING_SUCCESS:
    return "assets/sounds/kenney_ui/Audio/confirmation_001.ogg";
  default:
    return "assets/sounds/placeholder.wav";
  }
}

void audio_load_all_sounds(struct AudioSystem *sys) {
  if (!sys)
    return;

  for (int i = 0; i < SOUND_COUNT; i++) {
    const char *path = audio_get_sound_path((SoundType)i);
    audio_load_sound_buffer(sys, (SoundType)i, path);
  }
}

ma_sound *audio_loader_load_sound(const char *filepath, ma_engine *engine,
                                  u32 flags) {
  if (!filepath || !engine)
    return NULL;

  ma_sound *sound = (ma_sound *)malloc(sizeof(ma_sound));
  if (!sound) {
    printf("Failed to allocate memory for sound: %s\n", filepath);
    return NULL;
  }

  ma_result result =
      ma_sound_init_from_file(engine, filepath, flags, NULL, NULL, sound);
  if (result != MA_SUCCESS) {
    printf("Failed to load sound file: %s (Error: %d)\n", filepath, result);
    free(sound);
    return NULL;
  }

  return sound;
}

void audio_loader_unload_sound(ma_sound *sound) {
  if (!sound)
    return;

  ma_sound_uninit(sound);
  free(sound);
}

ma_sound *audio_loader_stream_sound(const char *filepath, ma_engine *engine) {
  return audio_loader_load_sound(filepath, engine, MA_SOUND_FLAG_STREAM);
}

ma_sound *audio_loader_create_sound_from_asset(ma_engine *engine,
                                               Asset *asset) {
  if (!engine || !asset || !asset->data || !asset->loaded)
    return NULL;

  // We need a decoder to read from memory
  ma_decoder *decoder = (ma_decoder *)malloc(sizeof(ma_decoder));
  if (!decoder)
    return NULL;

  ma_result result =
      ma_decoder_init_memory(asset->data, asset->size, NULL, decoder);
  if (result != MA_SUCCESS) {
    printf("Failed to init memory decoder for asset %s: %d\n", asset->id,
           result);
    free(decoder);
    return NULL;
  }

  // Now init sound from data source (the decoder)
  ma_sound *sound = (ma_sound *)malloc(sizeof(ma_sound));
  if (!sound) {
    ma_decoder_uninit(decoder);
    free(decoder);
    return NULL;
  }

  result = ma_sound_init_from_data_source(engine, decoder, 0, NULL, sound);
  if (result != MA_SUCCESS) {
    printf("Failed to init sound from asset %s: %d\n", asset->id, result);
    ma_decoder_uninit(decoder);
    free(decoder);
    free(sound);
    return NULL;
  }

  return sound;
}
