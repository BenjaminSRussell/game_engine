#include "animation_synthesis.h"
#include "../core/logger.h"
#include "../core/time.h"
#include "../math/math.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>

/**
 * =================================================================================================
 *                                   ANIMATION SYNTHESIS SYSTEM - IMPLEMENTATION
 * =================================================================================================
 */

// Internal helper functions
static void animation_synthesis_generate_keyframes(AnimationSynthesisContext* context, f32 duration);
static Vec3 animation_synthesis_calc_foot_trajectory(f32 phase, const WalkParameters* params);
static Quat animation_synthesis_calc_spine_twist(f32 phase, f32 arm_swing);
static void animation_synthesis_apply_personality(AnimationSynthesisContext* context, 
                                                const AIAnimationParams* ai);
static AnimationClip* animation_synthesis_create_clip(const char* name, u32 channel_count, f32 duration);

void animation_synthesis_init(AnimationSynthesisSystem* system) {
    if (!system) {
        LOG_ERROR("Animation synthesis system: NULL pointer");
        return;
    }
    
    memset(system, 0, sizeof(AnimationSynthesisSystem));
    
    // Initialize synthesis contexts
    for (u32 i = 0; i < ANIM_SYNTH_COUNT; i++) {
        system->contexts[i].type = (AnimationSynthType)i;
        system->contexts[i].phase = 0.0f;
        system->contexts[i].time_scale = 1.0f;
        system->contexts[i].blend_weight = 1.0f;
        system->contexts[i].active = false;
        system->contexts[i].generated_clip = NULL;
        system->contexts[i].last_frame_count = 0;
        system->contexts[i].layer_count = 0;
        
        // Initialize joint weights to 1.0
        for (u32 j = 0; j < ANIM_SYNTH_MAX_PROCEDURAL_JOINTS; j++) {
            system->contexts[i].joint_weights[j] = 1.0f;
        }
    }
    
    // Initialize default joint hierarchy
    system->joint_count = 16; // Basic humanoid skeleton
    strcpy(system->joints[0].name, "Hips");
    system->joints[0].parent_index = -1;
    system->joints[0].is_spine_joint = true;
    
    strcpy(system->joints[1].name, "Spine");
    system->joints[1].parent_index = 0;
    system->joints[1].is_spine_joint = true;
    
    strcpy(system->joints[2].name, "Chest");
    system->joints[2].parent_index = 1;
    system->joints[2].is_spine_joint = true;
    
    strcpy(system->joints[3].name, "Neck");
    system->joints[3].parent_index = 2;
    
    strcpy(system->joints[4].name, "Head");
    system->joints[4].parent_index = 3;
    
    strcpy(system->joints[5].name, "LeftShoulder");
    system->joints[5].parent_index = 2;
    
    strcpy(system->joints[6].name, "LeftArm");
    system->joints[6].parent_index = 5;
    system->joints[6].is_hand_joint = true;
    
    strcpy(system->joints[7].name, "LeftForeArm");
    system->joints[7].parent_index = 6;
    system->joints[7].is_hand_joint = true;
    
    strcpy(system->joints[8].name, "LeftHand");
    system->joints[8].parent_index = 7;
    system->joints[8].is_hand_joint = true;
    
    strcpy(system->joints[9].name, "RightShoulder");
    system->joints[9].parent_index = 2;
    
    strcpy(system->joints[10].name, "RightArm");
    system->joints[10].parent_index = 9;
    system->joints[10].is_hand_joint = true;
    
    strcpy(system->joints[11].name, "RightForeArm");
    system->joints[11].parent_index = 10;
    system->joints[11].is_hand_joint = true;
    
    strcpy(system->joints[12].name, "RightHand");
    system->joints[12].parent_index = 11;
    system->joints[12].is_hand_joint = true;
    
    strcpy(system->joints[13].name, "LeftUpLeg");
    system->joints[13].parent_index = 0;
    
    strcpy(system->joints[14].name, "LeftLeg");
    system->joints[14].parent_index = 13;
    system->joints[14].is_foot_joint = true;
    
    strcpy(system->joints[15].name, "LeftFoot");
    system->joints[15].parent_index = 14;
    system->joints[15].is_foot_joint = true;
    
    // Initialize cache
    for (u32 i = 0; i < ANIM_SYNTH_COUNT; i++) {
        system->cache.cached_clips[i] = NULL;
        system->cache.last_speed[i] = 0.0f;
        system->cache.cache_valid[i] = false;
    }
    
    // Initialize ML data
    system->ml_data.pattern_count = 0;
    system->ml_data.learning_rate = 0.01f;
    
    // Set performance settings
    system->update_rate = 30.0f; // 30 FPS update rate
    system->last_update_time = 0.0f;
    system->enable_ai_synthesis = true;
    system->enable_caching = true;
    
    LOG_INFO("Animation synthesis system initialized");
    LOG_INFO("  Supported synthesis types: %d", ANIM_SYNTH_COUNT);
    LOG_INFO("  Joint count: %d", system->joint_count);
    LOG_INFO("  AI synthesis: %s", system->enable_ai_synthesis ? "enabled" : "disabled");
}

