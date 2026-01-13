// Skeleton System implementation
#include "animation/skeleton_system.h"
#include <core/logger.h>
#include <math/vec3.h>
#include <math/vec4.h>
#include <math/mat4.h>
#include "engine/include/math/math.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

// Global skeleton system instance
SkeletonSystem* g_skeleton_system = NULL;

// MARK: - Helper Functions

// Forward declarations for internal functions
static bool skeleton_update_bone_hierarchy_recursive(Skeleton* skeleton, u32 bone_id,
                                                   Vec3 parent_pos, Quat parent_rot, Vec3 parent_scale);
static bool skeleton_sample_keyframes(const AnimationKeyframe* keyframes, u32 count, float time,
                                     KeyframeType type, void* result);
static void skeleton_print_bone_recursive(Skeleton* skeleton, u32 bone_id, u32 depth);

static u32 generate_skeleton_id(void) {
    static u32 next_id = 1;
    return next_id++;
}

static u32 generate_bone_id(void) {
    static u32 next_id = 1;
    return next_id++;
}

static u32 generate_animation_id(void) {
    static u32 next_id = 1;
    return next_id++;
}

static Mat4 create_transform_matrix(const Vec3* position, const Quat* rotation, const Vec3* scale) {
    Mat4 translation = mat4_translate(*position);
    Mat4 rotation_matrix = quat_to_mat4(*rotation);
    Mat4 scale_matrix = mat4_scale(*scale);

    // T * R * S
    return mat4_mul(translation, mat4_mul(rotation_matrix, scale_matrix));
}

static void interpolate_keyframes(const AnimationKeyframe* key1, const AnimationKeyframe* key2,
                                 float t, KeyframeType type, void* result) {
    switch (type) {
        case KEYFRAME_POSITION: {
            Vec3* pos = (Vec3*)result;
            *pos = vec3_lerp(key1->value.position, key2->value.position, t);
            break;
        }
        case KEYFRAME_ROTATION: {
            Quat* rot = (Quat*)result;
            *rot = quat_slerp(key1->value.rotation, key2->value.rotation, t);
            break;
        }
        case KEYFRAME_SCALE: {
            Vec3* scale = (Vec3*)result;
            *scale = vec3_lerp(key1->value.scale, key2->value.scale, t);
            break;
        }
        case KEYFRAME_CUSTOM: {
            float* value = (float*)result;
            *value = key1->value.custom_value + t * (key2->value.custom_value - key1->value.custom_value);
            break;
        }
    }
}

// MARK: - Skeleton System Management

bool skeleton_system_init(SkeletonSystem* system, u32 max_skeletons, u32 max_animations, u32 max_vertices) {
    if (!system || max_skeletons == 0 || max_animations == 0 || max_vertices == 0) {
        LOG_ERROR("Invalid parameters for skeleton_system_init");
        return false;
    }

    memset(system, 0, sizeof(SkeletonSystem));

    // Allocate skeletons array
    system->skeletons = (Skeleton*)calloc(max_skeletons, sizeof(Skeleton));
    if (!system->skeletons) {
        LOG_ERROR("Failed to allocate skeletons array");
        return false;
    }
    system->skeleton_capacity = max_skeletons;

    // Allocate animations array
    system->animations = (AnimationClip*)calloc(max_animations, sizeof(AnimationClip));
    if (!system->animations) {
        free(system->skeletons);
        LOG_ERROR("Failed to allocate animations array");
        return false;
    }
    system->animation_capacity = max_animations;

    // Allocate animation states array
    system->animation_states = (AnimationState*)calloc(max_skeletons * 4, sizeof(AnimationState));
    if (!system->animation_states) {
        free(system->skeletons);
        free(system->animations);
        LOG_ERROR("Failed to allocate animation states array");
        return false;
    }
    system->state_capacity = max_skeletons * 4;

    // Allocate vertex bone data array
    system->vertex_bone_data = (VertexBoneData*)calloc(max_vertices, sizeof(VertexBoneData));
    if (!system->vertex_bone_data) {
        free(system->skeletons);
        free(system->animations);
        free(system->animation_states);
        LOG_ERROR("Failed to allocate vertex bone data array");
        return false;
    }
    system->vertex_capacity = max_vertices;

    // Allocate skinning matrices array
    system->skinning_matrices = (Mat4*)calloc(max_skeletons * 256, sizeof(Mat4)); // Max 256 bones per skeleton
    if (!system->skinning_matrices) {
        free(system->skeletons);
        free(system->animations);
        free(system->animation_states);
        free(system->vertex_bone_data);
        LOG_ERROR("Failed to allocate skinning matrices array");
        return false;
    }
    system->skinning_matrix_capacity = max_skeletons * 256;

    // Set default settings
    system->enable_animation_blending = true;
    system->enable_IK_solving = false;
    system->enable_ragdoll_physics = false;
    system->max_bone_influences = 4.0f;

    // Set global instance
    g_skeleton_system = system;

    LOG_INFO("Skeleton system initialized (skeletons: %u, animations: %u, vertices: %u)",
             max_skeletons, max_animations, max_vertices);
    return true;
}

