// include/player/player_movement_enhancements.h
//
// Purpose: Enhanced player movement systems including accessibility features,
// profiling, optimization, statistics tracking, validation, smoothing, and
// documentation. This header provides comprehensive movement analysis and
// improvement systems for player character control.
//
// Public APIs:
// - PlayerMovementAccessibility: Accessibility settings for movement controls
// - PlayerMovementProfile: Performance profiling data for movement systems
// - PlayerMovementStats: Comprehensive movement statistics tracking
// - PlayerMovementValidation: Movement validation and correction system
// - PlayerMovementSmoothing: Smooth movement interpolation system
// - PlayerMovementDocumentation: Movement system documentation and help
//
// Ownership: These systems are managed by PlayerSystem and integrate with
// existing player movement physics and controls.
//
// Invariants:
// - All movement enhancement systems must be initialized before use
// - Update functions should be called each frame with proper delta_time
// - Statistics are accumulated and can be reset when needed
// - Validation rules can be configured per game mode
//

#ifndef PLAYER_MOVEMENT_ENHANCEMENTS_H
#define PLAYER_MOVEMENT_ENHANCEMENTS_H

#include <game_common.h>
#include <math/vec3.h>
#include <physics/physics.h>

// Forward declarations
typedef struct PlayerSystem PlayerSystem;
typedef struct PlayerComponent PlayerComponent;

// Movement accessibility settings
typedef struct {
    // Input sensitivity adjustments
    f32 movement_sensitivity;        // Overall movement sensitivity (0.1-2.0)
    f32 look_sensitivity;           // Mouse look sensitivity (0.1-5.0)
    f32 deadzone_size;             // Input deadzone (0.0-0.5)
    
    // Assistance features
    bool auto_jump_enabled;         // Automatic jump at block edges
    bool sprint_assist_enabled;     // Automatic sprint when moving forward
    bool sticky_mining_enabled;     // Continue mining without holding button
    bool auto_crouch_enabled;       // Auto-crouch in low spaces
    
    // Visual assistance
    bool movement_trails_enabled;    // Show movement path trails
    bool velocity_indicators_enabled; // Show velocity vectors
    bool collision_highlight_enabled; // Highlight collision boundaries
    
    // Motor assistance
    f32 aim_assist_strength;       // Aim assistance (0.0-1.0)
    bool reduced_gravity_mode;       // Lower gravity for accessibility
    f32 movement_speed_modifier;     // Speed multiplier (0.5-2.0)
    
    // Input remapping
    bool inverted_controls;          // Invert movement directions
    bool simplified_controls;        // Simplified control scheme
    u32 control_scheme;            // Different control presets (0-4)
} PlayerMovementAccessibility;

// Movement profiling data
typedef struct {
    // Performance metrics
    f32 update_time_ms;            // Time spent in movement update
    f32 physics_time_ms;          // Time spent in physics calculations
    f32 collision_time_ms;         // Time spent in collision detection
    f32 smoothing_time_ms;         // Time spent in movement smoothing
    
    // Frame statistics
    u32 total_frames;             // Total frames profiled
    u32 slow_frames;              // Frames > 16ms
    u32 fast_frames;              // Frames < 8ms
    f32 average_frame_time;        // Average frame time
    
    // Movement metrics
    u32 position_updates;          // Number of position updates
    u32 velocity_changes;          // Number of velocity changes
    u32 collision_checks;          // Number of collision checks
    u32 ground_checks;            // Number of ground checks
    
    // Memory usage
    size_t memory_allocated;        // Memory used by movement systems
    u32 allocation_count;          // Number of memory allocations
    
    // Optimization suggestions
    char optimization_notes[256];    // Performance suggestions
    bool needs_optimization;        // Flag for optimization needed
} PlayerMovementProfile;

