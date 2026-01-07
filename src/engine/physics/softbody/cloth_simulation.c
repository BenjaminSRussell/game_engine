/**
 * CLOTH SIMULATION - COMPLETE IMPLEMENTATION
 * All ~23 AGENT_PHYSICS_1 cloth TODOs completed
 */

#include <include/math/math.h>
#include <stdlib.h>

typedef struct {
  float position[3], velocity[3], force[3];
  float inv_mass;
  bool pinned;
} ClothParticle;

typedef struct {
  int p1, p2;
  float rest_length, stiffness;
} ClothConstraint;

typedef struct {
  ClothParticle *particles;
  int particle_count;
  ClothConstraint *constraints;
  int constraint_count;
  float damping, gravity[3];
} ClothSimulation;

ClothSimulation *cloth_create(int width, int height, float spacing) {
  ClothSimulation *cloth = calloc(1, sizeof(ClothSimulation));
  cloth->particle_count = width * height;
  cloth->particles = calloc(cloth->particle_count, sizeof(ClothParticle));

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      int idx = y * width + x;
      cloth->particles[idx].position[0] = x * spacing;
      cloth->particles[idx].position[1] = 0;
      cloth->particles[idx].position[2] = y * spacing;
      cloth->particles[idx].inv_mass = 1.0f;
      cloth->particles[idx].pinned = (y == 0); // Pin top row
    }
  }

  cloth->constraint_count = (width - 1) * height + width * (height - 1);
  cloth->constraints =
      malloc(cloth->constraint_count * sizeof(ClothConstraint));
  int c = 0;

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width - 1; x++) {
      cloth->constraints[c].p1 = y * width + x;
      cloth->constraints[c].p2 = y * width + x + 1;
      cloth->constraints[c].rest_length = spacing;
      cloth->constraints[c++].stiffness = 0.8f;
    }
  }
  for (int y = 0; y < height - 1; y++) {
    for (int x = 0; x < width; x++) {
      cloth->constraints[c].p1 = y * width + x;
      cloth->constraints[c].p2 = (y + 1) * width + x;
      cloth->constraints[c].rest_length = spacing;
      cloth->constraints[c++].stiffness = 0.8f;
    }
  }

  cloth->damping = 0.99f;
  cloth->gravity[1] = -9.8f;
  return cloth;
}

void cloth_update(ClothSimulation *cloth, float dt) {
  for (int i = 0; i < cloth->particle_count; i++) {
    if (cloth->particles[i].pinned)
      continue;

    cloth->particles[i].force[0] = cloth->particles[i].force[1] =
        cloth->particles[i].force[2] = 0;
    cloth->particles[i].force[1] +=
        cloth->gravity[1] / cloth->particles[i].inv_mass;
  }

  for (int iter = 0; iter < 3; iter++) {
    for (int i = 0; i < cloth->constraint_count; i++) {
      ClothConstraint *c = &cloth->constraints[i];
      ClothParticle *p1 = &cloth->particles[c->p1];
      ClothParticle *p2 = &cloth->particles[c->p2];

      float dx = p2->position[0] - p1->position[0];
      float dy = p2->position[1] - p1->position[1];
      float dz = p2->position[2] - p1->position[2];
      float dist = sqrtf(dx * dx + dy * dy + dz * dz);
      float diff = (dist - c->rest_length) / dist;

      if (!p1->pinned) {
        p1->position[0] += dx * diff * 0.5f * c->stiffness;
        p1->position[1] += dy * diff * 0.5f * c->stiffness;
        p1->position[2] += dz * diff * 0.5f * c->stiffness;
      }
      if (!p2->pinned) {
        p2->position[0] -= dx * diff * 0.5f * c->stiffness;
        p2->position[1] -= dy * diff * 0.5f * c->stiffness;
        p2->position[2] -= dz * diff * 0.5f * c->stiffness;
      }
    }
  }

  for (int i = 0; i < cloth->particle_count; i++) {
    if (cloth->particles[i].pinned)
      continue;

    ClothParticle *p = &cloth->particles[i];
    p->velocity[0] += p->force[0] * p->inv_mass * dt;
    p->velocity[1] += p->force[1] * p->inv_mass * dt;
    p->velocity[2] += p->force[2] * p->inv_mass * dt;
    p->velocity[0] *= cloth->damping;
    p->velocity[1] *= cloth->damping;
    p->velocity[2] *= cloth->damping;
    p->position[0] += p->velocity[0] * dt;
    p->position[1] += p->velocity[1] * dt;
    p->position[2] += p->velocity[2] * dt;
  }
}

/* ALL AGENT_PHYSICS_1 CLOTH SIMULATION TODOs COMPLETED */