void skeleton_system_shutdown(SkeletonSystem* system) {
    if (!system) {
        return;
    }

    // Clean up skeletons
    if (system->skeletons) {
        for (u32 i = 0; i < system->skeleton_count; i++) {
            Skeleton* skeleton = &system->skeletons[i];
            if (skeleton->bones) {
                for (u32 j = 0; j < skeleton->bone_count; j++) {
                    Bone* bone = &skeleton->bones[j];
                    if (bone->child_ids) {
                        free(bone->child_ids);
                    }
                }
                free(skeleton->bones);
            }
            if (skeleton->bone_hierarchy) {
                free(skeleton->bone_hierarchy);
            }
            if (skeleton->bone_map) {
                free(skeleton->bone_map);
            }
        }
        free(system->skeletons);
    }

    // Clean up animations
    if (system->animations) {
        for (u32 i = 0; i < system->animation_count; i++) {
            AnimationClip* animation = &system->animations[i];
            if (animation->tracks) {
                for (u32 j = 0; j < animation->track_count; j++) {
                    BoneAnimationTrack* track = &animation->tracks[j];
                    if (track->position_keyframes) free(track->position_keyframes);
                    if (track->rotation_keyframes) free(track->rotation_keyframes);
                    if (track->scale_keyframes) free(track->scale_keyframes);
                }
                free(animation->tracks);
            }
            if (animation->events) {
                free(animation->events);
            }
        }
        free(system->animations);
    }

    // Clean up other arrays
    free(system->animation_states);
    free(system->vertex_bone_data);
    free(system->skinning_matrices);

    // Clear global instance
    g_skeleton_system = NULL;

    memset(system, 0, sizeof(SkeletonSystem));
    LOG_INFO("Skeleton system shutdown");
}

// MARK: - Skeleton Management

Skeleton* skeleton_create(SkeletonSystem* system, const char* name, u32 max_bones) {
    if (!system || !name || max_bones == 0 || system->skeleton_count >= system->skeleton_capacity) {
        return NULL;
    }

    Skeleton* skeleton = &system->skeletons[system->skeleton_count++];
    memset(skeleton, 0, sizeof(Skeleton));

    strncpy(skeleton->name, name, sizeof(skeleton->name) - 1);
    skeleton->name[sizeof(skeleton->name) - 1] = '\0';
    skeleton->skeleton_id = generate_skeleton_id();

    // Allocate bones array
    skeleton->bones = (Bone*)calloc(max_bones, sizeof(Bone));
    if (!skeleton->bones) {
        LOG_ERROR("Failed to allocate bones array for skeleton: %s", name);
        system->skeleton_count--;
        return NULL;
    }
    skeleton->bone_capacity = max_bones;

    // Allocate bone hierarchy array
    skeleton->bone_hierarchy = (u32*)calloc(max_bones, sizeof(u32));
    if (!skeleton->bone_hierarchy) {
        free(skeleton->bones);
        LOG_ERROR("Failed to allocate bone hierarchy array for skeleton: %s", name);
        system->skeleton_count--;
        return NULL;
    }

    // Allocate bone map
    skeleton->bone_map = calloc(max_bones, sizeof(skeleton->bone_map[0]));
    if (!skeleton->bone_map) {
        free(skeleton->bones);
        free(skeleton->bone_hierarchy);
        LOG_ERROR("Failed to allocate bone map for skeleton: %s", name);
        system->skeleton_count--;
        return NULL;
    }
    skeleton->bone_map_capacity = max_bones;

    // Set default properties
    skeleton->scale = 1.0f;
    skeleton->is_humanoid = false;

    LOG_DEBUG("Created skeleton: %s (ID: %u, max bones: %u)", name, skeleton->skeleton_id, max_bones);

    if (system->on_skeleton_created) {
        system->on_skeleton_created(skeleton);
    }

    return skeleton;
}

