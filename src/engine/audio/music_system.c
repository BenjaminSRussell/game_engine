// src/audio/music_system.c
// Dynamic music system with mood-based track selection and seamless transitions

#include "audio/music_system.h"
#include <include/math/math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TRACKS 128
#define DEFAULT_TRANSITION_DURATION 3.0f
#define COMBAT_INTENSITY_THRESHOLD 0.3f

// Internal helper functions
static f32 music_calculate_suitability_score(MusicTrack *track, MusicMood mood,
                                             MusicContext context,
                                             f32 intensity);
static void music_select_next_track(MusicSystem *music);
static void music_update_transition(MusicSystem *music, f32 delta_time);
static void music_shuffle_playlist(MusicSystem *music);

void music_system_init(MusicSystem *music, AudioSystem *audio) {
  if (!music || !audio)
    return;

  memset(music, 0, sizeof(MusicSystem));
  music->initialized = true;

  // Allocate track array
  music->tracks = (MusicTrack *)calloc(MAX_TRACKS, sizeof(MusicTrack));
  music->track_count = 0;

  // Initialize playback state
  music->current_track_index = 0xFFFFFFFF;
  music->next_track_index = 0xFFFFFFFF;
  music->is_playing = false;
  music->is_transitioning = false;
  music->transition_duration = DEFAULT_TRANSITION_DURATION;

  // Initialize dynamic music control
  music->current_mood = MOOD_CALM_EXPLORATION;
  music->current_context = CONTEXT_EXPLORATION;
  music->combat_intensity = 0.0f;
  music->time_of_day = 0.5f; // Noon
  music->player_position = vec3(0.0f, 0.0f, 0.0f);
  music->underground_level = 0.0f;

  // Initialize audio channels
  music->music_channel = 0xFFFFFFFF;
  music->transition_channel = 0xFFFFFFFF;

  // Initialize playlist
  music->playlist = (u32 *)calloc(MAX_TRACKS, sizeof(u32));
  music->playlist_size = 0;
  music->playlist_index = 0;
  music->shuffle_enabled = false;
  music->repeat_enabled = true;

  // Initialize volumes
  music->master_volume = 0.7f;
  for (u32 i = 0; i < MOOD_COUNT; i++) {
    music->mood_volumes[i] = 1.0f;
  }
  for (u32 i = 0; i < CONTEXT_COUNT; i++) {
    music->context_volumes[i] = 1.0f;
  }
  music->transition_volume = 0.0f;

  // Load built-in music tracks
  music_load_track(music, "assets/music/overworld/calm_04_wallpaper.mp3",
                   MOOD_CALM_EXPLORATION, CONTEXT_EXPLORATION, 0.2f);
  music_load_track(music, "assets/music/overworld/calm_05_carefree.mp3",
                   MOOD_CALM_EXPLORATION, CONTEXT_EXPLORATION, 0.15f);
  music_load_track(music,
                   "assets/music/overworld/calm_06_floating_cities.mp3",
                   MOOD_CALM_EXPLORATION, CONTEXT_EXPLORATION, 0.25f);
  music_load_track(
      music, "assets/music/overworld/adventure_01_ascending_vale.mp3",
      MOOD_ADVENTURE, CONTEXT_EXPLORATION, 0.4f);
  music_load_track(
      music, "assets/music/overworld/adventure_02_enchanted_journey.mp3",
      MOOD_ADVENTURE, CONTEXT_EXPLORATION, 0.35f);
  music_load_track(music,
                   "assets/music/combat/battle_03_volatile_reaction.mp3",
                   MOOD_COMBAT, CONTEXT_COMBAT, 0.8f);
  music_load_track(music,
                   "assets/music/combat/battle_04_darkest_child.mp3",
                   MOOD_COMBAT, CONTEXT_COMBAT, 0.9f);
  music_load_track(music,
                   "assets/music/combat/battle_05_teller_tales.mp3",
                   MOOD_COMBAT, CONTEXT_COMBAT, 0.7f);
  music_load_track(music, "assets/music/menu/menu_02_meditation.mp3",
                   MOOD_MENU, CONTEXT_MAIN_MENU, 0.1f);

  // Create initial playlist
  music_create_playlist(music, music->current_mood, music->current_context);

         music->track_count);
}

void music_system_free(MusicSystem *music) {
  if (!music || !music->initialized)
    return;

  if (music->tracks) {
    free(music->tracks);
    music->tracks = NULL;
  }

  if (music->playlist) {
    free(music->playlist);
    music->playlist = NULL;
  }

  music->initialized = false;
}

