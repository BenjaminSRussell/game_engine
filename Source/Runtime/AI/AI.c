#include "Public/AI.h"
#include <stdlib.h>

// Struct Stubs
struct BehaviorTreeManager {
  int id;
};
struct NavigationManager {
  int id;
};
struct PerceptionManager {
  int id;
};
struct DecisionMaker {
  int id;
};

static AISystem g_ai_system = {0};

// Implementation Stubs
BehaviorTreeManager *BehaviorTreeManager_Create(void) {
  return malloc(sizeof(BehaviorTreeManager));
}
void BehaviorTreeManager_Destroy(BehaviorTreeManager *mgr) { free(mgr); }
void BehaviorTreeManager_Update(BehaviorTreeManager *mgr, float dt) {
  (void)mgr;
  (void)dt;
}

NavigationManager *NavigationManager_Create(void) {
  return malloc(sizeof(NavigationManager));
}
void NavigationManager_Destroy(NavigationManager *mgr) { free(mgr); }
void NavigationManager_Update(NavigationManager *mgr, float dt) {
  (void)mgr;
  (void)dt;
}

PerceptionManager *PerceptionManager_Create(void) {
  return malloc(sizeof(PerceptionManager));
}
void PerceptionManager_Destroy(PerceptionManager *mgr) { free(mgr); }
void PerceptionManager_Update(PerceptionManager *mgr, float dt) {
  (void)mgr;
  (void)dt;
}

DecisionMaker *DecisionMaker_Create(void) {
  return malloc(sizeof(DecisionMaker));
}
void DecisionMaker_Destroy(DecisionMaker *mgr) { free(mgr); }
void DecisionMaker_Update(DecisionMaker *mgr, float dt) {
  (void)mgr;
  (void)dt;
}

bool AI_Init(void) {
  g_ai_system.behavior_tree_manager = BehaviorTreeManager_Create();
  g_ai_system.navigation_manager = NavigationManager_Create();
  g_ai_system.perception_manager = PerceptionManager_Create();
  g_ai_system.decision_maker = DecisionMaker_Create();

  return true;
}

void AI_Update(float delta_time) {
  // Update perception
  PerceptionManager_Update(g_ai_system.perception_manager, delta_time);

  // Update decision making
  DecisionMaker_Update(g_ai_system.decision_maker, delta_time);

  // Update behavior trees
  BehaviorTreeManager_Update(g_ai_system.behavior_tree_manager, delta_time);

  // Update navigation
  NavigationManager_Update(g_ai_system.navigation_manager, delta_time);
}

void AI_Shutdown(void) {
  DecisionMaker_Destroy(g_ai_system.decision_maker);
  PerceptionManager_Destroy(g_ai_system.perception_manager);
  NavigationManager_Destroy(g_ai_system.navigation_manager);
  BehaviorTreeManager_Destroy(g_ai_system.behavior_tree_manager);
}
