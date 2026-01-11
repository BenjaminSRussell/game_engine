// src/engine/rendering/animation/advanced_animation.c
// Advanced Animation System - GPU-accelerated animation with skeletal and morph targets

#include <core/logger.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../render_pipeline.h"

// ============================================================================
// Animation System Types
// ============================================================================

typedef enum {
    ANIMATION_TYPE_SKELETAL,
    ANIMATION_TYPE_MORPH,
    ANIMATION_TYPE_VERTEX,
    ANIMATION_TYPE_TEXTURE,
    ANIMATION_TYPE_CAMERA,
    ANIMATION_TYPE_PARTICLE,
    ANIMATION_TYPE_COUNT
} AnimationType;

typedef struct {
    float position[3];
    float rotation[4];      // Quaternion
    float scale[3];
    float timestamp;
} AnimationKeyframe;

typedef struct {
    AnimationKeyframe *keyframes;
    uint32_t keyframe_count;
    uint32_t current_keyframe;
    uint32_t next_keyframe;
    float interpolation_factor;
    bool is_playing;
    bool loop;
    float playback_speed;
    float current_time;
    float duration;
    char name[256];
} AnimationTrack;

typedef struct {
    // Skeletal animation data
    uint32_t bone_count;
    float *bone_matrices;        // 4x4 matrices for each bone
    float *inverse_bind_matrices; // Inverse bind pose matrices
    char **bone_names;
    
    // Animation tracks
    AnimationTrack *tracks;
    uint32_t track_count;
    
    // GPU resources
    void *bone_buffer;
    void *animation_buffer;
    void *morph_buffer;
    
    // Morph target data
    float *morph_weights;
    uint32_t morph_target_count;
    float *morph_targets;
    
    // Animation state
    bool is_animated;
    bool has_skeletal_animation;
    bool has_morph_animation;
    bool has_vertex_animation;
    
    // Performance
    bool enable_gpu_skinning;
    bool enable_morphing;
    uint32_t max_bones;
    uint32_t max_morph_targets;
    
    // Statistics
    float animation_time_ms;
    uint32_t bones_animated;
    uint32_t morph_targets_animated;
    uint32_t vertices_animated;
    
    char name[256];
    bool initialized;
} AnimationSystem;

static AnimationSystem g_animation_system = {0};

// ============================================================================
// Animation Mathematics
// ============================================================================

static void quaternion_multiply(const float *q1, const float *q2, float *result) {
    result[0] = q1[0] * q2[0] - q1[1] * q2[1] - q1[2] * q2[2] - q1[3] * q2[3];
    result[1] = q1[0] * q2[1] + q1[1] * q2[0] + q1[2] * q2[3] - q1[3] * q2[2];
    result[2] = q1[0] * q2[2] - q1[1] * q2[3] + q1[2] * q2[0] + q1[3] * q2[1];
    result[3] = q1[0] * q2[3] + q1[1] * q2[2] - q1[2] * q2[1] + q1[3] * q2[0];
}

static void quaternion_normalize(float *q) {
    float length = sqrtf(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
    if (length > 0.0f) {
        q[0] /= length;
        q[1] /= length;
        q[2] /= length;
        q[3] /= length;
    }
}

static void quaternion_from_axis_angle(const float *axis, float angle, float *q) {
    float half_angle = angle * 0.5f;
    float sin_half = sinf(half_angle);
    
    q[0] = cosf(half_angle);
    q[1] = axis[0] * sin_half;
    q[2] = axis[1] * sin_half;
    q[3] = axis[2] * sin_half;
    
    quaternion_normalize(q);
}

static void quaternion_to_matrix(const float *q, float *matrix) {
    float xx = q[0] * q[0];
    float xy = q[0] * q[1];
    float xz = q[0] * q[2];
    float xw = q[0] * q[3];
    float yy = q[1] * q[1];
    float yz = q[1] * q[2];
    float yw = q[1] * q[3];
    float zz = q[2] * q[2];
    float zw = q[2] * q[3];
    
    matrix[0] = 1.0f - 2.0f * (yy + zz);
    matrix[1] = 2.0f * (xy - zw);
    matrix[2] = 2.0f * (xz + yw);
    matrix[3] = 0.0f;
    
    matrix[4] = 2.0f * (xy + zw);
    matrix[5] = 1.0f - 2.0f * (xx + zz);
    matrix[6] = 2.0f * (yz - xw);
    matrix[7] = 0.0f;
    
    matrix[8] = 2.0f * (xz - yw);
    matrix[9] = 2.0f * (yz + xw);
    matrix[10] = 1.0f - 2.0f * (xx + yy);
    matrix[11] = 0.0f;
    
    matrix[12] = 0.0f;
    matrix[13] = 0.0f;
    matrix[14] = 0.0f;
    matrix[15] = 1.0f;
}

static void matrix_multiply(const float *m1, const float *m2, float *result) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result[i * 4 + j] = 0.0f;
            for (int k = 0; k < 4; k++) {
                result[i * 4 + j] += m1[i * 4 + k] * m2[k * 4 + j];
            }
        }
    }
}

