#include "ai/perception/perception_system.h"
#include "include/core/logger.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define PERCEPTION_DEFAULT_CAPACITY 64
#define PERCEPTION_MEMORY_DURATION 30.0f  // 30 seconds
#define PERCEPTION_UPDATE_INTERVAL 0.1f   // 10 Hz

PerceptionSystem* perception_create(uint32_t max_memory_objects) {
    PerceptionSystem* perception = malloc(sizeof(PerceptionSystem));
    if (!perception) {
        log_error("Failed to allocate perception system");
        return NULL;
    }
    
    memset(perception, 0, sizeof(PerceptionSystem));
    
    // Initialize memory
    perception->memory.capacity = max_memory_objects > 0 ? max_memory_objects : PERCEPTION_DEFAULT_CAPACITY;
    perception->memory.objects = malloc(sizeof(PerceivedObject) * perception->memory.capacity);
    if (!perception->memory.objects) {
        log_error("Failed to allocate perception memory");
        free(perception);
        return NULL;
    }
    
    // Set default capabilities
    perception->capabilities.visual_range = 50.0f;
    perception->capabilities.visual_angle = 3.14159f / 2.0f;  // 90 degrees
    perception->capabilities.audio_range = 30.0f;
    perception->capabilities.smell_range = 10.0f;
    perception->capabilities.touch_range = 2.0f;
    perception->capabilities.attention_span = 5.0f;
    perception->capabilities.distraction_threshold = 0.3f;
    perception->capabilities.max_memory_objects = perception->memory.capacity;
    
    perception->memory.memory_duration = PERCEPTION_MEMORY_DURATION;
    perception->enabled = true;
    
    log_info("Perception system created with capacity %u", perception->memory.capacity);
    return perception;
}

void perception_destroy(PerceptionSystem* perception) {
    if (perception) {
        free(perception->memory.objects);
        free(perception);
        log_info("Perception system destroyed");
    }
}

void perception_update(PerceptionSystem* perception, float current_time, const Vec3* position, const Vec3* forward, const Vec3* up) {
    if (!perception || !perception->enabled) return;
    
    // Update position and orientation
    if (position) perception->position = *position;
    if (forward) perception->forward = *forward;
    if (up) perception->up = *up;
    
    perception->last_update_time = current_time;
    
    // Forget old objects
    perception_forget_old(perception, current_time);
}

void perception_process_stimulus(PerceptionSystem* perception, const StimulusEvent* stimulus, float current_time) {
    if (!perception || !stimulus || !perception->enabled) return;
    
    // Check if stimulus is within range
    float distance = vec3_distance(&perception->position, &stimulus->position);
    float max_range = 0.0f;
    
    switch (stimulus->type) {
        case PERCEPTION_VISUAL:
            max_range = perception->capabilities.visual_range;
            break;
        case PERCEPTION_AUDIO:
            max_range = perception->capabilities.audio_range;
            break;
        case PERCEPTION_SMELL:
            max_range = perception->capabilities.smell_range;
            break;
        case PERCEPTION_TOUCH:
            max_range = perception->capabilities.touch_range;
            break;
        default:
            return;
    }
    
    if (distance > max_range) return;
    
    // Check line of sight if required
    if (stimulus->requires_line_of_sight && stimulus->type == PERCEPTION_VISUAL) {
        if (!check_line_of_sight(&perception->position, &stimulus->position)) {
            return;
        }
    }
    
    // Calculate intensity at this distance
    StimulusIntensity intensity = calculate_intensity_from_distance(distance, max_range, stimulus->intensity);
    if (intensity == STIMULUS_NONE) return;
    
    // Check if already perceived this entity
    for (uint32_t i = 0; i < perception->memory.count; i++) {
        PerceivedObject* obj = &perception->memory.objects[i];
        if (obj->entity_id == stimulus->source_entity) {
            // Update existing perception
            obj->position = stimulus->position;
            obj->intensity = intensity;
            obj->timestamp = current_time;
            obj->confidence = fminf(1.0f, obj->confidence + 0.1f);
            return;
        }
    }
    
    // Add new perceived object if there's space
    if (perception->memory.count < perception->memory.capacity) {
        PerceivedObject* obj = &perception->memory.objects[perception->memory.count];
        memset(obj, 0, sizeof(PerceivedObject));
        
        obj->entity_id = stimulus->source_entity;
        obj->position = stimulus->position;
        obj->type = stimulus->type;
        obj->intensity = intensity;
        obj->timestamp = current_time;
        obj->confidence = 0.5f;  // Initial confidence
        
        // Simple threat/hostile detection based on intensity and type
        obj->is_threat = (intensity >= STIMULUS_STRONG);
        obj->is_hostile = obj->is_threat && (stimulus->type == PERCEPTION_AUDIO || stimulus->type == PERCEPTION_VISUAL);
        
        perception->memory.count++;
    }
}

void perception_forget_old(PerceptionSystem* perception, float current_time) {
    if (!perception) return;
    
    uint32_t write_index = 0;
    for (uint32_t i = 0; i < perception->memory.count; i++) {
        PerceivedObject* obj = &perception->memory.objects[i];
        float age = current_time - obj->timestamp;
        
        if (age < perception->memory.memory_duration) {
            // Keep this object
            if (write_index != i) {
                perception->memory.objects[write_index] = *obj;
            }
            write_index++;
        }
    }
    
    perception->memory.count = write_index;
}

