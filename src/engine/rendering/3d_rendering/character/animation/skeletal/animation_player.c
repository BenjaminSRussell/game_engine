/*
 * animation_player.c
 * Animation playback and sampling
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 *
 * Implements keyframe sampling, interpolation, and playback controls
 */

#include "animation_player.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define ANIMATION_PLAYER_MAX_COUNT 4096
#define ANIMATION_PLAYER_DEFAULT_CAPACITY 256
#define ANIMATION_MAX_KEYFRAMES 16384
#define ANIMATION_MAX_CHANNELS 256

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct vec3 {
    float x, y, z;
} vec3_t;

typedef struct quat {
    float x, y, z, w;
} quat_t;

typedef enum interpolation_mode {
    INTERP_STEP = 0,        // No interpolation, use nearest keyframe
    INTERP_LINEAR,          // Linear interpolation
    INTERP_CUBIC_SPLINE     // Cubic spline interpolation
} interpolation_mode_t;

typedef enum loop_mode {
    LOOP_ONCE = 0,          // Play once and stop
    LOOP_REPEAT,            // Loop from start
    LOOP_PING_PONG          // Alternate forward/backward
} loop_mode_t;

typedef enum playback_state {
    PLAYBACK_STOPPED = 0,
    PLAYBACK_PLAYING,
    PLAYBACK_PAUSED
} playback_state_t;

typedef enum channel_target {
    TARGET_TRANSLATION = 0,
    TARGET_ROTATION,
    TARGET_SCALE,
    TARGET_WEIGHTS         // Morph target weights
} channel_target_t;

typedef struct keyframe {
    float time;
    union {
        vec3_t vec3;        // For translation/scale
        quat_t quat;        // For rotation
        float weight;       // For morph weights
    } value;
    // For cubic spline
    union {
        vec3_t vec3;
        quat_t quat;
        float weight;
    } in_tangent;
    union {
        vec3_t vec3;
        quat_t quat;
        float weight;
    } out_tangent;
} keyframe_t;

typedef struct animation_channel {
    uint32_t target_bone;       // Bone index this channel affects
    channel_target_t target;    // What property to animate
    interpolation_mode_t interp;
    
    keyframe_t* keyframes;
    uint32_t keyframe_count;
    
    // Cached for fast lookup
    uint32_t last_keyframe_index;
} animation_channel_t;

typedef struct animation_clip {
    char name[64];
    float duration;
    float ticks_per_second;
    
    animation_channel_t* channels;
    uint32_t channel_count;
} animation_clip_t;

typedef struct animation_event {
    float time;
    uint32_t event_id;
    void* user_data;
} animation_event_t;

typedef void (*animation_event_callback_t)(uint32_t event_id, void* user_data);

typedef struct animation_animation_player_internal {
    uint32_t id;
    uint32_t flags;
    
    // Current clip
    animation_clip_t* current_clip;
    
    // Playback state
    playback_state_t state;
    loop_mode_t loop_mode;
    float current_time;
    float playback_speed;
    bool reverse;          // For ping-pong
    
    // Output pose (sampled each frame)
    vec3_t* output_translations;
    quat_t* output_rotations;
    vec3_t* output_scales;
    uint32_t output_bone_count;
    
    // Events
    animation_event_t* events;
    uint32_t event_count;
    animation_event_callback_t event_callback;
    uint32_t last_event_index;
    
    bool initialized;
    bool dirty;
    uint64_t frame_updated;
} animation_animation_player_internal_t;

typedef struct animation_animation_player_context {
    animation_animation_player_internal_t* items;
    uint32_t count;
    uint32_t capacity;
    void* allocator;
    bool initialized;
} animation_animation_player_context_t;

static animation_animation_player_context_t g_animation_player_ctx = {0};

/* ============================================================================
 * MATH HELPERS
 * ============================================================================ */

static float clampf(float v, float min, float max) {
    if (v < min) return min;
    if (v > max) return max;
    return v;
}

static float lerpf(float a, float b, float t) {
    return a + (b - a) * t;
}

static vec3_t vec3_lerp(const vec3_t* a, const vec3_t* b, float t) {
    vec3_t result;
    result.x = lerpf(a->x, b->x, t);
    result.y = lerpf(a->y, b->y, t);
    result.z = lerpf(a->z, b->z, t);
    return result;
}

static float quat_dot(const quat_t* a, const quat_t* b) {
    return a->x * b->x + a->y * b->y + a->z * b->z + a->w * b->w;
}

