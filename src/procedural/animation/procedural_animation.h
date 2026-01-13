#ifndef PROCEDURAL_ANIMATION_H
#define PROCEDURAL_ANIMATION_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float x, y, z, w;
} Quat;

typedef struct {
    float x, y, z;
} Vec3;

typedef struct {
    float m[16];
} Mat4;

typedef struct {
    Quat rotation;
    Vec3 translation;
    Vec3 scale;
} Transform;

typedef struct {
    Transform* bones;
    uint32_t bone_count;
    char** bone_names;
    uint32_t* bone_parents;
    Mat4* inverse_bind_matrices;
} Skeleton;

typedef struct {
    float time;
    float duration;
    bool loop;
    float speed;
} AnimationState;

typedef struct {
    uint32_t bone_index;
    float* keyframe_times;
    Transform* keyframes;
    uint32_t keyframe_count;
} AnimationChannel;

typedef struct {
    AnimationChannel* channels;
    uint32_t channel_count;
    char name[64];
    float duration;
} AnimationClip;

typedef struct {
    Skeleton* skeleton;
    AnimationClip* clips;
    uint32_t clip_count;
    AnimationState* states;
    uint32_t state_count;
    Transform* current_pose;
    bool is_playing;
} ProceduralAnimator;

// Procedural animation types
typedef enum {
    PROC_ANIM_WALK,
    PROC_ANIM_RUN,
    PROC_ANIM_JUMP,
    PROC_ANIM_IDLE,
    PROC_ANIM_DANCE,
    PROC_ANIM_SWIM,
    PROC_ANIM_FLY,
    PROC_ANIM_CUSTOM
} ProceduralAnimationType;

// Procedural animation parameters
typedef struct {
    float frequency;
    float amplitude;
    float phase_offset;
    float damping;
    float blend_weight;
} AnimationParameters;

typedef struct {
    float step_height;
    float step_length;
    float step_frequency;
    float foot_spacing;
    float body_bob;
    float arm_swing;
} WalkParameters;

typedef struct {
    float jump_height;
    float jump_duration;
    float takeoff_angle;
    float landing_impact;
    float air_control;
} JumpParameters;

typedef struct {
    float intensity;
    float speed;
    float style_variation;
    float randomness;
} DanceParameters;

// Procedural animation generation
ProceduralAnimator* proc_anim_create_animator(const Skeleton* skeleton);
bool proc_anim_generate_walk_cycle(ProceduralAnimator* animator, const WalkParameters* params);
bool proc_anim_generate_run_cycle(ProceduralAnimator* animator, const WalkParameters* params);
bool proc_anim_generate_jump_animation(ProceduralAnimator* animator, const JumpParameters* params);
bool proc_anim_generate_idle_animation(ProceduralAnimator* animator, const AnimationParameters* params);
bool proc_anim_generate_dance_animation(ProceduralAnimator* animator, const DanceParameters* params);

// Animation blending and layering
bool proc_anim_blend_animations(ProceduralAnimator* animator, uint32_t clip1, uint32_t clip2, float weight);
bool proc_anim_additive_blend(ProceduralAnimator* animator, uint32_t base_clip, uint32_t additive_clip, float weight);
bool proc_anim_layer_animation(ProceduralAnimator* animator, uint32_t base_clip, uint32_t layer_clip, uint32_t bone_mask);

// Physics-based animation
typedef struct {
    Vec3 velocity;
    Vec3 acceleration;
    Vec3 angular_velocity;
    float mass;
    float damping;
    float stiffness;
} PhysicsBody;

bool proc_anim_physics_simulation(ProceduralAnimator* animator, PhysicsBody* bodies, uint32_t body_count, float delta_time);
bool proc_anim_ragdoll_blend(ProceduralAnimator* animator, const PhysicsBody* bodies, float blend_weight);
bool proc_anim_collision_response(ProceduralAnimator* animator, const Vec3* collision_point, const Vec3* collision_normal);

// IK integration
typedef struct {
    uint32_t bone_chain_length;
    uint32_t* bone_chain;
    Vec3 target_position;
    float chain_length;
    bool solve_ik;
} IKChain;

bool proc_anim_solve_ik(ProceduralAnimator* animator, IKChain* chains, uint32_t chain_count);
bool proc_anim_foot_ik(ProceduralAnimator* animator, const Vec3* ground_heights, uint32_t num_points);
bool proc_anim_hand_ik(ProceduralAnimator* animator, const Vec3* grab_targets, uint32_t num_hands);

// Animation variation and randomization
bool proc_anim_add_variation(ProceduralAnimator* animator, uint32_t clip_id, float variation_amount);
bool proc_anim_randomize_timing(ProceduralAnimator* animator, uint32_t clip_id, float randomization);
bool proc_anim_add_noise(ProceduralAnimator* animator, float noise_amount);

// Real-time animation generation
bool proc_anim_generate_reaction_animation(ProceduralAnimator* animator, const Vec3* stimulus, float intensity);
bool proc_anim_generate_emotional_animation(ProceduralAnimator* animator, const char* emotion, float strength);
bool proc_anim_generate_environmental_animation(ProceduralAnimator* animator, const Vec3* wind, float temperature);

// Animation optimization
bool proc_anim_optimize_animation(ProceduralAnimator* animator, uint32_t clip_id, float tolerance);
bool proc_anim_compress_keyframes(ProceduralAnimator* animator, uint32_t clip_id, float compression_ratio);
bool proc_anim_remove_redundant_keyframes(ProceduralAnimator* animator, uint32_t clip_id);

// Export and import
bool proc_anim_export_animation(const ProceduralAnimator* animator, uint32_t clip_id, const char* filename);
bool proc_anim_import_animation(ProceduralAnimator* animator, const char* filename);
bool proc_anim_export_skeleton(const ProceduralAnimator* animator, const char* filename);

// Utility functions
Transform proc_anim_evaluate_animation(const ProceduralAnimator* animator, uint32_t clip_id, float time);
Transform proc_anim_interpolate_keyframes(const Transform* key1, const Transform* key2, float t);
Quat proc_anim_slerp(const Quat* q1, const Quat* q2, float t);
Mat4 proc_anim_transform_to_matrix(const Transform* transform);

// Animation state management
bool proc_anim_play_animation(ProceduralAnimator* animator, uint32_t clip_id);
bool proc_anim_stop_animation(ProceduralAnimator* animator);
bool proc_anim_pause_animation(ProceduralAnimator* animator);
bool proc_anim_resume_animation(ProceduralAnimator* animator);
bool proc_anim_set_animation_speed(ProceduralAnimator* animator, float speed);

// Cleanup
void proc_anim_destroy_animator(ProceduralAnimator* animator);
void proc_anim_destroy_skeleton(Skeleton* skeleton);
void proc_anim_destroy_clip(AnimationClip* clip);

#ifdef __cplusplus
}
#endif

#endif // PROCEDURAL_ANIMATION_H
