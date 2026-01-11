#include "include/common.h"

void motion_database_init(void) {}

void motion_database_build(void *clips, int count, void *output_db) {
  // Build motion matching database
  (void)clips;
  (void)count;
  (void)output_db;
}

void motion_database_query(void *db, void *query_pose, void *best_match) {
  (void)db;
  (void)query_pose;
  (void)best_match;
}
