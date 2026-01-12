#include "character_ik_manager.h"
#include "core/common/memory/allocator.h"
#include "core/logger.h"
#include <string.h>
#include <stdlib.h>

#define MAX_IK_CHAINS 16
#define MAX_IK_SOLVERS 32
#define IK_UPDATE_FREQUENCY 60.0f
#define MAX_IK_TARGETS 64

typedef enum ik_solver_type {
    IK_SOLVER_FOOT = 0,
    IK_SOLVER_HAND,
    IK_SOLVER_SPINE,
    IK_SOLVER_HEAD,
    IK_SOLVER_LOOK_AT,
    IK_SOLVER_FABRIK,
    IK_SOLVER_CCD,
    IK_SOLVER_TWO_BONE,
    IK_SOLVER_AIM
} ik_solver_type_t;

typedef enum ik_priority {
    IK_PRIORITY_LOW = 0,
    IK_PRIORITY_MEDIUM,
    IK_PRIORITY_HIGH,
    IK_PRIORITY_CRITICAL
} ik_priority_t;

typedef struct ik_target {
    uint32_t target_id;
    uint32_t entity_id;
    vec3_t world_position;
    vec3_t world_normal;
    quat_t world_rotation;
    
    ik_solver_type_t solver_type;
    ik_priority_t priority;
    
    float weight;
    float blend_speed;
    bool is_active;
    bool lock_rotation;
    bool lock_position;
    
    uint32_t creation_time;
    uint32_t last_update_time;
} ik_target_t;

typedef struct ik_chain {
    uint32_t chain_id;
    uint32_t entity_id;
    
    int bone_ids[MAX_IK_CHAIN_LENGTH];
    uint32_t bone_count;
    
    vec3_t bone_positions[MAX_IK_CHAIN_LENGTH];
    quat_t bone_rotations[MAX_IK_CHAIN_LENGTH];
    vec3_t bone_lengths[MAX_IK_CHAIN_LENGTH];
    
    ik_solver_type_t solver_type;
    float chain_length;
    float solve_tolerance;
    uint32_t max_iterations;
    
    bool is_enabled;
    float blend_weight;
} ik_chain_t;

typedef struct ik_solver {
    uint32_t solver_id;
    ik_solver_type_t type;
    
    union {
        foot_ik_solver_t foot_solver;
        hand_ik_solver_t hand_solver;
        spine_ik_solver_t spine_solver;
        look_at_solver_t look_at_solver;
        fabrik_solver_t fabrik_solver;
        ccd_solver_t ccd_solver;
        two_bone_ik_t two_bone_solver;
        aim_ik_solver_t aim_solver;
    } solver_data;
    
    uint32_t chain_ids[MAX_IK_CHAINS];
    uint32_t chain_count;
    
    bool is_enabled;
    float solve_time_ms;
    uint32_t solve_iterations;
} ik_solver_t;

typedef struct character_ik_manager {
    ik_target_t targets[MAX_IK_TARGETS];
    uint32_t target_count;
    
    ik_chain_t chains[MAX_IK_CHAINS];
    uint32_t chain_count;
    
    ik_solver_t solvers[MAX_IK_SOLVERS];
    uint32_t solver_count;
    
    uint32_t next_target_id;
    uint32_t next_chain_id;
    uint32_t next_solver_id;
    
    float update_frequency;
    float accumulated_time;
    
    bool initialized;
} character_ik_manager_t;

static character_ik_manager_t g_ik_manager = {0};

// Forward declarations
static ik_target_t* get_target(uint32_t target_id);
static ik_chain_t* get_chain(uint32_t chain_id);
static ik_solver_t* get_solver(uint32_t solver_id);
static bool solve_chain(ik_chain_t* chain, ik_solver_t* solver, const ik_target_t* target);
static void update_chain_transforms(ik_chain_t* chain);
static void blend_ik_results(ik_chain_t* chain, float blend_weight);

