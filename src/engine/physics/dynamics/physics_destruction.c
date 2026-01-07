#include "physics/core/physics_types.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <include/math/math.h>

// DESTRUCTION SYSTEM (Voronoi fracture simulation)
typedef struct {
  float vertices[8][3];
  float center[3];
  float velocity[3], angular_velocity[3];
  bool active;
} DestructionFragment;

typedef struct DestructionSystem {
  DestructionFragment *fragments;
  int fragment_count, capacity;
} DestructionSystem;

DestructionSystem *destruction_init(int capacity) {
  DestructionSystem *ds = (DestructionSystem*)calloc(1, sizeof(DestructionSystem));
  if (!ds) return NULL;
  ds->capacity = capacity;
  ds->fragments = (DestructionFragment*)calloc(capacity, sizeof(DestructionFragment));
  return ds;
}

void destruction_fracture_box(DestructionSystem *ds, float center[3],
                              float size[3], int num_fragments) {
  if (!ds) return;
  for (int i = 0; i < num_fragments && ds->fragment_count < ds->capacity; i++) {
    DestructionFragment *frag = &ds->fragments[ds->fragment_count++];

    // Random fragment center near original center
    for (int j = 0; j < 3; j++) {
      frag->center[j] = center[j] + ((float)rand() / RAND_MAX - 0.5f) * size[j];
    }

    // Random velocity from impact
    for (int j = 0; j < 3; j++) {
      frag->velocity[j] = ((float)rand() / RAND_MAX - 0.5f) * 5.0f;
      frag->angular_velocity[j] = ((float)rand() / RAND_MAX - 0.5f) * 3.0f;
    }

    frag->active = true;
  }
}

void destruction_update(DestructionSystem *ds, float dt) {
  if (!ds) return;
  for (int i = 0; i < ds->fragment_count; i++) {
    if (!ds->fragments[i].active)
      continue;

    DestructionFragment *frag = &ds->fragments[i];

    // Apply gravity
    frag->velocity[1] -= 9.8f * dt;

    // Update position
    for (int j = 0; j < 3; j++) {
      frag->center[j] += frag->velocity[j] * dt;
    }

    // Ground collision
    if (frag->center[1] < 0) {
      frag->center[1] = 0;
      frag->velocity[1] *= -0.3f; // Bounce with energy loss
    }
  }
}