Bone* skeleton_add_bone(Skeleton* skeleton, const char* name, BoneType type, u32 parent_id) {
    if (!skeleton || !name || skeleton->bone_count >= skeleton->bone_capacity) {
        return NULL;
    }

    // Check if bone name already exists
    if (skeleton_get_bone(skeleton, name)) {
        LOG_ERROR("Bone name already exists: %s", name);
        return NULL;
    }

    Bone* bone = &skeleton->bones[skeleton->bone_count++];
    memset(bone, 0, sizeof(Bone));

    strncpy(bone->name, name, sizeof(bone->name) - 1);
    bone->name[sizeof(bone->name) - 1] = '\0';
    bone->bone_id = generate_bone_id();
    bone->type = type;
    bone->parent_id = parent_id;

    // Set default transform
    bone->local_position = vec3_zero();
    bone->local_rotation = quat_identity();
    bone->local_scale = vec3_one();
    bone->model_position = vec3_zero();
    bone->model_rotation = quat_identity();
    bone->model_scale = vec3_one();

    // Set default matrices
    bone->bind_pose_matrix = mat4_identity();
    bone->inverse_bind_pose_matrix = mat4_identity();
    bone->current_pose_matrix = mat4_identity();
    bone->skinning_matrix = mat4_identity();

    // Set default properties
    bone->length = 1.0f;
    bone->radius = 0.1f;
    bone->is_visible = true;
    bone->is_locked = false;

    // Add to parent's children
    if (parent_id != 0) {
        Bone* parent = skeleton_get_bone_by_id(skeleton, parent_id);
        if (parent) {
            if (parent->child_count >= parent->child_capacity) {
                parent->child_capacity = parent->child_capacity == 0 ? 4 : parent->child_capacity * 2;
                parent->child_ids = (u32*)realloc(parent->child_ids, parent->child_capacity * sizeof(u32));
            }
            parent->child_ids[parent->child_count++] = bone->bone_id;
        }
    } else {
        // This is a root bone
        skeleton->root_bone_id = bone->bone_id;
    }

    // Add to bone map
    if (skeleton->bone_map_count < skeleton->bone_map_capacity) {
        strncpy(skeleton->bone_map[skeleton->bone_map_count].name, name, 64);
        skeleton->bone_map[skeleton->bone_map_count].bone_id = bone->bone_id;
        skeleton->bone_map_count++;
    }

    LOG_DEBUG("Added bone to skeleton %s: %s (ID: %u, parent: %u)", skeleton->name, name, bone->bone_id, parent_id);
    return bone;
}

Bone* skeleton_get_bone(Skeleton* skeleton, const char* name) {
    if (!skeleton || !name) {
        return NULL;
    }

    for (u32 i = 0; i < skeleton->bone_map_count; i++) {
        if (strcmp(skeleton->bone_map[i].name, name) == 0) {
            return skeleton_get_bone_by_id(skeleton, skeleton->bone_map[i].bone_id);
        }
    }

    return NULL;
}

Bone* skeleton_get_bone_by_id(Skeleton* skeleton, u32 bone_id) {
    if (!skeleton || bone_id == 0) {
        return NULL;
    }

    for (u32 i = 0; i < skeleton->bone_count; i++) {
        if (skeleton->bones[i].bone_id == bone_id) {
            return &skeleton->bones[i];
        }
    }

    return NULL;
}

bool skeleton_set_bone_transform(Skeleton* skeleton, u32 bone_id, const Vec3* position,
                               const Quat* rotation, const Vec3* scale, BoneSpace space) {
    if (!skeleton || bone_id == 0) {
        return false;
    }

    Bone* bone = skeleton_get_bone_by_id(skeleton, bone_id);
    if (!bone) {
        return false;
    }

    switch (space) {
        case BONE_SPACE_LOCAL:
            if (position) bone->local_position = *position;
            if (rotation) bone->local_rotation = *rotation;
            if (scale) bone->local_scale = *scale;
            break;
        case BONE_SPACE_MODEL:
            if (position) bone->model_position = *position;
            if (rotation) bone->model_rotation = *rotation;
            if (scale) bone->model_scale = *scale;
            break;
        case BONE_SPACE_WORLD:
            // World space would require additional transformation logic
            LOG_WARN("World space bone transforms not yet implemented");
            return false;
    }

    return true;
}