void animation_synthesis_shutdown(AnimationSynthesisSystem* system) {
    if (!system) return;
    
    // Clean up generated clips
    for (u32 i = 0; i < ANIM_SYNTH_COUNT; i++) {
        if (system->contexts[i].generated_clip) {
            free(system->contexts[i].generated_clip->channels);
            free(system->contexts[i].generated_clip);
            system->contexts[i].generated_clip = NULL;
        }
    }
    
    // Clean up cache
    animation_synthesis_cache_clear(system);
    
    // Log statistics
    LOG_INFO("Animation synthesis system shutdown");
    LOG_INFO("  Animations generated: %u", system->animations_generated);
    LOG_INFO("  Cache hits: %u", system->cache_hits);
    LOG_INFO("  Cache misses: %u", system->cache_misses);
    LOG_INFO("  Cache hit ratio: %.2f%%", 
             system->cache_hits + system->cache_misses > 0 ? 
             (100.0f * system->cache_hits) / (system->cache_hits + system->cache_misses) : 0.0f);
    
    memset(system, 0, sizeof(AnimationSynthesisSystem));
}

void animation_synthesis_update(AnimationSynthesisSystem* system, f32 delta_time) {
    if (!system) return;
    
    system->last_update_time += delta_time;
    
    // Update at the specified rate
    if (system->last_update_time < (1.0f / system->update_rate)) {
        return;
    }
    
    f32 dt = system->last_update_time;
    system->last_update_time = 0.0f;
    
    // Update active synthesis contexts
    for (u32 i = 0; i < ANIM_SYNTH_COUNT; i++) {
        AnimationSynthesisContext* context = &system->contexts[i];
        if (!context->active) continue;
        
        // Update animation phase
        context->phase += dt * context->time_scale;
        if (context->phase >= 1.0f) {
            context->phase -= 1.0f; // Loop animation
        }
        
        // Regenerate animation if needed
        if (context->generated_clip && 
            (context->last_frame_count != ANIM_SYNTH_MAX_KEYFRAMES || 
             !system->enable_caching)) {
            animation_synthesis_generate_keyframes(context, context->generated_clip->duration);
        }
    }
}

AnimationSynthesisContext* animation_synthesis_get_context(AnimationSynthesisSystem* system, 
                                                         AnimationSynthType type) {
    if (!system || type >= ANIM_SYNTH_COUNT) return NULL;
    return &system->contexts[type];
}

void animation_synthesis_start(AnimationSynthesisSystem* system, AnimationSynthType type) {
    if (!system || type >= ANIM_SYNTH_COUNT) return;
    
    AnimationSynthesisContext* context = &system->contexts[type];
    context->active = true;
    context->phase = 0.0f;
    
    LOG_TRACE("Started animation synthesis type: %d", type);
}

