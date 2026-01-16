#ifndef ULTIMATE_ENGINE_PHYSICS_H
#define ULTIMATE_ENGINE_PHYSICS_H

#include "../../Core/Public/core_types.h"
#include <stdbool.h>

// Structures
typedef struct Broadphase Broadphase;
typedef struct Narrowphase Narrowphase;
typedef struct ConstraintSolver ConstraintSolver;
typedef struct Vec3 Vec3;

typedef struct PhysicsWorld {
  Broadphase *broadphase;
  Narrowphase *narrowphase;
  ConstraintSolver *solver;
  float time_step;
  float accumulator;
  // Vec3 gravity; // Real Vec3 struct needed, using float[3] for stub
  float gravity[3];
} PhysicsWorld;

// Lifecycle
bool Physics_Init(void);
void Physics_Shutdown(void);
void Physics_Update(float delta_time);
void Physics_Step(float time_step);

// Subsystems (Stubs/Impl)
Broadphase *Broadphase_Create(void);
void Broadphase_Destroy(Broadphase *bp);
void Broadphase_Update(Broadphase *bp);

Narrowphase *Narrowphase_Create(void);
void Narrowphase_Destroy(Narrowphase *np);
void Narrowphase_Process(Narrowphase *np);

ConstraintSolver *ConstraintSolver_Create(void);
void ConstraintSolver_Destroy(ConstraintSolver *cs);
void ConstraintSolver_Solve(ConstraintSolver *cs, float dt);

void Physics_Integrate(float dt);

#endif // ULTIMATE_ENGINE_PHYSICS_H
