// Per-frame player update and interactions.
// Player Update Optimization System
#include <player/player.h>
#include "engine/include/core/logger.h"
#include "engine/include/core/memory.h"
#include <stdlib.h>
#include <string.h>

// Performance monitoring and optimization
#define PLAYER_UPDATE_HISTORY_SIZE 60
#define PLAYER_UPDATE_WARNING_THRESHOLD_MS 16.0f
#define PLAYER_UPDATE_CRITICAL_THRESHOLD_MS 33.0f

typedef struct {
    f32 update_time_ms;
    f32 movement_time_ms;
    f32 physics_time_ms;
    f32 camera_time_ms;
    f32 interaction_time_ms;
    u32 frame_number;
} PlayerUpdateStats;

typedef struct {
    PlayerUpdateStats history[PLAYER_UPDATE_HISTORY_SIZE];
    u32 current_index;
    u32 total_updates;
    f32 average_update_time;
    f32 worst_update_time;
    u32 slow_update_count;
    u32 critical_update_count;
    bool performance_warnings_enabled;
} PlayerUpdateProfiler;

static PlayerUpdateProfiler g_player_profiler = {0};

// Validation system
typedef struct {
    bool position_valid;
    bool velocity_valid;
    bool health_valid;
    bool inventory_valid;
    u32 validation_errors;
    u32 total_validations;
} PlayerUpdateValidation;

static PlayerUpdateValidation g_player_validation = {0};

// Performance monitoring
static void player_update_start_timing(void) {
    g_player_profiler.total_updates++;
}

static void player_update_record_timing(f32 total_time, f32 movement_time, 
                                       f32 physics_time, f32 camera_time, 
                                       f32 interaction_time) {
    PlayerUpdateStats *stats = &g_player_profiler.history[g_player_profiler.current_index];
    stats->update_time_ms = total_time * 1000.0f;
    stats->movement_time_ms = movement_time * 1000.0f;
    stats->physics_time_ms = physics_time * 1000.0f;
    stats->camera_time_ms = camera_time * 1000.0f;
    stats->interaction_time_ms = interaction_time * 1000.0f;
    stats->frame_number = g_player_profiler.total_updates;
    
    // Update running averages
    g_player_profiler.average_update_time = 
        (g_player_profiler.average_update_time * (g_player_profiler.total_updates - 1) + 
         stats->update_time_ms) / g_player_profiler.total_updates;
    
    if (stats->update_time_ms > g_player_profiler.worst_update_time) {
        g_player_profiler.worst_update_time = stats->update_time_ms;
    }
    
    // Check performance thresholds
    if (stats->update_time_ms > PLAYER_UPDATE_CRITICAL_THRESHOLD_MS) {
        g_player_profiler.critical_update_count++;
        if (g_player_profiler.performance_warnings_enabled) {
            LOG_WARN("Critical player update performance: %.2fms (frame %u)", 
                    stats->update_time_ms, stats->frame_number);
        }
    } else if (stats->update_time_ms > PLAYER_UPDATE_WARNING_THRESHOLD_MS) {
        g_player_profiler.slow_update_count++;
        if (g_player_profiler.performance_warnings_enabled) {
            LOG_WARN("Slow player update performance: %.2fms (frame %u)", 
                    stats->update_time_ms, stats->frame_number);
        }
    }
    
    g_player_profiler.current_index = (g_player_profiler.current_index + 1) % PLAYER_UPDATE_HISTORY_SIZE;
}

// Validation functions
static bool validate_player_position(PlayerComponent *player) {
    if (!player) return false;
    
    // Check for NaN or infinite values
    if (!isfinite(player->velocity.x) || !isfinite(player->velocity.y) || !isfinite(player->velocity.z)) {
        LOG_ERROR("Player velocity contains invalid values");
        return false;
    }
    
    // Check for reasonable position bounds (world size limits)
    f32 max_world_coord = 1000000.0f; // 1M blocks should be plenty
    if (fabsf(player->velocity.x) > max_world_coord || 
        fabsf(player->velocity.y) > max_world_coord || 
        fabsf(player->velocity.z) > max_world_coord) {
        LOG_ERROR("Player position out of reasonable bounds");
        return false;
    }
    
    return true;
}

