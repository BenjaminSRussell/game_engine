/**
 * GOAL ORIENTED ACTION PLANNING (GOAP)
 * Dynamic AI planning system
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ATOMS 64
#define MAX_ACTIONS 64
#define MAX_PLAN_LENGTH 32

typedef struct {
  char key[32];
  bool value;
} WorldStateAtom;

typedef struct {
  WorldStateAtom atoms[MAX_ATOMS];
  int count;
} WorldState;

typedef struct {
  char name[32];
  int cost;

  WorldState preconditions;
  WorldState effects;

  // Function pointers for execution
  bool (*check_procedural_precondition)(void *agent);
  bool (*perform)(void *agent);
  bool (*is_done)(void *agent);
} GoapAction;

typedef struct {
  GoapAction actions[MAX_ACTIONS];
  int count;
} ActionPlanner;

typedef struct {
  GoapAction *actions[MAX_PLAN_LENGTH];
  int length;
  int current_step;
} GoapPlan;

// State Management
void world_state_set(WorldState *ws, const char *key, bool value) {
  for (int i = 0; i < ws->count; i++) {
    if (strcmp(ws->atoms[i].key, key) == 0) {
      ws->atoms[i].value = value;
      return;
    }
  }
  if (ws->count < MAX_ATOMS) {
    strncpy(ws->atoms[ws->count].key, key, 31);
    ws->atoms[ws->count].value = value;
    ws->count++;
  }
}

bool world_state_get(const WorldState *ws, const char *key) {
  for (int i = 0; i < ws->count; i++) {
    if (strcmp(ws->atoms[i].key, key) == 0) {
      return ws->atoms[i].value;
    }
  }
  return false;
}

// Planner Node for A*
typedef struct {
  WorldState state;
  GoapAction *action;
  int parent_id;
  int g_cost; // Cost so far
  int h_cost; // Heuristic (num mismatched atoms)
} PlannerNode;

static int calculate_heuristic(const WorldState *current,
                               const WorldState *goal) {
  int mismatches = 0;
  for (int i = 0; i < goal->count; i++) {
    bool match = false;
    for (int j = 0; j < current->count; j++) {
      if (strcmp(current->atoms[j].key, goal->atoms[i].key) == 0) {
        if (current->atoms[j].value == goal->atoms[i].value) {
          match = true;
        }
        break;
      }
    }
    if (!match)
      mismatches++;
  }
  return mismatches;
}

static bool satisfy_preconditions(const WorldState *state,
                                  const WorldState *preconditions) {
  for (int i = 0; i < preconditions->count; i++) {
    bool met = false;
    for (int j = 0; j < state->count; j++) {
      if (strcmp(state->atoms[j].key, preconditions->atoms[i].key) == 0) {
        if (state->atoms[j].value == preconditions->atoms[i].value) {
          met = true;
        }
        break;
      }
    }
    if (!met)
      return false;
  }
  return true;
}

static void apply_effects(WorldState *state, const WorldState *effects) {
  for (int i = 0; i < effects->count; i++) {
    world_state_set(state, effects->atoms[i].key, effects->atoms[i].value);
  }
}

// A* Planner
GoapPlan *goap_plan(ActionPlanner *planner, WorldState start_state,
                    WorldState goal_state) {
  PlannerNode open_list[256];
  int open_count = 0;

  PlannerNode closed_list[256];
  int closed_count = 0;

  // Add start node
  open_list[0].state = start_state;
  open_list[0].action = NULL;
  open_list[0].parent_id = -1;
  open_list[0].g_cost = 0;
  open_list[0].h_cost = calculate_heuristic(&start_state, &goal_state);
  open_count++;

  while (open_count > 0) {
    // Find lowest F cost
    int best_idx = 0;
    int lowest_f = open_list[0].g_cost + open_list[0].h_cost;

    for (int i = 1; i < open_count; i++) {
      int f = open_list[i].g_cost + open_list[i].h_cost;
      if (f < lowest_f) {
        lowest_f = f;
        best_idx = i;
      }
    }

    PlannerNode current = open_list[best_idx];

    // Remove from open, add to closed
    open_list[best_idx] = open_list[open_count - 1];
    open_count--;

    int current_id = closed_count;
    closed_list[closed_count++] = current;

    // Check goal
    if (calculate_heuristic(&current.state, &goal_state) == 0) {
      // Reconstruct plan
      GoapPlan *plan = malloc(sizeof(GoapPlan));
      plan->length = 0;
      plan->current_step = 0;

      int curr = current_id;
      while (closed_list[curr].parent_id != -1) {
        if (plan->length < MAX_PLAN_LENGTH) {
          plan->actions[plan->length++] = closed_list[curr].action;
        }
        curr = closed_list[curr].parent_id;
      }

      // Reverse
      for (int i = 0; i < plan->length / 2; i++) {
        GoapAction *temp = plan->actions[i];
        plan->actions[i] = plan->actions[plan->length - 1 - i];
        plan->actions[plan->length - 1 - i] = temp;
      }

      return plan;
    }

    // Expand neighbors
    for (int i = 0; i < planner->count; i++) {
      GoapAction *action = &planner->actions[i];

      if (satisfy_preconditions(&current.state, &action->preconditions)) {
        // Apply effects
        WorldState next_state = current.state;
        apply_effects(&next_state, &action->effects);

        // Add to open
        if (open_count < 256) {
          PlannerNode neighbor;
          neighbor.state = next_state;
          neighbor.action = action;
          neighbor.parent_id = current_id;
          neighbor.g_cost = current.g_cost + action->cost;
          neighbor.h_cost = calculate_heuristic(&next_state, &goal_state);

          // Simply add (in real A*, check if duplicate with lower cost exists)
          open_list[open_count++] = neighbor;
        }
      }
    }
  }

  return NULL; // No plan found
}

ActionPlanner *goap_planner_create() {
  ActionPlanner *ap = calloc(1, sizeof(ActionPlanner));
  return ap;
}

void goap_add_action(ActionPlanner *ap, const char *name, int cost) {
  if (ap->count >= MAX_ACTIONS)
    return;
  GoapAction *a = &ap->actions[ap->count++];
  strncpy(a->name, name, 31);
  a->cost = cost;
  a->preconditions.count = 0;
  a->effects.count = 0;
}

void goap_action_add_precondition(GoapAction *action, const char *key,
                                  bool value) {
  world_state_set(&action->preconditions, key, value);
}

void goap_action_add_effect(GoapAction *action, const char *key, bool value) {
  world_state_set(&action->effects, key, value);
}
