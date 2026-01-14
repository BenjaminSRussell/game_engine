#pragma once

typedef enum {
    EMOTION_JOY,
    EMOTION_FEAR,
    EMOTION_ANGER,
    EMOTION_SADNESS,
    EMOTION_SURPRISE,
    EMOTION_DISGUST,
    EMOTION_COUNT
} EmotionType;

typedef struct {
    float values[EMOTION_COUNT]; // 0-1 for each emotion
    float decay_rate;
    
    // Personality modifiers
    float neuroticism;
    float extraversion;
} EmotionalState;

void emotion_init(EmotionalState *state);
void emotion_trigger(EmotionalState *state, EmotionType type, float intensity);
void emotion_update(EmotionalState *state, float delta_time);

float emotion_get_dominant(EmotionalState *state, EmotionType *out_type);
void emotion_blend(EmotionalState *state, EmotionalState *other, float factor);
