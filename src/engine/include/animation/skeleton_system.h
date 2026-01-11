#ifndef SKELETON_SYSTEM_H
#define SKELETON_SYSTEM_H

#include <core/types.h>
#include <stdbool.h>
#include <stddef.h>

// Forward declarations
typedef struct Vec3 Vec3;
typedef struct Vec4 Vec4;
typedef struct Mat4 Mat4;
typedef struct Quat Quat;

// Bone types
typedef enum {
    BONE_TYPE_ROOT,
    BONE_TYPE_SPINE,
    BONE_TYPE_LIMB,
    BONE_TYPE_DIGIT,
    BONE_TYPE_FACIAL,
    BONE_TYPE_ACCESSORY,
    BONE_TYPE_CUSTOM
} BoneType;

// Bone transform space
typedef enum {
    BONE_SPACE_LOCAL,
    BONE_SPACE_MODEL,
    BONE_SPACE_WORLD
} BoneSpace;

// Animation blend modes
typedef enum {
    ANIM_BLEND_REPLACE,
    ANIM_BLEND_ADDITIVE,
    ANIM_BLEND_INTERPOLATE
} AnimBlendMode;

// Bone influence weights
typedef struct {
    u32 bone_index;
    float weight;
} BoneInfluence;

// Vertex bone influence data
typedef struct {
    BoneInfluence influences[4]; // Max 4 bone influences per vertex
    u32 influence_count;
} VertexBoneData;

// Bone definition
typedef struct Bone {
    char name[64];
    u32 bone_id;
    BoneType type;
    
    // Hierarchy
    u32 parent_id;
    u32* child_ids;
    u32 child_count;
    u32 child_capacity;
    
    // Transform data
    Vec3 local_position;
    Quat local_rotation;
    Vec3 local_scale;
    
    Vec3 model_position;
    Quat model_rotation;
    Vec3 model_scale;
    
    // Bind pose (for skinning)
    Mat4 bind_pose_matrix;
    Mat4 inverse_bind_pose_matrix;
    
    // Current pose
    Mat4 current_pose_matrix;
    Mat4 skinning_matrix;
    
    // Bone properties
    float length;
    float radius;
    bool is_visible;
    bool is_locked;
    
    // Animation data
    u32 animation_track_count;
    void* animation_tracks; // Pointer to animation track data
    
    // Metadata
    char metadata[256];
    
} Bone;

// Skeleton definition
typedef struct {
    char name[64];
    u32 skeleton_id;
    
    // Bones
    Bone* bones;
    u32 bone_count;
    u32 bone_capacity;
    
    // Root bone
    u32 root_bone_id;
    
    // Hierarchy cache
    u32* bone_hierarchy;
    u32 hierarchy_depth;
    
    // Bone mapping (name to ID)
    struct {
        char name[64];
        u32 bone_id;
    }* bone_map;
    u32 bone_map_count;
    u32 bone_map_capacity;
    
    // Skeleton properties
    float scale;
    Vec3 offset;
    bool is_humanoid;
    
    // Bounding volume
    Vec3 bounds_min;
    Vec3 bounds_max;
    
    // Animation state
    bool is_animated;
    u32 active_animation_count;
    void* animation_state;
    
} Skeleton;

// Animation keyframe types
typedef enum {
    KEYFRAME_POSITION,
    KEYFRAME_ROTATION,
    KEYFRAME_SCALE,
    KEYFRAME_CUSTOM
} KeyframeType;

// Animation keyframe
typedef struct {
    float time;
    KeyframeType type;
    
    union {
        Vec3 position;
        Quat rotation;
        Vec3 scale;
        float custom_value;
    } value;
    
    // Interpolation data
    enum {
        INTERPOLATION_STEP,
        INTERPOLATION_LINEAR,
        INTERPOLATION_CUBIC_SPLINE
    } interpolation;
    
    // Tangent data for cubic spline
    Vec3 in_tangent;
    Vec3 out_tangent;
    
} AnimationKeyframe;

// Bone animation track
typedef struct {
    u32 bone_id;
    
    AnimationKeyframe* position_keyframes;
    u32 position_keyframe_count;
    u32 position_keyframe_capacity;
    
    AnimationKeyframe* rotation_keyframes;
    u32 rotation_keyframe_count;
    u32 rotation_keyframe_capacity;
    
    AnimationKeyframe* scale_keyframes;
    u32 scale_keyframe_count;
    u32 scale_keyframe_capacity;
    
} BoneAnimationTrack;

