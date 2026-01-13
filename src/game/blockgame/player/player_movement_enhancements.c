// player/player_movement_enhancements.c
//
// Purpose: Implementation of enhanced player movement systems including
// accessibility features, profiling, optimization, statistics tracking,
// validation, smoothing, and documentation systems.
//

#include <player/player_movement_enhancements.h>
#include <player/player.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Initialize player movement enhancements
void player_movement_enhancements_init(PlayerMovementEnhancements *enhancements) {
    if (!enhancements)
        return;
    
    // Initialize accessibility with defaults
    player_movement_accessibility_load_defaults(&enhancements->accessibility);
    
    // Initialize profiling
    memset(&enhancements->profile, 0, sizeof(PlayerMovementProfile));
    enhancements->profile.needs_optimization = false;
    
    // Initialize statistics
    memset(&enhancements->stats, 0, sizeof(PlayerMovementStats));
    enhancements->stats.session_start_time = (f32)time(NULL);
    
    // Initialize validation
    enhancements->validation.max_allowed_speed = 20.0f;
    enhancements->validation.max_vertical_speed = 15.0f;
    enhancements->validation.max_acceleration = 50.0f;
    enhancements->validation.max_angular_speed = 5.0f;
    enhancements->validation.max_penetration_depth = 0.1f;
    enhancements->validation.validate_collisions = true;
    enhancements->validation.auto_correct_penetration = true;
    enhancements->validation.validate_ground_contact = true;
    enhancements->validation.validate_swimming_state = true;
    enhancements->validation.validate_flying_state = true;
    enhancements->validation.log_validation_errors = false;
    
    // Initialize smoothing
    enhancements->smoothing.position_smoothing_factor = 0.2f;
    enhancements->smoothing.velocity_smoothing_factor = 0.3f;
    enhancements->smoothing.rotation_smoothing_factor = 0.15f;
    enhancements->smoothing.camera_smoothing_factor = 0.25f;
    enhancements->smoothing.smoothing_enabled = true;
    enhancements->smoothing.max_smoothing_time = 0.1f;
    enhancements->smoothing.prediction_enabled = false;
    enhancements->smoothing.network_smoothing_enabled = false;
    enhancements->smoothing.network_interpolation_delay = 0.1f;
    
    // Initialize documentation
    memset(&enhancements->documentation, 0, sizeof(PlayerMovementDocumentation));
    enhancements->documentation.help_enabled = true;
    enhancements->documentation.context_sensitive_help = true;
    enhancements->documentation.tutorial_enabled = true;
    enhancements->documentation.show_tips = true;
    enhancements->documentation.show_performance_info = false;
    enhancements->documentation.show_statistics = false;
    enhancements->documentation.show_profiling_info = false;
    
    // Set up movement tips
    const char* tips[] = {
        "Hold Shift to sprint and move faster",
        "Press Space to jump - double-tap for higher jumps",
        "Press Ctrl to crouch and fit in small spaces",
        "Swim by looking up and forward in water",
        "Fly by double-tapping Space in creative mode",
        "Mine blocks faster by using the right tool",
        "Place blocks by right-clicking with a block",
        "Use number keys to quickly select hotbar items",
        "Press E to open your inventory",
        "F3 toggles debug information display"
    };
    
    for (int i = 0; i < 10 && i < sizeof(tips)/sizeof(tips[0]); i++) {
        strncpy(enhancements->documentation.movement_tips[i], tips[i], 127);
        enhancements->documentation.movement_tips[i][127] = '\0';
    }
    
    // Initialize system state
    enhancements->initialized = true;
    enhancements->update_timer = 0.0f;
    enhancements->frame_count = 0;
    enhancements->enable_all_features = true;
    enhancements->debug_mode = false;
    
    LOG_INFO("Player movement enhancements initialized");
}

// Free player movement enhancements
void player_movement_enhancements_free(PlayerMovementEnhancements *enhancements) {
    if (!enhancements)
        return;
    
    memset(enhancements, 0, sizeof(PlayerMovementEnhancements));
    LOG_INFO("Player movement enhancements freed");
}