bool character_ik_init(void) {
    if (g_ik_manager.initialized) {
        return true;
    }
    
    memset(&g_ik_manager, 0, sizeof(g_ik_manager));
    g_ik_manager.next_target_id = 1;
    g_ik_manager.next_chain_id = 1;
    g_ik_manager.next_solver_id = 1;
    g_ik_manager.update_frequency = IK_UPDATE_FREQUENCY;
    
    g_ik_manager.initialized = true;
    log_info("Character IK manager initialized");
    return true;
}

void character_ik_shutdown(void) {
    if (!g_ik_manager.initialized) {
        return;
    }
    
    memset(&g_ik_manager, 0, sizeof(g_ik_manager));
    log_info("Character IK manager shutdown");
}

uint32_t character_ik_create_target(uint32_t entity_id, ik_solver_type_t type, ik_priority_t priority) {
    if (!g_ik_manager.initialized || g_ik_manager.target_count >= MAX_IK_TARGETS) {
        return 0;
    }
    
    ik_target_t* target = &g_ik_manager.targets[g_ik_manager.target_count];
    memset(target, 0, sizeof(ik_target_t));
    
    target->target_id = g_ik_manager.next_target_id++;
    target->entity_id = entity_id;
    target->solver_type = type;
    target->priority = priority;
    target->weight = 1.0f;
    target->blend_speed = 5.0f;
    target->is_active = true;
    target->creation_time = get_current_time_ms();
    target->last_update_time = target->creation_time;
    
    g_ik_manager.target_count++;
    log_debug("Created IK target %u for entity %u", target->target_id, entity_id);
    
    return target->target_id;
}

bool character_ik_set_target_position(uint32_t target_id, vec3_t position) {
    ik_target_t* target = get_target(target_id);
    if (!target) {
        return false;
    }
    
    target->world_position = position;
    target->last_update_time = get_current_time_ms();
    
    return true;
}

bool character_ik_set_target_rotation(uint32_t target_id, quat_t rotation) {
    ik_target_t* target = get_target(target_id);
    if (!target) {
        return false;
    }
    
    target->world_rotation = rotation;
    target->last_update_time = get_current_time_ms();
    
    return true;
}

bool character_ik_set_target_weight(uint32_t target_id, float weight) {
    ik_target_t* target = get_target(target_id);
    if (!target) {
        return false;
    }
    
    target->weight = clampf(weight, 0.0f, 1.0f);
    return true;
}

bool character_ik_lock_target_position(uint32_t target_id, bool lock) {
    ik_target_t* target = get_target(target_id);
    if (!target) {
        return false;
    }
    
    target->lock_position = lock;
    return true;
}

bool character_ik_lock_target_rotation(uint32_t target_id, bool lock) {
    ik_target_t* target = get_target(target_id);
    if (!target) {
        return false;
    }
    
    target->lock_rotation = lock;
    return true;
}

uint32_t character_ik_create_chain(uint32_t entity_id, const int* bone_ids, uint32_t bone_count, ik_solver_type_t solver_type) {
    if (!g_ik_manager.initialized || !bone_ids || bone_count == 0 || bone_count > MAX_IK_CHAIN_LENGTH ||
        g_ik_manager.chain_count >= MAX_IK_CHAINS) {
        return 0;
    }
    
    ik_chain_t* chain = &g_ik_manager.chains[g_ik_manager.chain_count];
    memset(chain, 0, sizeof(ik_chain_t));
    
    chain->chain_id = g_ik_manager.next_chain_id++;
    chain->entity_id = entity_id;
    chain->bone_count = bone_count;
    chain->solver_type = solver_type;
    chain->solve_tolerance = 0.001f;
    chain->max_iterations = 10;
    chain->is_enabled = true;
    chain->blend_weight = 1.0f;
    
    // Copy bone IDs
    memcpy(chain->bone_ids, bone_ids, bone_count * sizeof(int));
    
    // Calculate chain length
    chain->chain_length = 0.0f;
    for (uint32_t i = 0; i < bone_count - 1; i++) {
        // This would get bone positions from the animation system
        // For now, use placeholder values
        vec3_t bone_length = (vec3_t){0.0f, 0.5f, 0.0f};  // Placeholder
        chain->bone_lengths[i] = vec3_length(bone_length);
        chain->chain_length += chain->bone_lengths[i];
    }
    
    g_ik_manager.chain_count++;
    log_debug("Created IK chain %u for entity %u with %u bones", chain->chain_id, entity_id, bone_count);
    
    return chain->chain_id;
}

