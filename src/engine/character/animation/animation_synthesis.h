#ifndef ANIMATION_SYNTHESIS_H
#define ANIMATION_SYNTHESIS_H

#include "core/types.h"
#include "../math/vec3.h"
#include "../math/quat.h"
#include "../math/mat4.h"
#include "character/animation/animation_system.h"
#include "core/memory.h"

// Animation synthesis configuration
#define ANIM_SYNTH_MAX_PROCEDURAL_JOINTS 64
#define ANIM_SYNTH_MAX_KEYFRAMES 120  // 4 seconds at 30 FPS
#define ANIM_SYNTH_MAX_BLEND_WEIGHTS 8
#define ANIM_SYNTH_MAX_LAYERS 4

// Animation synthesis types
typedef enum {
    ANIM_SYNTH_WALK_CYCLE = 0,
    ANIM_SYNTH_RUN_CYCLE,
    ANIM_SYNTH_IDLE_BREATH,
    ANIM_SYNTH_JUMP,
    ANIM_SYNTH_FALL,
    ANIM_SYNTH_LAND,
    ANIM_SYNTH_SWIM,
    ANIM_SYNTH_FLY,
    ANIM_SYNTH_CUSTOM,
    ANIM_SYNTH_COUNT
} AnimationSynthType;

// Procedural animation parameters
typedef struct {
    f32 speed;           // Movement speed (m/s)
    f32 stride_length;   // Distance between steps
    f32 step_height;     // Maximum foot lift height
    f32 step_frequency;  // Steps per second
    f32 body_bob;        // Vertical body movement
    f32 body_lean;       // Forward lean angle
    f32 arm_swing;       // Arm swing amplitude
    f32 head_bob;        // Head movement amplitude
    bool foot_planting;  // Enable foot planting
    bool spine_curvature; // Enable spine curvature
} WalkParameters;

typedef struct {
    f32 speed;           // Movement speed (m/s)
    f32 stride_length;   // Distance between steps
    f32 step_height;     // Maximum foot lift height
    f32 step_frequency;  // Steps per second
    f32 body_bob;        // Vertical body movement
    f32 body_lean;       // Forward lean angle
    f32 arm_swing;       // Arm swing amplitude
    f32 impact_force;    // Landing impact
    bool foot_planting;  // Enable foot planting
    bool spine_curvature; // Enable spine curvature
} RunParameters;

typedef struct {
    f32 breath_rate;     // Breaths per minute
    f32 chest_expansion; // Chest expansion amount
    f32 shoulder_raise;  // Shoulder movement
    f32 head_tilt;       // Subtle head movement
    f32 weight_shift;    // Weight shifting amplitude
    bool micro_movements; // Small random movements
} IdleParameters;

typedef struct {
    f32 jump_height;     // Maximum jump height
    f32 jump_duration;   // Time in air
    f32 takeoff_force;   // Initial upward force
    f32 crouch_depth;    // Pre-jump crouch
    f32 arm_swing;       // Arm swing for balance
    bool leg_extension;  // Full leg extension at peak
} JumpParameters;

// AI-driven animation parameters
typedef struct {
    f32 personality_factor; // How expressive the animation is
    f32 energy_level;       // Overall energy/tension
    f32 confidence;         // Posture confidence
    f32 mood_modifier;      // Emotional influence
    Vec3 movement_bias;     // Preferred movement direction
    f32 reaction_time;      // How quickly to react
} AIAnimationParams;

// Animation synthesis context
typedef struct {
    AnimationSynthType type;
    union {
        WalkParameters walk;
        RunParameters run;
        IdleParameters idle;
        JumpParameters jump;
    } params;
    
    // AI parameters (applied to all types)
    AIAnimationParams ai_params;
    
    // Current state
    f32 phase;           // Animation phase [0, 1]
    f32 time_scale;      // Time scaling factor
    f32 blend_weight;    // Weight when blending
    bool active;         // Is this synthesis active?
    
    // Generated animation data
    AnimationClip* generated_clip;
    u32 last_frame_count;
    
    // Joint influence weights
    f32 joint_weights[ANIM_SYNTH_MAX_PROCEDURAL_JOINTS];
    
    // Layer information for blending
    struct {
        AnimationClip* base_clip;
        f32 layer_weight;
        bool additive;
    } layers[ANIM_SYNTH_MAX_LAYERS];
    u32 layer_count;
} AnimationSynthesisContext;