void music_system_update(MusicSystem *music, f32 delta_time) {
  if (!music || !music->initialized)
    return;

  // Update transitions
  music_update_transition(music, delta_time);

  // Check if we need to change music based on game state
  static f32 last_combat_intensity = 0.0f;
  static MusicMood last_mood = MOOD_CALM_EXPLORATION;

  // Combat intensity changes
  if (fabsf(music->combat_intensity - last_combat_intensity) >
      COMBAT_INTENSITY_THRESHOLD) {
    if (music->combat_intensity > 0.5f) {
      music_set_mood(music, MOOD_COMBAT);
    } else if (music->combat_intensity < 0.2f) {
      music_set_mood(music, MOOD_CALM_EXPLORATION);
    }
    last_combat_intensity = music->combat_intensity;
  }

  // Time of day affects mood
  if (music->time_of_day < 0.25f || music->time_of_day > 0.75f) {
    if (last_mood != MOOD_NIGHT &&
        music->current_context == CONTEXT_EXPLORATION) {
      music_set_mood(music, MOOD_NIGHT);
      last_mood = MOOD_NIGHT;
    }
  } else if (music->current_context == CONTEXT_EXPLORATION) {
    if (last_mood != MOOD_CALM_EXPLORATION) {
      music_set_mood(music, MOOD_CALM_EXPLORATION);
      last_mood = MOOD_CALM_EXPLORATION;
    }
  }

  // Underground affects mood
  if (music->underground_level > 0.5f) {
    if (last_mood != MOOD_UNDERGROUND) {
      music_set_mood(music, MOOD_UNDERGROUND);
      last_mood = MOOD_UNDERGROUND;
    }
  }

  // Check if current track finished and play next
  if (music->is_playing && music->current_track_index != 0xFFFFFFFF) {
    // This would need to be integrated with the audio system to check if track
    // finished For now, we'll use a simple timer-based approach
    music->total_play_time += delta_time;

    MusicTrack *current = &music->tracks[music->current_track_index];
    if (music->total_play_time > current->duration_seconds) {
      music_play_next(music);
    }
  }
}

void music_set_mood(MusicSystem *music, MusicMood mood) {
  if (!music || !music->initialized || mood >= MOOD_COUNT)
    return;

  if (music->current_mood != mood) {
    music->current_mood = mood;
    music_create_playlist(music, mood, music->current_context);

    // If we're playing, transition to a suitable track
    if (music->is_playing) {
      u32 suitable_track =
          music_find_suitable_track(music, mood, music->current_context);
      if (suitable_track != 0xFFFFFFFF) {
        music_fade_to_track(music, suitable_track, DEFAULT_TRANSITION_DURATION);
      }
    }

  }
}

void music_set_context(MusicSystem *music, MusicContext context) {
  if (!music || !music->initialized || context >= CONTEXT_COUNT)
    return;

  if (music->current_context != context) {
    music->current_context = context;
    music_create_playlist(music, music->current_mood, context);

    // If we're playing, transition to a suitable track
    if (music->is_playing) {
      u32 suitable_track =
          music_find_suitable_track(music, music->current_mood, context);
      if (suitable_track != 0xFFFFFFFF) {
        music_fade_to_track(music, suitable_track, DEFAULT_TRANSITION_DURATION);
      }
    }

  }
}

void music_set_combat_intensity(MusicSystem *music, f32 intensity) {
  if (!music)
    return;

  music->combat_intensity = fmaxf(0.0f, fminf(1.0f, intensity));
}

void music_set_time_of_day(MusicSystem *music, f32 time_of_day) {
  if (!music)
    return;

  music->time_of_day = fmaxf(0.0f, fminf(1.0f, time_of_day));
}

void music_set_player_position(MusicSystem *music, Vec3 position) {
  if (!music)
    return;

  music->player_position = position;

  // Update underground level based on Y position
  f32 sea_level = 64.0f; // Assuming sea level at Y=64
  f32 depth = fmaxf(0.0f, sea_level - position.y);
  music->underground_level =
      fminf(1.0f, depth / 100.0f); // 100 blocks = fully underground
}

void music_set_underground_level(MusicSystem *music, f32 level) {
  if (!music)
    return;

  music->underground_level = fmaxf(0.0f, fminf(1.0f, level));
}

void music_play_track(MusicSystem *music, u32 track_index) {
  if (!music || !music->initialized || track_index >= music->track_count)
    return;

  music->current_track_index = track_index;
  music->is_playing = true;
  music->total_play_time = 0.0f;

  MusicTrack *track = &music->tracks[track_index];

  // This would integrate with the audio system to actually play the track
  // For now, we'll just update the state
}