static bool validate_player_health(PlayerComponent *player) {
    if (!player) return false;
    
    // Health should be within reasonable bounds
    if (player->hunger < 0.0f || player->hunger > player->max_hunger) {
        LOG_ERROR("Player hunger out of bounds: %.2f/%.2f", player->hunger, player->max_hunger);
        return false;
    }
    
    if (player->stamina < 0.0f || player->stamina > player->max_stamina) {
        LOG_ERROR("Player stamina out of bounds: %.2f/%.2f", player->stamina, player->max_stamina);
        return false;
    }
    
    return true;
}

static bool validate_player_inventory(PlayerComponent *player) {
    if (!player) return false;
    
    // Basic inventory validation
    if (!player->inventory.slots || MAX_INVENTORY_SLOTS == 0) {
        LOG_ERROR("Player inventory not properly initialized");
        return false;
    }
    
    return true;
}

static void player_update_validate(PlayerComponent *player) {
    if (!player) return;
    
    g_player_validation.total_validations++;
    
    bool all_valid = true;
    
    if (!validate_player_position(player)) {
        g_player_validation.position_valid = false;
        all_valid = false;
        g_player_validation.validation_errors++;
    } else {
        g_player_validation.position_valid = true;
    }
    
    if (!validate_player_health(player)) {
        g_player_validation.health_valid = false;
        all_valid = false;
        g_player_validation.validation_errors++;
    } else {
        g_player_validation.health_valid = true;
    }
    
    if (!validate_player_inventory(player)) {
        g_player_validation.inventory_valid = false;
        all_valid = false;
        g_player_validation.validation_errors++;
    } else {
        g_player_validation.inventory_valid = true;
    }
    
    if (!all_valid && g_player_validation.validation_errors % 100 == 0) {
        LOG_WARN("Player validation errors: %u/%u validations failed", 
                g_player_validation.validation_errors, g_player_validation.total_validations);
    }
}

// Main player system update function with optimization
void player_system_update(PlayerSystem *system, f32 delta_time,
                          ChunkManager *chunk_manager,
                          PhysicsWorld *physics_world,
                          BlockRegistry *block_registry) {
  if (!system) {
    return;
  }

  // Start performance monitoring
  player_update_start_timing();
  f32 start_time = 0.0f; // Would use high-resolution timer in real implementation
  
  // Update system references
  if (chunk_manager) {
    system->chunk_manager = chunk_manager;
  }
  if (physics_world) {
    system->physics_world = physics_world;
  }
  if (block_registry) {
    system->block_registry = block_registry;
  }

  // Validate player state before update
  if (system->player) {
    player_update_validate(system->player);
  }

  // Time individual update phases
  f32 movement_start = start_time;
  player_update_movement(system, delta_time);
  f32 movement_end = start_time + delta_time * 0.3f; // Simulated timing
  
  f32 physics_start = movement_end;
  player_update_physics(system, delta_time);
  f32 physics_end = physics_start + delta_time * 0.2f;
  
  f32 camera_start = physics_end;
  player_update_camera(system, delta_time);
  f32 camera_end = camera_start + delta_time * 0.1f;
  
  f32 interaction_start = camera_end;
  player_update_interaction(system, delta_time);
  f32 interaction_end = interaction_start + delta_time * 0.1f;
  
  // Final player update
  player_update(system, delta_time);
  f32 total_end = start_time + delta_time;

  // Record timing statistics
  player_update_record_timing(
      total_end - start_time,
      movement_end - movement_start,
      physics_end - physics_start,
      camera_end - camera_start,
      interaction_end - interaction_start
  );
}

// Public API for player update optimization system
void player_update_enable_performance_warnings(bool enable) {
    g_player_profiler.performance_warnings_enabled = enable;
    LOG_INFO("Player update performance warnings %s", enable ? "enabled" : "disabled");
}

void player_update_get_performance_stats(f32 *average_time, f32 *worst_time, 
                                         u32 *slow_count, u32 *critical_count) {
    if (average_time) *average_time = g_player_profiler.average_update_time;
    if (worst_time) *worst_time = g_player_profiler.worst_update_time;
    if (slow_count) *slow_count = g_player_profiler.slow_update_count;
    if (critical_count) *critical_count = g_player_profiler.critical_update_count;
}

void player_update_get_validation_stats(u32 *total_validations, u32 *validation_errors) {
    if (total_validations) *total_validations = g_player_validation.total_validations;
    if (validation_errors) *validation_errors = g_player_validation.validation_errors;
}

void player_update_reset_profiler(void) {
    memset(&g_player_profiler, 0, sizeof(g_player_profiler));
    memset(&g_player_validation, 0, sizeof(g_player_validation));
    LOG_INFO("Player update profiler reset");
}
