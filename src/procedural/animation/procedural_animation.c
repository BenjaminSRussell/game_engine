/*
 * procedural_animation.c
 * Procedural animation generation algorithms
 *
 * Part of the Procedural Generation subsystem
 * Advanced 3D Rendering Engine
 */

#include "procedural_animation.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

static Quat quat_from_axis_angle(const Vec3* axis, float angle) {
    Quat q;
    float half_angle = angle * 0.5f;
    float s = sinf(half_angle);
    
    q.x = axis->x * s;
    q.y = axis->y * s;
    q.z = axis->z * s;
    q.w = cosf(half_angle);
    
    return q;
}

static Quat quat_normalize(const Quat* q) {
    float length = sqrtf(q->x * q->x + q->y * q->y + q->z * q->z + q->w * q->w);
    Quat result;
    result.x = q->x / length;
    result.y = q->y / length;
    result.z = q->z / length;
    result.w = q->w / length;
    return result;
}

static Quat quat_slerp(const Quat* q1, const Quat* q2, float t) {
    float dot = q1->x * q2->x + q1->y * q2->y + q1->z * q2->z + q1->w * q2->w;
    
    if (dot < 0.0f) {
        dot = -dot;
        Quat neg_q2 = {-q2->x, -q2->y, -q2->z, -q2->w};
        return quat_slerp(q1, &neg_q2, t);
    }
    
    if (dot > 0.9995f) {
        Quat result;
        result.x = q1->x + t * (q2->x - q1->x);
        result.y = q1->y + t * (q2->y - q1->y);
        result.z = q1->z + t * (q2->z - q1->z);
        result.w = q1->w + t * (q2->w - q1->w);
        return quat_normalize(&result);
    }
    
    float angle = acosf(dot);
    float sin_angle = sinf(angle);
    float a = sinf((1.0f - t) * angle) / sin_angle;
    float b = sinf(t * angle) / sin_angle;
    
    Quat result;
    result.x = a * q1->x + b * q2->x;
    result.y = a * q1->y + b * q2->y;
    result.z = a * q1->z + b * q2->z;
    result.w = a * q1->w + b * q2->w;
    
    return quat_normalize(&result);
}

static Transform transform_lerp(const Transform* t1, const Transform* t2, float t) {
    Transform result;
    
    result.rotation = quat_slerp(&t1->rotation, &t2->rotation, t);
    result.translation.x = t1->translation.x + t * (t2->translation.x - t1->translation.x);
    result.translation.y = t1->translation.y + t * (t2->translation.y - t1->translation.y);
    result.translation.z = t1->translation.z + t * (t2->translation.z - t1->translation.z);
    result.scale.x = t1->scale.x + t * (t2->scale.x - t1->scale.x);
    result.scale.y = t1->scale.y + t * (t2->scale.y - t1->scale.y);
    result.scale.z = t1->scale.z + t * (t2->scale.z - t1->scale.z);
    
    return result;
}

