#ifndef ULTIMATE_ENGINE_AI_H
#define ULTIMATE_ENGINE_AI_H

#include "../../Core/Public/core_types.h"
#include <stdbool.h>

// Structures
typedef struct BehaviorTreeManager BehaviorTreeManager;
typedef struct NavigationManager NavigationManager;
typedef struct PerceptionManager PerceptionManager;
typedef struct DecisionMaker DecisionMaker;

typedef struct AISystem {
  BehaviorTreeManager *behavior_tree_manager;
  NavigationManager *navigation_manager;
  PerceptionManager *perception_manager;
  DecisionMaker *decision_maker;
} AISystem;

// Lifecycle
bool AI_Init(void);
void AI_Shutdown(void);
void AI_Update(float delta_time);

// Subsystems
BehaviorTreeManager *BehaviorTreeManager_Create(void);
void BehaviorTreeManager_Destroy(BehaviorTreeManager *mgr);
void BehaviorTreeManager_Update(BehaviorTreeManager *mgr, float dt);

NavigationManager *NavigationManager_Create(void);
void NavigationManager_Destroy(NavigationManager *mgr);
void NavigationManager_Update(NavigationManager *mgr, float dt);

PerceptionManager *PerceptionManager_Create(void);
void PerceptionManager_Destroy(PerceptionManager *mgr);
void PerceptionManager_Update(PerceptionManager *mgr, float dt);

DecisionMaker *DecisionMaker_Create(void);
void DecisionMaker_Destroy(DecisionMaker *mgr);
void DecisionMaker_Update(DecisionMaker *mgr, float dt);

#endif // ULTIMATE_ENGINE_AI_H