bool character_ik_set_chain_solver(uint32_t chain_id, uint32_t solver_id) {
    ik_chain_t* chain = get_chain(chain_id);
    ik_solver_t* solver = get_solver(solver_id);
    
    if (!chain || !solver) {
        return false;
    }
    
    chain->solver_type = solver->type;
    
    // Add chain to solver
    if (solver->chain_count < MAX_IK_CHAINS) {
        solver->chain_ids[solver->chain_count++] = chain_id;
    }
    
    return true;
}

bool character_ik_set_chain_blend_weight(uint32_t chain_id, float weight) {
    ik_chain_t* chain = get_chain(chain_id);
    if (!chain) {
        return false;
    }
    
    chain->blend_weight = clampf(weight, 0.0f, 1.0f);
    return true;
}

uint32_t character_ik_create_solver(ik_solver_type_t type) {
    if (!g_ik_manager.initialized || g_ik_manager.solver_count >= MAX_IK_SOLVERS) {
        return 0;
    }
    
    ik_solver_t* solver = &g_ik_manager.solvers[g_ik_manager.solver_count];
    memset(solver, 0, sizeof(ik_solver_t));
    
    solver->solver_id = g_ik_manager.next_solver_id++;
    solver->type = type;
    solver->is_enabled = true;
    
    // Initialize specific solver data
    switch (type) {
        case IK_SOLVER_FOOT:
            // Initialize foot solver
            break;
        case IK_SOLVER_HAND:
            // Initialize hand solver
            break;
        case IK_SOLVER_SPINE:
            // Initialize spine solver
            break;
        case IK_SOLVER_LOOK_AT:
            // Initialize look-at solver
            break;
        case IK_SOLVER_FABRIK:
            // Initialize FABRIK solver
            break;
        case IK_SOLVER_CCD:
            // Initialize CCD solver
            break;
        case IK_SOLVER_TWO_BONE:
            // Initialize two-bone solver
            break;
        case IK_SOLVER_AIM:
            // Initialize aim solver
            break;
    }
    
    g_ik_manager.solver_count++;
    log_debug("Created IK solver %u of type %u", solver->solver_id, type);
    
    return solver->solver_id;
}

void character_ik_update(float delta_time) {
    if (!g_ik_manager.initialized) {
        return;
    }
    
    // Update based on frequency
    g_ik_manager.accumulated_time += delta_time;
    float update_interval = 1.0f / g_ik_manager.update_frequency;
    
    if (g_ik_manager.accumulated_time < update_interval) {
        return;
    }
    
    // Process all active targets by priority
    for (ik_priority_t priority = IK_PRIORITY_CRITICAL; priority <= IK_PRIORITY_LOW; priority++) {
        for (uint32_t i = 0; i < g_ik_manager.target_count; i++) {
            ik_target_t* target = &g_ik_manager.targets[i];
            
            if (!target->is_active || target->priority != priority) {
                continue;
            }
            
            // Find chains that use this target type
            for (uint32_t j = 0; j < g_ik_manager.chain_count; j++) {
                ik_chain_t* chain = &g_ik_manager.chains[j];
                
                if (!chain->is_enabled || chain->entity_id != target->entity_id ||
                    chain->solver_type != target->solver_type) {
                    continue;
                }
                
                // Find solver for this chain
                ik_solver_t* solver = NULL;
                for (uint32_t k = 0; k < g_ik_manager.solver_count; k++) {
                    if (g_ik_manager.solvers[k].type == chain->solver_type && g_ik_manager.solvers[k].is_enabled) {
                        solver = &g_ik_manager.solvers[k];
                        break;
                    }
                }
                
                if (solver && solve_chain(chain, solver, target)) {
                    update_chain_transforms(chain);
                    blend_ik_results(chain, chain->blend_weight);
                }
            }
        }
    }
    
    g_ik_manager.accumulated_time = 0.0f;
}