ProceduralAnimator* proc_anim_create_animator(const Skeleton* skeleton) {
    if (!skeleton) return NULL;
    
    ProceduralAnimator* animator = malloc(sizeof(ProceduralAnimator));
    if (!animator) return NULL;
    
    animator->skeleton = malloc(sizeof(Skeleton));
    if (!animator->skeleton) {
        free(animator);
        return NULL;
    }
    
    // Copy skeleton
    animator->skeleton->bone_count = skeleton->bone_count;
    animator->skeleton->bones = malloc(skeleton->bone_count * sizeof(Transform));
    animator->skeleton->bone_names = malloc(skeleton->bone_count * sizeof(char*));
    animator->skeleton->bone_parents = malloc(skeleton->bone_count * sizeof(uint32_t));
    animator->skeleton->inverse_bind_matrices = malloc(skeleton->bone_count * sizeof(Mat4));
    
    if (!animator->skeleton->bones || !animator->skeleton->bone_names || 
        !animator->skeleton->bone_parents || !animator->skeleton->inverse_bind_matrices) {
        proc_anim_destroy_animator(animator);
        return NULL;
    }
    
    memcpy(animator->skeleton->bones, skeleton->bones, skeleton->bone_count * sizeof(Transform));
    memcpy(animator->skeleton->bone_parents, skeleton->bone_parents, skeleton->bone_count * sizeof(uint32_t));
    memcpy(animator->skeleton->inverse_bind_matrices, skeleton->inverse_bind_matrices, skeleton->bone_count * sizeof(Mat4));
    
    for (uint32_t i = 0; i < skeleton->bone_count; i++) {
        animator->skeleton->bone_names[i] = malloc(64 * sizeof(char));
        if (animator->skeleton->bone_names[i]) {
            strcpy(animator->skeleton->bone_names[i], skeleton->bone_names[i]);
        }
    }
    
    animator->clips = NULL;
    animator->clip_count = 0;
    animator->states = NULL;
    animator->state_count = 0;
    animator->current_pose = malloc(skeleton->bone_count * sizeof(Transform));
    animator->is_playing = false;
    
    if (!animator->current_pose) {
        proc_anim_destroy_animator(animator);
        return NULL;
    }
    
    // Initialize current pose to bind pose
    memcpy(animator->current_pose, skeleton->bones, skeleton->bone_count * sizeof(Transform));
    
    return animator;
}

bool proc_anim_generate_walk_cycle(ProceduralAnimator* animator, const WalkParameters* params) {
    if (!animator || !params) return false;
    
    // Create new animation clip
    AnimationClip* new_clips = realloc(animator->clips, (animator->clip_count + 1) * sizeof(AnimationClip));
    if (!new_clips) return false;
    
    animator->clips = new_clips;
    AnimationClip* clip = &animator->clips[animator->clip_count];
    strcpy(clip->name, "Walk");
    clip->duration = 1.0f / params->step_frequency; // One cycle duration
    
    // Create channels for key bones (simplified - would identify actual bones)
    clip->channel_count = 4; // Left foot, right foot, left hand, right hand
    clip->channels = malloc(clip->channel_count * sizeof(AnimationChannel));
    
    if (!clip->channels) return false;
    
    // Generate keyframes for walk cycle
    for (uint32_t i = 0; i < clip->channel_count; i++) {
        AnimationChannel* channel = &clip->channels[i];
        channel->bone_index = i; // Simplified bone mapping
        channel->keyframe_count = 8; // 8 keyframes for smooth walk cycle
        channel->keyframe_times = malloc(channel->keyframe_count * sizeof(float));
        channel->keyframes = malloc(channel->keyframe_count * sizeof(Transform));
        
        if (!channel->keyframe_times || !channel->keyframes) return false;
        
        // Generate walk cycle keyframes
        for (uint32_t j = 0; j < channel->keyframe_count; j++) {
            float t = (float)j / (float)(channel->keyframe_count - 1);
            channel->keyframe_times[j] = t * clip->duration;
            
            Transform keyframe = {0};
            
            if (i == 0) { // Left foot
                float phase = t * 2.0f * M_PI;
                keyframe.translation.y = sinf(phase) * params->step_height;
                keyframe.translation.x = cosf(phase) * params->step_length * 0.5f;
            } else if (i == 1) { // Right foot (opposite phase)
                float phase = t * 2.0f * M_PI + M_PI;
                keyframe.translation.y = sinf(phase) * params->step_height;
                keyframe.translation.x = cosf(phase) * params->step_length * 0.5f;
            } else if (i == 2) { // Left hand
                float phase = t * 2.0f * M_PI + M_PI; // Opposite to left foot
                keyframe.translation.y = sinf(phase) * params->arm_swing * 0.5f;
                keyframe.translation.x = cosf(phase) * params->arm_swing * 0.3f;
            } else if (i == 3) { // Right hand
                float phase = t * 2.0f * M_PI; // Same as left foot
                keyframe.translation.y = sinf(phase) * params->arm_swing * 0.5f;
                keyframe.translation.x = cosf(phase) * params->arm_swing * 0.3f;
            }
            
            // Add body bob
            keyframe.translation.y += sinf(t * 4.0f * M_PI) * params->body_bob;
            
            channel->keyframes[j] = keyframe;
        }
    }
    
    animator->clip_count++;
    return true;
}