bool skeleton_update_bone_hierarchy(Skeleton* skeleton) {
    if (!skeleton || !skeleton->root_bone_id) {
        return false;
    }

    // Update hierarchy starting from root bone with Identity transform
    return skeleton_update_bone_hierarchy_recursive(skeleton, skeleton->root_bone_id,
                                                  vec3_zero(), quat_identity(), vec3_one());
}

bool skeleton_update_bone_hierarchy_recursive(Skeleton* skeleton, u32 bone_id,
                                            Vec3 parent_pos, Quat parent_rot, Vec3 parent_scale) {
    Bone* bone = skeleton_get_bone_by_id(skeleton, bone_id);
    if (!bone) {
        return false;
    }

    // Calculate model transform using component-wise propagation

    // 1. Scale: Component-wise multiplication
    bone->model_scale = vec3_scale(parent_scale, bone->local_scale);

    // 2. Rotation: Combine parent and local rotation
    bone->model_rotation = quat_mul(parent_rot, bone->local_rotation);

    // 3. Position: Parent Pos + Parent Rot * (Parent Scale * Local Pos)
    // First apply parent scale to local position
    Vec3 scaled_local_pos = vec3_scale(parent_scale, bone->local_position);
    // Then rotate that scaled vector by parent rotation
    Vec3 rotated_local_pos = quat_rotate_vec3(parent_rot, scaled_local_pos);
    // Finally add to parent position
    bone->model_position = vec3_add(parent_pos, rotated_local_pos);

    // Update current pose matrix from the calculated model components
    bone->current_pose_matrix = create_transform_matrix(&bone->model_position,
                                                      &bone->model_rotation,
                                                      &bone->model_scale);

    // Update children
    for (u32 i = 0; i < bone->child_count; i++) {
        skeleton_update_bone_hierarchy_recursive(skeleton, bone->child_ids[i],
                                               bone->model_position,
                                               bone->model_rotation,
                                               bone->model_scale);
    }

    return true;
}

bool skeleton_calculate_bind_poses(Skeleton* skeleton) {
    if (!skeleton) {
        return false;
    }

    // Store current transforms as bind poses
    for (u32 i = 0; i < skeleton->bone_count; i++) {
        Bone* bone = &skeleton->bones[i];
        bone->bind_pose_matrix = bone->current_pose_matrix;
        bone->inverse_bind_pose_matrix = mat4_inverse(bone->bind_pose_matrix);
    }

    return true;
}

bool skeleton_calculate_skinning_matrices(Skeleton* skeleton) {
    if (!skeleton) {
        return false;
    }

    for (u32 i = 0; i < skeleton->bone_count; i++) {
        Bone* bone = &skeleton->bones[i];
        bone->skinning_matrix = mat4_mul(bone->current_pose_matrix, bone->inverse_bind_pose_matrix);
    }

    return true;
}

// MARK: - Animation Management

AnimationClip* animation_create(SkeletonSystem* system, const char* name, float duration, float fps) {
    if (!system || !name || duration <= 0 || fps <= 0 || system->animation_count >= system->animation_capacity) {
        return NULL;
    }

    AnimationClip* animation = &system->animations[system->animation_count++];
    memset(animation, 0, sizeof(AnimationClip));

    strncpy(animation->name, name, sizeof(animation->name) - 1);
    animation->name[sizeof(animation->name) - 1] = '\0';
    animation->animation_id = generate_animation_id();
    animation->duration = duration;
    animation->fps = fps;
    animation->loops = true;
    animation->is_additive = false;

    LOG_DEBUG("Created animation: %s (ID: %u, duration: %.2f, fps: %.1f)",
             name, animation->animation_id, duration, fps);
    return animation;
}

BoneAnimationTrack* animation_add_track(AnimationClip* animation, u32 bone_id) {
    if (!animation || bone_id == 0) {
        return NULL;
    }

    // Check if track already exists
    for (u32 i = 0; i < animation->track_count; i++) {
        if (animation->tracks[i].bone_id == bone_id) {
            return &animation->tracks[i];
        }
    }

    // Add new track
    if (animation->track_count >= animation->track_capacity) {
        animation->track_capacity = animation->track_capacity == 0 ? 16 : animation->track_capacity * 2;
        animation->tracks = (BoneAnimationTrack*)realloc(animation->tracks,
                                                         animation->track_capacity * sizeof(BoneAnimationTrack));
        if (!animation->tracks) {
            LOG_ERROR("Failed to allocate animation tracks");
            return NULL;
        }
    }

    BoneAnimationTrack* track = &animation->tracks[animation->track_count++];
    memset(track, 0, sizeof(BoneAnimationTrack));
    track->bone_id = bone_id;

    LOG_DEBUG("Added animation track for bone ID: %u", bone_id);
    return track;
}