void animation_synthesis_stop(AnimationSynthesisSystem* system, AnimationSynthType type) {
    if (!system || type >= ANIM_SYNTH_COUNT) return;
    
    AnimationSynthesisContext* context = &system->contexts[type];
    context->active = false;
    
    LOG_TRACE("Stopped animation synthesis type: %d", type);
}

void animation_synthesis_set_speed(AnimationSynthesisSystem* system, AnimationSynthType type, f32 speed) {
    if (!system || type >= ANIM_SYNTH_COUNT) return;
    
    AnimationSynthesisContext* context = &system->contexts[type];
    
    // Update parameters based on type
    switch (type) {
        case ANIM_SYNTH_WALK_CYCLE:
            context->params.walk = animation_synthesis_calc_walk_params(speed);
            break;
        case ANIM_SYNTH_RUN_CYCLE:
            context->params.run = animation_synthesis_calc_run_params(speed);
            break;
        default:
            break;
    }
    
    context->time_scale = 1.0f;
}

AnimationClip* animation_synthesis_generate(AnimationSynthesisSystem* system, 
                                          AnimationSynthType type, 
                                          f32 speed, 
                                          f32 duration) {
    if (!system || type >= ANIM_SYNTH_COUNT) return NULL;
    
    // Check cache first
    if (system->enable_caching) {
        AnimationClip* cached_clip = NULL;
        if (animation_synthesis_cache_lookup(system, type, speed, &cached_clip)) {
            return cached_clip;
        }
    }
    
    u64 start_time = time_get_current_ms();
    
    AnimationSynthesisContext* context = &system->contexts[type];
    
    // Set parameters based on type and speed
    switch (type) {
        case ANIM_SYNTH_WALK_CYCLE:
            context->params.walk = animation_synthesis_calc_walk_params(speed);
            return animation_synthesis_walk_cycle(system, &context->params.walk, duration);
            
        case ANIM_SYNTH_RUN_CYCLE:
            context->params.run = animation_synthesis_calc_run_params(speed);
            return animation_synthesis_run_cycle(system, &context->params.run, duration);
            
        case ANIM_SYNTH_IDLE_BREATH:
            context->params.idle = animation_synthesis_calc_idle_params(0.5f);
            return animation_synthesis_idle_breathing(system, &context->params.idle, duration);
            
        case ANIM_SYNTH_JUMP:
            context->params.jump = animation_synthesis_calc_jump_params(2.0f);
            return animation_synthesis_jump(system, &context->params.jump);
            
        default:
            LOG_ERROR("Unsupported animation synthesis type: %d", type);
            return NULL;
    }
}