bool proc_anim_generate_jump_animation(ProceduralAnimator* animator, const JumpParameters* params) {
    if (!animator || !params) return NULL;
    
    AnimationClip* new_clips = realloc(animator->clips, (animator->clip_count + 1) * sizeof(AnimationClip));
    if (!new_clips) return false;
    
    animator->clips = new_clips;
    AnimationClip* clip = &animator->clips[animator->clip_count];
    strcpy(clip->name, "Jump");
    clip->duration = params->jump_duration;
    
    clip->channel_count = 2; // Main body and feet
    clip->channels = malloc(clip->channel_count * sizeof(AnimationChannel));
    
    if (!clip->channels) return false;
    
    for (uint32_t i = 0; i < clip->channel_count; i++) {
        AnimationChannel* channel = &clip->channels[i];
        channel->bone_index = i;
        channel->keyframe_count = 5; // Takeoff, ascent, peak, descent, landing
        channel->keyframe_times = malloc(channel->keyframe_count * sizeof(float));
        channel->keyframes = malloc(channel->keyframe_count * sizeof(Transform));
        
        if (!channel->keyframe_times || !channel->keyframes) return false;
        
        float times[] = {0.0f, 0.2f, 0.5f, 0.8f, 1.0f};
        float heights[] = {0.0f, params->jump_height * 0.3f, params->jump_height, params->jump_height * 0.3f, 0.0f};
        
        for (uint32_t j = 0; j < channel->keyframe_count; j++) {
            channel->keyframe_times[j] = times[j] * clip->duration;
            
            Transform keyframe = {0};
            keyframe.translation.y = heights[j];
            
            if (i == 1) { // Feet - tuck during jump
                if (j >= 1 && j <= 3) {
                    keyframe.rotation = quat_from_axis_angle(&(Vec3){1, 0, 0}, -0.5f);
                }
            }
            
            channel->keyframes[j] = keyframe;
        }
    }
    
    animator->clip_count++;
    return true;
}

bool proc_anim_generate_idle_animation(ProceduralAnimator* animator, const AnimationParameters* params) {
    if (!animator || !params) return false;
    
    AnimationClip* new_clips = realloc(animator->clips, (animator->clip_count + 1) * sizeof(AnimationClip));
    if (!new_clips) return false;
    
    animator->clips = new_clips;
    AnimationClip* clip = &animator->clips[animator->clip_count];
    strcpy(clip->name, "Idle");
    clip->duration = 4.0f; // 4 second idle cycle
    
    clip->channel_count = 3; // Body, head, breathing
    clip->channels = malloc(clip->channel_count * sizeof(AnimationChannel));
    
    if (!clip->channels) return false;
    
    for (uint32_t i = 0; i < clip->channel_count; i++) {
        AnimationChannel* channel = &clip->channels[i];
        channel->bone_index = i;
        channel->keyframe_count = 4;
        channel->keyframe_times = malloc(channel->keyframe_count * sizeof(float));
        channel->keyframes = malloc(channel->keyframe_count * sizeof(Transform));
        
        if (!channel->keyframe_times || !channel->keyframes) return false;
        
        for (uint32_t j = 0; j < channel->keyframe_count; j++) {
            float t = (float)j / (float)(channel->keyframe_count - 1);
            channel->keyframe_times[j] = t * clip->duration;
            
            Transform keyframe = {0};
            
            if (i == 0) { // Body sway
                keyframe.rotation = quat_from_axis_angle(&(Vec3){0, 1, 0}, 
                                                         sinf(t * 2.0f * M_PI) * params->amplitude * 0.1f);
            } else if (i == 1) { // Head movement
                keyframe.rotation = quat_from_axis_angle(&(Vec3){1, 0, 0}, 
                                                         sinf(t * 2.0f * M_PI + params->phase_offset) * 
                                                         params->amplitude * 0.05f);
            } else if (i == 2) { // Breathing
                keyframe.scale.x = 1.0f + sinf(t * 4.0f * M_PI) * params->amplitude * 0.02f;
                keyframe.scale.y = 1.0f + sinf(t * 4.0f * M_PI) * params->amplitude * 0.02f;
                keyframe.scale.z = 1.0f + sinf(t * 4.0f * M_PI) * params->amplitude * 0.02f;
            }
            
            channel->keyframes[j] = keyframe;
        }
    }
    
    animator->clip_count++;
    return true;
}

