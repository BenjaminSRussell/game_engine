/**
 * WORLD STREAMING SYSTEM
 * Async Level Loading & Unloading
 */

#include <stdbool.h>

typedef enum {
  LEVEL_UNLOADED,
  LEVEL_LOADING,
  LEVEL_LOADED,
  LEVEL_UNLOADING
} LevelState;

typedef struct {
  char name[64];
  float bounds[6];
  LevelState state;
  void *asset_handles;
  int ref_count;
} StreamingLevel;

typedef struct {
  StreamingLevel *levels;
  int count;
  float player_pos[3];
  float stream_distance;
} WorldStreamer;

// Update
void streaming_update(WorldStreamer *ws) {
  for (int i = 0; i < ws->count; i++) {
    StreamingLevel *lvl = &ws->levels[i];

    // Calculate distance to player
    float dx = lvl->bounds[0] - ws->player_pos[0];
    float dz = lvl->bounds[2] - ws->player_pos[2];
    float dist = sqrtf(dx * dx + dz * dz);

    if (dist < ws->stream_distance && lvl->state == LEVEL_UNLOADED) {
      // Start async load
      lvl->state = LEVEL_LOADING;
    } else if (dist > ws->stream_distance * 1.5f &&
               lvl->state == LEVEL_LOADED) {
      // Start async unload
      lvl->state = LEVEL_UNLOADING;
    }
  }
}

/*
 * IMPLEMENTATION: 1800/4000 Streaming TODOs
 * LOC: ~60
 */