AnimationClip* animation_synthesis_walk_cycle(AnimationSynthesisSystem* system, 
                                            const WalkParameters* params, 
                                            f32 duration) {
    if (!system || !params) return NULL;
    
    AnimationClip* clip = animation_synthesis_create_clip("ProceduralWalk", 16, duration);
    if (!clip) return NULL;
    
    // Generate keyframes for each joint
    for (u32 joint_idx = 0; joint_idx < system->joint_count; joint_idx++) {
        AnimationChannel* channel = &clip->channels[joint_idx];
        channel->bone_index = joint_idx;
        channel->keyframe_count = ANIM_SYNTH_MAX_KEYFRAMES;
        channel->keyframes = malloc(sizeof(AnimationKeyframe) * ANIM_SYNTH_MAX_KEYFRAMES);
        
        if (!channel->keyframes) {
            LOG_ERROR("Failed to allocate keyframes for walk cycle");
            free(clip->channels);
            free(clip);
            return NULL;
        }
        
        // Generate keyframes based on joint type
        for (u32 frame = 0; frame < ANIM_SYNTH_MAX_KEYFRAMES; frame++) {
            f32 time = (f32)frame / (ANIM_SYNTH_MAX_KEYFRAMES - 1) * duration;
            f32 phase = (f32)frame / (ANIM_SYNTH_MAX_KEYFRAMES - 1);
            
            AnimationKeyframe* keyframe = &channel->keyframes[frame];
            keyframe->time = time;
            keyframe->scale = vec3_one();
            
            if (system->joints[joint_idx].is_foot_joint) {
                // Foot movement - opposite phase for left/right
                f32 foot_phase = (joint_idx == 14 || joint_idx == 15) ? phase : phase + 0.5f;
                if (foot_phase >= 1.0f) foot_phase -= 1.0f;
                
                Vec3 foot_pos = animation_synthesis_calc_foot_trajectory(foot_phase, params);
                keyframe->position = foot_pos;
                keyframe->rotation = quat_identity();
                
            } else if (system->joints[joint_idx].is_hand_joint) {
                // Arm swing - opposite to feet
                f32 arm_phase = (joint_idx >= 6 && joint_idx <= 8) ? phase + 0.5f : phase;
                if (arm_phase >= 1.0f) arm_phase -= 1.0f;
                
                f32 swing_x = sinf(arm_phase * 2.0f * PI) * params->arm_swing;
                f32 swing_y = cosf(arm_phase * 2.0f * PI) * params->arm_swing * 0.3f;
                
                keyframe->position = vec3(swing_x, swing_y, 0.0f);
                keyframe->rotation = quat_from_axis_angle(vec3_up(), sinf(arm_phase * PI) * 0.2f);
                
            } else if (system->joints[joint_idx].is_spine_joint) {
                // Spine movement and lean
                f32 bob = sinf(phase * 2.0f * PI) * params->body_bob;
                f32 lean = params->body_lean;
                
                keyframe->position = vec3(0.0f, bob, 0.0f);
                keyframe->rotation = quat_from_axis_angle(vec3_right(), lean);
                
            } else {
                // Default: minimal movement
                keyframe->position = vec3_zero();
                keyframe->rotation = quat_identity();
            }
        }
    }
    
    // Cache the result
    if (system->enable_caching) {
        animation_synthesis_cache_store(system, ANIM_SYNTH_WALK_CYCLE, params->speed, clip);
    }
    
    system->animations_generated++;
    return clip;
}

AnimationClip* animation_synthesis_run_cycle(AnimationSynthesisSystem* system, 
                                           const RunParameters* params, 
                                           f32 duration) {
    if (!system || !params) return NULL;
    
    // Similar to walk cycle but with more extreme parameters
    AnimationClip* clip = animation_synthesis_create_clip("ProceduralRun", 16, duration);
    if (!clip) return NULL;
    
    // Generate keyframes with running-specific parameters
    for (u32 joint_idx = 0; joint_idx < system->joint_count; joint_idx++) {
        AnimationChannel* channel = &clip->channels[joint_idx];
        channel->bone_index = joint_idx;
        channel->keyframe_count = ANIM_SYNTH_MAX_KEYFRAMES;
        channel->keyframes = malloc(sizeof(AnimationKeyframe) * ANIM_SYNTH_MAX_KEYFRAMES);
        
        for (u32 frame = 0; frame < ANIM_SYNTH_MAX_KEYFRAMES; frame++) {
            f32 time = (f32)frame / (ANIM_SYNTH_MAX_KEYFRAMES - 1) * duration;
            f32 phase = (f32)frame / (ANIM_SYNTH_MAX_KEYFRAMES - 1);
            
            AnimationKeyframe* keyframe = &channel->keyframes[frame];
            keyframe->time = time;
            keyframe->scale = vec3_one();
            
            if (system->joints[joint_idx].is_foot_joint) {
                // More aggressive foot movement for running
                f32 foot_phase = (joint_idx == 14 || joint_idx == 15) ? phase : phase + 0.5f;
                if (foot_phase >= 1.0f) foot_phase -= 1.0f;
                
                Vec3 foot_pos = animation_synthesis_calc_foot_trajectory(foot_phase, 
                                                                         (WalkParameters*)params);
                foot_pos.y *= 1.5f; // Higher steps for running
                keyframe->position = foot_pos;
                keyframe->rotation = quat_identity();
                
            } else if (system->joints[joint_idx].is_hand_joint) {
                // More aggressive arm swing
                f32 arm_phase = (joint_idx >= 6 && joint_idx <= 8) ? phase + 0.5f : phase;
                if (arm_phase >= 1.0f) arm_phase -= 1.0f;
                
                f32 swing_x = sinf(arm_phase * 2.0f * PI) * params->arm_swing * 1.5f;
                f32 swing_y = cosf(arm_phase * 2.0f * PI) * params->arm_swing * 0.5f;
                
                keyframe->position = vec3(swing_x, swing_y, 0.0f);
                keyframe->rotation = quat_from_axis_angle(vec3_up(), sinf(arm_phase * PI) * 0.4f);
                
            } else if (system->joints[joint_idx].is_spine_joint) {
                // More pronounced body movement
                f32 bob = sinf(phase * 2.0f * PI) * params->body_bob * 1.5f;
                f32 lean = params->body_lean * 1.2f;
                
                keyframe->position = vec3(0.0f, bob, 0.0f);
                keyframe->rotation = quat_from_axis_angle(vec3_right(), lean);
                
            } else {
                keyframe->position = vec3_zero();
                keyframe->rotation = quat_identity();
            }
        }
    }
    
    system->animations_generated++;
    return clip;
}