Transform proc_anim_evaluate_animation(const ProceduralAnimator* animator, uint32_t clip_id, float time) {
    Transform result = {0};
    
    if (!animator || clip_id >= animator->clip_count) return result;
    
    const AnimationClip* clip = &animator->clips[clip_id];
    
    // Loop time if necessary
    if (time > clip->duration) {
        time = fmodf(time, clip->duration);
    }
    
    // Evaluate first channel (simplified)
    if (clip->channel_count > 0) {
        const AnimationChannel* channel = &clip->channels[0];
        
        // Find surrounding keyframes
        uint32_t key1 = 0, key2 = 0;
        for (uint32_t i = 0; i < channel->keyframe_count - 1; i++) {
            if (time >= channel->keyframe_times[i] && time <= channel->keyframe_times[i + 1]) {
                key1 = i;
                key2 = i + 1;
                break;
            }
        }
        
        if (key2 > key1) {
            float t1 = channel->keyframe_times[key1];
            float t2 = channel->keyframe_times[key2];
            float blend = (time - t1) / (t2 - t1);
            
            result = transform_lerp(&channel->keyframes[key1], &channel->keyframes[key2], blend);
        } else {
            result = channel->keyframes[key1];
        }
    }
    
    return result;
}

bool proc_anim_play_animation(ProceduralAnimator* animator, uint32_t clip_id) {
    if (!animator || clip_id >= animator->clip_count) return false;
    
    animator->is_playing = true;
    
    // Create or update animation state
    if (animator->state_count == 0) {
        animator->states = malloc(sizeof(AnimationState));
        if (!animator->states) return false;
        animator->state_count = 1;
    }
    
    animator->states[0].time = 0.0f;
    animator->states[0].duration = animator->clips[clip_id].duration;
    animator->states[0].loop = true;
    animator->states[0].speed = 1.0f;
    
    return true;
}

bool proc_anim_add_variation(ProceduralAnimator* animator, uint32_t clip_id, float variation_amount) {
    if (!animator || clip_id >= animator->clip_count) return false;
    
    AnimationClip* clip = &animator->clips[clip_id];
    
    for (uint32_t i = 0; i < clip->channel_count; i++) {
        AnimationChannel* channel = &clip->channels[i];
        
        for (uint32_t j = 0; j < channel->keyframe_count; j++) {
            Transform* keyframe = &channel->keyframes[j];
            
            // Add random variation
            keyframe->translation.x += (float)(rand() % 1000 - 500) / 1000.0f * variation_amount;
            keyframe->translation.y += (float)(rand() % 1000 - 500) / 1000.0f * variation_amount;
            keyframe->translation.z += (float)(rand() % 1000 - 500) / 1000.0f * variation_amount;
        }
    }
    
    return true;
}

void proc_anim_destroy_animator(ProceduralAnimator* animator) {
    if (!animator) return;
    
    if (animator->skeleton) {
        for (uint32_t i = 0; i < animator->skeleton->bone_count; i++) {
            free(animator->skeleton->bone_names[i]);
        }
        free(animator->skeleton->bone_names);
        free(animator->skeleton->bones);
        free(animator->skeleton->bone_parents);
        free(animator->skeleton->inverse_bind_matrices);
        free(animator->skeleton);
    }
    
    if (animator->clips) {
        for (uint32_t i = 0; i < animator->clip_count; i++) {
            for (uint32_t j = 0; j < animator->clips[i].channel_count; j++) {
                free(animator->clips[i].channels[j].keyframe_times);
                free(animator->clips[i].channels[j].keyframes);
            }
            free(animator->clips[i].channels);
        }
        free(animator->clips);
    }
    
    free(animator->states);
    free(animator->current_pose);
    free(animator);
}