void music_play_next(MusicSystem *music) {
  if (!music || !music->initialized)
    return;

  if (music->playlist_size == 0)
    return;

  music->playlist_index = (music->playlist_index + 1) % music->playlist_size;

  // If we've looped back to the start and shuffle is enabled, reshuffle
  if (music->playlist_index == 0 && music->shuffle_enabled) {
    music_shuffle_playlist(music);
  }

  u32 track_index = music->playlist[music->playlist_index];
  music_play_track(music, track_index);
}

void music_play_previous(MusicSystem *music) {
  if (!music || !music->initialized)
    return;

  if (music->playlist_size == 0)
    return;

  if (music->playlist_index == 0) {
    music->playlist_index = music->playlist_size - 1;
  } else {
    music->playlist_index--;
  }

  u32 track_index = music->playlist[music->playlist_index];
  music_play_track(music, track_index);
}

void music_pause(MusicSystem *music) {
  if (!music || !music->initialized)
    return;

  music->is_playing = false;
}

void music_resume(MusicSystem *music) {
  if (!music || !music->initialized)
    return;

  music->is_playing = true;
}

void music_stop(MusicSystem *music) {
  if (!music || !music->initialized)
    return;

  music->is_playing = false;
  music->current_track_index = 0xFFFFFFFF;
  music->total_play_time = 0.0f;
}

void music_create_playlist(MusicSystem *music, MusicMood mood,
                           MusicContext context) {
  if (!music || !music->initialized)
    return;

  music_clear_playlist(music);

  // Add all suitable tracks to playlist
  for (u32 i = 0; i < music->track_count; i++) {
    if (music_is_track_suitable(&music->tracks[i], mood, context)) {
      music_add_to_playlist(music, i);
    }
  }

  if (music->shuffle_enabled) {
    music_shuffle_playlist(music);
  }

         music->playlist_size, music_get_mood_name(mood),
         music_get_context_name(context));
}

void music_add_to_playlist(MusicSystem *music, u32 track_index) {
  if (!music || !music->initialized || track_index >= music->track_count)
    return;

  if (music->playlist_size < MAX_TRACKS) {
    music->playlist[music->playlist_size++] = track_index;
  }
}

void music_clear_playlist(MusicSystem *music) {
  if (!music || !music->initialized)
    return;

  music->playlist_size = 0;
  music->playlist_index = 0;
}

void music_set_shuffle(MusicSystem *music, bool enabled) {
  if (!music || !music->initialized)
    return;

  music->shuffle_enabled = enabled;

  if (enabled) {
    music_shuffle_playlist(music);
  }

}

void music_set_repeat(MusicSystem *music, bool enabled) {
  if (!music || !music->initialized)
    return;

  music->repeat_enabled = enabled;
}

void music_set_master_volume(MusicSystem *music, f32 volume) {
  if (!music)
    return;

  music->master_volume = fmaxf(0.0f, fminf(1.0f, volume));
}

void music_set_mood_volume(MusicSystem *music, MusicMood mood, f32 volume) {
  if (!music || mood >= MOOD_COUNT)
    return;

  music->mood_volumes[mood] = fmaxf(0.0f, fminf(1.0f, volume));
}

void music_set_context_volume(MusicSystem *music, MusicContext context,
                              f32 volume) {
  if (!music || context >= CONTEXT_COUNT)
    return;

  music->context_volumes[context] = fmaxf(0.0f, fminf(1.0f, volume));
}

u32 music_load_track(MusicSystem *music, const char *filepath, MusicMood mood,
                     MusicContext context, f32 energy_level) {
  if (!music || !music->initialized || !filepath ||
      music->track_count >= MAX_TRACKS) {
    return 0xFFFFFFFF;
  }

  u32 index = music->track_count++;
  MusicTrack *track = &music->tracks[index];

  // Initialize track metadata
  strncpy(track->filepath, filepath, sizeof(track->filepath) - 1);
  track->filepath[sizeof(track->filepath) - 1] = '\0';

  // Extract filename as title (simple implementation)
  const char *filename = strrchr(filepath, '/');
  if (filename) {
    filename++;
  } else {
    filename = filepath;
  }

  // Remove extension
  char title[128];
  strncpy(title, filename, sizeof(title) - 1);
  char *dot = strrchr(title, '.');
  if (dot)
    *dot = '\0';

  strncpy(track->title, title, sizeof(track->title) - 1);
  track->title[sizeof(track->title) - 1] = '\0';

  strcpy(track->artist, "Kevin MacLeod");
  track->mood = mood;
  track->context = context;
  track->energy_level = energy_level;
  track->tempo = 120.0f; // Default tempo
  track->has_intro = false;
  track->has_loop = true;
  track->loop_start_time = 10.0f;
  track->loop_end_time = 180.0f;

  // Try to get actual duration from file (simplified)
  track->duration_seconds = 180.0f; // Default 3 minutes

         music_get_mood_name(mood), energy_level);

  return index;
}