static quat_t quat_slerp(const quat_t* a, const quat_t* b, float t) {
    quat_t result;
    quat_t b_adj = *b;
    
    // Take shortest path
    float dot = quat_dot(a, b);
    if (dot < 0.0f) {
        b_adj.x = -b_adj.x;
        b_adj.y = -b_adj.y;
        b_adj.z = -b_adj.z;
        b_adj.w = -b_adj.w;
        dot = -dot;
    }
    
    // If very close, use linear interpolation
    if (dot > 0.9995f) {
        result.x = lerpf(a->x, b_adj.x, t);
        result.y = lerpf(a->y, b_adj.y, t);
        result.z = lerpf(a->z, b_adj.z, t);
        result.w = lerpf(a->w, b_adj.w, t);
    } else {
        // SLERP
        float theta_0 = acosf(dot);
        float theta = theta_0 * t;
        float sin_theta = sinf(theta);
        float sin_theta_0 = sinf(theta_0);
        
        float s0 = cosf(theta) - dot * sin_theta / sin_theta_0;
        float s1 = sin_theta / sin_theta_0;
        
        result.x = a->x * s0 + b_adj.x * s1;
        result.y = a->y * s0 + b_adj.y * s1;
        result.z = a->z * s0 + b_adj.z * s1;
        result.w = a->w * s0 + b_adj.w * s1;
    }
    
    // Normalize
    float len = sqrtf(result.x * result.x + result.y * result.y + 
                      result.z * result.z + result.w * result.w);
    if (len > 0.0001f) {
        result.x /= len;
        result.y /= len;
        result.z /= len;
        result.w /= len;
    }
    
    return result;
}

// Forward declarations for math functions
static float sqrtf(float x);
static float sinf(float x);
static float cosf(float x);
static float acosf(float x);

static float sqrtf(float x) {
    if (x <= 0) return 0;
    float guess = x / 2.0f;
    for (int i = 0; i < 10; i++) {
        guess = (guess + x / guess) / 2.0f;
    }
    return guess;
}

static float sinf(float x) {
    // Taylor series approximation
    float x2 = x * x;
    return x * (1.0f - x2 / 6.0f * (1.0f - x2 / 20.0f * (1.0f - x2 / 42.0f)));
}

static float cosf(float x) {
    float x2 = x * x;
    return 1.0f - x2 / 2.0f * (1.0f - x2 / 12.0f * (1.0f - x2 / 30.0f));
}

static float acosf(float x) {
    // Approximation for acos
    x = clampf(x, -1.0f, 1.0f);
    return 1.5707963f - x * (1.0f + x * x * (-0.141514f + x * x * 0.106478f));
}

/* ============================================================================
 * SAMPLING FUNCTIONS
 * ============================================================================ */

static uint32_t find_keyframe(const animation_channel_t* channel, float time) {
    // Binary search for keyframe
    if (channel->keyframe_count == 0) return 0;
    if (time <= channel->keyframes[0].time) return 0;
    if (time >= channel->keyframes[channel->keyframe_count - 1].time) {
        return channel->keyframe_count - 1;
    }
    
    // Check cached keyframe first (temporal coherence)
    uint32_t cached = channel->last_keyframe_index;
    if (cached < channel->keyframe_count - 1) {
        if (time >= channel->keyframes[cached].time && 
            time < channel->keyframes[cached + 1].time) {
            return cached;
        }
    }
    
    // Binary search
    uint32_t left = 0, right = channel->keyframe_count - 1;
    while (left < right) {
        uint32_t mid = (left + right) / 2;
        if (channel->keyframes[mid].time < time) {
            left = mid + 1;
        } else {
            right = mid;
        }
    }
    
    return (left > 0) ? left - 1 : 0;
}