PerceivedObject* perception_get_nearest_threat(const PerceptionSystem* perception, const PerceptionType types) {
    if (!perception || perception->memory.count == 0) return NULL;
    
    PerceivedObject* nearest = NULL;
    float nearest_distance = FLT_MAX;
    
    for (uint32_t i = 0; i < perception->memory.count; i++) {
        PerceivedObject* obj = &perception->memory.objects[i];
        if (!(obj->type & types) || !obj->is_threat) continue;
        
        float distance = vec3_distance(&perception->position, &obj->position);
        if (distance < nearest_distance) {
            nearest_distance = distance;
            nearest = obj;
        }
    }
    
    return nearest;
}

PerceivedObject* perception_get_nearest_hostile(const PerceptionSystem* perception, const PerceptionType types) {
    if (!perception || perception->memory.count == 0) return NULL;
    
    PerceivedObject* nearest = NULL;
    float nearest_distance = FLT_MAX;
    
    for (uint32_t i = 0; i < perception->memory.count; i++) {
        PerceivedObject* obj = &perception->memory.objects[i];
        if (!(obj->type & types) || !obj->is_hostile) continue;
        
        float distance = vec3_distance(&perception->position, &obj->position);
        if (distance < nearest_distance) {
            nearest_distance = distance;
            nearest = obj;
        }
    }
    
    return nearest;
}

uint32_t perception_get_objects_in_range(const PerceptionSystem* perception, const Vec3* center, float range, PerceptionType types, PerceivedObject* out_objects, uint32_t max_objects) {
    if (!perception || !center || !out_objects || max_objects == 0) return 0;
    
    uint32_t count = 0;
    for (uint32_t i = 0; i < perception->memory.count && count < max_objects; i++) {
        PerceivedObject* obj = &perception->memory.objects[i];
        if (!(obj->type & types)) continue;
        
        float distance = vec3_distance(center, &obj->position);
        if (distance <= range) {
            out_objects[count] = *obj;
            count++;
        }
    }
    
    return count;
}

bool perception_can_see(const PerceptionSystem* perception, const Vec3* target_position) {
    if (!perception || !target_position) return false;
    
    float distance = vec3_distance(&perception->position, target_position);
    if (distance > perception->capabilities.visual_range) return false;
    
    if (!is_in_field_of_view(&perception->forward, target_position, perception->capabilities.visual_angle)) {
        return false;
    }
    
    return check_line_of_sight(&perception->position, target_position);
}

bool perception_can_hear(const PerceptionSystem* perception, const Vec3* sound_position, StimulusIntensity intensity) {
    if (!perception || !sound_position) return false;
    
    float distance = vec3_distance(&perception->position, sound_position);
    if (distance > perception->capabilities.audio_range) return false;
    
    StimulusIntensity perceived_intensity = calculate_intensity_from_distance(distance, perception->capabilities.audio_range, intensity);
    return perceived_intensity > STIMULUS_NONE;
}

void perception_set_capabilities(PerceptionSystem* perception, const PerceptionCapabilities* capabilities) {
    if (!perception || !capabilities) return;
    
    perception->capabilities = *capabilities;
    perception->memory.capacity = capabilities->max_memory_objects;
    
    // Reallocate memory if needed
    PerceivedObject* new_objects = realloc(perception->memory.objects, sizeof(PerceivedObject) * perception->memory.capacity);
    if (new_objects) {
        perception->memory.objects = new_objects;
    }
}

void perception_clear_memory(PerceptionSystem* perception) {
    if (!perception) return;
    perception->memory.count = 0;
}

// Utility functions
float calculate_distance_attenuation(float distance, float max_range) {
    if (distance >= max_range) return 0.0f;
    if (distance <= 0.0f) return 1.0f;
    return 1.0f - (distance / max_range);
}

StimulusIntensity calculate_intensity_from_distance(float distance, float max_range, StimulusIntensity source_intensity) {
    float attenuation = calculate_distance_attenuation(distance, max_range);
    if (attenuation <= 0.0f) return STIMULUS_NONE;
    
    float perceived_intensity = (float)source_intensity * attenuation;
    if (perceived_intensity < 0.5f) return STIMULUS_NONE;
    if (perceived_intensity < 1.5f) return STIMULUS_VERY_WEAK;
    if (perceived_intensity < 2.5f) return STIMULUS_WEAK;
    if (perceived_intensity < 3.5f) return STIMULUS_MODERATE;
    if (perceived_intensity < 4.5f) return STIMULUS_STRONG;
    return STIMULUS_VERY_STRONG;
}

bool check_line_of_sight(const Vec3* from, const Vec3* to) {
    // Simplified line of sight check - in a real implementation this would
    // use raycasting against the collision system
    if (!from || !to) return false;
    
    // For now, assume clear line of sight
    // TODO: Implement proper raycasting
    return true;
}

bool is_in_field_of_view(const Vec3* forward, const Vec3* to_target, float fov_angle) {
    if (!forward || !to_target) return false;
    
    Vec3 to_direction = vec3_normalize(vec3_sub(to_target, &(Vec3){0, 0, 0}));
    float dot_product = vec3_dot(forward, &to_direction);
    float angle = acosf(fmaxf(-1.0f, fminf(1.0f, dot_product)));
    
    return angle <= fov_angle * 0.5f;
}