AnimationClip* animation_synthesis_idle_breathing(AnimationSynthesisSystem* system, 
                                                 const IdleParameters* params, 
                                                 f32 duration) {
    if (!system || !params) return NULL;
    
    AnimationClip* clip = animation_synthesis_create_clip("ProceduralIdle", 16, duration);
    if (!clip) return NULL;
    
    // Generate subtle breathing animation
    for (u32 joint_idx = 0; joint_idx < system->joint_count; joint_idx++) {
        AnimationChannel* channel = &clip->channels[joint_idx];
        channel->bone_index = joint_idx;
        channel->keyframe_count = ANIM_SYNTH_MAX_KEYFRAMES;
        channel->keyframes = malloc(sizeof(AnimationKeyframe) * ANIM_SYNTH_MAX_KEYFRAMES);
        
        for (u32 frame = 0; frame < ANIM_SYNTH_MAX_KEYFRAMES; frame++) {
            f32 time = (f32)frame / (ANIM_SYNTH_MAX_KEYFRAMES - 1) * duration;
            f32 phase = (f32)frame / (ANIM_SYNTH_MAX_KEYFRAMES - 1);
            
            AnimationKeyframe* keyframe = &channel->keyframes[frame];
            keyframe->time = time;
            keyframe->scale = vec3_one();
            
            if (system->joints[joint_idx].is_spine_joint || joint_idx == 2) { // Chest
                // Chest expansion for breathing
                f32 breath = sinf(phase * 2.0f * PI) * params->chest_expansion;
                keyframe->position = vec3(breath * 0.1f, 0.0f, 0.0f);
                keyframe->rotation = quat_identity();
                
            } else if (joint_idx == 4) { // Head
                // Subtle head movement
                f32 head_tilt = sinf(phase * 2.0f * PI) * params->head_tilt;
                keyframe->position = vec3_zero();
                keyframe->rotation = quat_from_axis_angle(vec3_forward(), head_tilt);
                
            } else {
                // Minimal movement for other joints
                f32 micro = params->micro_movements ? 
                           (sinf(phase * 7.3f) * 0.01f + cosf(phase * 11.7f) * 0.01f) : 0.0f;
                keyframe->position = vec3(micro, micro, micro);
                keyframe->rotation = quat_identity();
            }
        }
    }
    
    system->animations_generated++;
    return clip;
}

