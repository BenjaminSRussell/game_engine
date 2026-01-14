#include "ai/group/squad_tactics.h"
#include <core/logger.h>
#include <math/math_all.h>
#include <math/vec3.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Helper: Normalize vector safely
static Vec3 vec3_normalize_safe(Vec3 v) {
    float length = vec3_length(v);
    if (length < 0.0001f) {
        return (Vec3){0, 0, 0};
    }
    return vec3_mul(v, 1.0f / length);
}

// Helper: Rotate vector around Y axis
static Vec3 vec3_rotate_y(Vec3 v, float angle_rad) {
    float c = cosf(angle_rad);
    float s = sinf(angle_rad);
    return (Vec3){
        v.x * c + v.z * s,
        v.y,
        -v.x * s + v.z * c
    };
}

Squad* squad_create(u32 capacity) {
    Squad* squad = (Squad*)calloc(1, sizeof(Squad));
    if (!squad) {
        LOG_ERROR_CAT(LOG_CAT_AI, "Failed to allocate Squad");
        return NULL;
    }

    if (!squad_init(squad, capacity)) {
        free(squad);
        return NULL;
    }

    return squad;
}

void squad_destroy(Squad* squad) {
    if (!squad) return;
    squad_shutdown(squad);
    free(squad);
}

bool squad_init(Squad* squad, u32 capacity) {
    if (!squad) return false;

    memset(squad, 0, sizeof(Squad));

    if (capacity > 0) {
        squad->members = (SquadMember*)calloc(capacity, sizeof(SquadMember));
        if (!squad->members) {
            LOG_ERROR_CAT(LOG_CAT_AI, "Failed to allocate squad members");
            return false;
        }
    }

    squad->capacity = capacity;
    squad->member_count = 0;
    squad->current_formation = FORMATION_WEDGE;
    squad->spacing = 2.0f;
    squad->leader_direction = (Vec3){0, 0, 1}; // Default forward
    squad->initialized = true;

    LOG_INFO_CAT(LOG_CAT_AI, "Squad initialized with capacity %u", capacity);
    return true;
}

void squad_shutdown(Squad* squad) {
    if (!squad || !squad->initialized) return;

    if (squad->members) {
        free(squad->members);
        squad->members = NULL;
    }

    squad->initialized = false;
    LOG_INFO_CAT(LOG_CAT_AI, "Squad shutdown");
}

bool squad_add_member(Squad* squad, Entity entity) {
    if (!squad || !squad->initialized) return false;

    if (squad->member_count >= squad->capacity) {
        LOG_WARN(LOG_CAT_AI, "Squad full, cannot add member");
        return false;
    }

    // Check if already in squad
    for (u32 i = 0; i < squad->member_count; i++) {
        if (squad->members[i].entity.id == entity.id) {
            return false;
        }
    }

    squad->members[squad->member_count].entity = entity;
    squad->members[squad->member_count].is_leader = false;
    squad->member_count++;

    LOG_DEBUG_CAT(LOG_CAT_AI, "Added member to squad (total: %u)", squad->member_count);
    return true;
}

bool squad_remove_member(Squad* squad, Entity entity) {
    if (!squad || !squad->initialized) return false;

    for (u32 i = 0; i < squad->member_count; i++) {
        if (squad->members[i].entity.id == entity.id) {
            // Check if leader
            if (squad->members[i].is_leader) {
                squad->leader_entity = (Entity){0};
            }

            // Shift remaining
            for (u32 j = i; j < squad->member_count - 1; j++) {
                squad->members[j] = squad->members[j + 1];
            }
            squad->member_count--;
            return true;
        }
    }
    return false;
}

void squad_set_leader(Squad* squad, Entity entity) {
    if (!squad || !squad->initialized) return;

    squad->leader_entity = entity;

    bool found = false;
    for (u32 i = 0; i < squad->member_count; i++) {
        if (squad->members[i].entity.id == entity.id) {
            squad->members[i].is_leader = true;
            found = true;
        } else {
            squad->members[i].is_leader = false;
        }
    }

    if (!found) {
        // Optionally add leader to squad if not present
        if (squad_add_member(squad, entity)) {
            // Find newly added member and set as leader
             for (u32 i = 0; i < squad->member_count; i++) {
                if (squad->members[i].entity.id == entity.id) {
                    squad->members[i].is_leader = true;
                    break;
                }
            }
        }
    }

    LOG_INFO_CAT(LOG_CAT_AI, "Squad leader set to Entity %u", entity.id);
}

SquadMember* squad_get_member(Squad* squad, Entity entity) {
    if (!squad || !squad->initialized) return NULL;
    for (u32 i = 0; i < squad->member_count; i++) {
        if (squad->members[i].entity.id == entity.id) {
            return &squad->members[i];
        }
    }
    return NULL;
}

void squad_set_formation(Squad* squad, FormationType type, float spacing) {
    if (!squad) return;
    squad->current_formation = type;
    if (spacing > 0) squad->spacing = spacing;

    // Recalculate offsets immediately?
    // Usually updated in update loop, but can be forced here.
    LOG_DEBUG_CAT(LOG_CAT_AI, "Squad formation set to %d", type);
}

void squad_set_facing(Squad* squad, Vec3 direction) {
    if (!squad) return;
    if (vec3_length_sq(direction) > 0.0001f) {
        squad->leader_direction = vec3_normalize_safe(direction);
    }
}

