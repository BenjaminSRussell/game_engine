/*
 * temporal_reprojection.c
 * Temporal filtering for volumetric lighting
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#include "temporal_reprojection.h"
#include "../../math/vec3.h"
#include "../../math/vec2.h"
#include "../../math/mat4.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define LIGHTING_TEMPORAL_HISTORY_MAX 2
#define MAX_JITTER_SAMPLES 16

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lighting_temporal_context {
    uint32_t width;
    uint32_t height;
    uint32_t frame_index;
    
    // History buffers (texture IDs)
    uint32_t history_buffers[LIGHTING_TEMPORAL_HISTORY_MAX];
    uint32_t current_history_index;
    
    // Jitter sequence
    vec2_t jitter_samples[MAX_JITTER_SAMPLES];
    
    // Settings
    float blend_weight; // 0.95 usually
    bool enable_reprojection;
    
    bool initialized;
} lighting_temporal_context_t;

static lighting_temporal_context_t g_temporal_ctx = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

// Halton Sequence Generator (Base 2, 3)
static float halton(int index, int base) {
    float f = 1.0f;
    float r = 0.0f;
    while (index > 0) {
        f = f / (float)base;
        r = r + f * (float)(index % base);
        index = index / base;
    }
    return r;
}

static void generate_jitter_sequence(void) {
    for (int i = 0; i < MAX_JITTER_SAMPLES; i++) {
        // Offset by 1 to avoid 0,0 alignment if desired, widely used pattern
        g_temporal_ctx.jitter_samples[i].x = halton(i + 1, 2) - 0.5f;
        g_temporal_ctx.jitter_samples[i].y = halton(i + 1, 3) - 0.5f;
    }
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

int lighting_temporal_reprojection_init(uint32_t width, uint32_t height) {
    if (g_temporal_ctx.initialized) return 0;
    
    g_temporal_ctx.width = width;
    g_temporal_ctx.height = height;
    g_temporal_ctx.frame_index = 0;
    g_temporal_ctx.current_history_index = 0;
    g_temporal_ctx.blend_weight = 0.90f;
    g_temporal_ctx.enable_reprojection = true;
    
    generate_jitter_sequence();
    
    g_temporal_ctx.initialized = true;
    return 0;
}

void lighting_temporal_reprojection_shutdown(void) {
    g_temporal_ctx.initialized = false;
}

// Get jitter offset for current frame
vec2_t lighting_temporal_reprojection_get_jitter(void) {
    if (!g_temporal_ctx.initialized) return (vec2_t){0};
    int idx = g_temporal_ctx.frame_index % MAX_JITTER_SAMPLES;
    return g_temporal_ctx.jitter_samples[idx];
}

// Logic to blend current frame color with history
// returns blended color
// This is CPU simulation of the shader logic
vec3_t lighting_temporal_reprojection_resolve(
    vec3_t current_color, 
    vec3_t history_color, 
    vec2_t velocity_vector,
    bool history_valid
) {
    if (!g_temporal_ctx.enable_reprojection || !history_valid) {
        return current_color;
    }
    
    // Simple exponential moving average (EMA)
    // History weight depends on velocity validity usually (clamping/rectification)
    // Here using fixed weight for simplicity
    
    float alpha = g_temporal_ctx.blend_weight;
    
    // color = current * (1 - alpha) + history * alpha
    vec3_t result;
    result.x = current_color.x * (1.0f - alpha) + history_color.x * alpha;
    result.y = current_color.y * (1.0f - alpha) + history_color.y * alpha;
    result.z = current_color.z * (1.0f - alpha) + history_color.z * alpha;
    
    return result;
}

void lighting_temporal_reprojection_advance_frame(void) {
    if (!g_temporal_ctx.initialized) return;
    
    g_temporal_ctx.frame_index++;
    g_temporal_ctx.current_history_index = (g_temporal_ctx.current_history_index + 1) % LIGHTING_TEMPORAL_HISTORY_MAX;
}

void lighting_temporal_reprojection_set_history_buffer(uint32_t texture_id) {
    if (!g_temporal_ctx.initialized) return;
    g_temporal_ctx.history_buffers[g_temporal_ctx.current_history_index] = texture_id;
}

uint32_t lighting_temporal_reprojection_get_previous_history_buffer(void) {
    if (!g_temporal_ctx.initialized) return 0;
    int prev_idx = (g_temporal_ctx.current_history_index + LIGHTING_TEMPORAL_HISTORY_MAX - 1) % LIGHTING_TEMPORAL_HISTORY_MAX;
    return g_temporal_ctx.history_buffers[prev_idx];
}