static void matrix_multiply_4x4(const float *m1, const float *m2, float *result) {
    result[0] = m1[0] * m2[0] + m1[1] * m2[4] + m1[2] * m2[8] + m1[3] * m2[12];
    result[1] = m1[0] * m2[1] + m1[1] * m2[5] + m1[2] * m2[9] + m1[3] * m2[13];
    result[2] = m1[0] * m2[2] + m1[1] * m2[6] + m1[2] * m2[10] + m1[3] * m2[14];
    result[3] = m1[0] * m2[3] + m1[1] * m2[7] + m1[2] * m2[11] + m1[3] * m2[15];
    
    result[4] = m1[4] * m2[0] + m1[5] * m2[4] + m1[6] * m2[8] + m1[7] * m2[12];
    result[5] = m1[4] * m2[1] + m1[5] * m2[5] + m1[6] * m2[9] + m1[7] * m2[13];
    result[6] = m1[4] * m2[2] + m1[5] * m2[6] + m1[6] * m2[10] + m1[7] * m2[14];
    result[7] = m1[4] * m2[3] + m1[5] * m2[7] + m1[6] * m2[11] + m1[7] * m2[15];
    
    result[8] = m1[8] * m2[0] + m1[9] * m2[4] + m1[10] * m2[8] + m1[11] * m2[12];
    result[9] = m1[8] * m2[1] + m1[9] * m2[5] + m1[10] * m2[9] + m1[11] * m2[13];
    result[10] = m1[8] * m2[2] + m1[9] * m2[6] + m1[10] * m2[10] + m1[11] * m2[14];
    result[11] = m1[8] * m2[3] + m1[9] * m2[7] + m1[10] * m2[11] + m1[11] * m2[15];
    
    result[12] = m1[12] * m2[0] + m1[13] * m2[4] + m1[14] * m2[8] + m1[15] * m2[12];
    result[13] = m1[12] * m2[1] + m1[13] * m2[5] + m1[14] * m2[9] + m1[15] * m2[13];
    result[14] = m1[12] * m2[2] + m1[13] * m2[6] + m1[14] * m2[10] + m1[15] * m2[14];
    result[15] = m1[12] * m2[3] + m1[13] * m2[7] + m1[14] * m2[11] + m1[15] * m2[15];
}

static void matrix_translate(const float *matrix, const float *translation, float *result) {
    memcpy(result, matrix, 16 * sizeof(float));
    result[12] = translation[0];
    result[13] = translation[1];
    result[14] = translation[2];
}

static void matrix_scale(const float *matrix, const float *scale, float *result) {
    result[0] = matrix[0] * scale[0];
    result[1] = matrix[1] * scale[0];
    result[2] = matrix[2] * scale[0];
    result[3] = matrix[3];
    
    result[4] = matrix[4] * scale[1];
    result[5] = matrix[5] * scale[1];
    result[6] = matrix[6] * scale[1];
    result[7] = matrix[7];
    
    result[8] = matrix[8] * scale[2];
    result[9] = matrix[9] * scale[2];
    result[10] = matrix[10] * scale[2];
    result[11] = matrix[11];
    
    result[12] = matrix[12];
    result[13] = matrix[13];
    result[14] = matrix[14];
    result[15] = matrix[15];
}

// ============================================================================
// Animation Track Implementation
// ============================================================================

