#include <common.h>

void motion_matching_init(void) {}

void motion_matching_build_database(void *animation_clips, void *database) {
  (void)animation_clips;
  (void)database;
}

void motion_matching_search(void *database, void *query, void *best_match) {
  (void)database;
  (void)query;
  (void)best_match;
}

void motion_matching_synthesize(void *database, void *trajectory,
                                void *output_animation) {
  (void)database;
  (void)trajectory;
  (void)output_animation;
}