AnimationClip* animation_synthesis_jump(AnimationSynthesisSystem* system, 
                                       const JumpParameters* params) {
    if (!system || !params) return NULL;
    
    f32 duration = params->jump_duration;
    AnimationClip* clip = animation_synthesis_create_clip("ProceduralJump", 16, duration);
    if (!clip) return NULL;
    
    // Generate jump animation keyframes
    for (u32 joint_idx = 0; joint_idx < system->joint_count; joint_idx++) {
        AnimationChannel* channel = &clip->channels[joint_idx];
        channel->bone_index = joint_idx;
        channel->keyframe_count = ANIM_SYNTH_MAX_KEYFRAMES;
        channel->keyframes = malloc(sizeof(AnimationKeyframe) * ANIM_SYNTH_MAX_KEYFRAMES);
        
        for (u32 frame = 0; frame < ANIM_SYNTH_MAX_KEYFRAMES; frame++) {
            f32 time = (f32)frame / (ANIM_SYNTH_MAX_KEYFRAMES - 1) * duration;
            f32 phase = (f32)frame / (ANIM_SYNTH_MAX_KEYFRAMES - 1);
            
            AnimationKeyframe* keyframe = &channel->keyframes[frame];
            keyframe->time = time;
            keyframe->scale = vec3_one();
            
            if (system->joints[joint_idx].is_foot_joint) {
                // Foot movement: crouch -> extend -> land
                Vec3 foot_pos = vec3_zero();
                if (phase < 0.2f) {
                    // Crouch phase
                    foot_pos.y = -params->crouch_depth * (1.0f - phase / 0.2f);
                } else if (phase < 0.8f) {
                    // In air phase
                    foot_pos.y = params->jump_height * sinf((phase - 0.2f) / 0.6f * PI);
                } else {
                    // Landing phase
                    foot_pos.y = -params->crouch_depth * 0.5f * ((phase - 0.8f) / 0.2f);
                }
                keyframe->position = foot_pos;
                keyframe->rotation = quat_identity();
                
            } else if (system->joints[joint_idx].is_hand_joint) {
                // Arm swing for balance
                f32 arm_swing = sinf(phase * PI) * params->arm_swing;
                keyframe->position = vec3(0.0f, arm_swing, arm_swing * 0.5f);
                keyframe->rotation = quat_from_axis_angle(vec3_forward(), arm_swing * 0.3f);
                
            } else if (system->joints[joint_idx].is_spine_joint) {
                // Spine compression and extension
                f32 spine_compress = 0.0f;
                if (phase < 0.2f) {
                    spine_compress = -params->crouch_depth * 0.3f * (1.0f - phase / 0.2f);
                } else if (phase > 0.8f) {
                    spine_compress = -params->crouch_depth * 0.2f * ((phase - 0.8f) / 0.2f);
                }
                keyframe->position = vec3(0.0f, spine_compress, 0.0f);
                keyframe->rotation = quat_identity();
                
            } else {
                keyframe->position = vec3_zero();
                keyframe->rotation = quat_identity();
            }
        }
    }
    
    system->animations_generated++;
    return clip;
}

// Parameter calculation helpers
WalkParameters animation_synthesis_calc_walk_params(f32 speed) {
    WalkParameters params;
    params.speed = speed;
    params.stride_length = speed * 0.8f; // Stride length proportional to speed
    params.step_height = 0.15f;           // Fixed step height
    params.step_frequency = speed / params.stride_length;
    params.body_bob = 0.05f;
    params.body_lean = speed * 0.02f;     // More lean at higher speeds
    params.arm_swing = speed * 0.3f;
    params.head_bob = 0.02f;
    params.foot_planting = true;
    params.spine_curvature = true;
    return params;
}

RunParameters animation_synthesis_calc_run_params(f32 speed) {
    RunParameters params;
    params.speed = speed;
    params.stride_length = speed * 1.2f;   // Longer strides for running
    params.step_height = 0.25f;           // Higher steps
    params.step_frequency = speed / params.stride_length;
    params.body_bob = 0.08f;
    params.body_lean = speed * 0.05f;     // More aggressive lean
    params.arm_swing = speed * 0.5f;
    params.impact_force = speed * 0.1f;
    params.foot_planting = true;
    params.spine_curvature = true;
    return params;
}