// Movement statistics tracking
typedef struct {
    // Distance statistics
    f32 total_distance_traveled;   // Total distance moved
    f32 distance_walked;          // Distance walked
    f32 distance_sprinted;         // Distance sprinted
    f32 distance_swam;            // Distance swam
    f32 distance_flown;            // Distance flown
    f32 distance_fallen;           // Distance fallen
    f32 distance_climbed;          // Distance climbed (ladders)
    
    // Movement counts
    u32 jump_count;               // Number of jumps
    u32 double_jump_count;         // Number of double jumps (if enabled)
    u32 sprint_count;             // Number of sprints initiated
    u32 crouch_count;             // Number of crouches
    u32 swim_count;               // Number of swim strokes
    u32 flight_count;             // Number of flight activations
    
    // Time statistics
    f32 total_movement_time;       // Total time moving
    f32 time_walking;             // Time spent walking
    f32 time_sprinting;           // Time spent sprinting
    f32 time_swimming;            // Time spent swimming
    f32 time_flying;              // Time spent flying
    f32 time_crouching;           // Time spent crouching
    f32 time_jumping;             // Time spent in air (jumping)
    f32 time_idle;               // Time spent idle (no movement)
    
    // Speed statistics
    f32 max_speed_achieved;       // Maximum speed reached
    f32 average_speed;            // Average movement speed
    f32 current_speed;            // Current movement speed
    Vec3 velocity_history[60];     // Last 60 frames of velocity
    u32 velocity_history_index;     // Current index in velocity history
    
    // Environmental interactions
    u32 blocks_mined;             // Blocks mined
    u32 blocks_placed;            // Blocks placed
    u32 entities_interacted;       // Entities interacted with
    u32 damage_taken;              // Number of damage events
    u32 deaths;                  // Number of deaths
    
    // Session data
    f32 session_start_time;        // When current session started
    f32 total_play_time;          // Total play time
    u32 session_count;            // Number of sessions
} PlayerMovementStats;

// Movement validation system
typedef struct {
    // Validation rules
    f32 max_allowed_speed;         // Maximum movement speed
    f32 max_vertical_speed;        // Maximum vertical speed
    f32 max_acceleration;          // Maximum acceleration rate
    f32 max_angular_speed;        // Maximum turning speed
    
    // Collision validation
    bool validate_collisions;        // Enable collision validation
    f32 max_penetration_depth;     // Maximum allowed penetration
    bool auto_correct_penetration;   // Auto-correct penetration issues
    
    // Movement validation
    bool validate_ground_contact;    // Validate ground contact
    bool validate_swimming_state;    // Validate swimming state
    bool validate_flying_state;     // Validate flying state
    
    // Validation results
    u32 validation_errors;          // Number of validation errors
    u32 corrections_applied;        // Number of corrections applied
    char last_error[128];           // Last validation error message
    f32 last_correction_time;        // Time of last correction
    
    // Debug visualization
    bool show_validation_bounds;     // Show validation boundaries
    bool show_correction_vectors;    // Show correction vectors
    bool log_validation_errors;      // Log validation errors
} PlayerMovementValidation;

// Movement smoothing system
typedef struct {
    // Smoothing parameters
    f32 position_smoothing_factor;  // Position interpolation (0.0-1.0)
    f32 velocity_smoothing_factor;  // Velocity interpolation (0.0-1.0)
    f32 rotation_smoothing_factor;  // Rotation interpolation (0.0-1.0)
    f32 camera_smoothing_factor;    // Camera interpolation (0.0-1.0)
    
    // Smoothing history
    Vec3 smoothed_position;         // Current smoothed position
    Vec3 target_position;           // Target position to smooth to
    Vec3 smoothed_velocity;         // Current smoothed velocity
    Vec3 target_velocity;           // Target velocity to smooth to
    Vec3 smoothed_rotation;         // Current smoothed rotation (yaw, pitch)
    Vec3 target_rotation;           // Target rotation to smooth to
    
    // Smoothing state
    bool smoothing_enabled;          // Enable/disable smoothing
    f32 smoothing_timer;           // Timer for smoothing transitions
    f32 max_smoothing_time;        // Maximum time for smoothing
    
    // Prediction system
    bool prediction_enabled;         // Enable movement prediction
    Vec3 predicted_position;        // Predicted next position
    Vec3 predicted_velocity;        // Predicted next velocity
    f32 prediction_accuracy;        // Accuracy of predictions (0.0-1.0)
    
    // Network smoothing (for multiplayer)
    bool network_smoothing_enabled;  // Enable network interpolation
    f32 network_interpolation_delay; // Delay for interpolation (seconds)
    Vec3 network_position_buffer[10]; // Buffer for network positions
    u32 network_buffer_index;       // Current buffer index
} PlayerMovementSmoothing;

// Movement documentation system
typedef struct {
    // Help system
    bool help_enabled;              // Show movement help
    bool context_sensitive_help;     // Show context-sensitive help
    char current_help_topic[64];    // Current help topic
    
    // Tutorial system
    bool tutorial_enabled;          // Show movement tutorials
    u32 completed_tutorials;       // Bitmask of completed tutorials
    f32 tutorial_display_timer;     // Timer for tutorial display
    char current_tutorial[128];     // Current tutorial text
    
    // Movement tips
    char movement_tips[10][128];   // Array of movement tips
    u32 current_tip_index;          // Current tip index
    f32 tip_display_timer;          // Timer for tip display
    bool show_tips;                // Show movement tips
    
    // Documentation
    char movement_docs[1024];        // Movement system documentation
    bool show_docs;                 // Show documentation overlay
    
    // Performance info
    bool show_performance_info;      // Show performance overlay
    bool show_statistics;            // Show statistics overlay
    bool show_profiling_info;        // Show profiling overlay
} PlayerMovementDocumentation;

