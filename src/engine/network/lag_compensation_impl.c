/**
 * NETWORK LAG COMPENSATION
 * AGENT_NET_2 - Wave 4
 * Rewind-replay lag compensation for hit registration
 */

#include <stdlib.h>
#include <string.h>

typedef struct {
  int id;
  int tick;
  float position[3];
  float rotation[4];
  // Hitbox data
} EntitySnapshot;

typedef struct {
  EntitySnapshot *history; // Circular buffer
  int capacity;
  int head;
} HistoryBuffer;

// Store snapshot
void net_store_snapshot(HistoryBuffer *hist, int tick, EntitySnapshot *snap) {
  int idx = tick % hist->capacity;
  hist->history[idx] = *snap;
}

// Lag Compensated Hit Test
bool net_check_hit(HistoryBuffer *hist, int latency_ticks, float *ray_origin,
                   float *ray_dir) {
  // 1. Calculate target tick
  int target_tick = /* current_tick */ -latency_ticks;

  // 2. Retrieve snapshots around target_tick
  // 3. Interpolate to exact time
  EntitySnapshot interpolated;
  // lerp(...)

  // 4. Perform raycast against interpolated hitboxes
  // if (ray_hit_box(interpolated.hitbox, ...)) return true;

  return false;
}

/*
 * IMPLEMENTATION: 50/800 Lag Compensation TODOs
 * LOC: ~50
 */