static void sample_channel(animation_channel_t* channel, float time,
                           vec3_t* out_vec3, quat_t* out_quat) {
    if (channel->keyframe_count == 0) return;
    
    uint32_t k0 = find_keyframe(channel, time);
    channel->last_keyframe_index = k0;  // Cache for next frame
    
    keyframe_t* frame0 = &channel->keyframes[k0];
    
    // Single keyframe or at end
    if (k0 >= channel->keyframe_count - 1 || channel->interp == INTERP_STEP) {
        if (channel->target == TARGET_ROTATION && out_quat) {
            *out_quat = frame0->value.quat;
        } else if (out_vec3) {
            *out_vec3 = frame0->value.vec3;
        }
        return;
    }
    
    keyframe_t* frame1 = &channel->keyframes[k0 + 1];
    float t = (time - frame0->time) / (frame1->time - frame0->time);
    t = clampf(t, 0.0f, 1.0f);
    
    if (channel->interp == INTERP_LINEAR) {
        if (channel->target == TARGET_ROTATION && out_quat) {
            *out_quat = quat_slerp(&frame0->value.quat, &frame1->value.quat, t);
        } else if (out_vec3) {
            *out_vec3 = vec3_lerp(&frame0->value.vec3, &frame1->value.vec3, t);
        }
    } else if (channel->interp == INTERP_CUBIC_SPLINE && out_vec3) {
        // Hermite spline interpolation
        float t2 = t * t;
        float t3 = t2 * t;
        float h00 = 2*t3 - 3*t2 + 1;
        float h10 = t3 - 2*t2 + t;
        float h01 = -2*t3 + 3*t2;
        float h11 = t3 - t2;
        float dt = frame1->time - frame0->time;
        
        out_vec3->x = h00 * frame0->value.vec3.x + h10 * dt * frame0->out_tangent.vec3.x +
                      h01 * frame1->value.vec3.x + h11 * dt * frame1->in_tangent.vec3.x;
        out_vec3->y = h00 * frame0->value.vec3.y + h10 * dt * frame0->out_tangent.vec3.y +
                      h01 * frame1->value.vec3.y + h11 * dt * frame1->in_tangent.vec3.y;
        out_vec3->z = h00 * frame0->value.vec3.z + h10 * dt * frame0->out_tangent.vec3.z +
                      h01 * frame1->value.vec3.z + h11 * dt * frame1->in_tangent.vec3.z;
    }
}

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void cleanup_animation_clip(animation_clip_t* clip) {
    if (!clip) return;
    
    if (clip->channels) {
        for (uint32_t i = 0; i < clip->channel_count; i++) {
            if (clip->channels[i].keyframes) {
                free(clip->channels[i].keyframes);
            }
        }
        free(clip->channels);
        clip->channels = NULL;
    }
    clip->channel_count = 0;
}

static void animation_animation_player_cleanup_internal(animation_animation_player_internal_t* item) {
    if (!item) return;
    
    if (item->current_clip) {
        cleanup_animation_clip(item->current_clip);
        free(item->current_clip);
        item->current_clip = NULL;
    }
    
    if (item->output_translations) {
        free(item->output_translations);
        item->output_translations = NULL;
    }
    if (item->output_rotations) {
        free(item->output_rotations);
        item->output_rotations = NULL;
    }
    if (item->output_scales) {
        free(item->output_scales);
        item->output_scales = NULL;
    }
    if (item->events) {
        free(item->events);
        item->events = NULL;
    }
    
    item->initialized = false;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int animation_animation_player_init(void) {
    if (g_animation_player_ctx.initialized) {
        return 0;
    }

    g_animation_player_ctx.capacity = ANIMATION_PLAYER_DEFAULT_CAPACITY;
    g_animation_player_ctx.items = calloc(g_animation_player_ctx.capacity, 
                                           sizeof(animation_animation_player_internal_t));
    if (!g_animation_player_ctx.items) {
        return -1;
    }

    g_animation_player_ctx.count = 0;
    g_animation_player_ctx.initialized = true;

    return 0;
}

void animation_animation_player_shutdown(void) {
    if (!g_animation_player_ctx.initialized) {
        return;
    }

    for (uint32_t i = 0; i < g_animation_player_ctx.count; i++) {
        animation_animation_player_cleanup_internal(&g_animation_player_ctx.items[i]);
    }

    free(g_animation_player_ctx.items);
    g_animation_player_ctx.items = NULL;
    g_animation_player_ctx.count = 0;
    g_animation_player_ctx.capacity = 0;
    g_animation_player_ctx.initialized = false;
}

int animation_animation_player_create(animation_animation_player_handle_t* out_handle, 
                                        const animation_animation_player_desc_t* desc) {
    if (!out_handle || !desc) {
        return -1;
    }

    if (!g_animation_player_ctx.initialized) {
        return -2;
    }

    if (g_animation_player_ctx.count >= g_animation_player_ctx.capacity) {
        return -3;
    }

    uint32_t index = g_animation_player_ctx.count++;
    animation_animation_player_internal_t* item = &g_animation_player_ctx.items[index];

    item->id = index;
    item->flags = desc->flags;
    item->current_clip = NULL;
    item->state = PLAYBACK_STOPPED;
    item->loop_mode = LOOP_ONCE;
    item->current_time = 0.0f;
    item->playback_speed = 1.0f;
    item->reverse = false;
    item->output_translations = NULL;
    item->output_rotations = NULL;
    item->output_scales = NULL;
    item->output_bone_count = 0;
    item->events = NULL;
    item->event_count = 0;
    item->event_callback = NULL;
    item->last_event_index = 0;
    item->initialized = true;
    item->dirty = true;
    item->frame_updated = 0;

    out_handle->id = index;
    return 0;
}

void animation_animation_player_destroy(animation_animation_player_handle_t handle) {
    if (handle.id >= g_animation_player_ctx.count) {
        return;
    }

    animation_animation_player_cleanup_internal(&g_animation_player_ctx.items[handle.id]);
}

int animation_animation_player_setup(animation_animation_player_handle_t handle,
                                       uint32_t bone_count) {
    if (handle.id >= g_animation_player_ctx.count) {
        return -1;
    }
    
    animation_animation_player_internal_t* item = &g_animation_player_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }
    
    // Allocate output pose buffers
    item->output_translations = calloc(bone_count, sizeof(vec3_t));
    item->output_rotations = calloc(bone_count, sizeof(quat_t));
    item->output_scales = calloc(bone_count, sizeof(vec3_t));
    
    if (!item->output_translations || !item->output_rotations || !item->output_scales) {
        return -3;
    }
    
    // Initialize to identity pose
    for (uint32_t i = 0; i < bone_count; i++) {
        item->output_translations[i] = (vec3_t){0, 0, 0};
        item->output_rotations[i] = (quat_t){0, 0, 0, 1};
        item->output_scales[i] = (vec3_t){1, 1, 1};
    }
    
    item->output_bone_count = bone_count;
    return 0;
}

