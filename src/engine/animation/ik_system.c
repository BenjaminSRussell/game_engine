// Inverse Kinematics system implementation
#include "include/animation/ik_system.h"
#include "include/core/logger.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Include the specific IK solver implementations
#include "character/animation/ik/fabrik_solver.h"
#include "animation/ik_advanced/fabrik_solver.c" // For the simple FABRIK implementation
#include "animation/ik_advanced/ccd_ik.c"       // For the advanced CCD implementation

IKSystem *ik_system_create(void) {
    IKSystem *system = malloc(sizeof(IKSystem));
    if (!system) {
        LOG_ERROR(LOG_CAT_ANIMATION, "Failed to allocate IK system");
        return NULL;
    }
    
    memset(system, 0, sizeof(IKSystem));
    
    // Initialize FABRIK solver
    if (animation_fabrik_solver_init() != 0) {
        LOG_ERROR(LOG_CAT_ANIMATION, "Failed to initialize FABRIK solver");
        free(system);
        return NULL;
    }
    
    // Initialize simple FABRIK
    fabrik_init();
    
    LOG_INFO(LOG_CAT_ANIMATION, "IK system created successfully");
    return system;
}

void ik_system_destroy(IKSystem *system) {
    if (!system) return;
    
    // Shutdown FABRIK solvers
    animation_fabrik_solver_shutdown();
    
    free(system);
    LOG_INFO(LOG_CAT_ANIMATION, "IK system destroyed");
}

u32 ik_add_chain(IKSystem *system, const char *name, IKSolverType solver) {
    if (!system || !name || system->chain_count >= MAX_IK_CHAINS) {
        return UINT32_MAX;
    }
    
    u32 chain_id = system->chain_count++;
    IKChain *chain = &system->chains[chain_id];
    
    strncpy(chain->name, name, sizeof(chain->name) - 1);
    chain->name[sizeof(chain->name) - 1] = '\0';
    chain->solver_type = solver;
    chain->bone_count = 0;
    chain->blend_weight = 1.0f;
    chain->position_enabled = true;
    chain->rotation_enabled = false;
    chain->max_iterations = 10;
    chain->precision = 0.001f;
    
    // Initialize target to origin
    chain->target_position = (Vec3){0, 0, 0};
    chain->target_rotation = (Quat){0, 0, 0, 1};
    chain->pole_vector = (Vec3){0, 1, 0};
    
    LOG_INFO(LOG_CAT_ANIMATION, "Added IK chain '%s' with solver type %d", name, solver);
    return chain_id;
}

void ik_set_target(IKSystem *system, u32 chain_id, Vec3 position, Quat rotation) {
    if (!system || chain_id >= system->chain_count) return;
    
    IKChain *chain = &system->chains[chain_id];
    chain->target_position = position;
    chain->target_rotation = rotation;
}

void ik_solve(IKSystem *system, u32 chain_id) {
    if (!system || chain_id >= system->chain_count) return;
    
    IKChain *chain = &system->chains[chain_id];
    
    switch (chain->solver_type) {
        case IK_SOLVER_TWO_BONE:
            ik_solve_two_bone(chain);
            break;
        case IK_SOLVER_FABRIK:
            ik_solve_fabrik(chain);
            break;
        case IK_SOLVER_CCD:
            ik_solve_ccd(chain);
            break;
        case IK_SOLVER_LIMB:
            ik_solve_two_bone(chain); // Use two-bone for limbs
            break;
        default:
            LOG_WARN(LOG_CAT_ANIMATION, "Unknown IK solver type: %d", chain->solver_type);
            break;
    }
}