// Load default accessibility settings
void player_movement_accessibility_load_defaults(PlayerMovementAccessibility *accessibility) {
    if (!accessibility)
        return;
    
    accessibility->movement_sensitivity = 1.0f;
    accessibility->look_sensitivity = 1.0f;
    accessibility->deadzone_size = 0.1f;
    accessibility->auto_jump_enabled = false;
    accessibility->sprint_assist_enabled = false;
    accessibility->sticky_mining_enabled = false;
    accessibility->auto_crouch_enabled = false;
    accessibility->movement_trails_enabled = false;
    accessibility->velocity_indicators_enabled = false;
    accessibility->collision_highlight_enabled = false;
    accessibility->aim_assist_strength = 0.0f;
    accessibility->reduced_gravity_mode = false;
    accessibility->movement_speed_modifier = 1.0f;
    accessibility->inverted_controls = false;
    accessibility->simplified_controls = false;
    accessibility->control_scheme = 0;
}

// Update accessibility system
void player_movement_accessibility_update(PlayerMovementEnhancements *enhancements, 
                                     PlayerSystem *player_system, f32 delta_time) {
    if (!enhancements || !player_system || !player_system->player)
        return;
    
    PlayerComponent *player = player_system->player;
    PlayerMovementAccessibility *acc = &enhancements->accessibility;
    
    // Apply sensitivity adjustments
    if (acc->movement_sensitivity != 1.0f) {
        player->walk_speed *= acc->movement_sensitivity;
        player->sprint_speed *= acc->movement_sensitivity;
        player->run_speed *= acc->movement_sensitivity;
        player->fly_speed *= acc->movement_sensitivity;
    }
    
    // Apply speed modifier
    if (acc->movement_speed_modifier != 1.0f) {
        player->walk_speed *= acc->movement_speed_modifier;
        player->sprint_speed *= acc->movement_speed_modifier;
        player->run_speed *= acc->movement_speed_modifier;
        player->fly_speed *= acc->movement_speed_modifier;
    }
    
    // Apply reduced gravity
    if (acc->reduced_gravity_mode) {
        player->gravity *= 0.5f; // Reduce gravity by 50%
    }
    
    // Auto-jump assistance
    if (acc->auto_jump_enabled && player->on_ground && !player->is_jumping) {
        // Check if player is moving forward and approaching a block edge
        Vec3 forward = player_get_forward(player_system);
        Vec3 next_pos = vec3_add(player_get_position(player_system), 
                                  vec3_mul(forward, 0.5f));
        
        Vec3 check_pos = vec3(next_pos.x, next_pos.y - 1.0f, next_pos.z);
        Vec3 size = vec3(0.3f, 0.1f, 0.3f);
        
        if (!player_check_block_collision(player_system, check_pos, size, NULL)) {
            // Edge detected - auto jump
            player_jump(player_system);
        }
    }
    
    // Sprint assistance
    if (acc->sprint_assist_enabled && !player->is_sprinting) {
        Vec3 velocity = player->velocity;
        f32 forward_speed = sqrtf(velocity.x * velocity.x + velocity.z * velocity.z);
        if (forward_speed > player->walk_speed * 0.8f && player->on_ground) {
            player_sprint(player_system, true);
        }
    }
    
    // Auto-crouch in low spaces
    if (acc->auto_crouch_enabled && !player->is_crouching) {
        Vec3 head_pos = player_get_position(player_system);
        head_pos.y += 1.6f; // Head height
        
        Vec3 head_size = vec3(0.3f, 0.2f, 0.3f);
        if (player_check_block_collision(player_system, head_pos, head_size, NULL)) {
            player_crouch(player_system, true);
        }
    }
}

// Start profiling frame
void player_movement_profiling_start_frame(PlayerMovementEnhancements *enhancements) {
    if (!enhancements || !enhancements->enable_all_features)
        return;
    
    // Start timing - in a real implementation, this would use high-precision timers
    enhancements->profile.update_time_ms = 0.0f;
    enhancements->profile.physics_time_ms = 0.0f;
    enhancements->profile.collision_time_ms = 0.0f;
    enhancements->profile.smoothing_time_ms = 0.0f;
}

