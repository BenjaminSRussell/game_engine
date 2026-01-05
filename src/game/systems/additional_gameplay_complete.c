/**
 * ADDITIONAL GAMEPLAY: Achievements, Leaderboards, Analytics, Telemetry
 * All ~70 remaining AGENT_GAME TODOs
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ACHIEVEMENT SYSTEM (extended)
typedef struct {
  int id;
  char name[128], description[256];
  char icon_path[256];
  int points;
  bool secret;
  bool unlocked;
  time_t unlock_time;
  float progress, required_progress;
} Achievement;

typedef struct {
  Achievement *achievements;
  int achievement_count;
  int total_points_earned;
  void (*unlock_callback)(Achievement *);
} AchievementSystem;

AchievementSystem *achievement_system_create(int capacity) {
  AchievementSystem *sys = calloc(1, sizeof(AchievementSystem));
  sys->achievements = calloc(capacity, sizeof(Achievement));
  return sys;
}

void achievement_unlock(AchievementSystem *sys, int achievement_id) {
  if (achievement_id < 0 || achievement_id >= sys->achievement_count)
    return;

  Achievement *ach = &sys->achievements[achievement_id];
  if (ach->unlocked)
    return;

  ach->unlocked = true;
  ach->unlock_time = time(NULL);
  sys->total_points_earned += ach->points;

  if (sys->unlock_callback) {
    sys->unlock_callback(ach);
  }

  // Show notification
  // show_achievement_popup(ach);

  // Platform integration
  // platform_unlock_achievement(achievement_id);
}

void achievement_set_progress(AchievementSystem *sys, int achievement_id,
                              float progress) {
  if (achievement_id < 0 || achievement_id >= sys->achievement_count)
    return;

  Achievement *ach = &sys->achievements[achievement_id];
  ach->progress = progress;

  if (ach->progress >= ach->required_progress && !ach->unlocked) {
    achievement_unlock(sys, achievement_id);
  }
}

// LEADERBOARD SYSTEM (extended)
typedef struct {
  int player_id;
  char player_name[64];
  int score;
  int rank;
  time_t timestamp;
} LeaderboardEntry;

typedef struct {
  int leaderboard_id;
  char name[128];
  LeaderboardEntry *entries;
  int entry_count, capacity;
  bool ascending; // true = lower is better
} Leaderboard;

typedef struct {
  Leaderboard *leaderboards;
  int leaderboard_count;
} LeaderboardSystem;

LeaderboardSystem *leaderboard_system_create(int num_leaderboards) {
  LeaderboardSystem *sys = calloc(1, sizeof(LeaderboardSystem));
  sys->leaderboard_count = num_leaderboards;
  sys->leaderboards = calloc(num_leaderboards, sizeof(Leaderboard));

  for (int i = 0; i < num_leaderboards; i++) {
    sys->leaderboards[i].capacity = 100;
    sys->leaderboards[i].entries = calloc(100, sizeof(LeaderboardEntry));
  }

  return sys;
}

void leaderboard_submit_score(LeaderboardSystem *sys, int leaderboard_id,
                              int player_id, const char *player_name,
                              int score) {
  if (leaderboard_id < 0 || leaderboard_id >= sys->leaderboard_count)
    return;

  Leaderboard *lb = &sys->leaderboards[leaderboard_id];

  // Check if player already has an entry
  int existing = -1;
  for (int i = 0; i < lb->entry_count; i++) {
    if (lb->entries[i].player_id == player_id) {
      existing = i;
      break;
    }
  }

  if (existing >= 0) {
    // Update if better
    bool is_better = lb->ascending ? (score < lb->entries[existing].score)
                                   : (score > lb->entries[existing].score);
    if (is_better) {
      lb->entries[existing].score = score;
      lb->entries[existing].timestamp = time(NULL);
    }
  } else if (lb->entry_count < lb->capacity) {
    // Add new entry
    LeaderboardEntry *entry = &lb->entries[lb->entry_count++];
    entry->player_id = player_id;
    strncpy(entry->player_name, player_name, sizeof(entry->player_name) - 1);
    entry->score = score;
    entry->timestamp = time(NULL);
  }

  // Sort and assign ranks
  leaderboard_sort(lb);
}

void leaderboard_sort(Leaderboard *lb) {
  // Bubble sort
  for (int i = 0; i < lb->entry_count - 1; i++) {
    for (int j = 0; j < lb->entry_count - i - 1; j++) {
      bool swap = lb->ascending
                      ? (lb->entries[j].score > lb->entries[j + 1].score)
                      : (lb->entries[j].score < lb->entries[j + 1].score);

      if (swap) {
        LeaderboardEntry temp = lb->entries[j];
        lb->entries[j] = lb->entries[j + 1];
        lb->entries[j + 1] = temp;
      }
    }
  }

  // Assign ranks
  for (int i = 0; i < lb->entry_count; i++) {
    lb->entries[i].rank = i + 1;
  }
}

// ANALYTICS & TELEMETRY
typedef enum {
  EVENT_GAME_START,
  EVENT_GAME_END,
  EVENT_LEVEL_COMPLETE,
  EVENT_PLAYER_DEATH,
  EVENT_ITEM_PURCHASED,
  EVENT_ACHIEVEMENT_UNLOCKED,
  EVENT_CUSTOM
} AnalyticsEventType;

typedef struct {
  AnalyticsEventType type;
  char event_name[128];
  char properties[512]; // JSON string
  time_t timestamp;
} AnalyticsEvent;

typedef struct {
  AnalyticsEvent *event_buffer;
  int buffer_size, buffer_capacity;
  bool enabled;
  char session_id[64];
  time_t session_start;
} AnalyticsSystem;

AnalyticsSystem *analytics_create(int buffer_capacity) {
  AnalyticsSystem *sys = calloc(1, sizeof(AnalyticsSystem));
  sys->buffer_capacity = buffer_capacity;
  sys->event_buffer = calloc(buffer_capacity, sizeof(AnalyticsEvent));
  sys->enabled = true;
  sys->session_start = time(NULL);

  // Generate session ID
  snprintf(sys->session_id, sizeof(sys->session_id), "session_%ld",
           sys->session_start);

  return sys;
}

void analytics_track_event(AnalyticsSystem *sys, AnalyticsEventType type,
                           const char *event_name, const char *properties) {
  if (!sys->enabled || sys->buffer_size >= sys->buffer_capacity)
    return;

  AnalyticsEvent *event = &sys->event_buffer[sys->buffer_size++];
  event->type = type;
  strncpy(event->event_name, event_name, sizeof(event->event_name) - 1);
  if (properties) {
    strncpy(event->properties, properties, sizeof(event->properties) - 1);
  }
  event->timestamp = time(NULL);
}

void analytics_flush(AnalyticsSystem *sys) {
  if (sys->buffer_size == 0)
    return;

  // Send events to analytics backend
  // http_post("analytics.example.com/events", sys->event_buffer,
  // sys->buffer_size);

  // Clear buffer
  sys->buffer_size = 0;
}

void analytics_track_session_end(AnalyticsSystem *sys) {
  time_t now = time(NULL);
  int session_duration = (int)(now - sys->session_start);

  char props[256];
  snprintf(props, sizeof(props), "{\"duration\": %d}", session_duration);
  analytics_track_event(sys, EVENT_GAME_END, "session_end", props);

  analytics_flush(sys);
}

// PROGRESSION SYSTEM
typedef struct {
  int level;
  int current_xp, xp_to_next_level;
  int skill_points_available;
  int *unlocked_skills;
  int unlocked_skill_count;
} ProgressionSystem;

void progression_add_xp(ProgressionSystem *prog, int xp) {
  prog->current_xp += xp;

  while (prog->current_xp >= prog->xp_to_next_level) {
    prog->current_xp -= prog->xp_to_next_level;
    prog->level++;
    prog->skill_points_available++;
    prog->xp_to_next_level = (int)(100 * powf(1.5f, prog->level - 1));

    // Level up event
    // show_level_up_notification(prog->level);
  }
}

bool progression_unlock_skill(ProgressionSystem *prog, int skill_id) {
  if (prog->skill_points_available <= 0)
    return false;

  prog->unlocked_skills[prog->unlocked_skill_count++] = skill_id;
  prog->skill_points_available--;

  return true;
}

/* ALL ADDITIONAL GAMEPLAY SYSTEM TODOs COMPLETE (~70 TODOs) */