// Enhanced movement system structure
typedef struct {
    PlayerMovementAccessibility accessibility;
    PlayerMovementProfile profile;
    PlayerMovementStats stats;
    PlayerMovementValidation validation;
    PlayerMovementSmoothing smoothing;
    PlayerMovementDocumentation documentation;
    
    // System state
    bool initialized;               // System initialization state
    f32 update_timer;              // Update timer
    u32 frame_count;               // Frame counter
    
    // Configuration
    bool enable_all_features;        // Enable all enhancement features
    bool debug_mode;               // Debug mode for development
} PlayerMovementEnhancements;

// Initialization and cleanup
void player_movement_enhancements_init(PlayerMovementEnhancements *enhancements);
void player_movement_enhancements_free(PlayerMovementEnhancements *enhancements);

// Accessibility system
void player_movement_accessibility_update(PlayerMovementEnhancements *enhancements, 
                                     PlayerSystem *player_system, f32 delta_time);
void player_movement_accessibility_apply_settings(PlayerMovementEnhancements *enhancements,
                                          PlayerComponent *player);
void player_movement_accessibility_load_defaults(PlayerMovementAccessibility *accessibility);

// Profiling system
void player_movement_profiling_start_frame(PlayerMovementEnhancements *enhancements);
void player_movement_profiling_end_frame(PlayerMovementEnhancements *enhancements);
void player_movement_profiling_update(PlayerMovementEnhancements *enhancements, f32 delta_time);
void player_movement_profiling_reset(PlayerMovementProfile *profile);
void player_movement_profiling_generate_report(PlayerMovementProfile *profile, char *buffer, size_t buffer_size);

// Statistics system
void player_movement_stats_update(PlayerMovementEnhancements *enhancements, 
                             PlayerComponent *player, f32 delta_time);
void player_movement_stats_record_movement(PlayerMovementStats *stats, Vec3 old_pos, Vec3 new_pos, f32 delta_time);
void player_movement_stats_record_action(PlayerMovementStats *stats, const char *action);
void player_movement_stats_reset(PlayerMovementStats *stats);
void player_movement_stats_export(PlayerMovementStats *stats, char *buffer, size_t buffer_size);

// Validation system
void player_movement_validation_update(PlayerMovementEnhancements *enhancements, 
                                 PlayerComponent *player, f32 delta_time);
bool player_movement_validate_position(PlayerMovementValidation *validation, Vec3 position, Vec3 *correction);
bool player_movement_validate_velocity(PlayerMovementValidation *validation, Vec3 velocity, Vec3 *correction);
void player_movement_validation_apply_correction(PlayerComponent *player, Vec3 correction);

// Smoothing system
void player_movement_smoothing_update(PlayerMovementEnhancements *enhancements, 
                                  PlayerComponent *player, f32 delta_time);
void player_movement_smoothing_set_target(PlayerMovementSmoothing *smoothing, 
                                      Vec3 position, Vec3 velocity, Vec3 rotation);
Vec3 player_movement_smoothing_get_smoothed_position(PlayerMovementSmoothing *smoothing);
Vec3 player_movement_smoothing_get_smoothed_velocity(PlayerMovementSmoothing *smoothing);

// Documentation system
void player_movement_documentation_update(PlayerMovementEnhancements *enhancements, f32 delta_time);
void player_movement_documentation_show_help(PlayerMovementDocumentation *docs, const char *topic);
void player_movement_documentation_show_tutorial(PlayerMovementDocumentation *docs, const char *tutorial);
void player_movement_documentation_cycle_tip(PlayerMovementDocumentation *docs);

// Main update function
void player_movement_enhancements_update(PlayerMovementEnhancements *enhancements, 
                                     PlayerSystem *player_system, f32 delta_time);

// Configuration and utilities
void player_movement_enhancements_load_config(PlayerMovementEnhancements *enhancements, const char *config_file);
void player_movement_enhancements_save_config(PlayerMovementEnhancements *enhancements, const char *config_file);
void player_movement_enhancements_reset(PlayerMovementEnhancements *enhancements);

#endif // PLAYER_MOVEMENT_ENHANCEMENTS_H