static void update_animation_track(AnimationTrack *track, float dt) {
    if (!track || !track->is_playing || track->keyframe_count < 2) return;
    
    track->current_time += dt * track->playback_speed;
    
    // Handle looping
    if (track->current_time >= track->duration) {
        if (track->loop) {
            track->current_time = fmodf(track->current_time, track->duration);
        } else {
            track->current_time = track->duration;
            track->is_playing = false;
            return;
        }
    }
    
    // Find current and next keyframes
    for (uint32_t i = 0; i < track->keyframe_count - 1; i++) {
        if (track->keyframes[i].timestamp <= track->current_time && 
            track->keyframes[i + 1].timestamp > track->current_time) {
            track->current_keyframe = i;
            track->next_keyframe = i + 1;
            break;
        }
    }
    
    // Calculate interpolation factor
    float current_time = track->keyframes[track->current_keyframe].timestamp;
    float next_time = track->keyframes[track->next_keyframe].timestamp;
    float time_range = next_time - current_time;
    
    if (time_range > 0.0f) {
        track->interpolation_factor = (track->current_time - current_time) / time_range;
    } else {
        track->interpolation_factor = 0.0f;
    }
}

static void interpolate_keyframes(const AnimationKeyframe *keyframe1, const AnimationKeyframe *keyframe2, 
                                float t, AnimationKeyframe *result) {
    // Linear interpolation for position
    result->position[0] = keyframe1->position[0] + t * (keyframe2->position[0] - keyframe1->position[0]);
    result->position[1] = keyframe1->position[1] + t * (keyframe2->position[1] - keyframe1->position[1]);
    result->position[2] = keyframe1->position[2] + t * (keyframe2->position[2] - keyframe1->position[2]);
    
    // Spherical linear interpolation for rotation
    float q1[4], q2[4], q_result[4];
    memcpy(q1, keyframe1->rotation, 4 * sizeof(float));
    memcpy(q2, keyframe2->rotation, 4 * sizeof(float));
    
    // TODO: Implement proper spherical linear interpolation
    quaternion_multiply(q1, q2, q_result);
    quaternion_normalize(q_result);
    memcpy(result->rotation, q_result, 4 * sizeof(float));
    
    // Linear interpolation for scale
    result->scale[0] = keyframe1->scale[0] + t * (keyframe2->scale[0] - keyframe1->scale[0]);
    result->scale[1] = keyframe1->scale[1] + t * (keyframe2->scale[1] - keyframe1->scale[1]);
    result->scale[2] = keyframe1->scale[2] + t * (keyframe2->scale[2] - keyframe1->scale[2]);
    
    result->timestamp = keyframe1->timestamp + t * (keyframe2->timestamp - keyframe1->timestamp);
}

// ============================================================================
// Animation System API
// ============================================================================

bool animation_system_init(uint32_t max_animations, uint32_t max_bones, uint32_t max_morph_targets, 
                           bool enable_gpu_skinning, bool enable_morphing) {
    if (g_animation_system.initialized) {
        LOG_WARN("Animation system already initialized");
        return true;
    }
    
    memset(&g_animation_system, 0, sizeof(AnimationSystem));
    
    g_animation_system.max_bones = max_bones;
    g_animation_system.max_morph_targets = max_morph_targets;
    g_animation_system.enable_gpu_skinning = enable_gpu_skinning;
    g_animation_system.enable_morphing = enable_morphing;
    
    // Allocate bone matrices
    g_animation_system.bone_matrices = calloc(max_bones * 16, sizeof(float));
    g_animation_system.inverse_bind_matrices = calloc(max_bones * 16, sizeof(float));
    
    if (!g_animation_system.bone_matrices || !g_animation_system.inverse_bind_matrices) {
        LOG_ERROR("Failed to allocate bone matrices");
        free(g_animation_system.bone_matrices);
        free(g_animation_system.inverse_bind_matrices);
        return false;
    }
    
    // Allocate bone names
    g_animation_system.bone_names = calloc(max_bones, sizeof(char*));
    
    // Allocate morph data
    g_animation_system.morph_weights = calloc(max_morph_targets, sizeof(float));
    g_animation_system.morph_targets = calloc(max_morph_targets * 256, sizeof(float)); // 256 vertices per morph target
    
    if (!g_animation_system.morph_weights || !g_animation_system.morph_targets) {
        LOG_ERROR("Failed to allocate morph data");
        free(g_animation_system.bone_matrices);
        free(g_animation_system.inverse_bind_matrices);
        free(g_animation_system.bone_names);
        return false;
    }
    
    // TODO: Create GPU resources
    // g_animation_system.bone_buffer = create_buffer(max_bones * 16 * sizeof(float));
    // g_animation_system.animation_buffer = create_buffer(max_animations * sizeof(AnimationData));
    // g_animation_system.morph_buffer = create_buffer(max_morph_targets * 256 * sizeof(float));
    
    g_animation_system.initialized = true;
    LOG_INFO("Animation system initialized (max_animations: %u, max_bones: %u, max_morph_targets: %u, gpu_skinning: %s, morphing: %s)",
             max_animations, max_bones, max_morph_targets, enable_gpu_skinning ? "yes" : "no", enable_morphing ? "yes" : "no");
    return true;
}

