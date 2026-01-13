#ifndef MUSIC_SYSTEM_H
#define MUSIC_SYSTEM_H

#include "include/audio/audio_system.h"
#include "engine/include/common.h"
#include <math/vec3.h>

// Music mood types for dynamic selection
typedef enum {
  MOOD_MENU,
  MOOD_CALM_EXPLORATION,
  MOOD_ADVENTURE,
  MOOD_TENSION,
  MOOD_COMBAT,
  MOOD_BOSS,
  MOOD_NIGHT,
  MOOD_UNDERGROUND,
  MOOD_WATER,
  MOOD_COUNT
} MusicMood;

// Music context for situational awareness
typedef enum {
  CONTEXT_MAIN_MENU,
  CONTEXT_WORLD_GENERATION,
  CONTEXT_EXPLORATION,
  CONTEXT_BUILDING,
  CONTEXT_COMBAT,
  CONTEXT_BOSS_FIGHT,
  CONTEXT_MINING,
  CONTEXT_UNDERWATER,
  CONTEXT_NETHER,
  CONTEXT_END,
  CONTEXT_COUNT
} MusicContext;

// Music track metadata
typedef struct {
  char filepath[256];
  char title[128];
  char artist[128];
  f32 duration_seconds;
  MusicMood mood;
  MusicContext context;
  f32 energy_level; // 0.0 (calm) to 1.0 (intense)
  f32 tempo;        // BPM
  bool has_intro;
  bool has_loop;
  f32 loop_start_time; // seconds
  f32 loop_end_time;   // seconds
} MusicTrack;

// Music system state
typedef struct MusicSystem {
  bool initialized;
  MusicTrack *tracks;
  u32 track_count;

  // Current playback state
  u32 current_track_index;
  u32 next_track_index;
  bool is_playing;
  bool is_transitioning;
  f32 transition_time;
  f32 transition_duration;

  // Dynamic music control
  MusicMood current_mood;
  MusicContext current_context;
  f32 combat_intensity; // 0.0 to 1.0
  f32 time_of_day;      // 0.0 to 1.0 (dawn to dawn)
  Vec3 player_position;
  f32 underground_level; // 0.0 = surface, 1.0 = deep underground

  // Audio system references
  struct AudioSystem *audio_engine;
  u32 music_channel;
  u32 transition_channel;

  // Playlist management
  u32 *playlist;
  u32 playlist_size;
  u32 playlist_index;
  bool shuffle_enabled;
  bool repeat_enabled;

  // Volume and mixing
  f32 master_volume;
  f32 mood_volumes[MOOD_COUNT];
  f32 context_volumes[CONTEXT_COUNT];
  f32 transition_volume;

  // Statistics
  f32 total_play_time;
  u32 tracks_played_count;
} MusicSystem;

// Music system lifecycle
void music_system_init(MusicSystem *music, AudioSystem *audio);
void music_system_free(MusicSystem *music);
void music_system_update(MusicSystem *music, f32 delta_time);

// Dynamic music control
void music_set_mood(MusicSystem *music, MusicMood mood);
void music_set_context(MusicSystem *music, MusicContext context);
void music_set_combat_intensity(MusicSystem *music, f32 intensity);
void music_set_time_of_day(MusicSystem *music, f32 time_of_day);
void music_set_player_position(MusicSystem *music, Vec3 position);
void music_set_underground_level(MusicSystem *music, f32 level);

// Playback control
void music_play_track(MusicSystem *music, u32 track_index);
void music_play_next(MusicSystem *music);
void music_play_previous(MusicSystem *music);
void music_pause(MusicSystem *music);
void music_resume(MusicSystem *music);
void music_stop(MusicSystem *music);

// Playlist management
void music_create_playlist(MusicSystem *music, MusicMood mood,
                           MusicContext context);
void music_add_to_playlist(MusicSystem *music, u32 track_index);
void music_clear_playlist(MusicSystem *music);
void music_set_shuffle(MusicSystem *music, bool enabled);
void music_set_repeat(MusicSystem *music, bool enabled);

// Volume control
void music_set_master_volume(MusicSystem *music, f32 volume);
void music_set_mood_volume(MusicSystem *music, MusicMood mood, f32 volume);
void music_set_context_volume(MusicSystem *music, MusicContext context,
                              f32 volume);

// Track management
u32 music_load_track(MusicSystem *music, const char *filepath, MusicMood mood,
                     MusicContext context, f32 energy_level);
MusicTrack *music_get_track(MusicSystem *music, u32 index);
u32 music_find_suitable_track(MusicSystem *music, MusicMood mood,
                              MusicContext context);

// Transitions
void music_fade_to_track(MusicSystem *music, u32 track_index, f32 duration);
void music_crossfade(MusicSystem *music, f32 duration);

// Utility functions
const char *music_get_mood_name(MusicMood mood);
const char *music_get_context_name(MusicContext context);
bool music_is_track_suitable(MusicTrack *track, MusicMood mood,
                             MusicContext context);

#endif