bool character_ik_enable_target(uint32_t target_id, bool enable) {
    ik_target_t* target = get_target(target_id);
    if (!target) {
        return false;
    }
    
    target->is_active = enable;
    return true;
}

bool character_ik_enable_chain(uint32_t chain_id, bool enable) {
    ik_chain_t* chain = get_chain(chain_id);
    if (!chain) {
        return false;
    }
    
    chain->is_enabled = enable;
    return true;
}

bool character_ik_enable_solver(uint32_t solver_id, bool enable) {
    ik_solver_t* solver = get_solver(solver_id);
    if (!solver) {
        return false;
    }
    
    solver->is_enabled = enable;
    return true;
}

void character_ik_set_update_frequency(float frequency) {
    if (!g_ik_manager.initialized || frequency <= 0.0f) {
        return;
    }
    
    g_ik_manager.update_frequency = frequency;
    g_ik_manager.accumulated_time = 0.0f;
}

void character_ik_get_statistics(uint32_t* active_targets, uint32_t* active_chains, 
                                uint32_t* active_solvers, float* solve_time_ms) {
    if (!g_ik_manager.initialized) {
        return;
    }
    
    uint32_t targets = 0, chains = 0, solvers = 0;
    float total_solve_time = 0.0f;
    
    for (uint32_t i = 0; i < g_ik_manager.target_count; i++) {
        if (g_ik_manager.targets[i].is_active) targets++;
    }
    
    for (uint32_t i = 0; i < g_ik_manager.chain_count; i++) {
        if (g_ik_manager.chains[i].is_enabled) chains++;
    }
    
    for (uint32_t i = 0; i < g_ik_manager.solver_count; i++) {
        if (g_ik_manager.solvers[i].is_enabled) {
            solvers++;
            total_solve_time += g_ik_manager.solvers[i].solve_time_ms;
        }
    }
    
    if (active_targets) *active_targets = targets;
    if (active_chains) *active_chains = chains;
    if (active_solvers) *active_solvers = solvers;
    if (solve_time_ms) *solve_time_ms = solvers > 0 ? total_solve_time / solvers : 0.0f;
}

// Static helper functions
static ik_target_t* get_target(uint32_t target_id) {
    for (uint32_t i = 0; i < g_ik_manager.target_count; i++) {
        if (g_ik_manager.targets[i].target_id == target_id) {
            return &g_ik_manager.targets[i];
        }
    }
    return NULL;
}

static ik_chain_t* get_chain(uint32_t chain_id) {
    for (uint32_t i = 0; i < g_ik_manager.chain_count; i++) {
        if (g_ik_manager.chains[i].chain_id == chain_id) {
            return &g_ik_manager.chains[i];
        }
    }
    return NULL;
}

static ik_solver_t* get_solver(uint32_t solver_id) {
    for (uint32_t i = 0; i < g_ik_manager.solver_count; i++) {
        if (g_ik_manager.solvers[i].solver_id == solver_id) {
            return &g_ik_manager.solvers[i];
        }
    }
    return NULL;
}

static bool solve_chain(ik_chain_t* chain, ik_solver_t* solver, const ik_target_t* target) {
    // This would call the specific solver based on type
    // For now, return true as a placeholder
    return true;
}

static void update_chain_transforms(ik_chain_t* chain) {
    // This would update the actual bone transforms in the animation system
    // based on the IK solve results
}

static void blend_ik_results(ik_chain_t* chain, float blend_weight) {
    // This would blend the IK results with the original animation
    // based on the blend weight
}

uint32_t get_current_time_ms(void) {
    // Placeholder - would typically use platform-specific time function
    static uint32_t counter = 0;
    return counter += 16;  // Simulate 60 FPS
}
