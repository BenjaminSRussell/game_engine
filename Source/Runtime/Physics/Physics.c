#include "Public/Physics.h"
#include <stdlib.h>
#include <string.h>

// Struct Stubs
struct Broadphase {
  int id;
};
struct Narrowphase {
  int id;
};
struct ConstraintSolver {
  int id;
};

static PhysicsWorld g_physics_world = {0};

// Implementation Stubs
Broadphase *Broadphase_Create(void) { return malloc(sizeof(Broadphase)); }
void Broadphase_Destroy(Broadphase *bp) { free(bp); }
void Broadphase_Update(Broadphase *bp) { (void)bp; }

Narrowphase *Narrowphase_Create(void) { return malloc(sizeof(Narrowphase)); }
void Narrowphase_Destroy(Narrowphase *np) { free(np); }
void Narrowphase_Process(Narrowphase *np) { (void)np; }

ConstraintSolver *ConstraintSolver_Create(void) {
  return malloc(sizeof(ConstraintSolver));
}
void ConstraintSolver_Destroy(ConstraintSolver *cs) { free(cs); }
void ConstraintSolver_Solve(ConstraintSolver *cs, float dt) {
  (void)cs;
  (void)dt;
}

void Physics_Integrate(float dt) {
  (void)dt;
  // Integration moved to Physics_System.c (ECS)
}

bool Physics_Init(void) {
  g_physics_world.broadphase = Broadphase_Create();
  g_physics_world.narrowphase = Narrowphase_Create();
  g_physics_world.solver = ConstraintSolver_Create();

  g_physics_world.gravity[0] = 0.0f;
  g_physics_world.gravity[1] = -9.81f;
  g_physics_world.gravity[2] = 0.0f;

  g_physics_world.time_step = 1.0f / 60.0f;
  g_physics_world.accumulator = 0.0f;

  return true;
}

void Physics_Update(float delta_time) {
  // Fixed timestep
  g_physics_world.accumulator += delta_time;

  // Clamp accumulator to avoid spiral of death
  if (g_physics_world.accumulator > 0.2f)
    g_physics_world.accumulator = 0.2f;

  while (g_physics_world.accumulator >= g_physics_world.time_step) {
    Physics_Step(g_physics_world.time_step);
    g_physics_world.accumulator -= g_physics_world.time_step;
  }
}

void Physics_Step(float time_step) {
  // Broadphase collision detection
  Broadphase_Update(g_physics_world.broadphase);

  // Narrowphase collision detection
  Narrowphase_Process(g_physics_world.narrowphase);

  // Constraint solving
  ConstraintSolver_Solve(g_physics_world.solver, time_step);

  // Integrate physics
  Physics_Integrate(time_step);
}

void Physics_Shutdown(void) {
  ConstraintSolver_Destroy(g_physics_world.solver);
  Narrowphase_Destroy(g_physics_world.narrowphase);
  Broadphase_Destroy(g_physics_world.broadphase);
}
