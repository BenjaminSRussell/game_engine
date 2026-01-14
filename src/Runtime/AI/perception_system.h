#ifndef PERCEPTION_SYSTEM_H
#define PERCEPTION_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>
#include "include/math/math_all.h"

// Perception types
typedef enum {
    PERCEPTION_VISUAL = 1 << 0,
    PERCEPTION_AUDIO = 1 << 1,
    PERCEPTION_SMELL = 1 << 2,
    PERCEPTION_TOUCH = 1 << 3,
    PERCEPTION_ALL = PERCEPTION_VISUAL | PERCEPTION_AUDIO | PERCEPTION_SMELL | PERCEPTION_TOUCH
} PerceptionType;

// Stimulus intensity levels
typedef enum {
    STIMULUS_NONE = 0,
    STIMULUS_VERY_WEAK = 1,
    STIMULUS_WEAK = 2,
    STIMULUS_MODERATE = 3,
    STIMULUS_STRONG = 4,
    STIMULUS_VERY_STRONG = 5
} StimulusIntensity;

// Perceived object
typedef struct {
    uint32_t entity_id;
    Vec3 position;
    Vec3 velocity;
    PerceptionType type;
    StimulusIntensity intensity;
    float timestamp;
    float confidence;  // 0.0 to 1.0
    bool is_hostile;
    bool is_threat;
    void* custom_data;
} PerceivedObject;

// Memory of perceived objects
typedef struct {
    PerceivedObject* objects;
    uint32_t count;
    uint32_t capacity;
    float memory_duration;  // How long to remember objects
} PerceptionMemory;

// Agent perception capabilities
typedef struct {
    float visual_range;
    float visual_angle;      // Field of view in radians
    float audio_range;
    float smell_range;
    float touch_range;
    float attention_span;   // How long to maintain focus
    float distraction_threshold;
    uint32_t max_memory_objects;
} PerceptionCapabilities;

// Perception system for an agent
typedef struct {
    PerceptionCapabilities capabilities;
    PerceptionMemory memory;
    Vec3 position;
    Vec3 forward;
    Vec3 up;
    float last_update_time;
    bool enabled;
    void* agent_data;
} PerceptionSystem;

// Stimulus event
typedef struct {
    Vec3 position;
    Vec3 direction;
    PerceptionType type;
    StimulusIntensity intensity;
    float radius;
    float duration;
    uint32_t source_entity;
    bool requires_line_of_sight;
} StimulusEvent;

// Function declarations
PerceptionSystem* perception_create(uint32_t max_memory_objects);
void perception_destroy(PerceptionSystem* perception);
void perception_update(PerceptionSystem* perception, float current_time, const Vec3* position, const Vec3* forward, const Vec3* up);
void perception_process_stimulus(PerceptionSystem* perception, const StimulusEvent* stimulus, float current_time);
void perception_forget_old(PerceptionSystem* perception, float current_time);
PerceivedObject* perception_get_nearest_threat(const PerceptionSystem* perception, const PerceptionType types);
PerceivedObject* perception_get_nearest_hostile(const PerceptionSystem* perception, const PerceptionType types);
uint32_t perception_get_objects_in_range(const PerceptionSystem* perception, const Vec3* center, float range, PerceptionType types, PerceivedObject* out_objects, uint32_t max_objects);
bool perception_can_see(const PerceptionSystem* perception, const Vec3* target_position);
bool perception_can_hear(const PerceptionSystem* perception, const Vec3* sound_position, StimulusIntensity intensity);
void perception_set_capabilities(PerceptionSystem* perception, const PerceptionCapabilities* capabilities);
void perception_clear_memory(PerceptionSystem* perception);

// Utility functions
float calculate_distance_attenuation(float distance, float max_range);
StimulusIntensity calculate_intensity_from_distance(float distance, float max_range, StimulusIntensity source_intensity);
bool check_line_of_sight(const Vec3* from, const Vec3* to);
bool is_in_field_of_view(const Vec3* forward, const Vec3* to_target, float fov_angle);

#endif // PERCEPTION_SYSTEM_H
