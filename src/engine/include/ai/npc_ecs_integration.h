// include/ai/npc_ecs_integration.h
//
// Purpose: ECS integration layer for NPC system
// Provides seamless integration between NPC AI and the ECS architecture

#ifndef AI_NPC_ECS_INTEGRATION_H
#define AI_NPC_ECS_INTEGRATION_H

#include "ecs/ecs.h"
#include "ecs/components/npc.h"
#include "ecs/components/transform.h"
#include "ecs/components/health.h"
#include "physics/physics_consolidated.h"
#include "math/vec3.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// NPC ECS INTEGRATION SYSTEM
// ============================================================================

typedef struct {
    World* ecs_world;
    PhysicsWorld* physics_world;
    Entity* npc_entities;
    u32 max_npcs;
    u32 active_npc_count;
    f32 update_distance; // Distance from player for NPC updates
    f32 despawn_distance; // Distance to despawn NPCs
    bool is_initialized;
} NPCECSSystem;

// ============================================================================
// NPC ECS INTEGRATION API
// ============================================================================

// System management
NPCECSSystem* npc_ecs_system_create(World* ecs_world, PhysicsWorld* physics_world, u32 max_npcs);
void npc_ecs_system_destroy(NPCECSSystem* system);
void npc_ecs_system_update(NPCECSSystem* system, Vec3 player_position, f32 delta_time);

// NPC entity management
Entity npc_ecs_create(NPCECSSystem* system, const char* npc_type, Vec3 position);
void npc_ecs_destroy(NPCECSSystem* system, Entity npc_entity);
bool npc_ecs_is_active(NPCECSSystem* system, Entity npc_entity);

// Component access
NPCComponent* npc_ecs_get_component(NPCECSSystem* system, Entity npc_entity);
TransformComponent* npc_ecs_get_transform(NPCECSSystem* system, Entity npc_entity);
HealthComponent* npc_ecs_get_health(NPCECSSystem* system, Entity npc_entity);

// Distance-based culling
void npc_ecs_update_culling(NPCECSSystem* system, Vec3 player_position);
void npc_ecs_set_culling_distances(NPCECSSystem* system, f32 update_distance, f32 despawn_distance);

// Batch operations
void npc_ecs_update_all(NPCECSSystem* system, Vec3 player_position, f32 delta_time);
void npc_ecs_update_in_range(NPCECSSystem* system, Vec3 center, f32 radius, f32 delta_time);

// Query operations
u32 npc_ecs_find_in_radius(NPCECSSystem* system, Vec3 center, f32 radius, Entity* out_entities, u32 max_results);
Entity npc_ecs_find_nearest(NPCECSSystem* system, Vec3 position, f32 max_distance);
u32 npc_ecs_get_active_count(NPCECSSystem* system);

// ============================================================================
// NPC BEHAVIOR INTEGRATION
// ============================================================================

// AI behavior updates
void npc_ecs_update_ai(NPCECSSystem* system, Entity npc_entity, f32 delta_time);
void npc_ecs_update_pathfinding(NPCECSSystem* system, Entity npc_entity, Vec3 target);
void npc_ecs_update_combat(NPCECSSystem* system, Entity npc_entity, f32 delta_time);

// State management
void npc_ecs_set_state(NPCECSSystem* system, Entity npc_entity, const char* state);
const char* npc_ecs_get_state(NPCECSSystem* system, Entity npc_entity);

// ============================================================================
// PERFORMANCE OPTIMIZATIONS
// ============================================================================

// LOD management
void npc_ecs_update_lod(NPCECSSystem* system, Vec3 player_position);
void npc_ecs_set_lod_distances(NPCECSSystem* system, f32 high_detail, f32 medium_detail, f32 low_detail);

// Scheduled updates
void npc_ecs_schedule_update(NPCECSSystem* system, Entity npc_entity, f32 delay);
void npc_ecs_process_scheduled_updates(NPCECSSystem* system, f32 delta_time);

// ============================================================================
// STATISTICS AND DEBUGGING
// ============================================================================

typedef struct {
    u32 total_npcs_created;
    u32 total_npcs_destroyed;
    u32 active_npcs;
    u32 culled_npcs;
    u32 high_detail_npcs;
    u32 medium_detail_npcs;
    u32 low_detail_npcs;
    f64 average_update_time;
    f64 ai_update_time;
    f64 physics_update_time;
} NPCECSSystemStats;

NPCECSSystemStats npc_ecs_get_stats(NPCECSSystem* system);
void npc_ecs_reset_stats(NPCECSSystem* system);

#ifdef __cplusplus
}
#endif

#endif // AI_NPC_ECS_INTEGRATION_H