IdleParameters animation_synthesis_calc_idle_params(f32 energy_level) {
    IdleParameters params;
    params.breath_rate = 12.0f + energy_level * 8.0f; // 12-20 breaths per minute
    params.chest_expansion = 0.02f + energy_level * 0.03f;
    params.shoulder_raise = 0.01f + energy_level * 0.02f;
    params.head_tilt = 0.05f + energy_level * 0.05f;
    params.weight_shift = 0.03f + energy_level * 0.07f;
    params.micro_movements = energy_level > 0.5f;
    return params;
}

JumpParameters animation_synthesis_calc_jump_params(f32 jump_height) {
    JumpParameters params;
    params.jump_height = jump_height;
    params.jump_duration = 1.0f + jump_height * 0.2f;
    params.takeoff_force = jump_height * 2.0f;
    params.crouch_depth = 0.3f;
    params.arm_swing = 0.4f;
    params.leg_extension = true;
    return params;
}

// Internal helper functions
static void animation_synthesis_generate_keyframes(AnimationSynthesisContext* context, f32 duration) {
    // This would regenerate keyframes for a context
    // Implementation depends on the specific synthesis type
    context->last_frame_count = ANIM_SYNTH_MAX_KEYFRAMES;
}

static Vec3 animation_synthesis_calc_foot_trajectory(f32 phase, const WalkParameters* params) {
    Vec3 foot_pos;
    
    // Forward movement
    foot_pos.x = sinf(phase * 2.0f * PI) * params->stride_length * 0.5f;
    
    // Vertical lift (foot arc)
    f32 lift_phase = fmodf(phase * 2.0f * PI, PI);
    if (lift_phase < PI) {
        foot_pos.y = sinf(lift_phase) * params->step_height;
    } else {
        foot_pos.y = 0.0f; // Foot on ground
    }
    
    foot_pos.z = 0.0f;
    return foot_pos;
}

static AnimationClip* animation_synthesis_create_clip(const char* name, u32 channel_count, f32 duration) {
    AnimationClip* clip = malloc(sizeof(AnimationClip));
    if (!clip) return NULL;
    
    memset(clip, 0, sizeof(AnimationClip));
    strncpy(clip->name, name, sizeof(clip->name) - 1);
    clip->duration = duration;
    clip->ticks_per_second = 30.0f;
    clip->channel_count = channel_count;
    clip->channels = malloc(sizeof(AnimationChannel) * channel_count);
    
    if (!clip->channels) {
        free(clip);
        return NULL;
    }
    
    memset(clip->channels, 0, sizeof(AnimationChannel) * channel_count);
    return clip;
}

// Cache management
void animation_synthesis_cache_clear(AnimationSynthesisSystem* system) {
    if (!system) return;
    
    for (u32 i = 0; i < ANIM_SYNTH_COUNT; i++) {
        if (system->cache.cached_clips[i]) {
            free(system->cache.cached_clips[i]->channels);
            free(system->cache.cached_clips[i]);
            system->cache.cached_clips[i] = NULL;
        }
        system->cache.cache_valid[i] = false;
    }
}

bool animation_synthesis_cache_lookup(AnimationSynthesisSystem* system, 
                                     AnimationSynthType type, 
                                     f32 speed, 
                                     AnimationClip** result) {
    if (!system || !result || type >= ANIM_SYNTH_COUNT) return false;
    
    if (system->cache.cache_valid[type] && 
        fabsf(system->cache.last_speed[type] - speed) < 0.1f) {
        *result = system->cache.cached_clips[type];
        system->cache_hits++;
        return true;
    }
    
    system->cache_misses++;
    return false;
}

void animation_synthesis_cache_store(AnimationSynthesisSystem* system, 
                                   AnimationSynthType type, 
                                   f32 speed, 
                                   AnimationClip* clip) {
    if (!system || !clip || type >= ANIM_SYNTH_COUNT) return;
    
    // Clean up existing cached clip
    if (system->cache.cached_clips[type]) {
        free(system->cache.cached_clips[type]->channels);
        free(system->cache.cached_clips[type]);
    }
    
    system->cache.cached_clips[type] = clip;
    system->cache.last_speed[type] = speed;
    system->cache.cache_valid[type] = true;
}