void animation_system_shutdown(void) {
    if (!g_animation_system.initialized)
        return;
    
    LOG_INFO("Shutting down animation system");
    
    // Destroy all animation systems
    // TODO: Destroy individual animation systems
    
    // Free memory
    free(g_animation_system.bone_matrices);
    free(g_animation_system.inverse_bind_matrices);
    free(g_animation_system.bone_names);
    free(g_animation_system.morph_weights);
    free(g_animation_system.morph_targets);
    
    // TODO: Destroy GPU resources
    // destroy_buffer(g_animation_system.bone_buffer);
    // destroy_buffer(g_animation_system.animation_buffer);
    // destroy_buffer(g_animation_system.morph_buffer);
    
    memset(&g_animation_system, 0, sizeof(AnimationSystem));
    
    LOG_INFO("Animation system shutdown complete");
}

AnimationSystem *animation_system_create(const char *name, uint32_t bone_count, uint32_t morph_target_count) {
    if (!g_animation_system.initialized || !name) {
        LOG_ERROR("Animation system not initialized or invalid name");
        return NULL;
    }
    
    AnimationSystem *animation = calloc(1, sizeof(AnimationSystem));
    if (!animation) {
        LOG_ERROR("Failed to allocate animation system");
        return NULL;
    }
    
    strncpy(animation->name, name, sizeof(animation->name) - 1);
    animation->bone_count = bone_count;
    animation->morph_target_count = morph_target_count;
    
    animation->has_skeletal_animation = (bone_count > 0);
    animation->has_morph_animation = (morph_target_count > 0);
    animation->is_animated = animation->has_skeletal_animation || animation->has_morph_animation;
    
    // Allocate animation tracks
    animation->tracks = calloc(10, sizeof(AnimationTrack)); // Default 10 tracks
    animation->track_count = 10;
    
    if (!animation->tracks) {
        LOG_ERROR("Failed to allocate animation tracks");
        free(animation);
        return NULL;
    }
    
    // Initialize tracks
    for (uint32_t i = 0; i < animation->track_count; i++) {
        animation->tracks[i].keyframes = calloc(100, sizeof(AnimationKeyframe)); // Default 100 keyframes per track
        animation->tracks[i].keyframe_count = 0;
        animation->tracks[i].is_playing = false;
        animation->tracks[i].loop = true;
        animation->tracks[i].playback_speed = 1.0f;
        animation->tracks[i].current_time = 0.0f;
        animation->tracks[i].duration = 0.0f;
    }
    
    g_animation_system.initialized = true;
    LOG_INFO("Created animation system: %s (bones: %u, morph_targets: %u)", name, bone_count, morph_target_count);
    return animation;
}

void animation_system_destroy(AnimationSystem *animation) {
    if (!animation) return;
    
    // Free animation tracks
    for (uint32_t i = 0; i < animation->track_count; i++) {
        free(animation->tracks[i].keyframes);
    }
    free(animation->tracks);
    
    free(animation);
    
    LOG_DEBUG("Destroyed animation system: %s", animation->name);
}

void animation_system_update(AnimationSystem *animation, float dt) {
    if (!animation || !animation->is_animated) return;
    
    uint64_t start_time = get_time_nanos();
    
    // Update all tracks
    for (uint32_t i = 0; i < animation->track_count; i++) {
        update_animation_track(&animation->tracks[i], dt);
    }
    
    // Update bone matrices if skeletal animation
    if (animation->has_skeletal_animation) {
        // TODO: Update bone matrices from animation tracks
        animation->bones_animated = animation->bone_count;
    }
    
    // Update morph weights if morph animation
    if (animation->has_morph_animation) {
        // TODO: Update morph weights from animation tracks
        animation->morph_targets_animated = animation->morph_target_count;
    }
    
    uint64_t end_time = get_time_nanos();
    animation->animation_time_ms += nanos_to_ms(end_time - start_time);
    
    LOG_DEBUG("Updated animation system: %s (%.2f ms)", animation->name, animation->animation_time_ms);
}