// Animation synthesis system
typedef struct {
    // Synthesis contexts
    AnimationSynthesisContext contexts[ANIM_SYNTH_COUNT];
    
    // Procedural joint definitions
    struct {
        char name[32];
        i32 parent_index;
        Vec3 default_position;
        Quat default_rotation;
        bool is_foot_joint;
        bool is_hand_joint;
        bool is_spine_joint;
    } joints[ANIM_SYNTH_MAX_PROCEDURAL_JOINTS];
    u32 joint_count;
    
    // Animation generation cache
    struct {
        AnimationClip* cached_clips[ANIM_SYNTH_COUNT];
        f32 last_speed[ANIM_SYNTH_COUNT];
        bool cache_valid[ANIM_SYNTH_COUNT];
    } cache;
    
    // AI learning data (simplified)
    struct {
        f32 movement_patterns[100][3]; // Learned movement patterns
        u32 pattern_count;
        f32 learning_rate;
    } ml_data;
    
    // Performance settings
    f32 update_rate;      // How often to regenerate (Hz)
    f32 last_update_time;
    bool enable_ai_synthesis;
    bool enable_caching;
    
    // Statistics
    u32 animations_generated;
    u32 cache_hits;
    u32 cache_misses;
    f64 total_synthesis_time;
} AnimationSynthesisSystem;

// Core API functions
void animation_synthesis_init(AnimationSynthesisSystem* system);
void animation_synthesis_shutdown(AnimationSynthesisSystem* system);
void animation_synthesis_update(AnimationSynthesisSystem* system, f32 delta_time);

// Synthesis context management
AnimationSynthesisContext* animation_synthesis_get_context(AnimationSynthesisSystem* system, 
                                                         AnimationSynthType type);
void animation_synthesis_start(AnimationSynthesisSystem* system, AnimationSynthType type);
void animation_synthesis_stop(AnimationSynthesisSystem* system, AnimationSynthType type);
void animation_synthesis_set_speed(AnimationSynthesisSystem* system, AnimationSynthType type, f32 speed);

// Animation generation
AnimationClip* animation_synthesis_generate(AnimationSynthesisSystem* system, 
                                          AnimationSynthType type, 
                                          f32 speed, 
                                          f32 duration);
AnimationClip* animation_synthesis_walk_cycle(AnimationSynthesisSystem* system, 
                                            const WalkParameters* params, 
                                            f32 duration);
AnimationClip* animation_synthesis_run_cycle(AnimationSynthesisSystem* system, 
                                           const RunParameters* params, 
                                           f32 duration);
AnimationClip* animation_synthesis_idle_breathing(AnimationSynthesisSystem* system, 
                                                 const IdleParameters* params, 
                                                 f32 duration);
AnimationClip* animation_synthesis_jump(AnimationSynthesisSystem* system, 
                                       const JumpParameters* params);

// AI-driven synthesis
AnimationClip* animation_synthesis_ai_generate(AnimationSynthesisSystem* system, 
                                             AnimationSynthType type,
                                             const AIAnimationParams* ai_params,
                                             Vec3 target_movement,
                                             f32 urgency);
void animation_synthesis_ai_learn_pattern(AnimationSynthesisSystem* system, 
                                         const Vec3* movement_sequence, 
                                         u32 frame_count);

// Animation blending and layering
void animation_synthesis_blend_layers(AnimationSynthesisSystem* system, 
                                     AnimationSynthType base_type,
                                     AnimationClip* overlay_clip,
                                     f32 blend_weight,
                                     bool additive);
AnimationClip* animation_synthesis_create_reactive(AnimationSynthesisSystem* system,
                                                   AnimationSynthType base_type,
                                                   Vec3 impact_point,
                                                   f32 impact_force);

// Parameter calculation helpers
WalkParameters animation_synthesis_calc_walk_params(f32 speed);
RunParameters animation_synthesis_calc_run_params(f32 speed);
IdleParameters animation_synthesis_calc_idle_params(f32 energy_level);
JumpParameters animation_synthesis_calc_jump_params(f32 jump_height);

// Utility functions
f32 animation_synthesis_evaluate_phase(AnimationSynthesisContext* context, f32 time);
Vec3 animation_synthesis_foot_position(f32 phase, f32 stride_length, f32 step_height);
Quat animation_synthesis_spine_rotation(f32 phase, f32 body_lean, bool curvature);
void animation_synthesis_apply_ai_modifiers(AnimationSynthesisContext* context, 
                                          const AIAnimationParams* params);

// Cache management
void animation_synthesis_cache_clear(AnimationSynthesisSystem* system);
bool animation_synthesis_cache_lookup(AnimationSynthesisSystem* system, 
                                     AnimationSynthType type, 
                                     f32 speed, 
                                     AnimationClip** result);
void animation_synthesis_cache_store(AnimationSynthesisSystem* system, 
                                   AnimationSynthType type, 
                                   f32 speed, 
                                   AnimationClip* clip);

#endif // ANIMATION_SYNTHESIS_H
