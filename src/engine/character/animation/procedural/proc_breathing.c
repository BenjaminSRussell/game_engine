#include "animation/animation_types.h"
#include "core/core.h"
#include <include/math/math.h>

typedef struct {
    float current_breath;    // 0..1 cycle position
    float rate_hz;          // Breaths per second
    float amplitude;        // Intensity
    uint32_t affect_mask;   // Which bones to affect
    float exertion_level;   // 0-1, linked to stamina
} BreathingState;

static BreathingState g_breathing = {
    .current_breath = 0.0f,
    .rate_hz = 0.25f,  // 15 breaths/min at rest
    .amplitude = 1.0f,
    .exertion_level = 0.0f
};

void proc_breathing_init() {
    g_breathing.current_breath = 0.0f;
}

void proc_breathing_update(float dt, float stamina_normalized) {
    // Link to exertion
    g_breathing.exertion_level = 1.0f - stamina_normalized;
    g_breathing.rate_hz = 0.25f + g_breathing.exertion_level * 0.5f; // Up to 45 bpm when exhausted
    g_breathing.amplitude = 0.5f + g_breathing.exertion_level * 0.5f;
    
    // Advance breathing cycle
    g_breathing.current_breath += dt * g_breathing.rate_hz;
    if (g_breathing.current_breath >= 1.0f) g_breathing.current_breath -= 1.0f;
}

void proc_breathing_apply_to_bones(void *skeleton) {
    // Sine wave with noise
    float breath_val = sinf(g_breathing.current_breath * 6.28318f) * g_breathing.amplitude;
    float noise = ((float)rand() / RAND_MAX - 0.5f) * 0.1f;
    breath_val += noise;
    
    // Apply to chest (scaling), shoulders (rotation)
    // Actual bone manipulation would go here
    (void)skeleton; // Stub for now
}

int proc_breathing_is_gasping() {
    return g_breathing.exertion_level > 0.8f;
}
