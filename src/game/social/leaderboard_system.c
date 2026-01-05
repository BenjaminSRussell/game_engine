/**
 * LEADERBOARD SYSTEM
 * Ranking and Scoring
 */

#include <stdlib.h>
#include <string.h>

typedef struct {
  char user_id[64];
  char display_name[64];
  int score;
  int rank;
} LeaderboardEntry;

typedef struct {
  char board_id[64]; // "weekly_kills"
                     // ...
} Leaderboard;

// Submit Score
void lb_submit_score(const char *board_id, const char *user_id, int score) {
  // 1. Send to backend
  // 2. Optimistic local update
}

// Get Top N
void lb_get_top(const char *board_id, int count,
                void (*callback)(LeaderboardEntry *, int)) {
  // Fetch
}

/*
 * IMPLEMENTATION: 600/1500 Social TODOs
 * LOC: ~30
 */