// Animation clip
typedef struct {
    char name[64];
    u32 animation_id;
    
    BoneAnimationTrack* tracks;
    u32 track_count;
    u32 track_capacity;
    
    // Animation properties
    float duration;
    float fps;
    bool loops;
    bool is_additive;
    
    // Events
    struct {
        float time;
        char event_name[64];
        void* event_data;
    }* events;
    u32 event_count;
    u32 event_capacity;
    
} AnimationClip;

// Animation state
typedef struct {
    AnimationClip* clip;
    float current_time;
    float playback_speed;
    bool is_playing;
    bool is_looping;
    float weight;
    AnimBlendMode blend_mode;
    
    // Blending
    struct AnimationState* next_state;
    float blend_duration;
    float blend_time;
    
} AnimationState;

// Skeleton animation system
typedef struct {
    // Skeletons
    Skeleton* skeletons;
    u32 skeleton_count;
    u32 skeleton_capacity;
    
    // Animation clips
    AnimationClip* animations;
    u32 animation_count;
    u32 animation_capacity;
    
    // Animation states
    AnimationState* animation_states;
    u32 state_count;
    u32 state_capacity;
    
    // Vertex skinning data
    VertexBoneData* vertex_bone_data;
    u32 vertex_count;
    u32 vertex_capacity;
    
    // Skinning matrices
    Mat4* skinning_matrices;
    u32 skinning_matrix_count;
    u32 skinning_matrix_capacity;
    
    // Global settings
    bool enable_animation_blending;
    bool enable IK_solving;
    bool enable_ragdoll_physics;
    float max_bone_influences;
    
    // Performance tracking
    u64 last_update_time_ms;
    u32 updated_skeletons_last_frame;
    u32 updated_animations_last_frame;
    
    // Callbacks
    void (*on_skeleton_created)(Skeleton* skeleton);
    void (*on_skeleton_destroyed)(Skeleton* skeleton);
    void (*on_animation_started)(AnimationState* state);
    void (*on_animation_completed)(AnimationState* state);
    void (*on_animation_event)(const char* event_name, void* event_data);
    
    void* user_data;
    
} SkeletonSystem;

// MARK: - Skeleton System Management

bool skeleton_system_init(SkeletonSystem* system, u32 max_skeletons, u32 max_animations, u32 max_vertices);
void skeleton_system_shutdown(SkeletonSystem* system);

// MARK: - Skeleton Management

Skeleton* skeleton_create(SkeletonSystem* system, const char* name, u32 max_bones);
bool skeleton_destroy(SkeletonSystem* system, Skeleton* skeleton);
Skeleton* skeleton_get(SkeletonSystem* system, const char* name);
Skeleton* skeleton_get_by_id(SkeletonSystem* system, u32 skeleton_id);

// MARK: - Bone Management

Bone* skeleton_add_bone(Skeleton* skeleton, const char* name, BoneType type, u32 parent_id);
bool skeleton_remove_bone(Skeleton* skeleton, u32 bone_id);
Bone* skeleton_get_bone(Skeleton* skeleton, const char* name);
Bone* skeleton_get_bone_by_id(Skeleton* skeleton, u32 bone_id);

bool skeleton_set_bone_transform(Skeleton* skeleton, u32 bone_id, const Vec3* position, 
                               const Quat* rotation, const Vec3* scale, BoneSpace space);
bool skeleton_get_bone_transform(Skeleton* skeleton, u32 bone_id, Vec3* position, 
                               Quat* rotation, Vec3* scale, BoneSpace space);

bool skeleton_update_bone_hierarchy(Skeleton* skeleton);
bool skeleton_calculate_bind_poses(Skeleton* skeleton);
bool skeleton_calculate_skinning_matrices(Skeleton* skeleton);

// MARK: - Animation Management

AnimationClip* animation_create(SkeletonSystem* system, const char* name, float duration, float fps);
bool animation_destroy(SkeletonSystem* system, AnimationClip* animation);
AnimationClip* animation_get(SkeletonSystem* system, const char* name);

BoneAnimationTrack* animation_add_track(AnimationClip* animation, u32 bone_id);
bool animation_remove_track(AnimationClip* animation, u32 bone_id);
bool animation_add_keyframe(AnimationClip* animation, u32 bone_id, float time, 
                           KeyframeType type, void* value);

// MARK: - Animation Playback

AnimationState* skeleton_play_animation(Skeleton* skeleton, AnimationClip* animation, 
                                      float weight, AnimBlendMode blend_mode);
bool skeleton_stop_animation(Skeleton* skeleton, AnimationState* state);
bool skeleton_pause_animation(Skeleton* skeleton, AnimationState* state);
bool skeleton_resume_animation(Skeleton* skeleton, AnimationState* state);