// End profiling frame
void player_movement_profiling_end_frame(PlayerMovementEnhancements *enhancements) {
    if (!enhancements || !enhancements->enable_all_features)
        return;
    
    enhancements->profile.total_frames++;
    enhancements->frame_count++;
    
    f32 total_time = (enhancements->profile.update_time_ms + 
                      enhancements->profile.physics_time_ms + 
                      enhancements->profile.collision_time_ms + 
                      enhancements->profile.smoothing_time_ms);
    
    // Update frame statistics
    if (total_time > 16.0f) {
        enhancements->profile.slow_frames++;
    } else if (total_time < 8.0f) {
        enhancements->profile.fast_frames++;
    }
    
    // Update average frame time
    enhancements->profile.average_frame_time = 
        (enhancements->profile.average_frame_time * (enhancements->profile.total_frames - 1) + total_time) / 
        enhancements->profile.total_frames;
    
    // Check if optimization is needed
    if (total_time > 16.67f) { // 60 FPS threshold
        enhancements->profile.needs_optimization = true;
        snprintf(enhancements->profile.optimization_notes, sizeof(enhancements->profile.optimization_notes),
                "Movement update taking %.2fms - consider reducing collision checks or smoothing complexity",
                total_time);
    }
}

// Update movement statistics
void player_movement_stats_update(PlayerMovementEnhancements *enhancements, 
                             PlayerComponent *player, f32 delta_time) {
    if (!enhancements || !player || !enhancements->enable_all_features)
        return;
    
    PlayerMovementStats *stats = &enhancements->stats;
    
    // Update total movement time
    stats->total_movement_time += delta_time;
    
    // Calculate current speed
    f32 speed = vec3_length(player->velocity);
    stats->current_speed = speed;
    
    // Update velocity history
    stats->velocity_history[stats->velocity_history_index] = player->velocity;
    stats->velocity_history_index = (stats->velocity_history_index + 1) % 60;
    
    // Update max speed
    if (speed > stats->max_speed_achieved) {
        stats->max_speed_achieved = speed;
    }
    
    // Track movement states
    if (player->is_sprinting) {
        stats->time_sprinting += delta_time;
        stats->distance_sprinted += speed * delta_time;
    } else if (player->is_crouching) {
        stats->time_crouching += delta_time;
    } else if (speed > 0.1f) {
        stats->time_walking += delta_time;
        stats->distance_walked += speed * delta_time;
    } else {
        stats->time_idle += delta_time;
    }
    
    if (player->is_swimming) {
        stats->time_swimming += delta_time;
        stats->distance_swam += speed * delta_time;
        stats->swim_count++;
    }
    
    if (player->is_flying) {
        stats->time_flying += delta_time;
        stats->distance_flown += speed * delta_time;
    }
    
    if (!player->on_ground && player->velocity.y < 0) {
        stats->time_jumping += delta_time;
        stats->distance_fallen += fabsf(player->velocity.y) * delta_time;
    }
    
    // Update total distance
    stats->total_distance_traveled += speed * delta_time;
    
    // Calculate average speed
    if (stats->total_movement_time > 0) {
        stats->average_speed = stats->total_distance_traveled / stats->total_movement_time;
    }
    
    // Update play time
    stats->total_play_time += delta_time;
}

// Record movement action
void player_movement_stats_record_action(PlayerMovementStats *stats, const char *action) {
    if (!stats || !action)
        return;
    
    if (strcmp(action, "jump") == 0) {
        stats->jump_count++;
    } else if (strcmp(action, "double_jump") == 0) {
        stats->double_jump_count++;
    } else if (strcmp(action, "sprint") == 0) {
        stats->sprint_count++;
    } else if (strcmp(action, "crouch") == 0) {
        stats->crouch_count++;
    } else if (strcmp(action, "flight") == 0) {
        stats->flight_count++;
    } else if (strcmp(action, "mine") == 0) {
        stats->blocks_mined++;
    } else if (strcmp(action, "place") == 0) {
        stats->blocks_placed++;
    } else if (strcmp(action, "interact") == 0) {
        stats->entities_interacted++;
    } else if (strcmp(action, "damage") == 0) {
        stats->damage_taken++;
    } else if (strcmp(action, "death") == 0) {
        stats->deaths++;
    }
}

// Validate movement position
bool player_movement_validate_position(PlayerMovementValidation *validation, Vec3 position, Vec3 *correction) {
    if (!validation || !correction)
        return true;
    
    bool is_valid = true;
    *correction = vec3(0, 0, 0);
    
    // Check for NaN or infinite values
    if (!isfinite(position.x) || !isfinite(position.y) || !isfinite(position.z)) {
        *correction = vec3(0, 64, 0); // Reset to safe position
        is_valid = false;
        snprintf(validation->last_error, sizeof(validation->last_error), 
                "Invalid position values (NaN/Inf)");
    }
    
    // Check for extreme positions
    if (fabsf(position.x) > 1000000.0f || fabsf(position.y) > 1000000.0f || 
        fabsf(position.z) > 1000000.0f) {
        *correction = vec3(0, 64, 0); // Reset to safe position
        is_valid = false;
        snprintf(validation->last_error, sizeof(validation->last_error), 
                "Position out of world bounds");
    }
    
    if (!is_valid) {
        validation->validation_errors++;
        validation->last_correction_time = 0.0f;
    }
    
    return is_valid;
}