void ik_solve_two_bone(IKChain *chain) {
    if (!chain || chain->bone_count < 2) return;
    
    // Two-bone IK analytical solution
    Vec3 start_pos = chain->bones[0].position;
    Vec3 target = chain->target_position;
    
    f32 l1 = chain->bones[0].length;
    f32 l2 = chain->bones[1].length;
    
    Vec3 to_target = vec3_sub(target, start_pos);
    f32 target_dist = vec3_length(to_target);
    
    // Check if target is reachable
    if (target_dist > l1 + l2) {
        // Stretch towards target
        Vec3 direction = vec3_normalize(to_target);
        chain->bones[1].position = vec3_add(start_pos, vec3_mul(direction, l1));
        if (chain->bone_count > 2) {
            chain->bones[2].position = vec3_add(chain->bones[1].position, vec3_mul(direction, l2));
        }
        return;
    }
    
    if (target_dist < fabsf(l1 - l2)) {
        // Target too close, bend as much as possible
        target_dist = fabsf(l1 - l2);
    }
    
    // Calculate joint angles using law of cosines
    f32 cos_angle2 = (l1 * l1 + target_dist * target_dist - l2 * l2) / (2.0f * l1 * target_dist);
    cos_angle2 = fmaxf(-1.0f, fminf(1.0f, cos_angle2));
    f32 angle2 = acosf(cos_angle2);
    
    f32 cos_angle1 = (l1 * l1 + l2 * l2 - target_dist * target_dist) / (2.0f * l1 * l2);
    cos_angle1 = fmaxf(-1.0f, fminf(1.0f, cos_angle1));
    f32 angle1 = acosf(cos_angle1);
    
    // Calculate rotation for first bone
    Quat rotation1 = quat_from_axis_angle(chain->pole_vector, angle2);
    chain->bones[0].rotation = quat_mul(rotation1, chain->bones[0].rotation);
    
    // Update positions
    Vec3 bone1_vec = vec3_create(l1, 0, 0);
    Vec3 rotated_bone1 = quat_rotate_vec3(chain->bones[0].rotation, bone1_vec);
    chain->bones[1].position = vec3_add(start_pos, rotated_bone1);
    
    if (chain->bone_count > 2) {
        Quat rotation2 = quat_from_axis_angle(chain->pole_vector, angle1);
        chain->bones[1].rotation = quat_mul(rotation2, chain->bones[1].rotation);

        Vec3 bone2_vec = vec3_create(l2, 0, 0);
        Vec3 rotated_bone2 = quat_rotate_vec3(chain->bones[1].rotation, bone2_vec);
        chain->bones[2].position = vec3_add(chain->bones[1].position, rotated_bone2);
    }
}

void ik_solve_fabrik(IKChain *chain) {
    if (!chain || chain->bone_count < 2) return;
    
    // Use the consolidated FABRIK implementation
    // Create positions array for FABRIK
    Vec3 positions[MAX_IK_CHAIN_LENGTH];
    for (u32 i = 0; i < chain->bone_count; i++) {
        positions[i] = chain->bones[i].position;
    }
    
    // Create a FABRIK chain using the simple implementation
    u32 chain_id = fabrik_create_chain(positions, chain->bone_count);
    if (chain_id == UINT32_MAX) {
        LOG_ERROR(LOG_CAT_ANIMATION, "Failed to create FABRIK chain");
        return;
    }
    
    // Add constraints if needed
    for (u32 i = 1; i < chain->bone_count - 1; i++) {
        // Add simple angle constraints (45 degrees min, 135 degrees max)
        fabrik_add_constraint(chain_id, i, 45.0f, 135.0f);
    }
    
    // Solve for target
    fabrik_solve(chain_id, &chain->target_position, chain->max_iterations);
    
    // Update bone positions from FABRIK result
    for (u32 i = 0; i < chain->bone_count; i++) {
        chain->bones[i].position = fabrik_get_joint_position(chain_id, i);
    }
    
    // Clean up
    fabrik_destroy_chain(chain_id);
}

void ik_solve_ccd(IKChain *chain) {
    // Use the advanced CCD solver implementation
    ik_ccd_solve_chain(chain);
}