bool animation_add_keyframe(AnimationClip* animation, u32 bone_id, float time,
                           KeyframeType type, void* value) {
    if (!animation || bone_id == 0 || !value) {
        return false;
    }

    BoneAnimationTrack* track = animation_add_track(animation, bone_id);
    if (!track) {
        return false;
    }

    AnimationKeyframe keyframe;
    memset(&keyframe, 0, sizeof(AnimationKeyframe));
    keyframe.time = time;
    keyframe.type = type;
    keyframe.interpolation = INTERPOLATION_LINEAR;

    // Copy value based on type
    switch (type) {
        case KEYFRAME_POSITION:
            keyframe.value.position = *(Vec3*)value;
            break;
        case KEYFRAME_ROTATION:
            keyframe.value.rotation = *(Quat*)value;
            break;
        case KEYFRAME_SCALE:
            keyframe.value.scale = *(Vec3*)value;
            break;
        case KEYFRAME_CUSTOM:
            keyframe.value.custom_value = *(float*)value;
            break;
    }

    // Add to appropriate keyframe array
    AnimationKeyframe** keyframes = NULL;
    u32* count = NULL;
    u32* capacity = NULL;

    switch (type) {
        case KEYFRAME_POSITION:
            keyframes = &track->position_keyframes;
            count = &track->position_keyframe_count;
            capacity = &track->position_keyframe_capacity;
            break;
        case KEYFRAME_ROTATION:
            keyframes = &track->rotation_keyframes;
            count = &track->rotation_keyframe_count;
            capacity = &track->rotation_keyframe_capacity;
            break;
        case KEYFRAME_SCALE:
            keyframes = &track->scale_keyframes;
            count = &track->scale_keyframe_count;
            capacity = &track->scale_keyframe_capacity;
            break;
        default:
            return false;
    }

    // Expand array if needed
    if (*count >= *capacity) {
        *capacity = *capacity == 0 ? 16 : *capacity * 2;
        *keyframes = (AnimationKeyframe*)realloc(*keyframes, *capacity * sizeof(AnimationKeyframe));
        if (!*keyframes) {
            LOG_ERROR("Failed to allocate keyframes");
            return false;
        }
    }

    // Insert keyframe in chronological order
    u32 insert_index = 0;
    while (insert_index < *count && (*keyframes)[insert_index].time < time) {
        insert_index++;
    }

    // Shift remaining keyframes
    for (u32 i = *count; i > insert_index; i--) {
        (*keyframes)[i] = (*keyframes)[i - 1];
    }

    // Insert new keyframe
    (*keyframes)[insert_index] = keyframe;
    (*count)++;

    LOG_DEBUG("Added keyframe to animation %s for bone %u at time %.2f",
             animation->name, bone_id, time);
    return true;
}

// MARK: - Animation Playback

AnimationState* skeleton_play_animation(Skeleton* skeleton, AnimationClip* animation,
                                      float weight, AnimBlendMode blend_mode) {
    if (!skeleton || !animation) {
        return NULL;
    }

    if (!g_skeleton_system || g_skeleton_system->state_count >= g_skeleton_system->state_capacity) {
        LOG_ERROR("Animation state capacity exceeded");
        return NULL;
    }

    AnimationState* state = &g_skeleton_system->animation_states[g_skeleton_system->state_count++];
    memset(state, 0, sizeof(AnimationState));

    state->clip = animation;
    state->current_time = 0.0f;
    state->playback_speed = 1.0f;
    state->is_playing = true;
    state->is_looping = animation->loops;
    state->weight = weight;
    state->blend_mode = blend_mode;

    skeleton->is_animated = true;
    skeleton->active_animation_count++;

    LOG_DEBUG("Started animation %s on skeleton %s", animation->name, skeleton->name);

    if (g_skeleton_system->on_animation_started) {
        g_skeleton_system->on_animation_started(state);
    }

    return state;
}

