#ifndef SQUAD_TACTICS_H
#define SQUAD_TACTICS_H

#include <core/types.h>
#include <ecs/ecs.h>
#include <math/vec3.h>
#include <stdbool.h>

typedef enum {
    FORMATION_LINE,
    FORMATION_WEDGE,
    FORMATION_CIRCLE,
    FORMATION_COLUMN,
    FORMATION_V,
    FORMATION_LOOSE // No strict formation, just stay close
} FormationType;

typedef enum {
    SQUAD_STATE_IDLE,
    SQUAD_STATE_MOVING,
    SQUAD_STATE_ATTACKING,
    SQUAD_STATE_DEFENDING,
    SQUAD_STATE_RETREATING
} SquadState;

typedef struct {
    Entity entity;
    Vec3 offset_from_leader; // Assigned position in formation (relative to leader)
    Vec3 current_target_pos; // World space target position
    bool is_leader;
} SquadMember;

typedef struct {
    SquadMember* members;
    u32 member_count;
    u32 capacity;

    Entity leader_entity;
    Vec3 leader_position;   // Cached leader position
    Vec3 leader_direction;  // Cached leader forward direction

    FormationType current_formation;
    SquadState current_state;

    float spacing; // Distance between members

    // For specific commands
    Entity current_target_entity;
    Vec3 move_target;

    bool initialized;
} Squad;

// Management
Squad* squad_create(u32 capacity);
void squad_destroy(Squad* squad);
bool squad_init(Squad* squad, u32 capacity); // In-place init
void squad_shutdown(Squad* squad);

bool squad_add_member(Squad* squad, Entity entity);
bool squad_remove_member(Squad* squad, Entity entity);
void squad_set_leader(Squad* squad, Entity entity);
SquadMember* squad_get_member(Squad* squad, Entity entity);

// Behavior
void squad_update(Squad* squad, float delta_time);
void squad_set_formation(Squad* squad, FormationType type, float spacing);
void squad_set_facing(Squad* squad, Vec3 direction);

// Tactics
void squad_form_up(Squad* squad);
void squad_attack_target(Squad* squad, Entity target);
void squad_move_to(Squad* squad, Vec3 position);
void squad_take_cover(Squad* squad);

// Helpers
Vec3 squad_get_formation_position(const Squad* squad, u32 member_index);
u32 squad_get_member_count(const Squad* squad);

#endif // SQUAD_TACTICS_H