// Validate movement velocity
bool player_movement_validate_velocity(PlayerMovementValidation *validation, Vec3 velocity, Vec3 *correction) {
    if (!validation || !correction)
        return true;
    
    bool is_valid = true;
    *correction = vec3(0, 0, 0);
    
    f32 speed = vec3_length(velocity);
    
    // Check maximum speed
    if (speed > validation->max_allowed_speed) {
        Vec3 normalized_vel = vec3_normalize(velocity);
        *correction = vec3_mul(normalized_vel, validation->max_allowed_speed);
        is_valid = false;
        snprintf(validation->last_error, sizeof(validation->last_error), 
                "Speed %.2f exceeds maximum %.2f", speed, validation->max_allowed_speed);
    }
    
    // Check vertical speed
    if (fabsf(velocity.y) > validation->max_vertical_speed) {
        correction->y = (velocity.y > 0) ? validation->max_vertical_speed : -validation->max_vertical_speed;
        is_valid = false;
        if (strlen(validation->last_error) == 0) {
            snprintf(validation->last_error, sizeof(validation->last_error), 
                    "Vertical speed %.2f exceeds maximum %.2f", 
                    fabsf(velocity.y), validation->max_vertical_speed);
        }
    }
    
    if (!is_valid) {
        validation->validation_errors++;
        validation->last_correction_time = 0.0f;
    }
    
    return is_valid;
}

// Update validation system
void player_movement_validation_update(PlayerMovementEnhancements *enhancements, 
                                 PlayerComponent *player, f32 delta_time) {
    if (!enhancements || !player || !enhancements->enable_all_features)
        return;
    
    PlayerMovementValidation *validation = &enhancements->validation;
    validation->last_correction_time += delta_time;
    
    // Validate position
    Vec3 position_correction;
    if (!player_movement_validate_position(validation, player->physics_body ? 
        rigid_body_get_position(player->physics_body) : vec3(0, 0, 0), &position_correction)) {
        if (validation->auto_correct_penetration && validation->last_correction_time > 0.1f) {
            player_movement_validation_apply_correction(player, position_correction);
            validation->corrections_applied++;
        }
    }
    
    // Validate velocity
    Vec3 velocity_correction;
    if (!player_movement_validate_velocity(validation, player->velocity, &velocity_correction)) {
        if (validation->auto_correct_penetration && validation->last_correction_time > 0.1f) {
            player->velocity = velocity_correction;
            if (player->physics_body) {
                rigid_body_set_velocity(player->physics_body, velocity_correction);
            }
            validation->corrections_applied++;
        }
    }
    
    // Log validation errors if enabled
    if (validation->log_validation_errors && strlen(validation->last_error) > 0) {
        LOG_WARN("Movement validation: %s", validation->last_error);
        validation->last_error[0] = '\0';
    }
}

// Apply validation correction
void player_movement_validation_apply_correction(PlayerComponent *player, Vec3 correction) {
    if (!player || !player->physics_body)
        return;
    
    rigid_body_set_position(player->physics_body, correction);
}