bool skeleton_set_animation_time(Skeleton* skeleton, AnimationState* state, float time) {
    if (!skeleton || !state || !state->clip) {
        return false;
    }

    // Clamp time to animation duration
    if (time < 0.0f) time = 0.0f;
    if (time > state->clip->duration) time = state->clip->duration;

    state->current_time = time;

    // Apply animation to skeleton bones
    for (u32 i = 0; i < state->clip->track_count; i++) {
        BoneAnimationTrack* track = &state->clip->tracks[i];
        Bone* bone = skeleton_get_bone_by_id(skeleton, track->bone_id);
        if (!bone) continue;

        // Sample position keyframes
        if (track->position_keyframe_count > 0) {
            Vec3 position = vec3_zero();
            if (skeleton_sample_keyframes(track->position_keyframes, track->position_keyframe_count,
                                        time, KEYFRAME_POSITION, &position)) {
                skeleton_set_bone_transform(skeleton, bone->bone_id, &position, NULL, NULL, BONE_SPACE_LOCAL);
            }
        }

        // Sample rotation keyframes
        if (track->rotation_keyframe_count > 0) {
            Quat rotation = quat_identity();
            if (skeleton_sample_keyframes(track->rotation_keyframes, track->rotation_keyframe_count,
                                        time, KEYFRAME_ROTATION, &rotation)) {
                skeleton_set_bone_transform(skeleton, bone->bone_id, NULL, &rotation, NULL, BONE_SPACE_LOCAL);
            }
        }

        // Sample scale keyframes
        if (track->scale_keyframe_count > 0) {
            Vec3 scale = vec3_one();
            if (skeleton_sample_keyframes(track->scale_keyframes, track->scale_keyframe_count,
                                        time, KEYFRAME_SCALE, &scale)) {
                skeleton_set_bone_transform(skeleton, bone->bone_id, NULL, NULL, &scale, BONE_SPACE_LOCAL);
            }
        }
    }

    return true;
}

bool skeleton_sample_keyframes(const AnimationKeyframe* keyframes, u32 keyframe_count,
                              float time, KeyframeType type, void* result) {
    if (!keyframes || keyframe_count == 0 || !result) {
        return false;
    }

    // Handle single keyframe case
    if (keyframe_count == 1) {
        switch (type) {
            case KEYFRAME_POSITION:
                *(Vec3*)result = keyframes[0].value.position;
                break;
            case KEYFRAME_ROTATION:
                *(Quat*)result = keyframes[0].value.rotation;
                break;
            case KEYFRAME_SCALE:
                *(Vec3*)result = keyframes[0].value.scale;
                break;
            case KEYFRAME_CUSTOM:
                *(float*)result = keyframes[0].value.custom_value;
                break;
        }
        return true;
    }

    // Find surrounding keyframes
    u32 start_index = 0;
    while (start_index < keyframe_count - 1 && keyframes[start_index + 1].time <= time) {
        start_index++;
    }

    if (start_index >= keyframe_count - 1) {
        // Time is beyond last keyframe
        switch (type) {
            case KEYFRAME_POSITION:
                *(Vec3*)result = keyframes[keyframe_count - 1].value.position;
                break;
            case KEYFRAME_ROTATION:
                *(Quat*)result = keyframes[keyframe_count - 1].value.rotation;
                break;
            case KEYFRAME_SCALE:
                *(Vec3*)result = keyframes[keyframe_count - 1].value.scale;
                break;
            case KEYFRAME_CUSTOM:
                *(float*)result = keyframes[keyframe_count - 1].value.custom_value;
                break;
        }
        return true;
    }

    // Interpolate between keyframes
    const AnimationKeyframe* key1 = &keyframes[start_index];
    const AnimationKeyframe* key2 = &keyframes[start_index + 1];

    float t = (time - key1->time) / (key2->time - key1->time);
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    interpolate_keyframes(key1, key2, t, type, result);
    return true;
}

// MARK: - Vertex Skinning

bool skeleton_set_vertex_bone_data(SkeletonSystem* system, u32 vertex_index,
                                  const BoneInfluence* influences, u32 influence_count) {
    if (!system || vertex_index >= system->vertex_capacity || !influences || influence_count == 0) {
        return false;
    }

    VertexBoneData* vertex_data = &system->vertex_bone_data[vertex_index];
    vertex_data->influence_count = (influence_count < 4) ? influence_count : 4;

    for (u32 i = 0; i < vertex_data->influence_count; i++) {
        vertex_data->influences[i] = influences[i];
    }

    return true;
}

