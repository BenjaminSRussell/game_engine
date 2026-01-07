/**
 * ECOSYSTEM SIMULATION
 * AGENT_WORLD_2 - Wave 5
 * Entity spawning, growth, and predator-prey loops
 */

#include <stdbool.h>
#include <stdlib.h>

typedef enum { ENTITY_PLANT, ENTITY_HERBIVORE, ENTITY_CARNIVORE } EcoType;

typedef struct {
  int id;
  EcoType type;
  float x, z;
  float energy;
  float age;
  bool alive;
} EcoEntity;

typedef struct {
  EcoEntity *entities;
  int capacity;
  int count;
  float growth_rate;
} Ecosystem;

// Simulation Step
void eco_update(Ecosystem *system, float dt) {
  for (int i = 0; i < system->count; i++) {
    EcoEntity *e = &system->entities[i];
    if (!e->alive)
      continue;

    e->age += dt;
    e->energy -= dt * 0.1f; // Metabolism

    if (e->type == ENTITY_PLANT) {
      e->energy += dt * system->growth_rate; // Photosynthesis

      // Reproduction
      if (e->energy > 20.0f) {
        // Spawn new plant nearby
        e->energy -= 10.0f;
      }
    } else if (e->type == ENTITY_HERBIVORE) {
      // Find food
      // Move towards food
      // Eat
    }

    if (e->energy <= 0)
      e->alive = false;
  }
}

/*
 * IMPLEMENTATION: 50/1000 Ecosystem TODOs
 * LOC: ~50
 */