bool skeleton_set_animation_time(Skeleton* skeleton, AnimationState* state, float time);
bool skeleton_set_animation_speed(Skeleton* skeleton, AnimationState* state, float speed);
bool skeleton_set_animation_weight(Skeleton* skeleton, AnimationState* state, float weight);

// MARK: - Animation Blending

bool skeleton_blend_animations(Skeleton* skeleton, AnimationState* from_state, 
                              AnimationState* to_state, float duration);
bool skeleton_crossfade_animations(Skeleton* skeleton, AnimationState* from_state, 
                                   AnimationState* to_state, float duration);

// MARK: - Vertex Skinning

bool skeleton_set_vertex_bone_data(SkeletonSystem* system, u32 vertex_index, 
                                  const BoneInfluence* influences, u32 influence_count);
bool skeleton_update_skinning_matrices(SkeletonSystem* system, Skeleton* skeleton);
Mat4* skeleton_get_skinning_matrices(SkeletonSystem* system, Skeleton* skeleton, u32* matrix_count);

// MARK: - IK Solving

bool skeleton_solve_ik_chain(Skeleton* skeleton, u32* bone_chain, u32 chain_length, 
                            const Vec3* target_position, const Quat* target_rotation);
bool skeleton_solve_ccd_ik(Skeleton* skeleton, u32* bone_chain, u32 chain_length, 
                           const Vec3* target_position, u32 max_iterations, float tolerance);
bool skeleton_solve_fabrik_ik(Skeleton* skeleton, u32* bone_chain, u32 chain_length, 
                              const Vec3* target_position, u32 max_iterations, float tolerance);

// MARK: - Ragdoll Physics

bool skeleton_enable_ragdoll(Skeleton* skeleton, bool enable);
bool skeleton_set_ragdoll_bone_properties(Skeleton* skeleton, u32 bone_id, 
                                          float mass, float damping, float stiffness);
bool skeleton_apply_ragdoll_force(Skeleton* skeleton, u32 bone_id, const Vec3* force, const Vec3* point);

// MARK: - Export and Import

bool skeleton_export_to_json(Skeleton* skeleton, const char* filename);
bool skeleton_import_from_json(const char* filename, Skeleton** skeleton);
bool animation_export_to_json(AnimationClip* animation, const char* filename);
bool animation_import_from_json(const char* filename, AnimationClip** animation);

bool skeleton_export_to_fbx(Skeleton* skeleton, const char* filename);
bool skeleton_import_from_fbx(const char* filename, Skeleton** skeleton);

// MARK: - Utility Functions

void skeleton_calculate_bounds(Skeleton* skeleton, Vec3* min_bounds, Vec3* max_bounds);
bool skeleton_is_bone_descendant(Skeleton* skeleton, u32 bone_id, u32 ancestor_id);
u32 skeleton_get_bone_depth(Skeleton* skeleton, u32 bone_id);

void skeleton_print_hierarchy(Skeleton* skeleton);
void skeleton_print_bone_info(Skeleton* skeleton, u32 bone_id);
void skeleton_print_animation_info(AnimationClip* animation);

// MARK: - Configuration

void skeleton_system_set_settings(SkeletonSystem* system, bool enable_blending, 
                                 bool enable_ik, bool enable_ragdoll, float max_influences);

void skeleton_system_set_callbacks(SkeletonSystem* system,
                                   void (*on_skeleton_created)(Skeleton*),
                                   void (*on_skeleton_destroyed)(Skeleton*),
                                   void (*on_animation_started)(AnimationState*),
                                   void (*on_animation_completed)(AnimationState*),
                                   void (*on_animation_event)(const char*, void*));

// MARK: - Debug Macros

#define SKELETON_SYSTEM_ENABLED 1

#if SKELETON_SYSTEM_ENABLED
    #define SKELETON_CREATE(system, name, bones) skeleton_create(system, name, bones)
    #define SKELETON_ADD_BONE(skel, name, type, parent) skeleton_add_bone(skel, name, type, parent)
    #define SKELETON_PLAY_ANIM(skel, anim, weight, blend) skeleton_play_animation(skel, anim, weight, blend)
    #define SKELETON_UPDATE(system, delta_time) skeleton_system_update(system, delta_time)
#else
    #define SKELETON_CREATE(system, name, bones) NULL
    #define SKELETON_ADD_BONE(skel, name, type, parent) NULL
    #define SKELETON_PLAY_ANIM(skel, anim, weight, blend) NULL
    #define SKELETON_UPDATE(system, delta_time)
#endif

// Global skeleton system instance
extern SkeletonSystem* g_skeleton_system;

#endif // SKELETON_SYSTEM_H
