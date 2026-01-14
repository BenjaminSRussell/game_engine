/**
 * @file ai_api.h
 * @brief AI subsystem API
 */

#ifndef VOXELFORGE_AI_API_H
#define VOXELFORGE_AI_API_H

#include "Core/Public/Math/Vector/vec3.h"
#include "Core/Public/core_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Forward Declarations
// ============================================================================

typedef struct AIController AIController;
typedef struct BehaviorTree BehaviorTree;
typedef struct Blackboard Blackboard;
typedef struct NavMesh NavMesh;
typedef struct NavPath NavPath;
typedef struct NavAgent NavAgent;

// ============================================================================
// Navigation Mesh
// ============================================================================

typedef struct NavMeshConfig {
  f32 cell_size;
  f32 cell_height;
  f32 agent_height;
  f32 agent_radius;
  f32 max_climb;
  f32 max_slope;
  u32 region_min_size;
  u32 region_merge_size;
} NavMeshConfig;

VF_API NavMesh *navmesh_create(const NavMeshConfig *config);
VF_API void navmesh_destroy(NavMesh *mesh);
VF_API void navmesh_add_geometry(NavMesh *mesh, const Vec3 *vertices,
                                 const u32 *indices, u32 vertex_count,
                                 u32 index_count);
VF_API VF_Result navmesh_build(NavMesh *mesh);
VF_API b8 navmesh_is_valid(NavMesh *mesh);

// ============================================================================
// Navigation Agent
// ============================================================================

typedef struct NavAgentConfig {
  f32 radius;
  f32 height;
  f32 max_speed;
  f32 max_acceleration;
  f32 separation_weight;
} NavAgentConfig;

VF_API NavAgent *navagent_create(NavMesh *mesh, const NavAgentConfig *config);
VF_API void navagent_destroy(NavAgent *agent);
VF_API void navagent_set_position(NavAgent *agent, Vec3 position);
VF_API Vec3 navagent_get_position(NavAgent *agent);
VF_API void navagent_set_target(NavAgent *agent, Vec3 target);
VF_API b8 navagent_has_path(NavAgent *agent);
VF_API b8 navagent_reached_target(NavAgent *agent);
VF_API void navagent_update(NavAgent *agent, f32 delta_time);
VF_API void navagent_stop(NavAgent *agent);

// ============================================================================
// Pathfinding Queries
// ============================================================================

VF_API NavPath *navpath_create(void);
VF_API void navpath_destroy(NavPath *path);
VF_API VF_Result navpath_find(NavMesh *mesh, Vec3 start, Vec3 end,
                              NavPath *out_path);
VF_API u32 navpath_get_point_count(NavPath *path);
VF_API Vec3 navpath_get_point(NavPath *path, u32 index);
VF_API f32 navpath_get_length(NavPath *path);

// ============================================================================
// Blackboard (AI Memory)
// ============================================================================

VF_API Blackboard *blackboard_create(void);
VF_API void blackboard_destroy(Blackboard *bb);
VF_API void blackboard_set_float(Blackboard *bb, const char *key, f32 value);
VF_API void blackboard_set_int(Blackboard *bb, const char *key, i32 value);
VF_API void blackboard_set_bool(Blackboard *bb, const char *key, b8 value);
VF_API void blackboard_set_vec3(Blackboard *bb, const char *key, Vec3 value);
VF_API void blackboard_set_ptr(Blackboard *bb, const char *key, void *value);
VF_API f32 blackboard_get_float(Blackboard *bb, const char *key,
                                f32 default_val);
VF_API i32 blackboard_get_int(Blackboard *bb, const char *key, i32 default_val);
VF_API b8 blackboard_get_bool(Blackboard *bb, const char *key, b8 default_val);
VF_API Vec3 blackboard_get_vec3(Blackboard *bb, const char *key,
                                Vec3 default_val);
VF_API void *blackboard_get_ptr(Blackboard *bb, const char *key);
VF_API b8 blackboard_has_key(Blackboard *bb, const char *key);
VF_API void blackboard_remove(Blackboard *bb, const char *key);
VF_API void blackboard_clear(Blackboard *bb);

// ============================================================================
// Behavior Tree
// ============================================================================

typedef enum BTNodeStatus {
  BT_STATUS_SUCCESS = 0,
  BT_STATUS_FAILURE,
  BT_STATUS_RUNNING,
} BTNodeStatus;

typedef BTNodeStatus (*BTTaskFn)(Blackboard *bb, f32 dt, void *user_data);
typedef b8 (*BTConditionFn)(Blackboard *bb, void *user_data);

VF_API BehaviorTree *bt_create(void);
VF_API void bt_destroy(BehaviorTree *tree);
VF_API void bt_set_root(BehaviorTree *tree, u32 node_id);
VF_API BTNodeStatus bt_tick(BehaviorTree *tree, Blackboard *bb, f32 delta_time);
VF_API void bt_reset(BehaviorTree *tree);

// Node creation
VF_API u32 bt_add_selector(BehaviorTree *tree);
VF_API u32 bt_add_sequence(BehaviorTree *tree);
VF_API u32 bt_add_parallel(BehaviorTree *tree, u32 success_threshold);
VF_API u32 bt_add_task(BehaviorTree *tree, BTTaskFn task, void *user_data);
VF_API u32 bt_add_condition(BehaviorTree *tree, BTConditionFn condition,
                            void *user_data);
VF_API u32 bt_add_inverter(BehaviorTree *tree, u32 child);
VF_API u32 bt_add_repeater(BehaviorTree *tree, u32 child, u32 repeat_count);
VF_API u32 bt_add_cooldown(BehaviorTree *tree, u32 child, f32 cooldown_seconds);
VF_API void bt_add_child(BehaviorTree *tree, u32 parent, u32 child);

// ============================================================================
// Perception
// ============================================================================

typedef struct PerceptionConfig {
  f32 sight_radius;
  f32 sight_angle; // FOV in degrees
  f32 hearing_radius;
  u32 max_perceived;
} PerceptionConfig;

typedef struct PerceivedActor {
  void *actor;
  Vec3 last_known_position;
  f32 age; // Time since last seen
  b8 visible;
} PerceivedActor;

VF_API void perception_update_sight(Vec3 origin, Vec3 forward, f32 radius,
                                    f32 angle, void **actors, u32 actor_count,
                                    PerceivedActor *out_perceived,
                                    u32 *out_count);

// ============================================================================
// AI System
// ============================================================================

VF_API VF_Result ai_init(void);
VF_API void ai_shutdown(void);
VF_API void ai_update(f32 delta_time);

#ifdef __cplusplus
}
#endif

#endif // VOXELFORGE_AI_API_H