MusicTrack *music_get_track(MusicSystem *music, u32 index) {
  if (!music || !music->initialized || index >= music->track_count) {
    return NULL;
  }

  return &music->tracks[index];
}

u32 music_find_suitable_track(MusicSystem *music, MusicMood mood,
                              MusicContext context) {
  if (!music || !music->initialized)
    return 0xFFFFFFFF;

  f32 best_score = -1.0f;
  u32 best_track = 0xFFFFFFFF;

  for (u32 i = 0; i < music->track_count; i++) {
    MusicTrack *track = &music->tracks[i];
    if (music_is_track_suitable(track, mood, context)) {
      f32 score = music_calculate_suitability_score(track, mood, context,
                                                    music->combat_intensity);
      if (score > best_score) {
        best_score = score;
        best_track = i;
      }
    }
  }

  return best_track;
}

void music_fade_to_track(MusicSystem *music, u32 track_index, f32 duration) {
  if (!music || !music->initialized || track_index >= music->track_count)
    return;

  music->next_track_index = track_index;
  music->is_transitioning = true;
  music->transition_time = 0.0f;
  music->transition_duration = duration;
  music->transition_volume = 0.0f;

         music->tracks[track_index].title);
}

void music_crossfade(MusicSystem *music, f32 duration) {
  if (!music || !music->initialized)
    return;

  // This would implement a true crossfade between two tracks
  // For now, we'll use the simpler fade_to_track approach
  if (music->playlist_size > 0) {
    music_play_next(music);
  }
}

// Helper function implementations
static f32 music_calculate_suitability_score(MusicTrack *track, MusicMood mood,
                                             MusicContext context,
                                             f32 intensity) {
  f32 score = 0.0f;

  // Base score for matching mood and context
  if (track->mood == mood)
    score += 10.0f;
  if (track->context == context)
    score += 10.0f;

  // Energy level matching
  f32 energy_diff = fabsf(track->energy_level - intensity);
  score += (1.0f - energy_diff) * 5.0f;

  // Add some randomness to avoid repetition
  score += ((f32)rand() / RAND_MAX) * 2.0f;

  return score;
}

static void music_select_next_track(MusicSystem *music) {
  if (!music || music->playlist_size == 0)
    return;

  // Simple selection - could be made more sophisticated
  music->playlist_index = (music->playlist_index + 1) % music->playlist_size;
}

static void music_update_transition(MusicSystem *music, f32 delta_time) {
  if (!music->is_transitioning)
    return;

  music->transition_time += delta_time;
  f32 progress = music->transition_time / music->transition_duration;

  if (progress >= 1.0f) {
    // Transition complete
    music->is_transitioning = false;
    music_play_track(music, music->next_track_index);
    music->next_track_index = 0xFFFFFFFF;
    music->transition_volume = 0.0f;
  } else {
    // Update transition volume (fade in)
    music->transition_volume = progress;
  }
}

static void music_shuffle_playlist(MusicSystem *music) {
  if (!music || music->playlist_size <= 1)
    return;

  // Fisher-Yates shuffle
  for (u32 i = music->playlist_size - 1; i > 0; i--) {
    u32 j = rand() % (i + 1);
    u32 temp = music->playlist[i];
    music->playlist[i] = music->playlist[j];
    music->playlist[j] = temp;
  }
}

// Utility functions
const char *music_get_mood_name(MusicMood mood) {
  static const char *mood_names[] = {"Menu",    "Calm Exploration", "Adventure",
                                     "Tension", "Combat",           "Boss",
                                     "Night",   "Underground",      "Water"};

  if (mood >= MOOD_COUNT)
    return "Unknown";
  return mood_names[mood];
}

const char *music_get_context_name(MusicContext context) {
  static const char *context_names[] = {
      "Main Menu",  "World Generation", "Exploration", "Building", "Combat",
      "Boss Fight", "Mining",           "Underwater",  "Nether",   "End"};

  if (context >= CONTEXT_COUNT)
    return "Unknown";
  return context_names[context];
}

bool music_is_track_suitable(MusicTrack *track, MusicMood mood,
                             MusicContext context) {
  if (!track)
    return false;

  return (track->mood == mood || track->context == context);
}