int animation_animation_player_play(animation_animation_player_handle_t handle) {
    if (handle.id >= g_animation_player_ctx.count) {
        return -1;
    }
    
    animation_animation_player_internal_t* item = &g_animation_player_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }
    
    item->state = PLAYBACK_PLAYING;
    return 0;
}

int animation_animation_player_pause(animation_animation_player_handle_t handle) {
    if (handle.id >= g_animation_player_ctx.count) {
        return -1;
    }
    
    g_animation_player_ctx.items[handle.id].state = PLAYBACK_PAUSED;
    return 0;
}

int animation_animation_player_stop(animation_animation_player_handle_t handle) {
    if (handle.id >= g_animation_player_ctx.count) {
        return -1;
    }
    
    animation_animation_player_internal_t* item = &g_animation_player_ctx.items[handle.id];
    item->state = PLAYBACK_STOPPED;
    item->current_time = 0.0f;
    item->reverse = false;
    return 0;
}

int animation_animation_player_set_time(animation_animation_player_handle_t handle, float time) {
    if (handle.id >= g_animation_player_ctx.count) {
        return -1;
    }
    
    animation_animation_player_internal_t* item = &g_animation_player_ctx.items[handle.id];
    item->current_time = time;
    item->dirty = true;
    return 0;
}

int animation_animation_player_set_speed(animation_animation_player_handle_t handle, float speed) {
    if (handle.id >= g_animation_player_ctx.count) {
        return -1;
    }
    
    g_animation_player_ctx.items[handle.id].playback_speed = speed;
    return 0;
}

int animation_animation_player_set_loop_mode(animation_animation_player_handle_t handle, 
                                               loop_mode_t mode) {
    if (handle.id >= g_animation_player_ctx.count) {
        return -1;
    }
    
    g_animation_player_ctx.items[handle.id].loop_mode = mode;
    return 0;
}

int animation_animation_player_advance(animation_animation_player_handle_t handle, float delta_time) {
    if (handle.id >= g_animation_player_ctx.count) {
        return -1;
    }
    
    animation_animation_player_internal_t* item = &g_animation_player_ctx.items[handle.id];
    if (!item->initialized || item->state != PLAYBACK_PLAYING) {
        return 0;
    }
    
    if (!item->current_clip || item->current_clip->duration <= 0.0f) {
        return 0;
    }
    
    float duration = item->current_clip->duration;
    float speed = item->playback_speed * (item->reverse ? -1.0f : 1.0f);
    
    item->current_time += delta_time * speed;
    
    // Handle looping
    if (item->loop_mode == LOOP_ONCE) {
        if (item->current_time >= duration) {
            item->current_time = duration;
            item->state = PLAYBACK_STOPPED;
        } else if (item->current_time < 0.0f) {
            item->current_time = 0.0f;
            item->state = PLAYBACK_STOPPED;
        }
    } else if (item->loop_mode == LOOP_REPEAT) {
        while (item->current_time >= duration) {
            item->current_time -= duration;
        }
        while (item->current_time < 0.0f) {
            item->current_time += duration;
        }
    } else if (item->loop_mode == LOOP_PING_PONG) {
        if (item->current_time >= duration) {
            item->current_time = duration - (item->current_time - duration);
            item->reverse = !item->reverse;
        } else if (item->current_time < 0.0f) {
            item->current_time = -item->current_time;
            item->reverse = !item->reverse;
        }
    }
    
    item->dirty = true;
    return 0;
}

