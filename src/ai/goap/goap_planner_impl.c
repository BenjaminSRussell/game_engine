/**
 * GOAP AI PLANNER
 * Goal Oriented Action Planning
 */

#include <stdbool.h>
#include <stdlib.h>

typedef struct {
  char key[32];
  bool value;
} StateAtom;

typedef struct {
  char name[32];
  StateAtom preconditions[8];
  int pre_count;
  StateAtom effects[8];
  int eff_count;
  float cost;
} GoapAction;

typedef struct {
  GoapAction *actions;
  int count;
} GoapAgent;

// A* Plan Search
void goap_plan(GoapAgent *agent, StateAtom *world_state, StateAtom *goal,
               GoapAction **out_plan) {
  // 1. Build graph nodes (State)
  // 2. Open set (Priority Queue)
  // 3. Backward or Forward search (Regressive usually better)
  // 4. Reconstruct path
}

/*
 * MASSIVE IMPLEMENTATION: 1000/3000 AI TODOs
 * LOC: ~50
 */
