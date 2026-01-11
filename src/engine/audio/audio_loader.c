#include "include/audio/audio_loader.h"
#include "include/core/asset_manager.h"
#include <audio/audio_system.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

// Stubs for Audio Loader

const char *audio_get_sound_path(SoundType type) {
  switch (type) {
  case SOUND_FOOTSTEP_GRASS:
    return "assets/sounds/footstep_grass.wav";
  case SOUND_FOOTSTEP_STONE:
    return "assets/sounds/footstep_stone.wav";
  case SOUND_FOOTSTEP_WOOD:
    return "assets/sounds/footstep_wood.wav";
  case SOUND_FOOTSTEP_SAND:
    return "assets/sounds/footstep_sand.wav";
  case SOUND_FOOTSTEP_GRAVEL:
    return "assets/sounds/footstep_gravel.wav";
  case SOUND_FOOTSTEP_SNOW:
    return "assets/sounds/footstep_snow.wav";
  case SOUND_BLOCK_BREAK:
    return "assets/sounds/block_break.wav";
  case SOUND_BLOCK_PLACE:
    return "assets/sounds/block_place.wav";
  case SOUND_SWORD_SWING:
    return "assets/sounds/sword_swing.wav";
  case SOUND_SWORD_HIT:
    return "assets/sounds/sword_hit.wav";
  case SOUND_BOW_DRAW:
    return "assets/sounds/bow_draw.wav";
  case SOUND_BOW_FIRE:
    return "assets/sounds/bow_fire.wav";
  case SOUND_ITEM_PICKUP:
    return "assets/sounds/item_pickup.wav";
  case SOUND_ITEM_DROP:
    return "assets/sounds/item_drop.wav";
  case SOUND_MOB_ZOMBIE_GROAN:
    return "assets/sounds/zombie_groan.wav";
  case SOUND_MOB_SKELETON_RATTLE:
    return "assets/sounds/skeleton_rattle.wav";
  case SOUND_MOB_CREEPER_HISS:
    return "assets/sounds/creeper_hiss.wav";
  case SOUND_MOB_SPIDER_SKITTER:
    return "assets/sounds/spider_skitter.wav";
  case SOUND_MOB_COW_MOO:
    return "assets/sounds/cow_moo.wav";
  case SOUND_MOB_PIG_OINK:
    return "assets/sounds/pig_oink.wav";
  case SOUND_WATER_SPLASH:
    return "assets/sounds/water_splash.wav";
  case SOUND_FIRE_BURN:
    return "assets/sounds/fire_burn.wav";
  case SOUND_AMBIENT_WIND:
    return "assets/sounds/ambient_wind.wav";
  case SOUND_AMBIENT_CAVE:
    return "assets/sounds/ambient_cave.wav";
  case SOUND_MUSIC_CALM:
    return "assets/sounds/music_calm.wav";
  case SOUND_MUSIC_ACTION:
    return "assets/sounds/music_action.wav";
  case SOUND_RAIN_LIGHT:
    return "assets/sounds/rain_light.wav";
  case SOUND_RAIN_MODERATE:
    return "assets/sounds/rain_moderate.wav";
  case SOUND_RAIN_HEAVY:
    return "assets/sounds/rain_heavy.wav";
  case SOUND_WIND_LIGHT:
    return "assets/sounds/wind_light.wav";
  case SOUND_WIND_MODERATE:
    return "assets/sounds/wind_moderate.wav";
  case SOUND_WIND_HEAVY:
    return "assets/sounds/wind_heavy.wav";
  case SOUND_THUNDER_01:
    return "assets/sounds/thunder_01.wav";
  case SOUND_THUNDER_02:
    return "assets/sounds/thunder_02.wav";
  case SOUND_SNOW_LIGHT:
    return "assets/sounds/snow_light.wav";
  case SOUND_SNOW_MODERATE:
    return "assets/sounds/snow_moderate.wav";
  case SOUND_SNOW_HEAVY:
    return "assets/sounds/snow_heavy.wav";
  case SOUND_FOG_LIGHT:
    return "assets/sounds/fog_light.wav";
  case SOUND_FOG_MODERATE:
    return "assets/sounds/fog_moderate.wav";
  case SOUND_FOG_HEAVY:
    return "assets/sounds/fog_heavy.wav";
  case SOUND_PLAYER_HURT:
    return "assets/sounds/player_hurt.wav";
  case SOUND_PLAYER_HEAL:
    return "assets/sounds/player_heal.wav";
  case SOUND_EXPLOSION:
    return "assets/sounds/explosion.wav";
  case SOUND_CRAFTING_SUCCESS:
    return "assets/sounds/crafting_success.wav";
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
