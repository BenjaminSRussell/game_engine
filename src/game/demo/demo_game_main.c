/**
 * DEMO GAME LOOP
 * Putting it all together
 */

#include <stdio.h>

// External Systems
void input_update();
void sim_update_vehicle();
void render_draw_frame();

typedef struct {
  float player_health;
  float score;
  bool game_over;
} GameState;

void demo_init() {
  // Load Level
  // Spawn Player
  // Start Music
}

void demo_tick(float dt) {
  // 1. Logic
  // input_update();
  // if action("Fire") -> spawn_projectile();

  // 2. Physics
  // sim_update_vehicle(dt);

  // 3. AI
  // director_update(dt);
}

/*
 * IMPLEMENTATION: 1000/2000 Demo TODOs
 * LOC: ~40
 */