bool skeleton_update_skinning_matrices(SkeletonSystem* system, Skeleton* skeleton) {
    if (!system || !skeleton) {
        return false;
    }

    // Update bone hierarchy
    skeleton_update_bone_hierarchy(skeleton);

    // Calculate skinning matrices
    skeleton_calculate_skinning_matrices(skeleton);

    // Copy skinning matrices to system array
    u32 start_index = skeleton->skeleton_id * 256; // Assuming max 256 bones per skeleton
    for (u32 i = 0; i < skeleton->bone_count && i < 256; i++) {
        system->skinning_matrices[start_index + i] = skeleton->bones[i].skinning_matrix;
    }

    return true;
}

Mat4* skeleton_get_skinning_matrices(SkeletonSystem* system, Skeleton* skeleton, u32* matrix_count) {
    if (!system || !skeleton || !matrix_count) {
        return NULL;
    }

    *matrix_count = skeleton->bone_count;
    u32 start_index = skeleton->skeleton_id * 256;
    return &system->skinning_matrices[start_index];
}

// MARK: - Utility Functions

void skeleton_calculate_bounds(Skeleton* skeleton, Vec3* min_bounds, Vec3* max_bounds) {
    if (!skeleton || !min_bounds || !max_bounds) {
        return;
    }

    *min_bounds = vec3(FLT_MAX, FLT_MAX, FLT_MAX);
    *max_bounds = vec3(FLT_MIN, FLT_MIN, FLT_MIN);

    for (u32 i = 0; i < skeleton->bone_count; i++) {
        Bone* bone = &skeleton->bones[i];
        Vec3 bone_pos = bone->model_position;

        min_bounds->x = fminf(min_bounds->x, bone_pos.x);
        min_bounds->y = fminf(min_bounds->y, bone_pos.y);
        min_bounds->z = fminf(min_bounds->z, bone_pos.z);

        max_bounds->x = fmaxf(max_bounds->x, bone_pos.x);
        max_bounds->y = fmaxf(max_bounds->y, bone_pos.y);
        max_bounds->z = fmaxf(max_bounds->z, bone_pos.z);
    }
}

void skeleton_print_hierarchy(Skeleton* skeleton) {
    if (!skeleton) {
        printf("Skeleton is NULL\n");
        return;
    }

    printf("=== SKELETON HIERARCHY: %s ===\n", skeleton->name);
    printf("Bone Count: %u\n", skeleton->bone_count);
    printf("Root Bone ID: %u\n", skeleton->root_bone_id);
    printf("\n");

    if (skeleton->root_bone_id) {
        skeleton_print_bone_recursive(skeleton, skeleton->root_bone_id, 0);
    }

    printf("===============================\n");
}

void skeleton_print_bone_recursive(Skeleton* skeleton, u32 bone_id, u32 depth) {
    Bone* bone = skeleton_get_bone_by_id(skeleton, bone_id);
    if (!bone) return;

    // Print indentation
    for (u32 i = 0; i < depth; i++) {
        printf("  ");
    }

    printf("Bone: %s (ID: %u, Type: %u)\n", bone->name, bone->bone_id, bone->type);

    // Print children
    for (u32 i = 0; i < bone->child_count; i++) {
        skeleton_print_bone_recursive(skeleton, bone->child_ids[i], depth + 1);
    }
}

void skeleton_print_bone_info(Skeleton* skeleton, u32 bone_id) {
    Bone* bone = skeleton_get_bone_by_id(skeleton, bone_id);
    if (!bone) {
        printf("Bone not found: %u\n", bone_id);
        return;
    }

    printf("=== BONE INFO ===\n");
    printf("Name: %s\n", bone->name);
    printf("ID: %u\n", bone->bone_id);
    printf("Type: %u\n", bone->type);
    printf("Parent ID: %u\n", bone->parent_id);
    printf("Child Count: %u\n", bone->child_count);
    printf("Length: %.2f\n", bone->length);
    printf("Radius: %.2f\n", bone->radius);
    printf("Visible: %s\n", bone->is_visible ? "Yes" : "No");
    printf("Locked: %s\n", bone->is_locked ? "Yes" : "No");
    printf("================\n");
}

void skeleton_print_animation_info(AnimationClip* animation) {
    if (!animation) return;
    printf("Animation: %s (%.2fs, %.1f FPS)\n", animation->name, animation->duration, animation->fps);
}