Vec3 squad_get_formation_position(const Squad* squad, u32 member_index) {
    if (!squad) return (Vec3){0, 0, 0};

    Vec3 pos = {0, 0, 0};
    float spacing = squad->spacing;

    // Skip leader slot for logic usually, but member_index here is "position index"
    // Let's assume index 0 is center/leader position

    if (member_index == 0) return pos;

    switch (squad->current_formation) {
        case FORMATION_LINE:
            // Left/Right
            {
                // 1=L, 2=R, 3=L2, 4=R2
                float side = (member_index % 2 != 0) ? -1.0f : 1.0f;
                float dist = ((member_index + 1) / 2) * spacing;
                pos.x = side * dist;
            }
            break;

        case FORMATION_COLUMN:
            // Behind
            pos.z = -((float)member_index * spacing);
            break;

        case FORMATION_WEDGE:
            {
                // V shape behind
                float side = (member_index % 2 != 0) ? -1.0f : 1.0f;
                float row = (float)((member_index + 1) / 2);
                pos.x = side * row * spacing;
                pos.z = -row * spacing;
            }
            break;

        case FORMATION_V:
             {
                // Same as wedge but maybe wider angle?
                float side = (member_index % 2 != 0) ? -1.0f : 1.0f;
                float row = (float)((member_index + 1) / 2);
                pos.x = side * row * spacing * 1.5f; // Wider
                pos.z = -row * spacing * 0.5f; // Shallower
            }
            break;

        case FORMATION_CIRCLE:
            {
                u32 count = squad->member_count > 1 ? squad->member_count - 1 : 1;
                float angle = (float)(member_index - 1) * (2.0f * M_PI / (float)count);
                pos.x = cosf(angle) * spacing * 2.0f;
                pos.z = sinf(angle) * spacing * 2.0f;
            }
            break;

        case FORMATION_LOOSE:
            {
                // Deterministic pseudo-random
                u32 seed = member_index * 2654435761;
                float rx = ((seed & 0xFFFF) / 65536.0f - 0.5f) * 2.0f;
                float rz = (((seed >> 16) & 0xFFFF) / 65536.0f - 0.5f) * 2.0f;
                pos.x = rx * spacing * 3.0f;
                pos.z = rz * spacing * 3.0f;
            }
            break;
    }

    return pos;
}

void squad_update(Squad* squad, float delta_time) {
    if (!squad || !squad->initialized) return;

    // Calculate rotation angle from default forward (0,0,1) to leader_direction
    // theta = atan2(dir.x, dir.z)
    // Actually, atan2 returns angle from X axis (usually).
    // Let's compute angle to rotate (0,0,1) to (dir.x, dir.y, dir.z) around Y.
    // atan2(x, z) gives angle from Z axis?
    // z=1, x=0 -> atan2(0, 1) = 0. Correct.
    // z=0, x=1 -> atan2(1, 0) = PI/2. Correct.
    float rotation_angle = atan2f(squad->leader_direction.x, squad->leader_direction.z);

    // Assign formation positions
    u32 current_slot = 0;

    // First pass: find leader index to skip or treat specially
    // But we iterate members. We assign slots 0..N-1?
    // Leader takes slot 0. Others take 1..N-1.

    for (u32 i = 0; i < squad->member_count; i++) {
        SquadMember* member = &squad->members[i];
        u32 slot;

        if (member->is_leader) {
            slot = 0;
        } else {
            // Increment slot counter (skip 0 for leader)
            current_slot++;
            slot = current_slot;
        }

        // Calculate offset in local space
        Vec3 local_offset = squad_get_formation_position(squad, slot);
        member->offset_from_leader = local_offset;

        // Rotate offset to world space
        Vec3 world_offset = vec3_rotate_y(local_offset, rotation_angle);

        // Add to leader position
        member->current_target_pos = vec3_add(squad->leader_position, world_offset);

        // If it's the leader, target is their own position (or move target?)
        if (member->is_leader) {
            // Logic depends on state. If moving, leader moves to move_target.
            if (squad->current_state == SQUAD_STATE_MOVING) {
                member->current_target_pos = squad->move_target;
            } else {
                member->current_target_pos = squad->leader_position;
            }
        }
    }
}

void squad_form_up(Squad* squad) {
    if (!squad) return;
    squad->current_state = SQUAD_STATE_MOVING; // Moving to formation
    LOG_INFO_CAT(LOG_CAT_AI, "Squad ordered to form up");
}

void squad_attack_target(Squad* squad, Entity target) {
    if (!squad) return;
    squad->current_state = SQUAD_STATE_ATTACKING;
    squad->current_target_entity = target;
    LOG_INFO_CAT(LOG_CAT_AI, "Squad ordered to attack target %u", target.id);
}

void squad_move_to(Squad* squad, Vec3 position) {
    if (!squad) return;
    squad->current_state = SQUAD_STATE_MOVING;
    squad->move_target = position;
    LOG_INFO_CAT(LOG_CAT_AI, "Squad ordered to move to (%.2f, %.2f, %.2f)", position.x, position.y, position.z);
}

void squad_take_cover(Squad* squad) {
    if (!squad) return;
    squad->current_state = SQUAD_STATE_DEFENDING; // Or separate COVER state
    LOG_INFO_CAT(LOG_CAT_AI, "Squad ordered to take cover");
}

u32 squad_get_member_count(const Squad* squad) {
    return squad ? squad->member_count : 0;
}
