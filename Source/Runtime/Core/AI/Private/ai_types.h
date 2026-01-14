#ifndef AI_TYPES_H
#define AI_TYPES_H

#include "../Public/unified_ai.h"
#include <pthread.h>
#include <unified_logger.h>
#include <unified_memory.h>

#define MAX_NPCS 1024
#define MAX_PERCEPTIONS 32

typedef struct {
  Vec3 position;
  f32 intensity;
  f32 age;
} Perception;

struct AINPC {
  NPCId id;
  bool active;
  AIState state;
  Vec3 position;
  Vec3 velocity;
  Vec3 target_position;
  bool has_target;

  f32 health;
  f32 morale;

  Perception perceptions[MAX_PERCEPTIONS];
  u32 perception_count;

  // Pathfinding
  Vec3 *current_path; // Simple dynamic array for now
  u32 path_length;
  u32 current_path_index;
};

struct AISystem {
  AINPC *npcs; // Array of MAX_NPCS
  u32 npc_count;
  u32 max_npcs;

  u32 update_rate;
  f32 time_since_last_update;

  pthread_mutex_t lock;
};

// Behavior Tree Internal
typedef enum { BT_SUCCESS, BT_FAILURE, BT_RUNNING } BTStatus;

typedef BTStatus (*BTNodeTick)(AIBehaviorNode *node, void *context);

struct AIBehaviorNode {
  BTNodeTick tick;
  struct AIBehaviorNode **children;
  u32 child_count;
};

#endif // AI_TYPES_H