int animation_animation_player_sample(animation_animation_player_handle_t handle) {
    if (handle.id >= g_animation_player_ctx.count) {
        return -1;
    }
    
    animation_animation_player_internal_t* item = &g_animation_player_ctx.items[handle.id];
    if (!item->initialized || !item->current_clip) {
        return -2;
    }
    
    animation_clip_t* clip = item->current_clip;
    float time = item->current_time;
    
    // Sample each channel
    for (uint32_t c = 0; c < clip->channel_count; c++) {
        animation_channel_t* channel = &clip->channels[c];
        uint32_t bone = channel->target_bone;
        
        if (bone >= item->output_bone_count) continue;
        
        switch (channel->target) {
            case TARGET_TRANSLATION:
                sample_channel(channel, time, &item->output_translations[bone], NULL);
                break;
            case TARGET_ROTATION:
                sample_channel(channel, time, NULL, &item->output_rotations[bone]);
                break;
            case TARGET_SCALE:
                sample_channel(channel, time, &item->output_scales[bone], NULL);
                break;
            default:
                break;
        }
    }
    
    item->dirty = false;
    item->frame_updated++;
    return 0;
}

int animation_animation_player_update(animation_animation_player_handle_t handle, 
                                        const void* data, size_t size) {
    if (handle.id >= g_animation_player_ctx.count) {
        return -1;
    }

    animation_animation_player_internal_t* item = &g_animation_player_ctx.items[handle.id];
    if (!item->initialized) {
        return -2;
    }

    item->dirty = true;
    return 0;
}

bool animation_animation_player_is_valid(animation_animation_player_handle_t handle) {
    if (handle.id >= g_animation_player_ctx.count) {
        return false;
    }
    return g_animation_player_ctx.items[handle.id].initialized;
}

int animation_animation_player_get_info(animation_animation_player_handle_t handle, 
                                          animation_animation_player_info_t* out_info) {
    if (!out_info) {
        return -1;
    }

    if (handle.id >= g_animation_player_ctx.count) {
        return -2;
    }

    const animation_animation_player_internal_t* item = &g_animation_player_ctx.items[handle.id];
    out_info->id = item->id;
    out_info->flags = item->flags;
    out_info->initialized = item->initialized;

    return 0;
}

void animation_animation_player_mark_dirty(animation_animation_player_handle_t handle) {
    if (handle.id < g_animation_player_ctx.count) {
        g_animation_player_ctx.items[handle.id].dirty = true;
    }
}

int animation_animation_player_process_pending(void) {
    int processed = 0;
    for (uint32_t i = 0; i < g_animation_player_ctx.count; i++) {
        animation_animation_player_internal_t* item = &g_animation_player_ctx.items[i];
        if (item->initialized && item->dirty) {
            animation_animation_player_handle_t handle = {.id = i};
            animation_animation_player_sample(handle);
            processed++;
        }
    }

    return processed;
}

uint32_t animation_animation_player_get_count(void) {
    return g_animation_player_ctx.count;
}

size_t animation_animation_player_get_memory_usage(void) {
    size_t total = sizeof(g_animation_player_ctx);
    total += g_animation_player_ctx.capacity * sizeof(animation_animation_player_internal_t);

    for (uint32_t i = 0; i < g_animation_player_ctx.count; i++) {
        animation_animation_player_internal_t* item = &g_animation_player_ctx.items[i];
        if (item->output_translations) {
            total += item->output_bone_count * sizeof(vec3_t);
        }
        if (item->output_rotations) {
            total += item->output_bone_count * sizeof(quat_t);
        }
        if (item->output_scales) {
            total += item->output_bone_count * sizeof(vec3_t);
        }
    }

    return total;
}

void animation_animation_player_debug_print(void) {
    // Debug output
}

/* End of animation_player.c */