void animation_system_add_keyframe(AnimationSystem *animation, uint32_t track_index, 
                                 const float *position, const float *rotation, const float *scale, float timestamp) {
    if (!animation || !animation->tracks || track_index >= animation->track_count) {
        return;
    }
    
    AnimationTrack *track = &animation->tracks[track_index];
    
    if (track->keyframe_count >= 100) {
        LOG_WARN("Animation track %u has reached maximum keyframes", track_index);
        return;
    }
    
    AnimationKeyframe *keyframe = &track->keyframes[track->keyframe_count++];
    
    if (position) {
        keyframe->position[0] = position[0];
        keyframe->position[1] = position[1];
        keyframe->position[2] = position[2];
    }
    
    if (rotation) {
        keyframe->rotation[0] = rotation[0];
        keyframe->rotation[1] = rotation[1];
        keyframe->rotation[2] = rotation[2];
        keyframe->rotation[3] = rotation[3];
    }
    
    if (scale) {
        keyframe->scale[0] = scale[0];
        keyframe->scale[1] = scale[1];
        keyframe->scale[2] = scale[2];
    }
    
    keyframe->timestamp = timestamp;
    
    // Update duration
    if (timestamp > track->duration) {
        track->duration = timestamp;
    }
    
    LOG_DEBUG("Added keyframe to track %u at time %.2f", track_index, timestamp);
}

void animation_system_play_animation(AnimationSystem *animation, uint32_t track_index, bool loop, float playback_speed) {
    if (!animation || !animation->tracks || track_index >= animation->track_count) {
        return;
    }
    
    AnimationTrack *track = &animation->tracks[track_index];
    
    track->is_playing = true;
    track->loop = loop;
    track->playback_speed = playback_speed;
    track->current_time = 0.0f;
    track->current_keyframe = 0;
    track->next_keyframe = 1;
    
    LOG_DEBUG("Playing animation track %u (loop: %s, speed: %.2f)", track_index, loop ? "yes" : "no", playback_speed);
}

void animation_system_stop_animation(AnimationSystem *animation, uint32_t track_index) {
    if (!animation || !animation->tracks || track_index >= animation->track_count) {
        return;
    }
    
    animation->tracks[track_index].is_playing = false;
    
    LOG_DEBUG("Stopped animation track %u", track_index);
}

void animation_system_set_bone_matrix(AnimationSystem *animation, uint32_t bone_index, const float *matrix) {
    if (!animation || !animation->bone_matrices || bone_index >= animation->bone_count) {
        return;
    }
    
    memcpy(&animation->bone_matrices[bone_index * 16], matrix, 16 * sizeof(float));
    
    LOG_DEBUG("Set bone matrix %u", bone_index);
}

void animation_system_set_morph_weight(AnimationSystem *animation, uint32_t morph_index, float weight) {
    if (!animation || !animation->morph_weights || morph_index >= animation->morph_target_count) {
        return;
    }
    
    animation->morph_weights[morph_index] = weight;
    
    LOG_DEBUG("Set morph weight %u: %.2f", morph_index, weight);
}

void animation_system_get_bone_matrix(AnimationSystem *animation, uint32_t bone_index, float *matrix) {
    if (!animation || !animation->bone_matrices || bone_index >= animation->bone_count || !matrix) {
        return;
    }
    
    memcpy(matrix, &animation->bone_matrices[bone_index * 16], 16 * sizeof(float));
}

void animation_system_get_morph_weight(AnimationSystem *animation, uint32_t morph_index, float *weight) {
    if (!animation || !animation->morph_weights || morph_index >= animation->morph_target_count || !weight) {
        return;
    }
    
    *weight = animation->morph_weights[morph_index];
}

void animation_system_get_stats(AnimationSystem *animation, float *animation_time, uint32_t *bones_animated, 
                                 uint32_t *morph_targets_animated, uint32_t *vertices_animated) {
    if (!animation) return;
    
    if (animation_time) *animation_time = animation->animation_time_ms;
    if (bones_animated) *bones_animated = animation->bones_animated;
    if (morph_targets_animated) *morph_targets_animated = animation->morph_targets_animated;
    if (vertices_animated) *vertices_animated = animation->vertices_animated;
    
    LOG_DEBUG("Animation stats: time=%.2f ms, bones=%u, morphs=%u, vertices=%u",
             *animation_time, *bones_animated, *morph_targets_animated, *vertices_animated);
}

bool animation_system_is_initialized(void) {
    return g_animation_system.initialized;
}
