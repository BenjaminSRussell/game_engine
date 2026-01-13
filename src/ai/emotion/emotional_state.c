/**
 * =================================================================================================
 *                          EMOTIONAL STATE
 * =================================================================================================
 */

#include "ai/emotion/emotional_state.h"
#include <string.h>
#include <include/math/math.h>

void emotion_init(EmotionalState *state) {
    memset(state->values, 0, sizeof(state->values));
    state->decay_rate = 0.5f; // Emotions decay slowly
    state->neuroticism = 0.5f;
    state->extraversion = 0.5f;
}

void emotion_trigger(EmotionalState *state, EmotionType type, float intensity) {
    if (type < 0 || type >= EMOTION_COUNT) return;
    
    // Apply personality modifiers
    float modified_intensity = intensity;
    
    if (type == EMOTION_FEAR || type == EMOTION_ANGER) {
        modified_intensity *= state->neuroticism;
    }
    
    if (type == EMOTION_JOY) {
        modified_intensity *= state->extraversion;
    }
    
    // Add to current emotion value (clamped to 1.0)
    state->values[type] += modified_intensity;
    if (state->values[type] > 1.0f) {
        state->values[type] = 1.0f;
    }
    
    // Emotional contagion: related emotions also increase slightly
    if (type == EMOTION_FEAR) {
        state->values[EMOTION_SURPRISE] += modified_intensity * 0.3f;
    } else if (type == EMOTION_ANGER) {
        state->values[EMOTION_DISGUST] += modified_intensity * 0.2f;
    }
}

void emotion_update(EmotionalState *state, float delta_time) {
    // Decay all emotions over time
    for (int i = 0; i < EMOTION_COUNT; i++) {
        state->values[i] -= state->decay_rate * delta_time;
        if (state->values[i] < 0.0f) {
            state->values[i] = 0.0f;
        }
    }
}

float emotion_get_dominant(EmotionalState *state, EmotionType *out_type) {
    float max_value = 0.0f;
    EmotionType max_type = EMOTION_JOY;
    
    for (int i = 0; i < EMOTION_COUNT; i++) {
        if (state->values[i] > max_value) {
            max_value = state->values[i];
            max_type = (EmotionType)i;
        }
    }
    
    if (out_type) {
        *out_type = max_type;
    }
    
    return max_value;
}

void emotion_blend(EmotionalState *state, EmotionalState *other, float factor) {
    // Blend two emotional states (useful for group mood)
    for (int i = 0; i < EMOTION_COUNT; i++) {
        state->values[i] = state->values[i] * (1.0f - factor) + other->values[i] * factor;
    }
}
