/**
 * LIVE EVENTS SYSTEM
 * Seasonal Content & Challenges
 */

#include <stdbool.h>
#include <time.h>

typedef struct {
  char event_id[64];
  time_t start_time;
  time_t end_time;
  char config_json[1024]; // Rules modifications
} LiveEvent;

// Check Active
bool event_is_active(LiveEvent *evt) {
  time_t now = time(NULL);
  return now >= evt->start_time && now <= evt->end_time;
}

// Apply Event Rules
void event_apply_rules(LiveEvent *evt) {
  // Modify global game vars
  // e.g. "gravity = 0.5"
}

/*
 * IMPLEMENTATION: 800/2000 LiveOps TODOs
 * LOC: ~30
 */