// Update movement smoothing
void player_movement_smoothing_update(PlayerMovementEnhancements *enhancements, 
                                  PlayerComponent *player, f32 delta_time) {
    if (!enhancements || !player || !enhancements->enable_all_features)
        return;
    
    PlayerMovementSmoothing *smoothing = &enhancements->smoothing;
    
    if (!smoothing->smoothing_enabled)
        return;
    
    smoothing->smoothing_timer += delta_time;
    
    // Get current position and velocity
    Vec3 current_pos = player->physics_body ? 
        rigid_body_get_position(player->physics_body) : vec3(0, 0, 0);
    Vec3 current_vel = player->velocity;
    Vec3 current_rot = vec3(player->yaw, player->pitch, 0);
    
    // Set targets if not already set
    if (vec3_length_sq(vec3_sub(smoothing->target_position, current_pos)) > 0.001f) {
        smoothing->target_position = current_pos;
        smoothing->target_velocity = current_vel;
        smoothing->target_rotation = current_rot;
        
        if (vec3_length_sq(smoothing->smoothed_position) < 0.001f) {
            // Initialize smoothed values
            smoothing->smoothed_position = current_pos;
            smoothing->smoothed_velocity = current_vel;
            smoothing->smoothed_rotation = current_rot;
        }
    }
    
    // Smooth position
    Vec3 pos_diff = vec3_sub(smoothing->target_position, smoothing->smoothed_position);
    smoothing->smoothed_position = vec3_add(smoothing->smoothed_position, 
                                          vec3_mul(pos_diff, smoothing->position_smoothing_factor));
    
    // Smooth velocity
    Vec3 vel_diff = vec3_sub(smoothing->target_velocity, smoothing->smoothed_velocity);
    smoothing->smoothed_velocity = vec3_add(smoothing->smoothed_velocity, 
                                          vec3_mul(vel_diff, smoothing->velocity_smoothing_factor));
    
    // Smooth rotation
    Vec3 rot_diff = vec3_sub(smoothing->target_rotation, smoothing->smoothed_rotation);
    smoothing->smoothed_rotation = vec3_add(smoothing->smoothed_rotation, 
                                          vec3_mul(rot_diff, smoothing->rotation_smoothing_factor));
    
    // Apply smoothed values
    if (player->physics_body) {
        rigid_body_set_position(player->physics_body, smoothing->smoothed_position);
        rigid_body_set_velocity(player->physics_body, smoothing->smoothed_velocity);
    }
    player->velocity = smoothing->smoothed_velocity;
    player->yaw = smoothing->smoothed_rotation.x;
    player->pitch = smoothing->smoothed_rotation.y;
    
    // Clamp smoothing timer
    if (smoothing->smoothing_timer > smoothing->max_smoothing_time) {
        smoothing->smoothing_timer = smoothing->max_smoothing_time;
    }
}

// Update documentation system
void player_movement_documentation_update(PlayerMovementEnhancements *enhancements, f32 delta_time) {
    if (!enhancements || !enhancements->enable_all_features)
        return;
    
    PlayerMovementDocumentation *docs = &enhancements->documentation;
    
    // Update tip display timer
    if (docs->show_tips) {
        docs->tip_display_timer += delta_time;
        if (docs->tip_display_timer > 10.0f) { // Show new tip every 10 seconds
            player_movement_documentation_cycle_tip(docs);
            docs->tip_display_timer = 0.0f;
        }
    }
    
    // Update tutorial display timer
    if (docs->tutorial_enabled && docs->tutorial_display_timer > 0.0f) {
        docs->tutorial_display_timer -= delta_time;
        if (docs->tutorial_display_timer <= 0.0f) {
            docs->current_tutorial[0] = '\0';
        }
    }
}

// Cycle through movement tips
void player_movement_documentation_cycle_tip(PlayerMovementDocumentation *docs) {
    if (!docs)
        return;
    
    docs->current_tip_index = (docs->current_tip_index + 1) % 10;
    LOG_INFO("Movement Tip: %s", docs->movement_tips[docs->current_tip_index]);
}

// Show help topic
void player_movement_documentation_show_help(PlayerMovementDocumentation *docs, const char *topic) {
    if (!docs || !topic)
        return;
    
    strncpy(docs->current_help_topic, topic, sizeof(docs->current_help_topic) - 1);
    docs->current_help_topic[sizeof(docs->current_help_topic) - 1] = '\0';
    
    LOG_INFO("Movement Help: %s", topic);
}

// Show tutorial
void player_movement_documentation_show_tutorial(PlayerMovementDocumentation *docs, const char *tutorial) {
    if (!docs || !tutorial)
        return;
    
    strncpy(docs->current_tutorial, tutorial, sizeof(docs->current_tutorial) - 1);
    docs->current_tutorial[sizeof(docs->current_tutorial) - 1] = '\0';
    docs->tutorial_display_timer = 5.0f; // Show for 5 seconds
    
    LOG_INFO("Movement Tutorial: %s", tutorial);
}

// Main update function
void player_movement_enhancements_update(PlayerMovementEnhancements *enhancements, 
                                     PlayerSystem *player_system, f32 delta_time) {
    if (!enhancements || !player_system || !enhancements->initialized)
        return;
    
    // Start profiling
    player_movement_profiling_start_frame(enhancements);
    
    // Update all systems
    player_movement_accessibility_update(enhancements, player_system, delta_time);
    player_movement_stats_update(enhancements, player_system->player, delta_time);
    player_movement_validation_update(enhancements, player_system->player, delta_time);
    player_movement_smoothing_update(enhancements, player_system->player, delta_time);
    player_movement_documentation_update(enhancements, delta_time);
    
    // Update profiling
    player_movement_profiling_end_frame(enhancements);
    
    enhancements->update_timer += delta_time;
}

// Reset movement enhancements
void player_movement_enhancements_reset(PlayerMovementEnhancements *enhancements) {
    if (!enhancements)
        return;
    
    // Reset statistics
    player_movement_stats_reset(&enhancements->stats);
    
    // Reset profiling
    player_movement_profiling_reset(&enhancements->profile);
    
    // Reset validation
    enhancements->validation.validation_errors = 0;
    enhancements->validation.corrections_applied = 0;
    enhancements->last_error[0] = '\0';
    
    // Reset smoothing
    memset(&enhancements->smoothing.smoothed_position, 0, sizeof(Vec3));
    memset(&enhancements->smoothing.target_position, 0, sizeof(Vec3));
    enhancements->smoothing.smoothing_timer = 0.0f;
    
    // Reset documentation
    enhancements->documentation.current_tip_index = 0;
    enhancements->documentation.tip_display_timer = 0.0f;
    enhancements->documentation.tutorial_display_timer = 0.0f;
    
    LOG_INFO("Player movement enhancements reset");
}

// Reset movement statistics
void player_movement_stats_reset(PlayerMovementStats *stats) {
    if (!stats)
        return;
    
    // Keep session data but reset movement data
    f32 session_start = stats->session_start_time;
    f32 total_play_time = stats->total_play_time;
    u32 session_count = stats->session_count;
    
    memset(stats, 0, sizeof(PlayerMovementStats));
    
    // Restore session data
    stats->session_start_time = session_start;
    stats->total_play_time = total_play_time;
    stats->session_count = session_count;
}

// Reset movement profiling
void player_movement_profiling_reset(PlayerMovementProfile *profile) {
    if (!profile)
        return;
    
    memset(profile, 0, sizeof(PlayerMovementProfile));
    profile->needs_optimization = false;
}

// Export movement statistics
void player_movement_stats_export(PlayerMovementStats *stats, char *buffer, size_t buffer_size) {
    if (!stats || !buffer || buffer_size == 0)
        return;
    
    snprintf(buffer, buffer_size,
            "=== MOVEMENT STATISTICS ===\n"
            "Total Distance: %.2f blocks\n"
            "Distance Walked: %.2f blocks\n"
            "Distance Sprinted: %.2f blocks\n"
            "Distance Swam: %.2f blocks\n"
            "Distance Flown: %.2f blocks\n"
            "Jump Count: %u\n"
            "Sprint Count: %u\n"
            "Max Speed: %.2f blocks/s\n"
            "Average Speed: %.2f blocks/s\n"
            "Total Play Time: %.2f minutes\n"
            "Blocks Mined: %u\n"
            "Blocks Placed: %u\n"
            "Deaths: %u\n",
            stats->total_distance_traveled,
            stats->distance_walked,
            stats->distance_sprinted,
            stats->distance_swam,
            stats->distance_flown,
            stats->jump_count,
            stats->sprint_count,
            stats->max_speed_achieved,
            stats->average_speed,
            stats->total_play_time / 60.0f,
            stats->blocks_mined,
            stats->blocks_placed,
            stats->deaths);
}

// Generate profiling report
void player_movement_profiling_generate_report(PlayerMovementProfile *profile, char *buffer, size_t buffer_size) {
    if (!profile || !buffer || buffer_size == 0)
        return;
    
    snprintf(buffer, buffer_size,
            "=== MOVEMENT PROFILING REPORT ===\n"
            "Total Frames: %u\n"
            "Average Frame Time: %.2f ms\n"
            "Slow Frames (>16ms): %u\n"
            "Fast Frames (<8ms): %u\n"
            "Position Updates: %u\n"
            "Collision Checks: %u\n"
            "Memory Allocated: %zu bytes\n"
            "Optimization Needed: %s\n"
            "Notes: %s\n",
            profile->total_frames,
            profile->average_frame_time,
            profile->slow_frames,
            profile->fast_frames,
            profile->position_updates,
            profile->collision_checks,
            profile->memory_allocated,
            profile->needs_optimization ? "YES" : "NO",
            profile->optimization_notes);
}
